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

#include <TBinaryFile>
#include <TIdList>
#include <TSampleType>
#include <TTexProject>

#include <BeQt>
#include <BSqlResult>
#include <BSqlWhere>

#include <QByteArray>
#include <QList>
#include <QString>
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
    values.insert("deleted", false);
    values.insert("admin_remark", QString());
    values.insert("creation_date_time", dt.toMSecsSinceEpoch());
    values.insert("description", entity.description());
    values.insert("last_modification_date_time", dt.toMSecsSinceEpoch());
    values.insert("rating", quint8(0));
    values.insert("title", entity.title());
    values.insert("type", int(entity.type()));
    BSqlResult result = Source->insert("samples", values);
    if (!result.success())
        return bRet(ok, false, 0);
    quint64 id = result.lastInsertId().toULongLong();
    if (!RepositoryTools::setAuthorInfoList(Source, "sample_authors", "sample_id", id, entity.authors()))
        return bRet(ok, false, 0);
    if (!RepositoryTools::setTags(Source, "sample_tags", "sample_id", id, entity.tags()))
        return bRet(ok, false, 0);
    if (!createSource(id, entity.source()))
        return bRet(ok, false, 0);
    return bRet(ok, false, id);
}

DataSource *SampleRepository::dataSource() const
{
    return Source;
}

void SampleRepository::edit(const Sample &entity, bool *ok)
{
    if (!isValid() || !entity.isValid() || !entity.id() || entity.isCreatedByRepo())
        return bSet(ok, false);
    QDateTime dt = QDateTime::currentDateTimeUtc();
    QVariantMap values;
    values.insert("sender_id", entity.senderId());
    values.insert("deleted", false);
    if (entity.saveAdminRemark())
        values.insert("admin_remark", entity.adminRemark());
    values.insert("description", entity.description());
    values.insert("last_modification_date_time", dt.toMSecsSinceEpoch());
    values.insert("rating", quint8(0));
    values.insert("title", entity.title());
    values.insert("type", int(entity.type()));
    BSqlResult result = Source->update("samples", values, BSqlWhere("id = :id", ":id", entity.id()));
    if (!result.success())
        return bSet(ok, false);
    static const QStringList Tables = QStringList() << "sample_authors" << "sample_tags";
    if (!RepositoryTools::deleteHelper(Source, Tables, "sample_id", entity.id()))
        return bSet(ok, false);
    if (!RepositoryTools::setAuthorInfoList(Source, "sample_authors", "sample_id", entity.id(), entity.authors()))
        return bSet(ok, false);
    if (!RepositoryTools::setTags(Source, "sample_tags", "sample_id", entity.id(), entity.tags()))
        return bSet(ok, false);
    if (entity.saveData() && !updateSource(entity.id(), entity.source()))
        return bSet(ok, false);
    return bSet(ok, true);
}

QList<Sample> SampleRepository::findAllNewerThan(const QDateTime &newerThan, bool *ok)
{
    QList<Sample> list;
    if (!isValid())
        return bRet(ok, false, list);
    static const QStringList Fields = QStringList() << "id" << "sender_id" << "deleted" << "admin_remark"
        << "creation_date_time" << "description" << "last_modification_date_time" << "rating" << "title" << "type";
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
        entity.mdeleted = m.value("deleted").toBool();
        entity.madminRemark = m.value("admin_remark").toString();
        entity.mcreationDateTime.setMSecsSinceEpoch(m.value("creation_date_time").toLongLong());
        entity.mdescription = m.value("description").toString();
        entity.mlastModificationDateTime.setMSecsSinceEpoch(m.value("last_modification_date_time").toLongLong());
        entity.mrating = m.value("rating").toUInt();
        entity.mtitle = m.value("title").toString();
        entity.mtype = m.value("type").toInt();
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

Sample SampleRepository::findOne(quint64 id, bool *ok)
{
    Sample entity(this);
    if (!isValid() || !id)
        return bRet(ok, false, entity);
    static const QStringList Fields = QStringList() << "sender_id" << "deleted" << "admin_remark"
        << "creation_date_time" << "description" << "last_modification_date_time" << "rating" << "title" << "type";
    BSqlResult result = Source->select("samples", Fields, BSqlWhere("id = :id", ":id", id));
    if (!result.success())
        return bRet(ok, false, entity);
    if (result.value().isEmpty())
        return bRet(ok, true, entity);
    entity.mid = result.value("id").toULongLong();
    entity.msenderId = result.value("sender_id").toULongLong();
    entity.mdeleted = result.value("deleted").toBool();
    entity.madminRemark = result.value("admin_remark").toString();
    entity.mcreationDateTime.setMSecsSinceEpoch(result.value("creation_date_time").toLongLong());
    entity.mdescription = result.value("description").toString();
    entity.mlastModificationDateTime.setMSecsSinceEpoch(result.value("last_modification_date_time").toLongLong());
    entity.mrating = result.value("rating").toUInt();
    entity.mtitle = result.value("title").toString();
    entity.mtype = result.value("type").toInt();
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

/*============================== Private methods ===========================*/

bool SampleRepository::createSource(quint64 sampleId, const TTexProject &data)
{
    if (!isValid() || !sampleId)
        return false;
    return Source->insert("sample_sources", "sample_id", sampleId, "source", BeQt::serialize(data)).success();
}

TBinaryFile SampleRepository::fetchPreview(quint64 sampleId, bool *ok)
{
    if (!isValid() || !sampleId)
        return bRet(ok, false, TBinaryFile());
    BSqlResult result = Source->select("sample_previews", "main_file",
                                       BSqlWhere("sample_id = :sample_id", ":sample_id", sampleId));
    if (!result.success() || result.value().isEmpty())
        return bRet(ok, false, TBinaryFile());
    TBinaryFile mainFile = BeQt::deserialize(result.value("main_file").toByteArray()).value<TBinaryFile>();
    return bRet(ok, true, mainFile);
}

TTexProject SampleRepository::fetchSource(quint64 sampleId, bool *ok)
{
    if (!isValid() || !sampleId)
        return bRet(ok, false, TTexProject());
    BSqlResult result = Source->select("sample_sources", "source",
                                       BSqlWhere("sample_id = :sample_id", ":sample_id", sampleId));
    if (!result.success() || result.value().isEmpty())
        return bRet(ok, false, TTexProject());
    TTexProject source = BeQt::deserialize(result.value("source").toByteArray()).value<TTexProject>();
    return bRet(ok, true, source);
}


bool SampleRepository::updateSource(quint64 sampleId, const TTexProject &data)
{
    if (!isValid() || !sampleId)
        return false;
    return Source->update("sample_previews", "sample_id", sampleId, "preview", BeQt::serialize(data)).success();
}
