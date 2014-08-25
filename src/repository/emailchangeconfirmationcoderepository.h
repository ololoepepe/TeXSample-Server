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

#ifndef EMAILCHANGECONFIRMATIONCODEREPOSITORY_H
#define EMAILCHANGECONFIRMATIONCODEREPOSITORY_H

class DataSource;

class BUuid;

#include "entity/emailchangeconfirmationcode.h"

#include <TIdList>

#include <QList>

/*============================================================================
================================ EmailChangeConfirmationCodeRepository =======
============================================================================*/

class EmailChangeConfirmationCodeRepository
{
private:
    DataSource * const Source;
public:
    explicit EmailChangeConfirmationCodeRepository(DataSource *source);
    ~EmailChangeConfirmationCodeRepository();
public:
    bool add(const EmailChangeConfirmationCode &entity);
    bool deleteExpired();
    bool deleteOneByUserId(quint64 userId);
    bool emailOccupied(const QString &email);
    QList<EmailChangeConfirmationCode> findExpired();
    EmailChangeConfirmationCode findOneByCode(const BUuid &code);
    DataSource *dataSource() const;
    bool isValid() const;
private:
    friend class EmailChangeConfirmationCode;
    Q_DISABLE_COPY(EmailChangeConfirmationCodeRepository)
};

#endif // EMAILCHANGECONFIRMATIONCODEREPOSITORY_H
