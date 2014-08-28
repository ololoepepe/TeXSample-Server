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
    if (!Source->deleteFrom("labs", BSqlWhere("id = :id", ":id", id)).success())
        return bRet(ok, false, QDateTime());
    if (!Source->insert("deleted_labs", "id", id, "deletion_date_time", dt.toMSecsSinceEpoch()).success())
        return bRet(ok, false, QDateTime());
    return bRet(ok, true, dt);
}

void LabRepository::edit(const Lab &entity, bool *ok)
{
    if (!isValid() || !entity.isValid() || !entity.id() || entity.isCreatedByRepo())
        return bSet(ok, false);
    QDateTime dt = QDateTime::currentDateTimeUtc();
    QVariantMap values;
    values.insert("sender_id", entity.senderId());
    values.insert("description", entity.description());
    values.insert("last_modification_date_time", dt.toMSecsSinceEpoch());
    values.insert("title", entity.title());
    values.insert("type", int(entity.type()));
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
        "labs.last_modification_date_time, labs.title, labs.type FROM labs";
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

Lab LabRepository::findOne(quint64 id, bool *ok)
{
    Lab entity(this);
    if (!isValid() || !id)
        return bRet(ok, false, entity);
    static const QStringList Fields = QStringList() << "sender_id" << "creation_date_time" << "description"
                                                    << "last_modification_date_time" << "title" << "type";
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

QByteArray LabRepository::serializedExtraFileInfos(const TBinaryFileList &list)
{
    TFileInfoList infos;
    foreach (const TBinaryFile &f, list) {
        TFileInfo info;
        info.setFileName(f.fileName());
        info.setFileSize(f.size());
        info.setDescription(f.description());
        infos << info;
    }
    return BeQt::serialize(infos);
}

/*============================== Private methods ===========================*/

//fetch

bool LabRepository::createData(quint64 labId, const TLabDataList &data)
{
    //
}

bool LabRepository::createExtraFiles(quint64 labId, const TBinaryFileList &files)
{
    //
}

bool LabRepository::deleteExtraFiles(quint64 labId, const QStringList &fileNames)
{
    //
}

bool LabRepository::updateData(quint64 labId, const TLabDataList &data)
{
    //
}
