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

#include "userrepository.h"

#include "datasource.h"
#include "entity/user.h"
#include "repositorytools.h"

#include <TAccessLevel>
#include <TIdList>
#include <TUserIdentifier>

#include <BSqlResult>
#include <BSqlWhere>
#include <BTextTools>

#include <QBuffer>
#include <QByteArray>
#include <QDateTime>
#include <QDebug>
#include <QImage>
#include <QList>
#include <QString>
#include <QVariant>
#include <QVariantMap>

/*============================================================================
================================ UserRepository ==============================
============================================================================*/

/*============================== Public constructors =======================*/

UserRepository::UserRepository(DataSource *source) :
    Source(source)
{
    //
}

UserRepository::~UserRepository()
{
    //
}

/*============================== Public methods ============================*/

quint64 UserRepository::add(const User &entity, bool *ok)
{
    if (!isValid() || !entity.isValid() || entity.isCreatedByRepo() || entity.id())
        return bRet(ok, false, 0);
    QDateTime dt = QDateTime::currentDateTimeUtc();
    QVariantMap values;
    values.insert("access_level", int(entity.accessLevel()));
    values.insert("active", int(entity.active()));
    values.insert("email", entity.email());
    values.insert("last_modification_date_time", dt.toMSecsSinceEpoch());
    values.insert("login", entity.login());
    values.insert("name", entity.name());
    values.insert("password", entity.password());
    values.insert("patronymic", entity.patronymic());
    values.insert("registration_date_time", dt.toMSecsSinceEpoch());
    values.insert("surname", entity.surname());
    BSqlResult result = Source->insert("users", values);
    if (!result.success())
        return bRet(ok, false, 0);
    quint64 id = result.lastInsertId().toULongLong();
    if (!RepositoryTools::setGroupIdList(Source, "user_groups", "user_id", id, entity.groups()))
        return bRet(ok, false, 0);
    if (!RepositoryTools::setServices(Source, "user_services", "user_id", id, entity.availableServices()))
        return bRet(ok, false, 0);
    if (!createAvatar(id, entity.avatar()))
        return bRet(ok, false, 0);
    return bRet(ok, true, id);
}

long UserRepository::countByAccessLevel(const TAccessLevel &accessLevel, bool *ok)
{
    if (!isValid())
        return bRet(ok, false, 0);
    BSqlResult result = Source->select("users", "COUNT(id)",
                                       BSqlWhere("access_level = :access_level", ":access_level", int(accessLevel)));
    if (!result.success())
        return bRet(ok, false, 0);
    return bRet(ok, true, result.value("COUNT(id)").toLongLong());
}

DataSource *UserRepository::dataSource() const
{
    return Source;
}

QDateTime UserRepository::deleteOne(const TUserIdentifier &id, bool *ok)
{
    if (!isValid() || !id.isValid())
        return bRet(ok, false, QDateTime());
    QDateTime dt = QDateTime::currentDateTimeUtc();
    quint64 userId = 0;
    BSqlResult result;
    bool b = false;
    switch (id.type()) {
    case TUserIdentifier::IdType: {
        userId = id.id();
        result = Source->deleteFrom("users", BSqlWhere("id = :id", ":id", id.id()));
        break;
    }
    case TUserIdentifier::LoginType: {
        User entity = findOne(id, &b);
        if (!b)
            return bRet(ok, false, QDateTime());
        userId = entity.id();
        result = Source->deleteFrom("users", BSqlWhere("login = :login", ":login", id.login()));
        break;
    }
    default: {
        break;
    }
    }
    if (!result.success())
        return bRet(ok, false, QDateTime());
    if (!Source->insert("deleted_users", "id", userId, "deletion_date_time", dt.toMSecsSinceEpoch()).success())
        return bRet(ok, false, QDateTime());
    return bRet(ok, true, dt);
}

QDateTime UserRepository::edit(const User &entity, bool *ok)
{
    if (!isValid() || !entity.isValid() || entity.isCreatedByRepo() || !entity.id())
        return bRet(ok, false, QDateTime());
    QDateTime dt = QDateTime::currentDateTimeUtc();
    QVariantMap values;
    values.insert("access_level", int(entity.accessLevel()));
    values.insert("active", int(entity.active()));
    values.insert("email", entity.email());
    values.insert("last_modification_date_time", dt.toMSecsSinceEpoch());
    values.insert("login", entity.login());
    values.insert("name", entity.name());
    values.insert("password", entity.password());
    values.insert("patronymic", entity.patronymic());
    values.insert("surname", entity.surname());
    BSqlResult result = Source->update("users", values, BSqlWhere("id = :id", ":id", entity.id()));
    if (!result.success())
        return bRet(ok, false, QDateTime());
    static const QStringList Tables = QStringList() << "user_groups" << "user_services";
    if (!RepositoryTools::deleteHelper(Source, Tables, "user_id", entity.id()))
        return bRet(ok, false, QDateTime());
    if (!RepositoryTools::setGroupIdList(Source, "user_groups", "user_id", entity.id(), entity.groups()))
        return bRet(ok, false, QDateTime());
    if (!RepositoryTools::setServices(Source, "user_services", "user_id", entity.id(), entity.availableServices()))
        return bRet(ok, false, QDateTime());
    if (!updateAvatar(entity.id(), entity.avatar()))
        return bRet(ok, false, QDateTime());
    return bRet(ok, true, dt);
}

bool UserRepository::emailOccupied(const QString &email, bool *ok)
{
    if (!isValid() || email.isEmpty())
        return bRet(ok, false, false);
    BSqlResult result = Source->select("users", "COUNT(*)", BSqlWhere("email = :email", ":email", email));
    if (!result.success())
        return bRet(ok, false, false);
    return bRet(ok, true, (result.value("COUNT(*)").toInt() > 0));
}

bool UserRepository::exists(const TUserIdentifier &id, bool *ok)
{
    if (!isValid() || !id.isValid())
        return bRet(ok, false, false);
    BSqlWhere where = (id.type() == TUserIdentifier::IdType) ? BSqlWhere("id = :id", ":id", id.id()) :
                                                               BSqlWhere("login = :login", ":login", id.login());
    BSqlResult result = Source->select("users", "COUNT(*)", where);
    if (!result.success())
        return bRet(ok, false, false);
    return bRet(ok, true, (result.value("COUNT(*)").toInt() > 0));
}

TAccessLevel UserRepository::findAccessLevel(quint64 id, bool *ok)
{
    if (!isValid() || !id)
        return bRet(ok, false, TAccessLevel());
    BSqlResult result = Source->select("users", "access_level", BSqlWhere("id = :id", ":id", id));
    if (!result.success())
        return bRet(ok, false, TAccessLevel());
    return bRet(ok, true, result.value("access_level").toInt());
}

TIdList UserRepository::findAllDeletedNewerThan(const QDateTime &newerThan, bool *ok)
{
    TIdList list;
    if (!isValid())
        return bRet(ok, false, list);
    BSqlWhere where;
    if (newerThan.isValid()) {
        where = BSqlWhere("deletion_date_time > :deletion_date_time", ":deletion_date_time",
                          newerThan.toUTC().toMSecsSinceEpoch());
    }
    BSqlResult result = Source->select("deleted_users", "id", where);
    if (!result.success())
        return bRet(ok, false, list);
    foreach (const QVariantMap &m, result.values())
        list << m.value("id").toULongLong();
    return bRet(ok, true, list);
}

QList<User> UserRepository::findAllNewerThan(const QDateTime &newerThan, bool *ok)
{
    QList<User> list;
    if (!isValid())
        return bRet(ok, false, list);
    static const QStringList Fields = QStringList() << "id" << "access_level" << "active" << "email"
        << "last_modification_date_time" << "login" << "name" << "password" << "patronymic" << "registration_date_time"
        << "surname";
    BSqlWhere where;
    if (newerThan.isValid()) {
        where = BSqlWhere("last_modification_date_time > :last_modification_date_time", ":last_modification_date_time",
                          newerThan.toUTC().toMSecsSinceEpoch());
    }
    BSqlResult result = Source->select("users", Fields, where);
    if (!result.success())
        return bRet(ok, false, list);
    foreach (const QVariantMap &m, result.values()) {
        User entity(this);
        entity.mid = m.value("id").toULongLong();
        entity.maccessLevel = m.value("access_level").toInt();
        entity.mactive = m.value("active").toBool();
        entity.memail = m.value("email").toString();
        entity.mlastModificationDateTime.setMSecsSinceEpoch(m.value("last_modification_date_time").toLongLong());
        entity.mlogin = m.value("login").toString();
        entity.mname = m.value("name").toString();
        entity.mpassword = m.value("password").toByteArray();
        entity.mpatronymic = m.value("patronymic").toString();
        entity.mregistrationDateTime.setMSecsSinceEpoch(m.value("registration_date_time").toLongLong());
        entity.msurname = m.value("surname").toString();
        bool b = false;
        entity.mgroups = RepositoryTools::getGroupIdList(Source, "user_groups", "user_id", entity.id(), &b);
        if (!b)
            return bRet(ok, false, QList<User>());
        entity.mavailableServices = RepositoryTools::getServices(Source, "user_services", "user_id", entity.id(), &b);
        if (!b)
            return bRet(ok, false, QList<User>());
        entity.valid = true;
        list << entity;
    }
    return bRet(ok, true, list);
}

QDateTime UserRepository::findLastModificationDateTime(const TUserIdentifier &id, bool *ok)
{
    if (!isValid() || !id.isValid())
        return bRet(ok, false, QDateTime());
    BSqlResult result;
    switch (id.type()) {
    case TUserIdentifier::IdType:
        result = Source->select("users", "last_modification_date_time", BSqlWhere("id = :id", ":id", id.id()));
        break;
    case TUserIdentifier::LoginType:
        result = Source->select("users", "last_modification_date_time",
                                BSqlWhere("login = :login", ":login", id.login()));
        break;
    default:
        break;
    }
    if (!result.success())
        return bRet(ok, false, QDateTime());
    if (result.values().isEmpty())
        return bRet(ok, true, QDateTime());
    QDateTime dt;
    dt.setTimeSpec(Qt::UTC);
    dt.setMSecsSinceEpoch(result.value("last_modification_date_time").toLongLong());
    return bRet(ok, true, dt);
}

QString UserRepository::findLogin(quint64 id, bool *ok)
{
    if (!isValid() || !id)
        return bRet(ok, false, QString());
    BSqlResult result = Source->select("users", "login", BSqlWhere("id = :id", ":id", id));
    if (!result.success())
        return bRet(ok, false, QString());
    if (result.values().isEmpty())
        return bRet(ok, true, QString());
    return bRet(ok, true, result.value("login").toString());
}

User UserRepository::findOne(const TUserIdentifier &id, bool *ok)
{
    User entity(this);
    if (!isValid() || !id.isValid())
        return bRet(ok, false, entity);
    static const QStringList Fields = QStringList() << "id" << "access_level" << "active" << "email"
        << "last_modification_date_time" << "login" << "name" << "password" << "patronymic" << "registration_date_time"
        << "surname";
    BSqlWhere where = (id.type() == TUserIdentifier::IdType) ? BSqlWhere("id = :id", ":id", id.id()) :
                                                               BSqlWhere("login = :login", ":login", id.login());
    BSqlResult result = Source->select("users", Fields, where);
    if (!result.success())
        return bRet(ok, false, entity);
    if (result.value().isEmpty())
        return bRet(ok, true, entity);
    entity.mid = result.value("id").toULongLong();
    entity.maccessLevel = result.value("access_level").toInt();
    entity.mactive = result.value("active").toBool();
    entity.memail = result.value("email").toString();
    entity.mlastModificationDateTime.setMSecsSinceEpoch(result.value("last_modification_date_time").toLongLong());
    entity.mlogin = result.value("login").toString();
    entity.mname = result.value("name").toString();
    entity.mpassword = result.value("password").toByteArray();
    entity.mpatronymic = result.value("patronymic").toString();
    entity.mregistrationDateTime.setMSecsSinceEpoch(result.value("registration_date_time").toLongLong());
    entity.msurname = result.value("surname").toString();
    bool b = false;
    entity.mgroups = RepositoryTools::getGroupIdList(Source, "user_groups", "user_id", entity.id(), &b);
    if (!b)
        return bRet(ok, false, entity);
    entity.mavailableServices = RepositoryTools::getServices(Source, "user_services", "user_id", entity.id(), &b);
    if (!b)
        return bRet(ok, false, entity);
    entity.valid = true;
    return bRet(ok, true, entity);
}

User UserRepository::findOne(const QString &identifier, const QByteArray &password, bool *ok)
{
    User entity(this);
    if (!isValid() || identifier.isEmpty() || password.isEmpty())
        return bRet(ok, false, entity);
    static const QStringList Fields = QStringList() << "id" << "access_level" << "active" << "email"
        << "last_modification_date_time" << "login" << "name" << "password" << "patronymic" << "registration_date_time"
        << "surname";
    QString ws = "(login = :login OR email = :email) AND password = :password";
    QVariantMap wvalues;
    wvalues.insert(":login", identifier);
    wvalues.insert(":email", identifier);
    wvalues.insert(":password", password);
    BSqlResult result = Source->select("users", Fields, BSqlWhere(ws, wvalues));
    if (!result.success())
        return bRet(ok, false, entity);
    if (result.value().isEmpty())
        return bRet(ok, true, entity);
    entity.mid = result.value("id").toULongLong();
    entity.maccessLevel = result.value("access_level").toInt();
    entity.mactive = result.value("active").toBool();
    entity.memail = result.value("email").toString();
    entity.mlastModificationDateTime.setMSecsSinceEpoch(result.value("last_modification_date_time").toLongLong());
    entity.mlogin = result.value("login").toString();
    entity.mname = result.value("name").toString();
    entity.mpassword = result.value("password").toByteArray();
    entity.mpatronymic = result.value("patronymic").toString();
    entity.mregistrationDateTime.setMSecsSinceEpoch(result.value("registration_date_time").toLongLong());
    entity.msurname = result.value("surname").toString();
    bool b = false;
    entity.mgroups = RepositoryTools::getGroupIdList(Source, "user_groups", "user_id", entity.id(), &b);
    if (!b)
        return bRet(ok, false, entity);
    entity.mavailableServices = RepositoryTools::getServices(Source, "user_services", "user_id", entity.id(), &b);
    if (!b)
        return bRet(ok, false, entity);
    entity.valid = true;
    return bRet(ok, true, entity);
}

User UserRepository::findOneByEmail(const QString &email, bool *ok)
{
    User entity(this);
    if (!isValid() || email.isEmpty())
        return bRet(ok, false, entity);
    static const QStringList Fields = QStringList() << "id" << "access_level" << "active" << "email"
        << "last_modification_date_time" << "login" << "name" << "password" << "patronymic" << "registration_date_time"
        << "surname";
    BSqlResult result = Source->select("users", Fields, BSqlWhere("email = :email", ":email", email));
    if (!result.success())
        return bRet(ok, false, entity);
    if (result.value().isEmpty())
        return bRet(ok, true, entity);
    entity.mid = result.value("id").toULongLong();
    entity.maccessLevel = result.value("access_level").toInt();
    entity.mactive = result.value("active").toBool();
    entity.memail = result.value("email").toString();
    entity.mlastModificationDateTime.setMSecsSinceEpoch(result.value("last_modification_date_time").toLongLong());
    entity.mlogin = result.value("login").toString();
    entity.mname = result.value("name").toString();
    entity.mpassword = result.value("password").toByteArray();
    entity.mpatronymic = result.value("patronymic").toString();
    entity.mregistrationDateTime.setMSecsSinceEpoch(result.value("registration_date_time").toLongLong());
    entity.msurname = result.value("surname").toString();
    bool b = false;
    entity.mgroups = RepositoryTools::getGroupIdList(Source, "user_groups", "user_id", entity.id(), &b);
    if (!b)
        return bRet(ok, false, entity);
    entity.mavailableServices = RepositoryTools::getServices(Source, "user_services", "user_id", entity.id(), &b);
    if (!b)
        return bRet(ok, false, entity);
    entity.valid = true;
    return bRet(ok, true, entity);
}

bool UserRepository::isValid() const
{
    return Source && Source->isValid();
}

bool UserRepository::loginOccupied(const QString &login, bool *ok)
{
    return exists(login, ok);
}

/*============================== Private methods ===========================*/

bool UserRepository::createAvatar(quint64 userId, const QImage &avatar)
{
    if (!isValid() || !userId)
        return false;
    QByteArray data;
    if (!avatar.isNull()) {
        QBuffer buff(&data);
        buff.open(QBuffer::WriteOnly);
        if (!avatar.save(&buff, "png"))
            return false;
        buff.close();
    }
    return Source->insert("user_avatars", "user_id", userId, "avatar", data).success();
}

QImage UserRepository::fetchAvatar(quint64 userId, bool *ok)
{
    if (!isValid() || !userId)
        return bRet(ok, false, QImage());
    BSqlResult result = Source->select("user_avatars", "avatar", BSqlWhere("user_id = :user_id", ":user_id", userId));
    if (!result.success() || result.value().isEmpty())
        return bRet(ok, false, QImage());
    QByteArray avatar = result.value("avatar").toByteArray();
    if (avatar.isEmpty())
        return bRet(ok, true, QImage());
    QBuffer buff(&avatar);
    if (!buff.open(QBuffer::ReadOnly))
        return bRet(ok, false, QImage());
    QImage img;
    if (!img.load(&buff, "png"))
        return bRet(ok, false, QImage());
    return bRet(ok, true, img);
}

bool UserRepository::updateAvatar(quint64 userId, const QImage &avatar)
{
    if (!isValid() || !userId)
        return false;
    QByteArray data;
    if (!avatar.isNull()) {
        QBuffer buff(&data);
        buff.open(QBuffer::WriteOnly);
        if (!avatar.save(&buff, "png"))
            return false;
        buff.close();
    }
    return Source->update("user_avatars", "avatar", data, BSqlWhere("user_id = :user_id", ":user_id", userId));
}
