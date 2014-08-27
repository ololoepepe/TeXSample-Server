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

#ifndef REGISTRATIONCONFIRMATIONCODE_H
#define REGISTRATIONCONFIRMATIONCODE_H

class RegistrationConfirmationCodeRepository;

#include <BUuid>

#include <QDateTime>

/*============================================================================
================================ RegistrationConfirmationCode ================
============================================================================*/

class RegistrationConfirmationCode
{
private:
    quint64 muserId;
    BUuid mcode;
    QDateTime mexpirationDateTime;
    bool createdByRepo;
    RegistrationConfirmationCodeRepository *repo;
    bool valid;
public:
    explicit RegistrationConfirmationCode();
    RegistrationConfirmationCode(const RegistrationConfirmationCode &other);
    ~RegistrationConfirmationCode();
protected:
    explicit RegistrationConfirmationCode(RegistrationConfirmationCodeRepository *repo);
public:
    BUuid code() const;
    void convertToCreatedByUser();
    QDateTime expirationDateTime() const;
    bool isCreatedByRepo() const;
    bool isValid() const;
    RegistrationConfirmationCodeRepository *repository() const;
    void setCode(const BUuid &code);
    void setExpirationDateTime(const QDateTime &dt);
    void setUserId(quint64 userId);
    quint64 userId() const;
public:
    RegistrationConfirmationCode &operator =(const RegistrationConfirmationCode &other);
private:
    void init();
private:
    friend class RegistrationConfirmationCodeRepository;
};

#endif // REGISTRATIONCONFIRMATIONCODE_H
