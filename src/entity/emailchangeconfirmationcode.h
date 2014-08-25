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

#ifndef EMAILCHANGECONFIRMATIONCODE_H
#define EMAILCHANGECONFIRMATIONCODE_H

class EmailChangeConfirmationCodeRepository;

#include <BUuid>

#include <QDateTime>
#include <QString>

/*============================================================================
================================ EmailChangeConfirmationCode =================
============================================================================*/

class EmailChangeConfirmationCode
{
private:
    quint64 muserId;
    BUuid mcode;
    QString memail;
    QDateTime mexpirationDateTime;
    bool createdByRepo;
    EmailChangeConfirmationCodeRepository *repo;
    bool valid;
public:
    explicit EmailChangeConfirmationCode();
    EmailChangeConfirmationCode(const EmailChangeConfirmationCode &other);
    ~EmailChangeConfirmationCode();
protected:
    explicit EmailChangeConfirmationCode(EmailChangeConfirmationCodeRepository *repo);
public:
    BUuid code() const;
    void convertToCreatedByUser();
    QString email() const;
    QDateTime expirationDateTime() const;
    bool isCreatedByRepo() const;
    bool isValid() const;
    EmailChangeConfirmationCodeRepository *repository() const;
    void setCode(const BUuid &code);
    void setEmail(const QString &email);
    void setExpirationDateTime(const QDateTime &dt);
    void setUserId(quint64 userId);
    quint64 userId() const;
public:
    EmailChangeConfirmationCode &operator =(const EmailChangeConfirmationCode &other);
private:
    void init();
private:
    friend class EmailChangeConfirmationCodeRepository;
};

#endif // EMAILCHANGECONFIRMATIONCODE_H
