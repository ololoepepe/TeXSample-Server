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

#include "invitecoderepository.h"

#include "datasource.h"
#include "entity/invitecode.h"
#include "repositorytools.h"

#include <TAccessLevel>
#include <TIdList>
#include <TService>
#include <TServiceList>

#include <BSqlResult>
#include <BSqlWhere>
#include <BUuid>

#include <QDateTime>
#include <QDebug>
#include <QList>
#include <QString>
#include <QStringList>

/*============================================================================
================================ InviteCodeRepository ========================
============================================================================*/

/*============================== Public constructors =======================*/

InviteCodeRepository::InviteCodeRepository(DataSource *source) :
    Source(source)
{
    //
}

InviteCodeRepository::~InviteCodeRepository()
{
    //
}

/*============================== Public methods ============================*/

quint64 InviteCodeRepository::add(const InviteCode &entity, bool *ok)
{
    if (!isValid() || !entity.isValid() || entity.isCreatedByRepo())
        return bRet(ok, false, 0);
    QDateTime dt = QDateTime::currentDateTimeUtc();
    QVariantMap values;
    values.insert("owner_id", entity.ownerId());
    values.insert("access_level", int(entity.accessLevel()));
    values.insert("code", entity.code().toString(true));
    values.insert("creation_date_time", dt.toMSecsSinceEpoch());
    values.insert("expiration_date_time", entity.expirationDateTime().toUTC().toMSecsSinceEpoch());
    BSqlResult result = Source->insert("invite_codes", values);
    if (!result.success())
        return bRet(ok, false, 0);
    quint64 id = result.lastInsertId().toULongLong();
    if (!RepositoryTools::setGroupIdList(Source, "invite_code_groups", "invite_code_id", id, entity.groups()))
        return bRet(ok, false, 0);
    TServiceList services = entity.availableServices();
    if (!RepositoryTools::setServices(Source, "invite_code_services", "invite_code_id", id, services))
        return bRet(ok, false, 0);
    return bRet(ok, true, id);
}

DataSource *InviteCodeRepository::dataSource() const
{
    return Source;
}

void InviteCodeRepository::deleteExpired(bool *ok)
{
    if (!isValid())
        return bSet(ok, false);
    QDateTime dt = QDateTime::currentDateTimeUtc();
    BSqlWhere where("expiration_date_time <= :date_time", ":date_time", dt.toMSecsSinceEpoch());
    bSet(ok, Source->deleteFrom("invite_codes", where).success());
}

void InviteCodeRepository::deleteOne(quint64 id, bool *ok)
{
    if (!id)
        return bSet(ok, false);
    TIdList list;
    list << id;
    deleteSome(list, ok);
}

void InviteCodeRepository::deleteSome(const TIdList &ids, bool *ok)
{
    if (!isValid())
        return bSet(ok, false);
    if (ids.isEmpty())
        return bSet(ok, true);
    QString ws = "id IN (";
    QVariantMap values;
    foreach (int i, bRangeD(0, ids.size() - 1)) {
        ws += ":id" + QString::number(i);
        if (i < ids.size() - 1)
            ws += ", ";
        values.insert(":id" + QString::number(i), ids.at(i));
    }
    ws += ")";
    bSet(ok, Source->deleteFrom("invite_codes", BSqlWhere(ws, values)).success());
}

QList<InviteCode> InviteCodeRepository::findAll(bool *ok)
{
    QList<InviteCode> list;
    if (!isValid())
        return bRet(ok, false, list);
    static const QStringList Fields = QStringList() << "id" << "access_level" << "code" << "creation_date_time"
                                                    << "expiration_date_time" << "owner_id";
    BSqlResult result = Source->select("invite_codes", Fields);
    if (!result.success())
        return bRet(ok, false, list);
    foreach (const QVariantMap &m, result.values()) {
        quint64 id = m.value("id").toULongLong();
        InviteCode entity(this);
        entity.mid = id;
        entity.mownerId = m.value("owner_id").toULongLong();
        entity.maccessLevel = m.value("access_level").toInt();
        entity.mcode = BUuid(m.value("code").toString());
        entity.mcreationDateTime.setMSecsSinceEpoch(m.value("creation_date_time").toLongLong());
        entity.mexpirationDateTime.setMSecsSinceEpoch(m.value("expiration_date_time").toLongLong());
        bool b = false;
        entity.mgroups = RepositoryTools::getGroupIdList(Source, "invite_code_groups", "invite_code_id", id, &b);
        if (!b)
            return bRet(ok, false, QList<InviteCode>());
        entity.mavailableServices = RepositoryTools::getServices(Source, "invite_code_services", "invite_code_id", id,
                                                                 &b);
        if (!b)
            return bRet(ok, false, QList<InviteCode>());
        entity.valid = true;
        list << entity;
    }
    return bRet(ok, true, list);
}

QList<InviteCode> InviteCodeRepository::findAllByOwnerId(quint64 ownerId, bool *ok)
{
    QList<InviteCode> list;
    if (!isValid() || !ownerId)
        return bRet(ok, false, list);
    static const QStringList Fields = QStringList() << "id" << "access_level" << "code" << "creation_date_time"
                                                    << "expiration_date_time";
    BSqlWhere where("owner_id = :owner_id", ":owner_id", ownerId);
    BSqlResult result = Source->select("invite_codes", Fields, where);
    if (!result.success())
        return bRet(ok, false, list);
    foreach (const QVariantMap &m, result.values()) {
        quint64 id = m.value("id").toULongLong();
        InviteCode entity(this);
        entity.mid = id;
        entity.mownerId = ownerId;
        entity.maccessLevel = m.value("access_level").toInt();
        entity.mcode = BUuid(m.value("code").toString());
        entity.mcreationDateTime.setMSecsSinceEpoch(m.value("creation_date_time").toLongLong());
        entity.mexpirationDateTime.setMSecsSinceEpoch(m.value("expiration_date_time").toLongLong());
        bool b = false;
        entity.mgroups = RepositoryTools::getGroupIdList(Source, "invite_code_groups", "invite_code_id", id, &b);
        if (!b)
            return bRet(ok, false, QList<InviteCode>());
        entity.mavailableServices = RepositoryTools::getServices(Source, "invite_code_services", "invite_code_id", id,
                                                                 &b);
        if (!b)
            return bRet(ok, false, QList<InviteCode>());
        entity.valid = true;
        list << entity;
    }
    return bRet(ok, true, list);
}

InviteCode InviteCodeRepository::findOne(quint64 id, bool *ok)
{
    InviteCode entity(this);
    if (!isValid() || !id)
        return bRet(ok, false, entity);
    static const QStringList Fields = QStringList() << "owner_id" << "access_level" << "code" << "creation_date_time"
                                                    << "expiration_date_time";
    BSqlResult result = Source->select("invite_codes", Fields, BSqlWhere("id = :id", ":id", id));
    if (!result.success())
        return bRet(ok, false, entity);
    if (result.values().isEmpty())
        return bRet(ok, true, entity);
    entity.mid = id;
    entity.mownerId = result.value("owner_id").toULongLong();
    entity.maccessLevel = result.value("access_level").toInt();
    entity.mcreationDateTime.setMSecsSinceEpoch(result.value("creation_date_time").toLongLong());
    entity.mexpirationDateTime.setMSecsSinceEpoch(result.value("expiration_date_time").toLongLong());
    entity.mcode = BUuid(result.value("code").toString());
    bool b = false;
    entity.mgroups = RepositoryTools::getGroupIdList(Source, "invite_code_groups", "invite_code_id", entity.id(), &b);
    if (!b)
        return bRet(ok, false, entity);
    entity.mavailableServices = RepositoryTools::getServices(Source, "invite_code_services", "invite_code_id",
                                                             entity.id(), &b);
    if (!b)
        return bRet(ok, false, entity);
    entity.valid = true;
    return bRet(ok, true, entity);
}

InviteCode InviteCodeRepository::findOneByCode(const BUuid &code, bool *ok)
{
    InviteCode entity(this);
    if (!isValid() || code.isNull())
        return bRet(ok, false, entity);
    static const QStringList Fields = QStringList() << "id" << "owner_id" << "access_level" << "creation_date_time"
                                                    << "expiration_date_time";
    BSqlWhere where("code = :code", ":code", code.toString(true));
    BSqlResult result = Source->select("invite_codes", Fields, where);
    if (!result.success())
        return bRet(ok, false, entity);
    if (result.values().isEmpty())
        return bRet(ok, true, entity);
    entity.mid = result.value("id").toULongLong();
    entity.mownerId = result.value("owner_id").toULongLong();
    entity.maccessLevel = result.value("access_level").toInt();
    entity.mcreationDateTime.setMSecsSinceEpoch(result.value("creation_date_time").toLongLong());
    entity.mexpirationDateTime.setMSecsSinceEpoch(result.value("expiration_date_time").toLongLong());
    entity.mcode = code;
    bool b = false;
    entity.mgroups = RepositoryTools::getGroupIdList(Source, "invite_code_groups", "invite_code_id", entity.id(), &b);
    if (!b)
        return bRet(ok, false, entity);
    entity.mavailableServices = RepositoryTools::getServices(Source, "invite_code_services", "invite_code_id",
                                                             entity.id(), &b);
    if (!b)
        return bRet(ok, false, entity);
    entity.valid = true;
    return bRet(ok, true, entity);
}

bool InviteCodeRepository::isValid() const
{
    return Source && Source->isValid();
}
