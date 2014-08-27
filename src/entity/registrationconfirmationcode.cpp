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

#include "registrationconfirmationcode.h"

#include "repository/registrationconfirmationcoderepository.h"

#include <TeXSample>

#include <QDateTime>

/*============================================================================
================================ RegistrationConfirmationCode ================
============================================================================*/

/*============================== Public constructors =======================*/

RegistrationConfirmationCode::RegistrationConfirmationCode()
{
    init();
}

RegistrationConfirmationCode::RegistrationConfirmationCode(const RegistrationConfirmationCode &other)
{
    init();
    *this = other;
}

RegistrationConfirmationCode::~RegistrationConfirmationCode()
{
    //
}

/*============================== Protected constructors ====================*/

RegistrationConfirmationCode::RegistrationConfirmationCode(RegistrationConfirmationCodeRepository *repo)
{
    init();
    this->repo = repo;
    createdByRepo = true;
}

/*============================== Public methods ============================*/

BUuid RegistrationConfirmationCode::code() const
{
    return mcode;
}

void RegistrationConfirmationCode::convertToCreatedByUser()
{
    if (!createdByRepo)
        return;
    createdByRepo = false;
    repo = 0;
    valid = false;
}

QDateTime RegistrationConfirmationCode::expirationDateTime() const
{
    return mexpirationDateTime;
}

bool RegistrationConfirmationCode::isCreatedByRepo() const
{
    return createdByRepo;
}

bool RegistrationConfirmationCode::isValid() const
{
    if (createdByRepo)
        return valid;
    return !mcode.isNull() && mexpirationDateTime.isValid() && muserId;
}

RegistrationConfirmationCodeRepository *RegistrationConfirmationCode::repository() const
{
    return repo;
}

void RegistrationConfirmationCode::setCode(const BUuid &code)
{
    mcode = code;
}

void RegistrationConfirmationCode::setExpirationDateTime(const QDateTime &dt)
{
    mexpirationDateTime = dt.toUTC();
}

void RegistrationConfirmationCode::setUserId(quint64 userId)
{
    muserId = userId;
}

quint64 RegistrationConfirmationCode::userId() const
{
    return muserId;
}

/*============================== Public operators ==========================*/

RegistrationConfirmationCode &RegistrationConfirmationCode::operator =(const RegistrationConfirmationCode &other)
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

void RegistrationConfirmationCode::init()
{
    mexpirationDateTime = QDateTime().toUTC();
    muserId = 0;
    createdByRepo = false;
    repo = 0;
    valid = false;
}
