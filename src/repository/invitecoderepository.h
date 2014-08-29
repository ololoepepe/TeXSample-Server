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

#ifndef INVITECODEREPOSITORY_H
#define INVITECODEREPOSITORY_H

class DataSource;

class BUuid;

#include "entity/invitecode.h"

#include <TIdList>

#include <QList>

/*============================================================================
================================ InviteCodeRepository ========================
============================================================================*/

class InviteCodeRepository
{
private:
    DataSource * const Source;
public:
    explicit InviteCodeRepository(DataSource *source);
    ~InviteCodeRepository();
public:
    quint64 add(const InviteCode &entity, bool *ok = 0);
    DataSource *dataSource() const;
    QDateTime deleteExpired(bool *ok = 0);
    QDateTime deleteOne(quint64 id, bool *ok = 0);
    QDateTime deleteSome(const TIdList &ids, bool *ok = 0);
    QList<InviteCode> findAll(const QDateTime &newerThan = QDateTime(), bool *ok = 0);
    QList<InviteCode> findAllByOwnerId(quint64 ownerId, const QDateTime &newerThan = QDateTime(), bool *ok = 0);
    TIdList findAllDeleted(const QDateTime &newerThan = QDateTime(), bool *ok = 0);
    TIdList findAllDeletedByOwnerId(quint64 ownerId, const QDateTime &newerThan = QDateTime(), bool *ok = 0);
    InviteCode findOne(quint64 id, bool *ok = 0);
    InviteCode findOneByCode(const BUuid &code, bool *ok = 0);
    bool isValid() const;
private:
    friend class InviteCode;
    Q_DISABLE_COPY(InviteCodeRepository)
};

#endif // INVITECODEREPOSITORY_H
