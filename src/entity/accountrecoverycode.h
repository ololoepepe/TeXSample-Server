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

#ifndef ACCOUNTRECOVERYCODE_H
#define ACCOUNTRECOVERYCODE_H

class AccountRecoveryCodeRepository;

#include <BUuid>

#include <QDateTime>

/*============================================================================
================================ AccountRecoveryCode =========================
============================================================================*/

class AccountRecoveryCode
{
private:
    quint64 muserId;
    BUuid mcode;
    QDateTime mexpirationDateTime;
    bool createdByRepo;
    AccountRecoveryCodeRepository *repo;
    bool valid;
public:
    explicit AccountRecoveryCode();
    AccountRecoveryCode(const AccountRecoveryCode &other);
    ~AccountRecoveryCode();
protected:
    explicit AccountRecoveryCode(AccountRecoveryCodeRepository *repo);
public:
    BUuid code() const;
    void convertToCreatedByUser();
    bool isCreatedByRepo() const;
    QDateTime expirationDateTime() const;
    bool isValid() const;
    AccountRecoveryCodeRepository *repository() const;
    void setCode(const BUuid &code);
    void setExpirationDateTime(const QDateTime &dt);
    void setUserId(quint64 userId);
    quint64 userId() const;
public:
    AccountRecoveryCode &operator =(const AccountRecoveryCode &other);
private:
    void init();
private:
    friend class AccountRecoveryCodeRepository;
};

#endif // ACCOUNTRECOVERYCODE_H
