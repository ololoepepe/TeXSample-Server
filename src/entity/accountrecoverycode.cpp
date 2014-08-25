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

#include "accountrecoverycode.h"

#include "repository/accountrecoverycoderepository.h"

#include <TeXSample>

#include <QDateTime>

/*============================================================================
================================ AccountRecoveryCode =========================
============================================================================*/

/*============================== Public constructors =======================*/

AccountRecoveryCode::AccountRecoveryCode()
{
    init();
}

AccountRecoveryCode::AccountRecoveryCode(const AccountRecoveryCode &other)
{
    init();
    *this = other;
}

AccountRecoveryCode::~AccountRecoveryCode()
{
    //
}

/*============================== Protected constructors ====================*/

AccountRecoveryCode::AccountRecoveryCode(AccountRecoveryCodeRepository *repo)
{
    init();
    this->repo = repo;
    createdByRepo = true;
}

/*============================== Public methods ============================*/

BUuid AccountRecoveryCode::code() const
{
    return mcode;
}

void AccountRecoveryCode::convertToCreatedByUser()
{
    if (!createdByRepo)
        return;
    createdByRepo = false;
    repo = 0;
    valid = false;
}

QDateTime AccountRecoveryCode::expirationDateTime() const
{
    return mexpirationDateTime;
}

bool AccountRecoveryCode::isCreatedByRepo() const
{
    return createdByRepo;
}

bool AccountRecoveryCode::isValid() const
{
    if (createdByRepo)
        return valid;
    return !mcode.isNull() && mexpirationDateTime.isValid() && muserId;
}

AccountRecoveryCodeRepository *AccountRecoveryCode::repository() const
{
    return repo;
}

void AccountRecoveryCode::setCode(const BUuid &code)
{
    mcode = code;
}

void AccountRecoveryCode::setExpirationDateTime(const QDateTime &dt)
{
    mexpirationDateTime = dt.toUTC();
}

void AccountRecoveryCode::setUserId(quint64 userId)
{
    muserId = userId;
}

quint64 AccountRecoveryCode::userId() const
{
    return muserId;
}

/*============================== Public operators ==========================*/

AccountRecoveryCode &AccountRecoveryCode::operator =(const AccountRecoveryCode &other)
{
    mcode = other.mcode;
    mexpirationDateTime = other.mexpirationDateTime;
    muserId = other.muserId;
    createdByRepo = other.createdByRepo;
    repo = other.repo;
    valid = other.valid;
    return *this;
}

/*============================== Private methods ===========================*/

void AccountRecoveryCode::init()
{
    mexpirationDateTime = QDateTime().toUTC();
    muserId = 0;
    createdByRepo = false;
    repo = 0;
    valid = false;
}
