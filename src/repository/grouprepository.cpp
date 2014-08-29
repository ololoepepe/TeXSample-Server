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

#include "grouprepository.h"

#include "datasource.h"
#include "entity/group.h"

#include <TIdList>

#include <BSqlResult>
#include <BSqlWhere>

#include <QDateTime>
#include <QDebug>
#include <QList>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <QVariantList>

/*============================================================================
================================ GroupRepository =============================
============================================================================*/

/*============================== Public constructors =======================*/

GroupRepository::GroupRepository(DataSource *source) :
    Source(source)
{
    //
}

GroupRepository::~GroupRepository()
{
    //
}

/*============================== Public methods ============================*/

quint64 GroupRepository::add(const Group &entity, bool *ok)
{
    if (!isValid() || !entity.isValid() || entity.isCreatedByRepo() || entity.id())
        return bRet(ok, false, 0);
    QDateTime dt = QDateTime::currentDateTimeUtc();
    QVariantMap values;
    values.insert("owner_id", entity.ownerId());
    values.insert("creation_date_time", dt.toMSecsSinceEpoch());
    values.insert("last_modification_date_time", dt.toMSecsSinceEpoch());
    values.insert("name", entity.name());
    BSqlResult result = Source->insert("groups", values);
    if (!result.success())
        return bRet(ok, false, 0);
    return bRet(ok, true, result.lastInsertId().toULongLong());
}

DataSource *GroupRepository::dataSource() const
{
    return Source;
}

QDateTime GroupRepository::deleteOne(quint64 id, bool *ok)
{
    if (!isValid() || !id)
        return bRet(ok, false, QDateTime());
    QDateTime dt = QDateTime::currentDateTimeUtc();
    BSqlWhere where("id = :id", ":id", id);
    BSqlResult result = Source->select("groups", "owner_id", where);
    if (!result.success() || result.values().isEmpty())
        return bRet(ok, false, QDateTime());
    if (!Source->deleteFrom("groups", where).success())
        return bRet(ok, false, QDateTime());
    QVariantMap values;
    values.insert("id", id);
    values.insert("owner_id", result.value("owner_id").toULongLong());
    values.insert("deletion_date_time", dt.toMSecsSinceEpoch());
    if (!Source->insert("deleted_groups", values))
        return bRet(ok, false, QDateTime());
    return bRet(ok, true, dt);
}

void GroupRepository::edit(const Group &entity, bool *ok)
{
    if (!isValid() || !entity.isValid() || !entity.id() || entity.isCreatedByRepo())
        return bSet(ok, false);
    QVariantMap values;
    values.insert("owner_id", entity.ownerId());
    values.insert("last_modification_date_time", entity.lastModificationDateTime().toUTC().toMSecsSinceEpoch());
    values.insert("name", entity.name());
    bSet(ok, Source->update("groups", values, BSqlWhere("id = :id", ":id", entity.id())).success());
}

QList<Group> GroupRepository::findAll(const QDateTime &newerThan, bool *ok)
{
    QList<Group> list;
    if (!isValid())
        return bRet(ok, false, list);
    static const QStringList Fields = QStringList() << "id" << "owner_id" << "creation_date_time"
                                                    << "last_modification_date_time" << "name";
    BSqlWhere where;
    if (newerThan.isValid()) {
        where = BSqlWhere("last_modification_date_time > :last_modification_date_time", ":last_modification_date_time",
                          newerThan.toUTC().toMSecsSinceEpoch());
    }
    BSqlResult result = Source->select("groups", Fields, where);
    if (!result.success())
        return bRet(ok, false, list);
    foreach (const QVariantMap &m, result.values()) {
        Group entity(this);
        entity.mid = m.value("id").toULongLong();
        entity.mownerId = m.value("owner_id").toULongLong();
        entity.mcreationDateTime.setMSecsSinceEpoch(m.value("creation_date_time").toLongLong());
        entity.mlastModificationDateTime.setMSecsSinceEpoch(m.value("last_modification_date_time").toLongLong());
        entity.mname = m.value("name").toString();
        entity.valid = true;
        list << entity;
    }
    return bRet(ok, true, list);
}

QList<Group> GroupRepository::findAll(const TIdList &ids, bool *ok)
{
    QList<Group> list;
    if (!isValid())
        return bRet(ok, false, list);
    if (ids.isEmpty())
        return bRet(ok, true, list);
    static const QStringList Fields = QStringList() << "id" << "owner_id" << "creation_date_time"
                                                    << "last_modification_date_time" << "name";
    QString ws = "id IN (";
    QVariantMap values;
    foreach (int i, bRangeD(0, ids.size() - 1)) {
        ws += ":id" + QString::number(i);
        if (i < ids.size() - 1)
            ws += ", ";
        values.insert(":id" + QString::number(i), ids.at(i));
    }
    ws += ")";
    BSqlWhere where(ws, values);
    BSqlResult result = Source->select("groups", Fields, where);
    if (!result.success())
        return bRet(ok, false, list);
    foreach (const QVariantMap &m, result.values()) {
        Group entity(this);
        entity.mid = m.value("id").toULongLong();
        entity.mownerId = m.value("owner_id").toULongLong();
        entity.mcreationDateTime.setMSecsSinceEpoch(m.value("creation_date_time").toLongLong());
        entity.mlastModificationDateTime.setMSecsSinceEpoch(m.value("last_modification_date_time").toLongLong());
        entity.mname = m.value("name").toString();
        entity.valid = true;
        list << entity;
    }
    return bRet(ok, true, list);
}

QList<Group> GroupRepository::findAllByUserId(quint64 userId, const QDateTime &newerThan, bool *ok)
{
    QList<Group> list;
    if (!isValid() || !userId)
        return bRet(ok, false, list);
    static const QStringList Fields = QStringList() << "id" << "creation_date_time" << "last_modification_date_time"
                                                    << "name";
    QString ws = "owner_id = :owner_id";
    QVariantMap wvalues;
    wvalues.insert(":owner_id", userId);
    if (newerThan.isValid()) {
        ws += " AND last_modification_date_time > :last_modification_date_time";
        wvalues.insert("last_modification_date_time", newerThan.toUTC().toMSecsSinceEpoch());
    }
    BSqlResult result = Source->select("groups", Fields, BSqlWhere(ws, wvalues));
    if (!result.success())
        return bRet(ok, false, list);
    foreach (const QVariantMap &m, result.values()) {
        Group entity(this);
        entity.mid = m.value("id").toULongLong();
        entity.mownerId = userId;
        entity.mcreationDateTime.setMSecsSinceEpoch(m.value("creation_date_time").toLongLong());
        entity.mlastModificationDateTime.setMSecsSinceEpoch(m.value("last_modification_date_time").toLongLong());
        entity.mname = m.value("name").toString();
        entity.valid = true;
        list << entity;
    }
    return bRet(ok, true, list);
}

TIdList GroupRepository::findAllDeleted(const QDateTime &newerThan, bool *ok)
{
    TIdList list;
    if (!isValid())
        return bRet(ok, false, list);
    BSqlWhere where;
    if (newerThan.isValid()) {
        where = BSqlWhere("deletion_date_time > :deletion_date_time", ":deletion_date_time",
                          newerThan.toUTC().toMSecsSinceEpoch());
    }
    BSqlResult result = Source->select("deleted_groups", "id", where);
    if (!result.success())
        return bRet(ok, false, list);
    foreach (const QVariantMap &m, result.values())
        list << m.value("id").toULongLong();
    return bRet(ok, true, list);
}

TIdList GroupRepository::findAllDeletedByUserId(quint64 userId, const QDateTime &newerThan, bool *ok)
{
    TIdList list;
    if (!isValid() || !userId)
        return bRet(ok, false, list);
    QString ws = "owner_id = :owner_id";
    QVariantMap wvalues;
    wvalues.insert(":owner_id", userId);
    BSqlWhere where;
    if (newerThan.isValid()) {
        ws += " AND deletion_date_time > :deletion_date_time";
        wvalues.insert(":deletion_date_time", newerThan.toUTC().toMSecsSinceEpoch());
    }
    BSqlResult result = Source->select("deleted_invites", "id", where);
    if (!result.success())
        return bRet(ok, false, list);
    foreach (const QVariantMap &m, result.values())
        list << m.value("id").toULongLong();
    return bRet(ok, true, list);
}

Group GroupRepository::findOne(quint64 id, bool *ok)
{
    Group entity(this);
    if (!isValid() || !id)
        return bRet(ok, false, entity);
    static const QStringList Fields = QStringList() << "creation_date_time" << "last_modification_date_time"
                                                    << "owner_id" << "name";
    BSqlResult result = Source->select("groups", Fields, BSqlWhere("id = :id", ":id", id));
    if (!result.success())
        return bRet(ok, false, entity);
    if (result.value().isEmpty())
        return bRet(ok, true, entity);
    entity.mid = id;
    entity.mownerId = result.value("owner_id").toULongLong();
    entity.mcreationDateTime.setMSecsSinceEpoch(result.value("creation_date_time").toLongLong());
    entity.mlastModificationDateTime.setMSecsSinceEpoch(result.value("last_modification_date_time").toLongLong());
    entity.mname = result.value("name").toString();
    entity.valid = true;
    return bRet(ok, true, entity);
}

bool GroupRepository::isValid() const
{
    return Source && Source->isValid();
}
