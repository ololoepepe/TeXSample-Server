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

#ifndef INVITECODE_H
#define INVITECODE_H

class InviteCodeRepository;

#include <TAccessLevel>
#include <TIdList>
#include <TServiceList>

#include <BUuid>

#include <QDateTime>
#include <QString>

/*============================================================================
================================ InviteCode ==================================
============================================================================*/

class InviteCode
{
private:
    quint64 mid;
    quint64 mownerId;
    TAccessLevel maccessLevel;
    TServiceList mavailableServices;
    BUuid mcode;
    QDateTime mcreationDateTime;
    QDateTime mexpirationDateTime;
    TIdList mgroups;
    bool createdByRepo;
    InviteCodeRepository *repo;
    bool valid;
public:
    explicit InviteCode();
    InviteCode(const InviteCode &other);
    ~InviteCode();
protected:
    explicit InviteCode(InviteCodeRepository *repo);
public:
    TAccessLevel accessLevel() const;
    TServiceList availableServices() const;
    BUuid code() const;
    void convertToCreatedByUser();
    QDateTime creationDateTime() const;
    QDateTime expirationDateTime() const;
    TIdList groups() const;
    quint64 id() const;
    bool isCreatedByRepo() const;
    bool isValid() const;
    quint64 ownerId() const;
    InviteCodeRepository *repository() const;
    void setAccessLevel(const TAccessLevel &accessLevel);
    void setAvailableServices(const TServiceList &services);
    void setCode(const BUuid &code);
    void setExpirationDateTime(const QDateTime &dt);
    void setGroups(const TIdList &ids);
    void setId(quint64 id);
    void setOwnerId(quint64 ownerId);
public:
    InviteCode &operator =(const InviteCode &other);
private:
    void init();
private:
    friend class InviteCodeRepository;
};

#endif // INVITECODE_H
