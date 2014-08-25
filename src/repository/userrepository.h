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
    quint64 add(const User &entity);
    long countByAccessLevel(const TAccessLevel &accessLevel);
    DataSource *dataSource() const;
    bool deleteOne(quint64 userId);
    bool edit(const User &entity);
    bool emailOccupied(const QString &email);
    bool exists(const TUserIdentifier &id);
    TAccessLevel findAccessLevel(quint64 id);
    QList<User> findAllNewerThan(const QDateTime &newerThan);
    QDateTime findLastModificationDateTime(const TUserIdentifier &id);
    QString findLogin(quint64 id);
    User findOne(const TUserIdentifier &id);
    User findOne(const QString &identifier, const QByteArray &password);
    bool isValid() const;
    bool loginOccupied(const QString &login);
private:
    bool createAvatar(quint64 userId, const QImage &avatar);
    bool deleteAvatar(quint64 userId);
    QImage fetchAvatar(quint64 userId, bool *ok = 0);
    bool updateAvatar(quint64 userId, const QImage &avatar);
private:
    friend class User;
    Q_DISABLE_COPY(UserRepository)
};

#endif // USERREPOSITORY_H
