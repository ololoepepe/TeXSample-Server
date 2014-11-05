/****************************************************************************
**
** Copyright (C) 2012-2014 Andrey Bogdanov
**
** This file is part of TeXSample Server.
**
** TeXSample Server is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** TeXSample Server is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with TeXSample Server.  If not, see <http://www.gnu.org/licenses/>.
**
****************************************************************************/

#include "samplerepository.h"

#include "datasource.h"
#include "entity/sample.h"
#include "repositorytools.h"
#include "temporarylocation.h"

#include <TBinaryFile>
#include <TBinaryFileList>
#include <TFileInfo>
#include <TFileInfoList>
#include <TIdList>
#include <TSampleType>
#include <TTexFile>
#include <TTexFileList>
#include <TTexProject>

#include <BeQt>
#include <BSqlResult>
#include <BSqlWhere>

#include <QByteArray>
#include <QDebug>
#include <QFileInfo>
#include <QList>
#include <QString>
#include <QTextCodec>
#include <QVariant>

/*============================================================================
================================ SampleRepository ============================
============================================================================*/

/*============================== Public constructors =======================*/

SampleRepository::SampleRepository(DataSource *source) :
    Source(source)
{
    //
}

SampleRepository::~SampleRepository()
{
    //
}

/*============================== Public methods ============================*/

quint64 SampleRepository::add(const Sample &entity, bool *ok)
{
    if (!isValid() || !entity.isValid() || entity.isCreatedByRepo() || entity.id())
        return bRet(ok, false, 0);
    QDateTime dt = QDateTime::currentDateTimeUtc();
    QVariantMap values;
    values.insert("sender_id", entity.senderId());
    values.insert("admin_remark", QString());
    values.insert("creation_date_time", dt.toMSecsSinceEpoch());
    values.insert("description", entity.description());
    values.insert("last_modification_date_time", dt.toMSecsSinceEpoch());
    values.insert("rating", quint8(0));
    values.insert("title", entity.title());
    values.insert("type", int(TSampleType::Unverified));
    values.insert("source_main_file_info", serializedSouceMainFileInfo(entity.source()));
    values.insert("source_extra_file_infos", serializedSouceExtraFileInfos(entity.source()));
    values.insert("preview_main_file_info", serializedPreviewMainFileInfo(entity.previewMainFile()));
    BSqlResult result = Source->insert("samples", values);
    if (!result.success())
        return bRet(ok, false, 0);
    quint64 id = result.lastInsertId().toULongLong();
    if (!RepositoryTools::setAuthorInfoList(Source, "sample_authors", "sample_id", id, entity.authors()))
        return bRet(ok, false, 0);
    if (!RepositoryTools::setTags(Source, "sample_tags", "sample_id", id, entity.tags()))
        return bRet(ok, false, 0);
    if (!createSource(id, entity.source()) || !createPreview(id, entity.previewMainFile()))
        return bRet(ok, false, 0);
    return bRet(ok, true, id);
}

DataSource *SampleRepository::dataSource() const
{
    return Source;
}

QDateTime SampleRepository::deleteOne(quint64 id, bool *ok)
{
    if (!isValid() || !id)
        return bRet(ok, false, QDateTime());
    QDateTime dt = QDateTime::currentDateTimeUtc();
    if (!Source->deleteFrom("samples", BSqlWhere("id = :id", ":id", id)).success())
        return bRet(ok, false, QDateTime());
    if (!Source->insert("deleted_samples", "id", id, "deletion_date_time", dt.toMSecsSinceEpoch()).success())
        return bRet(ok, false, QDateTime());
    return bRet(ok, true, dt);
}

void SampleRepository::edit(const Sample &entity, bool *ok)
{
    if (!isValid() || !entity.isValid() || !entity.id() || entity.isCreatedByRepo())
        return bSet(ok, false);
    QDateTime dt = QDateTime::currentDateTimeUtc();
    quint64 id = entity.id();
    QVariantMap values;
    values.insert("sender_id", entity.senderId());
    if (entity.saveAdminRemark())
        values.insert("admin_remark", entity.adminRemark());
    values.insert("description", entity.description());
    values.insert("last_modification_date_time", dt.toMSecsSinceEpoch());
    values.insert("rating", entity.rating());
    values.insert("title", entity.title());
    values.insert("type", int(entity.type()));
    if (entity.saveData()) {
        values.insert("source_main_file_info", serializedSouceMainFileInfo(entity.source()));
        values.insert("source_extra_file_infos", serializedSouceExtraFileInfos(entity.source()));
        values.insert("preview_main_file_info", serializedPreviewMainFileInfo(entity.previewMainFile()));
    }
    BSqlResult result = Source->update("samples", values, BSqlWhere("id = :id", ":id", id));
    if (!result.success())
        return bSet(ok, false);
    static const QStringList Tables = QStringList() << "sample_authors" << "sample_tags";
    if (!RepositoryTools::deleteHelper(Source, Tables, "sample_id", id))
        return bSet(ok, false);
    if (!RepositoryTools::setAuthorInfoList(Source, "sample_authors", "sample_id", id, entity.authors()))
        return bSet(ok, false);
    if (!RepositoryTools::setTags(Source, "sample_tags", "sample_id", id, entity.tags()))
        return bSet(ok, false);
    if (entity.saveData() && (!updateSource(id, entity.source()) || !updatePreview(id, entity.previewMainFile())))
        return bSet(ok, false);
    bSet(ok, true);
}

TIdList SampleRepository::findAllDeletedNewerThan(const QDateTime &newerThan, bool *ok)
{
    TIdList list;
    if (!isValid())
        return bRet(ok, false, list);
    BSqlWhere where;
    if (newerThan.isValid()) {
        where = BSqlWhere("deletion_date_time > :deletion_date_time", ":deletion_date_time",
                          newerThan.toUTC().toMSecsSinceEpoch());
    }
    BSqlResult result = Source->select("deleted_samples", "id", where);
    if (!result.success())
        return bRet(ok, false, list);
    foreach (const QVariantMap &m, result.values())
        list << m.value("id").toULongLong();
    return bRet(ok, true, list);
}

QList<Sample> SampleRepository::findAllNewerThan(const QDateTime &newerThan, bool *ok)
{
    QList<Sample> list;
    if (!isValid())
        return bRet(ok, false, list);
    static const QStringList Fields = QStringList() << "id" << "sender_id" << "admin_remark" << "creation_date_time"
        << "description" << "last_modification_date_time" << "rating" << "title" << "type" << "source_main_file_info"
        << "source_extra_file_infos" << "preview_main_file_info";
    BSqlWhere where;
    if (newerThan.isValid()) {
        where = BSqlWhere("last_modification_date_time > :last_modification_date_time", ":last_modification_date_time",
                          newerThan.toUTC().toMSecsSinceEpoch());
    }
    BSqlResult result = Source->select("samples", Fields, where);
    if (!result.success())
        return bRet(ok, false, list);
    foreach (const QVariantMap &m, result.values()) {
        Sample entity(this);
        entity.mid = m.value("id").toULongLong();
        entity.msenderId = m.value("sender_id").toULongLong();
        entity.madminRemark = m.value("admin_remark").toString();
        entity.mcreationDateTime.setMSecsSinceEpoch(m.value("creation_date_time").toLongLong());
        entity.mdescription = m.value("description").toString();
        entity.mlastModificationDateTime.setMSecsSinceEpoch(m.value("last_modification_date_time").toLongLong());
        entity.mrating = m.value("rating").toUInt();
        entity.mtitle = m.value("title").toString();
        entity.mtype = m.value("type").toInt();
        entity.msourceMainFileInfo = deserializedSouceMainFileInfo(m.value("source_main_file_info").toByteArray());
        entity.msourceExtraFileInfos =
                deserializedSouceExtraFileInfos(m.value("source_extra_file_infos").toByteArray());
        entity.mpreviewMainFileInfo = deserializedPreviewMainFileInfo(m.value("preview_main_file_info").toByteArray());
        bool b = false;
        entity.mauthors = RepositoryTools::getAuthorInfoList(Source, "sample_authors", "sample_id", entity.id(), &b);
        if (!b)
            return bRet(ok, false, QList<Sample>());
        entity.mtags = RepositoryTools::getTags(Source, "sample_tags", "sample_id", entity.id(), &b);
        if (!b)
            return bRet(ok, false, QList<Sample>());
        entity.valid = true;
        list << entity;
    }
    return bRet(ok, true, list);
}

QDateTime SampleRepository::findLastModificationDateTime(quint64 id, bool *ok)
{
    if (!isValid() || !id)
        return bRet(ok, false, QDateTime());
    BSqlResult result = Source->select("samples", "last_modification_date_time", BSqlWhere("id = :id", ":id", id));
    if (!result.success())
        return bRet(ok, false, QDateTime());
    if (result.values().isEmpty())
        return bRet(ok, true, QDateTime());
    QDateTime dt;
    dt.setTimeSpec(Qt::UTC);
    dt.setMSecsSinceEpoch(result.value("last_modification_date_time").toLongLong());
    return bRet(ok, true, dt);
}

Sample SampleRepository::findOne(quint64 id, bool *ok)
{
    Sample entity(this);
    if (!isValid() || !id)
        return bRet(ok, false, entity);
    static const QStringList Fields = QStringList() << "sender_id" << "admin_remark" << "creation_date_time"
        << "description" << "last_modification_date_time" << "rating" << "title" << "type" << "source_main_file_info"
        << "source_extra_file_infos" << "preview_main_file_info";
    BSqlResult result = Source->select("samples", Fields, BSqlWhere("id = :id", ":id", id));
    if (!result.success())
        return bRet(ok, false, entity);
    if (result.values().isEmpty())
        return bRet(ok, true, entity);
    entity.mid = id;
    entity.msenderId = result.value("sender_id").toULongLong();
    entity.madminRemark = result.value("admin_remark").toString();
    entity.mcreationDateTime.setMSecsSinceEpoch(result.value("creation_date_time").toLongLong());
    entity.mdescription = result.value("description").toString();
    entity.mlastModificationDateTime.setMSecsSinceEpoch(result.value("last_modification_date_time").toLongLong());
    entity.mrating = result.value("rating").toUInt();
    entity.mtitle = result.value("title").toString();
    entity.mtype = result.value("type").toInt();
    entity.msourceMainFileInfo = deserializedSouceMainFileInfo(result.value("source_main_file_info").toByteArray());
    entity.msourceExtraFileInfos =
            deserializedSouceExtraFileInfos(result.value("source_extra_file_infos").toByteArray());
    entity.mpreviewMainFileInfo =
            deserializedPreviewMainFileInfo(result.value("preview_main_file_info").toByteArray());
    bool b = false;
    entity.mauthors = RepositoryTools::getAuthorInfoList(Source, "sample_authors", "sample_id", id, &b);
    if (!b)
        return bRet(ok, false, entity);
    entity.mtags = RepositoryTools::getTags(Source, "sample_tags", "sample_id", id, &b);
    if (!b)
        return bRet(ok, false, entity);
    entity.valid = true;
    return bRet(ok, true, entity);
}

bool SampleRepository::isValid() const
{
    return Source && Source->isValid();
}

/*============================== Static private methods ====================*/

TFileInfo SampleRepository::deserializedPreviewMainFileInfo(const QByteArray &data)
{
    return BeQt::deserialize(data).value<TFileInfo>();
}

TFileInfoList SampleRepository::deserializedSouceExtraFileInfos(const QByteArray &data)
{
    return BeQt::deserialize(data).value<TFileInfoList>();
}

TFileInfo SampleRepository::deserializedSouceMainFileInfo(const QByteArray &data)
{
    return BeQt::deserialize(data).value<TFileInfo>();
}

QByteArray SampleRepository::serializedPreviewMainFileInfo(const TBinaryFile &preview)
{
    if (!preview.isValid())
        return QByteArray();
    TFileInfo fi;
    fi.setFileName(preview.fileName());
    fi.setFileSize(preview.size());
    return BeQt::serialize(fi);
}

QByteArray SampleRepository::serializedSouceExtraFileInfos(const TTexProject &source)
{
    if (!source.isValid())
        return QByteArray();
    TFileInfoList list;
    foreach (const TTexFile &f, source.texFiles()) {
        TFileInfo fi;
        fi.setFileName(f.fileName());
        fi.setFileSize(f.size());
        list << fi;
    }
    foreach (const TBinaryFile &f, source.binaryFiles()) {
        TFileInfo fi;
        fi.setFileName(f.fileName());
        fi.setFileSize(f.size());
        list << fi;
    }
    return BeQt::serialize(list);
}

QByteArray SampleRepository::serializedSouceMainFileInfo(const TTexProject &source)
{
    if (!source.isValid())
        return QByteArray();
    TFileInfo fi;
    fi.setFileName(source.rootFile().fileName());
    fi.setFileSize(source.rootFile().size());
    return BeQt::serialize(fi);
}

/*============================== Private methods ===========================*/

bool SampleRepository::createPreview(quint64 sampleId, const TBinaryFile &previewMainFile)
{
    if (!isValid() || !sampleId || !previewMainFile.isValid() || !previewMainFile.size())
        return false;
    QByteArray data = BeQt::serialize(previewMainFile);
    return Source->insert("sample_previews", "sample_id", sampleId, "main_file", data).success();
}

bool SampleRepository::createSource(quint64 sampleId, const TTexProject &source)
{
    if (!isValid() || !sampleId || !source.isValid())
        return false;
    TTexProject src = source;
    src.removeRestrictedCommands();
    return Source->insert("sample_sources", "sample_id", sampleId, "source", BeQt::serialize(src)).success();
}

TBinaryFile SampleRepository::fetchPreview(quint64 sampleId, bool *ok)
{
    if (!isValid() || !sampleId)
        return bRet(ok, false, TBinaryFile());
    BSqlWhere where("sample_id = :sample_id", ":sample_id", sampleId);
    BSqlResult result = Source->select("sample_previews", "main_file", where);
    if (!result.success() || result.value().isEmpty())
        return bRet(ok, false, TBinaryFile());
    TBinaryFile mainFile = BeQt::deserialize(result.value("main_file").toByteArray()).value<TBinaryFile>();
    return bRet(ok, true, mainFile);
}

TTexProject SampleRepository::fetchSource(quint64 sampleId, bool *ok)
{
    if (!isValid() || !sampleId)
        return bRet(ok, false, TTexProject());
    BSqlWhere where("sample_id = :sample_id", ":sample_id", sampleId);
    BSqlResult result = Source->select("sample_sources", "source", where);
    if (!result.success() || result.value().isEmpty())
        return bRet(ok, false, TTexProject());
    TTexProject source = BeQt::deserialize(result.value("source").toByteArray()).value<TTexProject>();
    return bRet(ok, true, source);
}

bool SampleRepository::updatePreview(quint64 sampleId, const TBinaryFile &previewMainFile)
{
    if (!isValid() || !sampleId || !previewMainFile.isValid() || !previewMainFile.size())
        return false;
    BSqlWhere where("sample_id = :sample_id", ":sample_id", sampleId);
    return Source->update("sample_previews", "main_file", BeQt::serialize(previewMainFile), where).success();
}

bool SampleRepository::updateSource(quint64 sampleId, const TTexProject &source)
{
    if (!isValid() || !sampleId || !source.isValid())
        return false;
    TTexProject src = source;
    src.removeRestrictedCommands();
    BSqlWhere where("sample_id = :sample_id", ":sample_id", sampleId);
    return Source->update("sample_sources", "source", BeQt::serialize(src), where).success();
}
