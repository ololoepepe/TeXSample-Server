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

#include "labrepository.h"

#include "datasource.h"
#include "entity/lab.h"
#include "repositorytools.h"

#include <TAuthorInfo>
#include <TAuthorInfoList>
#include <TBinaryFile>
#include <TBinaryFileList>
#include <TFileInfo>
#include <TFileInfoList>
#include <TIdList>
#include <TLabApplication>
#include <TLabDataInfo>
#include <TLabDataInfoList>
#include <TLabData>
#include <TLabDataList>
#include <TLabType>
#include <TTexProject>

#include <BeQt>
#include <BSqlResult>
#include <BSqlWhere>

#include <QByteArray>
#include <QList>
#include <QString>
#include <QStringList>
#include <QVariant>

/*============================================================================
================================ LabRepository ===============================
============================================================================*/

/*============================== Public constructors =======================*/

LabRepository::LabRepository(DataSource *source) :
    Source(source)
{
    //
}

LabRepository::~LabRepository()
{
    //
}

/*============================== Public methods ============================*/

quint64 LabRepository::add(const Lab &entity, bool *ok)
{
    if (!isValid() || !entity.isValid() || entity.isCreatedByRepo() || entity.id())
        return bRet(ok, false, 0);
    QDateTime dt = QDateTime::currentDateTimeUtc();
    QVariantMap values;
    values.insert("sender_id", entity.senderId());
    values.insert("creation_date_time", dt.toMSecsSinceEpoch());
    values.insert("description", entity.description());
    values.insert("last_modification_date_time", dt.toMSecsSinceEpoch());
    values.insert("title", entity.title());
    values.insert("type", int(entity.type()));
    BSqlResult result = Source->insert("labs", values);
    if (!result.success())
        return bRet(ok, false, 0);
    quint64 id = result.lastInsertId().toULongLong();
    if (!RepositoryTools::setAuthorInfoList(Source, "lab_authors", "lab_id", id, entity.authors()))
        return bRet(ok, false, 0);
    if (!RepositoryTools::setGroupIdList(Source, "lab_groups", "lab_id", id, entity.groups()))
        return bRet(ok, false, 0);
    if (!RepositoryTools::setTags(Source, "lab_tags", "lab_id", id, entity.tags()))
        return bRet(ok, false, 0);
    if (!createData(id, entity.labDataList()) || !createExtraFiles(id, entity.extraFiles()))
        return bRet(ok, false, 0);
    return bRet(ok, true, id);
}

DataSource *LabRepository::dataSource() const
{
    return Source;
}

QDateTime LabRepository::deleteOne(quint64 id, bool *ok)
{
    if (!isValid() || !id)
        return bRet(ok, false, QDateTime());
    QDateTime dt = QDateTime::currentDateTimeUtc();
    BSqlResult result = Source->select("lab_groups", "group_id", BSqlWhere("lab_id = :lab_id", ":lab_id", id));
    if (!result.success())
        return bRet(ok, false, QDateTime());
    if (!Source->deleteFrom("labs", BSqlWhere("id = :id", ":id", id)).success())
        return bRet(ok, false, QDateTime());
    if (!Source->insert("deleted_labs", "id", id, "deletion_date_time", dt.toMSecsSinceEpoch()).success())
        return bRet(ok, false, QDateTime());
    foreach (const QVariantMap &m, result.values()) {
        quint64 groupId = m.value("group_id").toULongLong();
        if (!Source->insert("deleted_lab_groups", "lab_id", id, "group_id", groupId).success())
            return bRet(ok, false, QDateTime());
    }
    return bRet(ok, true, dt);
}

void LabRepository::edit(const Lab &entity, bool *ok)
{
    if (!isValid() || !entity.isValid() || !entity.id() || entity.isCreatedByRepo())
        return bSet(ok, false);
    QDateTime dt = QDateTime::currentDateTimeUtc();
    BSqlResult r = Source->select("labs", "extra_file_infos", BSqlWhere("id = :id", ":id", entity.id()));
    if (!r.success())
        return bSet(ok, false);
    TFileInfoList fil = deserializedExtraFileInfos(r.value("extra_file_infos").toByteArray());
    foreach (const QString &fn, entity.deletedExtraFiles()) {
        foreach (int i, bRangeR(fil.size() - 1, 0)) {
            if (fil.at(i).fileName() == fn) {
                fil.removeAt(i);
                break;
            }
        }
    }
    QVariantMap values;
    values.insert("sender_id", entity.senderId());
    values.insert("description", entity.description());
    values.insert("last_modification_date_time", dt.toMSecsSinceEpoch());
    values.insert("title", entity.title());
    values.insert("type", int(entity.type()));
    values.insert("data_infos", serializedDataInfos(entity.labDataList(), entity.type()));
    values.insert("extra_file_infos", serializedExtraFileInfos(entity.extraFiles(), fil));
    BSqlResult result = Source->update("labs", values, BSqlWhere("id = :id", ":id", entity.id()));
    if (!result.success())
        return bSet(ok, false);
    static const QStringList Tables = QStringList() << "lab_authors" << "lab_groups" << "lab_tags";
    if (!RepositoryTools::deleteHelper(Source, Tables, "lab_id", entity.id()))
        return bSet(ok, false);
    if (!RepositoryTools::setAuthorInfoList(Source, "lab_authors", "lab_id", entity.id(), entity.authors()))
        return bSet(ok, false);
    if (!RepositoryTools::setGroupIdList(Source, "lab_groups", "lab_id", entity.id(), entity.groups()))
        return bSet(ok, false);
    if (!RepositoryTools::setTags(Source, "lab_tags", "lab_id", entity.id(), entity.tags()))
        return bSet(ok, false);
    if (entity.saveData() && !updateData(entity.id(), entity.labDataList()))
        return bSet(ok, false);
    if (!deleteExtraFiles(entity.id(), entity.deletedExtraFiles()))
        return bSet(ok, false);
    if (!createExtraFiles(entity.id(), entity.extraFiles()))
        return bSet(ok, false);
    bSet(ok, true);
}

TIdList LabRepository::findAllDeletedNewerThan(const QDateTime &newerThan, const TIdList &groups, bool *ok)
{
    TIdList list;
    if (!isValid())
        return bRet(ok, false, list);
    QString qs = "SELECT deleted_labs.id FROM deleted_labs";
    QVariantMap bv;
    if (newerThan.isValid()) {
        qs += " WHERE labs.deletion_date_time > :deletion_date_time";
        bv.insert(":deletion_date_time", newerThan.toUTC().toMSecsSinceEpoch());
    }
    if (!groups.isEmpty()) {
        qs += newerThan.isValid() ? " AND" : " WHERE";
        qs += " (SELECT COUNT(*) FROM deleted_lab_groups WHERE deleted_lab_groups.lab_id = deleted_labs.id "
              "AND deleted_lab_groups.group_id IN (";
        foreach (int i, bRangeD(0, groups.size() - 1)) {
            qs += ":" + QString::number(groups.at(i));
            if (i < groups.size() - 1)
                qs += ", ";
            bv.insert(":" + QString::number(groups.at(i)), groups.at(i));
        }
        qs += ")) > 0";
    }
    BSqlResult result = Source->exec(qs, bv);
    if (!result.success())
        return bRet(ok, false, list);
    foreach (const QVariantMap &m, result.values())
        list << m.value("id").toULongLong();
    return bRet(ok, true, list);
}

QList<Lab> LabRepository::findAllNewerThan(const TIdList &groups, bool *ok)
{
    return findAllNewerThan(QDateTime(), groups, ok);
}

QList<Lab> LabRepository::findAllNewerThan(const QDateTime &newerThan, const TIdList &groups, bool *ok)
{
    QList<Lab> list;
    if (!isValid())
        return bRet(ok, false, list);
    QString qs = "SELECT labs.id, labs.sender_id, labs.creation_date_time, labs.description, "
        "labs.last_modification_date_time, labs.title, labs.type labs.data_infos labs.extra_file_infos FROM labs";
    QVariantMap bv;
    if (newerThan.isValid()) {
        qs += " WHERE labs.last_modification_date_time > :last_modification_date_time";
        bv.insert(":last_modification_date_time", newerThan.toUTC().toMSecsSinceEpoch());
    }
    if (!groups.isEmpty()) {
        qs += newerThan.isValid() ? " AND" : " WHERE";
        qs += " (SELECT COUNT(*) FROM lab_groups WHERE lab_groups.lab_id = labs.id AND lab_groups.group_id IN (";
        foreach (int i, bRangeD(0, groups.size() - 1)) {
            qs += ":" + QString::number(groups.at(i));
            if (i < groups.size() - 1)
                qs += ", ";
            bv.insert(":" + QString::number(groups.at(i)), groups.at(i));
        }
        qs += ")) > 0";
    }
    BSqlResult result = Source->exec(qs, bv);
    if (!result.success())
        return bRet(ok, false, list);
    foreach (const QVariantMap &m, result.values()) {
        Lab entity(this);
        entity.mid = m.value("labs.id").toULongLong();
        entity.msenderId = m.value("labs.sender_id").toULongLong();
        entity.mcreationDateTime.setMSecsSinceEpoch(m.value("labs.creation_date_time").toLongLong());
        entity.mdescription = m.value("labs.description").toString();
        entity.mlastModificationDateTime.setMSecsSinceEpoch(m.value("labs.last_modification_date_time").toLongLong());
        entity.mtitle = m.value("labs.title").toString();
        entity.mtype = m.value("labs.type").toInt();
        entity.mdataInfoList = deserializedDataInfos(m.value("data_infos").toByteArray());
        entity.mextraFileInfos = deserializedExtraFileInfos(m.value("extra_file_infos").toByteArray());
        bool b = false;
        entity.mauthors = RepositoryTools::getAuthorInfoList(Source, "lab_authors", "lab_id", entity.id(), &b);
        if (!b)
            return bRet(ok, false, QList<Lab>());
        entity.mgroups = RepositoryTools::getGroupIdList(Source, "lab_groups", "lab_id", entity.id(), &b);
        if (!b)
            return bRet(ok, false, QList<Lab>());
        entity.mtags = RepositoryTools::getTags(Source, "lab_tags", "lab_id", entity.id(), &b);
        if (!b)
            return bRet(ok, false, QList<Lab>());
        entity.valid = true;
        list << entity;
    }
    return bRet(ok, true, list);
}

QDateTime LabRepository::findLastModificationDateTime(quint64 id, bool *ok)
{
    if (!isValid() || !id)
        return bRet(ok, false, QDateTime());
    BSqlResult result = Source->select("labs", "last_modification_date_time", BSqlWhere("id = :id", ":id", id));
    if (!result.success())
        return bRet(ok, false, QDateTime());
    if (result.values().isEmpty())
        return bRet(ok, true, QDateTime());
    QDateTime dt;
    dt.setTimeSpec(Qt::UTC);
    dt.setMSecsSinceEpoch(result.value("last_modification_date_time").toLongLong());
    return bRet(ok, true, dt);
}

Lab LabRepository::findOne(quint64 id, bool *ok)
{
    Lab entity(this);
    if (!isValid() || !id)
        return bRet(ok, false, entity);
    static const QStringList Fields = QStringList() << "sender_id" << "creation_date_time" << "description"
        << "last_modification_date_time" << "title" << "type" << "data_infos" << "extra_file_infos";
    BSqlResult result = Source->select("labs", Fields, BSqlWhere("id = :id", ":id", id));
    if (!result.success())
        return bRet(ok, false, entity);
    if (result.value().isEmpty())
        return bRet(ok, true, entity);
    entity.mid = id;
    entity.msenderId = result.value("sender_id").toULongLong();
    entity.mcreationDateTime.setMSecsSinceEpoch(result.value("creation_date_time").toLongLong());
    entity.mdescription = result.value("description").toString();
    entity.mlastModificationDateTime.setMSecsSinceEpoch(result.value("last_modification_date_time").toLongLong());
    entity.mtitle = result.value("title").toString();
    entity.mtype = result.value("type").toInt();
    entity.mdataInfoList = deserializedDataInfos(result.value("data_infos").toByteArray());
    entity.mextraFileInfos = deserializedExtraFileInfos(result.value("extra_file_infos").toByteArray());
    bool b = false;
    entity.mauthors = RepositoryTools::getAuthorInfoList(Source, "lab_authors", "lab_id", id, &b);
    if (!b)
        return bRet(ok, false, entity);
    entity.mgroups = RepositoryTools::getGroupIdList(Source, "lab_groups", "lab_id", id, &b);
    if (!b)
        return bRet(ok, false, entity);
    entity.mtags = RepositoryTools::getTags(Source, "lab_tags", "lab_id", id, &b);
    if (!b)
        return bRet(ok, false, entity);
    entity.valid = true;
    return bRet(ok, true, entity);
}

bool LabRepository::isValid() const
{
    return Source && Source->isValid();
}

/*============================== Static private methods ====================*/

TLabDataInfoList LabRepository::deserializedDataInfos(const QByteArray &data)
{
    return BeQt::deserialize(data).value<TLabDataInfoList>();
}

TFileInfoList LabRepository::deserializedExtraFileInfos(const QByteArray &data)
{
    return BeQt::deserialize(data).value<TFileInfoList>();
}

QByteArray LabRepository::serializedDataInfos(const TLabDataList &list, const TLabType &type)
{
    TLabDataInfoList infos;
    foreach (const TLabData &data, list) {
        TLabDataInfo info;
        info.setOs(data.os());
        info.setSize(data.size());
        info.setType(type);
        infos << info;
    }
    return BeQt::serialize(infos);
}

QByteArray LabRepository::serializedExtraFileInfos(const TBinaryFileList &list, const TFileInfoList &previous)
{
    TFileInfoList infos;
    foreach (const TBinaryFile &f, list) {
        TFileInfo info;
        info.setFileName(f.fileName());
        info.setFileSize(f.size());
        info.setDescription(f.description());
        infos << info;
    }
    infos << previous;
    return BeQt::serialize(infos);
}

/*============================== Private methods ===========================*/

bool LabRepository::createData(quint64 labId, const TLabDataList &list)
{
    if (!isValid() || !labId)
        return false;
    foreach (const TLabData &data, list) {
        BSqlResult result;
        switch (data.type()) {
        case TLabType::DesktopApplication: {
            QByteArray ba = BeQt::serialize(data.application());
            QVariantMap values;
            values.insert("lab_id", labId);
            values.insert("os_type", int(data.os()));
            values.insert("data", ba);
            result = Source->insert("lab_desktop_applications", values);
            break;
        }
        case TLabType::Url: {
            result = Source->insert("lab_urls", "lab_id", labId, "url", data.url());
            break;
        }
        case TLabType::WebApplication: {
            QByteArray ba = BeQt::serialize(data.application());
            result = Source->insert("lab_web_applications", "lab_id", labId, "data", ba);
            break;
        }
        case TLabType::NoType:
        default: {
            return false;
        }
        }
        if (!result.success())
            return false;
    }
    return true;
}

bool LabRepository::createExtraFiles(quint64 labId, const TBinaryFileList &files)
{
    if (!isValid() || !labId)
        return false;
    foreach (const TBinaryFile &f, files) {
        if (!f.isValid())
            return false;
        QVariantMap values;
        values.insert("lab_id", labId);
        values.insert("file_name", f.fileName());
        values.insert("data", f.data());
        values.insert("description", f.description());
        if (!Source->insert("lab_extra_files", values).success())
            return false;
    }
    return true;
}

bool LabRepository::deleteExtraFiles(quint64 labId, const QStringList &fileNames)
{
    if (!isValid() || !labId)
        return false;
    foreach (const QString &fn, fileNames) {
        if (fn.isEmpty())
            return false;
        BSqlWhere where("lab_id = :lab_id AND file_name = :file_name", ":lab_id", labId, ":file_name", fn);
        if (!Source->deleteFrom("lab_extra_files", where).success())
            return false;
    }
    return true;
}

TLabData LabRepository::fetchData(quint64 labId, const TLabType &type, BeQt::OSType os, bool *ok)
{
    if (!isValid() || !labId || !type.isValid())
        return bRet(ok, false, TLabData());
    switch (type) {
    case TLabType::DesktopApplication: {
        if (BeQt::UnknownOS == os || BeQt::UnixOS == os)
            return bRet(ok, false, TLabData());
        BSqlWhere where("lab_id = :lab_id AND os_type = :os_type", ":lab_id", labId, ":os_type", int(os));
        BSqlResult result = Source->select("lab_desktop_applications", "data", where);
        if (!result.success() || result.values().isEmpty())
            return bRet(ok, false, TLabData());
        TLabData data;
        TLabApplication app = BeQt::deserialize(result.value("data").toByteArray()).value<TLabApplication>();
        data.setApplication(app);
        return bRet(ok, true, data);
    }
    case TLabType::Url: {
        BSqlResult result = Source->select("lab_urls", "url", BSqlWhere("lab_id = :lab_id", ":lab_id", labId));
        if (!result.success() || result.values().isEmpty())
            return bRet(ok, false, TLabData());
        TLabData data;
        data.setUrl(result.value("url").toString());
        return bRet(ok, true, data);
    }
    case TLabType::WebApplication: {
        BSqlWhere where("lab_id = :lab_id", ":lab_id", labId);
        BSqlResult result = Source->select("lab_web_applications", "data", where);
        if (!result.success() || result.values().isEmpty())
            return bRet(ok, false, TLabData());
        TLabData data;
        TLabApplication app = BeQt::deserialize(result.value("data").toByteArray()).value<TLabApplication>();
        data.setApplication(app);
        return bRet(ok, true, data);
    }
    case TLabType::NoType:
    default: {
        return bRet(ok, false, TLabData());
    }
    }
}

TBinaryFile LabRepository::fetchExtraFile(quint64 labId, const QString &fileName, bool *ok)
{
    if (!isValid() || !labId || fileName.isEmpty())
        return bRet(ok, false, TBinaryFile());
    static const QStringList Fields = QStringList() << "data" << "description";
    BSqlWhere where("lab_id = :lab_id AND file_name = :file_name", ":lab_id", labId, ":file_name", fileName);
    BSqlResult result = Source->select("lab_extra_files", Fields, where);
    if (!result.success() || result.values().isEmpty())
        return bRet(ok, false, TBinaryFile());
    TBinaryFile f;
    f.setFileName(fileName);
    f.setData(result.value("data").toByteArray());
    f.setDescription(result.value("description").toString());
    return bRet(ok, true, f);
}

bool LabRepository::updateData(quint64 labId, const TLabDataList &list)
{
    if (!isValid() || !labId)
        return false;
    static const QStringList Tables = QStringList() << "lab_desktop_applications" << "lab_web_applications"
                                                    << "lab_urls";
    if (!RepositoryTools::deleteHelper(Source, Tables, "lab_id", labId))
        return false;
    return createData(labId, list);
}
