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
#include <TIdList>
#include <TTexProject>

#include <BSqlResult>
#include <BSqlWhere>

#include <QList>
#include <QString>
#include <QStringList>

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

quint64 LabRepository::add(const Lab &entity)
{
    if (!isValid() || !entity.isValid() || entity.isCreatedByRepo() || entity.id())
        return 0;
    QDateTime dt = QDateTime::currentDateTimeUtc();
    QVariantMap values;
    values.insert("sender_id", entity.senderId());
    values.insert("deleted", false);
    values.insert("creation_date_time", dt.toMSecsSinceEpoch());
    values.insert("description", entity.description());
    values.insert("last_modification_date_time", dt.toMSecsSinceEpoch());
    values.insert("title", entity.title());
    values.insert("type", int(entity.type()));
    BSqlResult result = Source->insert("labs", values);
    if (!result.success())
        return 0;
    quint64 id = result.lastInsertId().toULongLong();
    if (!RepositoryTools::setAuthorInfoList(Source, "lab_authors", "lab_id", id, entity.authors()))
        return 0;
    if (!RepositoryTools::setGroupIdList(Source, "lab_groups", "lab_id", id, entity.groups()))
        return 0;
    if (!RepositoryTools::setTags(Source, "lab_tags", "lab_id", id, entity.tags()))
        return 0;
    if (!createData(id, entity.labDataList()) || !createExtraFiles(id, entity.extraFiles()))
        return 0;
    return id;
}

DataSource *LabRepository::dataSource() const
{
    return Source;
}

bool LabRepository::edit(const Lab &entity)
{
    if (!isValid() || !entity.isValid() || !entity.id() || entity.isCreatedByRepo())
        return false;
    QDateTime dt = QDateTime::currentDateTimeUtc();
    QVariantMap values;
    values.insert("sender_id", entity.senderId());
    values.insert("deleted", false);
    values.insert("description", entity.description());
    values.insert("last_modification_date_time", dt.toMSecsSinceEpoch());
    values.insert("title", entity.title());
    values.insert("type", int(entity.type()));
    BSqlResult result = Source->update("labs", values, BSqlWhere("id = :id", ":id", entity.id()));
    if (!result.success())
        return false;
    if (!RepositoryTools::deleteHelper(Source, QStringList() << "lab_authors" << "lab_groups" << "lab_tags", "lab_id",
                                       entity.id())) {
        return false;
    }
    if (!RepositoryTools::setAuthorInfoList(Source, "lab_authors", "lab_id", entity.id(), entity.authors()))
        return false;
    if (!RepositoryTools::setGroupIdList(Source, "lab_groups", "lab_id", entity.id(), entity.groups()))
        return false;
    if (!RepositoryTools::setTags(Source, "lab_tags", "lab_id", entity.id(), entity.tags()))
        return false;
    if (entity.saveData() && !updateData(entity.id(), entity.labDataList()))
        return false;
    if (!deleteExtraFiles(entity.id(), entity.deletedExtraFiles()))
        return false;
    if (!createExtraFiles(entity.id(), entity.extraFiles()))
        return false;
    return true;
}

QList<Lab> LabRepository::findAllNewerThan(const TIdList &groups)
{
    return findAllNewerThan(QDateTime(), groups);
}

QList<Lab> LabRepository::findAllNewerThan(const QDateTime &newerThan, const TIdList &groups)
{
    QList<Lab> list;
    if (!isValid())
        return list;
    QString qs = "SELECT labs.id, labs.sender_id, labs.deleted, labs.creation_date_time, labs.description, "
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
    if (!result.success() || result.values().isEmpty())
        return list;
    foreach (const QVariantMap &m, result.values()) {
        Lab entity(this);
        entity.mid = m.value("labs.id").toULongLong();
        entity.msenderId = m.value("labs.sender_id").toULongLong();
        entity.mdeleted = m.value("labs.deleted").toBool();
        entity.mcreationDateTime.setMSecsSinceEpoch(m.value("labs.creation_date_time").toLongLong());
        entity.mdescription = m.value("labs.description").toString();
        entity.mlastModificationDateTime.setMSecsSinceEpoch(m.value("labs.last_modification_date_time").toLongLong());
        entity.mtitle = m.value("labs.title").toString();
        entity.mtype = m.value("labs.type").toInt();
        bool ok = false;
        entity.mauthors = RepositoryTools::getAuthorInfoList(Source, "lab_authors", "lab_id", entity.id(), &ok);
        if (!ok)
            return QList<Lab>();
        entity.mgroups = RepositoryTools::getGroupIdList(Source, "lab_groups", "lab_id", entity.id(), &ok);
        if (!ok)
            return QList<Lab>();
        entity.mtags = RepositoryTools::getTags(Source, "lab_tags", "lab_id", entity.id(), &ok);
        if (!ok)
            return QList<Lab>();
        entity.valid = true;
        list << entity;
    }
    return list;
}

Lab LabRepository::findOne(quint64 id)
{
    Lab entity(this);
    if (!isValid() || !id)
        return entity;
    BSqlResult result = Source->select("labs", QStringList() << "sender_id" << "deleted" << "creation_date_time"
                                       << "description" << "last_modification_date_time" << "title" << "type",
                                       BSqlWhere("id = :id", ":id", id));
    if (!result.success() || result.value().isEmpty())
        return entity;
    entity.mid = id;
    entity.msenderId = result.value("sender_id").toULongLong();
    entity.mdeleted = result.value("deleted").toBool();
    entity.mcreationDateTime.setMSecsSinceEpoch(result.value("creation_date_time").toLongLong());
    entity.mdescription = result.value("description").toString();
    entity.mlastModificationDateTime.setMSecsSinceEpoch(result.value("last_modification_date_time").toLongLong());
    entity.mtitle = result.value("title").toString();
    entity.mtype = result.value("type").toInt();
    bool ok = false;
    entity.mauthors = RepositoryTools::getAuthorInfoList(Source, "lab_authors", "lab_id", id, &ok);
    if (!ok)
        return entity;
    entity.mgroups = RepositoryTools::getGroupIdList(Source, "lab_groups", "lab_id", id, &ok);
    if (!ok)
        return entity;
    entity.mtags = RepositoryTools::getTags(Source, "lab_tags", "lab_id", id, &ok);
    if (!ok)
        return entity;
    entity.valid = true;
    return entity;
}

bool LabRepository::isValid() const
{
    return Source && Source->isValid();
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
