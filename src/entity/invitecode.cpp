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

#include "invitecode.h"

#include "repository/invitecoderepository.h"

#include <TAccessLevel>
#include <TeXSample>
#include <TIdList>
#include <TServiceList>

#include <BUuid>

#include <QDateTime>
#include <QString>

/*============================================================================
================================ InviteCode ==================================
============================================================================*/

/*============================== Public constructors =======================*/

InviteCode::InviteCode()
{
    init();
}

InviteCode::InviteCode(const InviteCode &other)
{
    init();
    *this = other;
}

InviteCode::~InviteCode()
{
    //
}

/*============================== Protected constructors ====================*/

InviteCode::InviteCode(InviteCodeRepository *repo)
{
    init();
    this->repo = repo;
    createdByRepo = true;
}

/*============================== Public methods ============================*/

TAccessLevel InviteCode::accessLevel() const
{
    return maccessLevel;
}

TServiceList InviteCode::availableServices() const
{
    return mavailableServices;
}

BUuid InviteCode::code() const
{
    return mcode;
}

void InviteCode::convertToCreatedByUser()
{
    if (!createdByRepo)
        return;
    createdByRepo = false;
    repo = 0;
    valid = false;
    mgroups.clear();
}

QDateTime InviteCode::creationDateTime() const
{
    return mcreationDateTime;
}

QDateTime InviteCode::expirationDateTime() const
{
    return mexpirationDateTime;
}

TIdList InviteCode::groups() const
{
    return mgroups;
}

quint64 InviteCode::id() const
{
    return mid;
}

bool InviteCode::isCreatedByRepo() const
{
    return createdByRepo;
}

bool InviteCode::isValid() const
{
    if (createdByRepo)
        return valid;
    if (mid)
        return true;
    return maccessLevel.isValid() && !mcode.isNull() && mexpirationDateTime.isValid() && mownerId;
}

quint64 InviteCode::ownerId() const
{
    return mownerId;
}

InviteCodeRepository *InviteCode::repository() const
{
    return repo;
}

void InviteCode::setAccessLevel(const TAccessLevel &accessLevel)
{
    maccessLevel = accessLevel;
}

void InviteCode::setAvailableServices(const TServiceList &services)
{
    mavailableServices = services;
}

void InviteCode::setCode(const BUuid &code)
{
    mcode = code;
}

void InviteCode::setExpirationDateTime(const QDateTime &dt)
{
    mexpirationDateTime = dt.toUTC();
}

void InviteCode::setGroups(const TIdList &ids)
{
    mgroups = ids;
    mgroups.removeAll(0);
    bRemoveDuplicates(mgroups);
}

void InviteCode::setId(quint64 id)
{
    mid = id;
}

void InviteCode::setOwnerId(quint64 ownerId)
{
    mownerId = ownerId;
}

/*============================== Public operators ==========================*/

InviteCode &InviteCode::operator =(const InviteCode &other)
{
    maccessLevel = other.maccessLevel;
    mavailableServices = other.mavailableServices;
    mcode = other.mcode;
    mcreationDateTime = other.mcreationDateTime;
    mexpirationDateTime = other.mexpirationDateTime;
    mgroups = other.mgroups;
    mid = other.mid;
    mownerId = other.mownerId;
    createdByRepo = other.createdByRepo;
    repo = other.repo;
    valid = other.valid;
    return *this;
}

/*============================== Private methods ===========================*/

void InviteCode::init()
{
    mcreationDateTime = QDateTime().toUTC();
    mexpirationDateTime = QDateTime().toUTC();
    mid = 0;
    mownerId = 0;
    createdByRepo = false;
    repo = 0;
    valid = false;
}
