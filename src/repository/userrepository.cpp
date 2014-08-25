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

quint64 UserRepository::add(const User &entity)
{
    if (!isValid() || !entity.isValid() || entity.isCreatedByRepo() || entity.id())
        return 0;
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
        return 0;
    quint64 id = result.lastInsertId().toULongLong();
    if (!RepositoryTools::setGroupIdList(Source, "user_groups", "user_id", id, entity.groups()))
        return 0;
    if (!RepositoryTools::setServices(Source, "user_services", "user_id", id, entity.availableServices()))
        return 0;
    if (!createAvatar(id, entity.avatar()))
        return 0;
    return id;
}

long UserRepository::countByAccessLevel(const TAccessLevel &accessLevel)
{
    if (!isValid())
        return 0;
    BSqlResult result = Source->select("users", "COUNT(id)",
                                       BSqlWhere("access_level = :access_level", ":access_level", int(accessLevel)));
    if (!result.success())
        return 0;
    return result.value("COUNT(id)").toLongLong();
}

DataSource *UserRepository::dataSource() const
{
    return Source;
}

bool UserRepository::deleteOne(quint64 userId)
{
    if (!isValid() || !userId)
        return false;
    return deleteAvatar(userId) && Source->deleteFrom("users", BSqlWhere("user_id = :user_id", ":user_id", userId));
}

bool UserRepository::edit(const User &entity)
{
    if (!isValid() || !entity.isValid() || entity.isCreatedByRepo() || !entity.id())
        return false;
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
        return false;
    if (!RepositoryTools::deleteHelper(Source, QStringList() << "user_groups" << "user_services", "user_id",
                                       entity.id())) {
        return false;
    }
    if (!RepositoryTools::setGroupIdList(Source, "user_groups", "user_id", entity.id(), entity.groups()))
        return false;
    if (!RepositoryTools::setServices(Source, "user_services", "user_id", entity.id(), entity.availableServices()))
        return false;
    if (entity.saveAvatar() && !updateAvatar(entity.id(), entity.avatar()))
        return false;
    return true;
}

bool UserRepository::emailOccupied(const QString &email)
{
    if (!isValid() || email.isEmpty())
        return false;
    BSqlResult result = Source->select("users", "COUNT(*)", BSqlWhere("email = :email", ":email", email));
    return result.success() && result.value("COUNT(*)").toInt() > 0;
}

bool UserRepository::exists(const TUserIdentifier &id)
{
    if (!isValid() || !id.isValid())
        return false;
    BSqlWhere where = (id.type() == TUserIdentifier::IdType) ? BSqlWhere("id = :id", ":id", id.id()) :
                                                               BSqlWhere("login = :login", ":login", id.login());
    BSqlResult result = Source->select("users", "COUNT(*)", where);
    return result.success() && result.value("COUNT(*)").toInt() > 0;
}

TAccessLevel UserRepository::findAccessLevel(quint64 id)
{
    if (!isValid() || !id)
        return TAccessLevel();
    BSqlResult result = Source->select("users", "access_level", BSqlWhere("id = :id", ":id", id));
    if (!result.success())
        return TAccessLevel();
    return result.value("access_level").toInt();
}

QList<User> UserRepository::findAllNewerThan(const QDateTime &newerThan)
{
    QList<User> list;
    if (!isValid())
        return list;
    static const QStringList Fields = QStringList() << "id" << "access_level" << "active" << "email"
        << "last_modification_date_time" << "login" << "name" << "password" << "patronymic" << "registration_date_time"
        << "surname";
    BSqlWhere where;
    if (newerThan.isValid())
        where = BSqlWhere("last_modification_date_time > :last_modification_date_time", ":last_modification_date_time",
                          newerThan.toUTC().toMSecsSinceEpoch());
    BSqlResult result = Source->select("users", Fields, where);
    if (!result.success() || result.values().isEmpty())
        return list;
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
        bool ok = false;
        entity.mgroups = RepositoryTools::getGroupIdList(Source, "user_groups", "user_id", entity.id(), &ok);
        if (!ok)
            return QList<User>();
        entity.mavailableServices = RepositoryTools::getServices(Source, "user_services", "user_id", entity.id(), &ok);
        if (!ok)
            return QList<User>();
        entity.valid = true;
        list << entity;
    }
    return list;
}

QDateTime UserRepository::findLastModificationDateTime(const TUserIdentifier &id)
{
    if (!isValid() || !id.isValid())
        return QDateTime();
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
        return QDateTime();
    QDateTime dt;
    dt.setTimeSpec(Qt::UTC);
    dt.setMSecsSinceEpoch(result.value("last_modification_date_time").toLongLong());
    return dt;
}

QString UserRepository::findLogin(quint64 id)
{
    if (!isValid() || !id)
        return QString();
    BSqlResult result = Source->select("users", "login", BSqlWhere("id = :id", ":id", id));
    if (!result.success())
        return QString();
    return result.value("login").toString();
}

User UserRepository::findOne(const TUserIdentifier &id)
{
    User entity(this);
    if (!isValid() || !id.isValid())
        return entity;
    static const QStringList Fields = QStringList() << "id" << "access_level" << "active" << "email"
        << "last_modification_date_time" << "login" << "name" << "password" << "patronymic" << "registration_date_time"
        << "surname";
    BSqlWhere where = (id.type() == TUserIdentifier::IdType) ? BSqlWhere("id = :id", ":id", id.id()) :
                                                               BSqlWhere("login = :login", ":login", id.login());
    BSqlResult result = Source->select("users", Fields, where);
    if (!result.success() || result.value().isEmpty())
        return entity;
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
    bool ok = false;
    entity.mgroups = RepositoryTools::getGroupIdList(Source, "user_groups", "user_id", entity.id(), &ok);
    if (!ok)
        return entity;
    entity.mavailableServices = RepositoryTools::getServices(Source, "user_services", "user_id", entity.id(), &ok);
    if (!ok)
        return entity;
    entity.valid = true;
    return entity;
}

User UserRepository::findOne(const QString &identifier, const QByteArray &password)
{
    User entity(this);
    if (!isValid() || identifier.isEmpty() || password.isEmpty())
        return entity;
    static const QStringList Fields = QStringList() << "id" << "access_level" << "active" << "email"
        << "last_modification_date_time" << "login" << "name" << "password" << "patronymic" << "registration_date_time"
        << "surname";
    QString ws = "(login = :login OR email = :email) AND password = :password";
    QVariantMap wvalues;
    wvalues.insert(":login", identifier);
    wvalues.insert(":email", identifier);
    wvalues.insert(":password", password);
    BSqlResult result = Source->select("users", Fields, BSqlWhere(ws, wvalues));
    if (!result.success() || result.value().isEmpty())
        return entity;
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
    bool ok = false;
    entity.mgroups = RepositoryTools::getGroupIdList(Source, "user_groups", "user_id", entity.id(), &ok);
    if (!ok)
        return entity;
    entity.mavailableServices = RepositoryTools::getServices(Source, "user_services", "user_id", entity.id(), &ok);
    if (!ok)
        return entity;
    entity.valid = true;
    return entity;
}

bool UserRepository::isValid() const
{
    return Source && Source->isValid();
}

bool UserRepository::loginOccupied(const QString &login)
{
    return exists(login);
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

bool UserRepository::deleteAvatar(quint64 userId)
{
    if (!isValid() || !userId)
        return false;
    return Source->deleteFrom("user_avatars", BSqlWhere("user_id = :user_id", ":user_id", userId)).success();
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
    return Source->update("user_avatars", "user_id", userId, "avatar", data).success();
}
