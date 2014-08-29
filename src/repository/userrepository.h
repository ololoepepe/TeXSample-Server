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

#ifndef USERREPOSITORY_H
#define USERREPOSITORY_H

class DataSource;

class TAccessLevel;
class TUserIdentifier;

class QByteArray;
class QDateTime;
class QImage;
class QString;

#include "entity/user.h"

#include <TIdList>

#include <QList>

/*============================================================================
================================ UserRepository ==============================
============================================================================*/

class UserRepository
{
private:
    DataSource * const Source;
public:
    explicit UserRepository(DataSource *source);
    ~UserRepository();
public:
    quint64 add(const User &entity, bool *ok = 0);
    long countByAccessLevel(const TAccessLevel &accessLevel, bool *ok = 0);
    DataSource *dataSource() const;
    QDateTime deleteOne(const TUserIdentifier &id, bool *ok = 0);
    QDateTime edit(const User &entity, bool *ok = 0);
    bool emailOccupied(const QString &email, bool *ok = 0);
    bool exists(const TUserIdentifier &id, bool *ok = 0);
    TAccessLevel findAccessLevel(quint64 id, bool *ok = 0);
    TIdList findAllDeletedNewerThan(const QDateTime &newerThan = QDateTime(), bool *ok = 0);
    QList<User> findAllNewerThan(const QDateTime &newerThan, bool *ok = 0);
    QDateTime findLastModificationDateTime(const TUserIdentifier &id, bool *ok = 0);
    QString findLogin(quint64 id, bool *ok = 0);
    User findOne(const TUserIdentifier &id, bool *ok = 0);
    User findOne(const QString &identifier, const QByteArray &password, bool *ok = 0);
    User findOneByEmail(const QString &email, bool *ok = 0);
    bool isValid() const;
    bool loginOccupied(const QString &login, bool *ok = 0);
private:
    bool createAvatar(quint64 userId, const QImage &avatar);
    QImage fetchAvatar(quint64 userId, bool *ok = 0);
    bool updateAvatar(quint64 userId, const QImage &avatar);
private:
    friend class User;
    Q_DISABLE_COPY(UserRepository)
};

#endif // USERREPOSITORY_H
