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

#ifndef REGISTRATIONCONFIRMATIONCODEREPOSITORY_H
#define REGISTRATIONCONFIRMATIONCODEREPOSITORY_H

class DataSource;

class BUuid;

#include "entity/registrationconfirmationcode.h"

#include <TIdList>

#include <QList>

/*============================================================================
================================ RegistrationConfirmationCodeRepository ======
============================================================================*/

class RegistrationConfirmationCodeRepository
{
private:
    DataSource * const Source;
public:
    explicit RegistrationConfirmationCodeRepository(DataSource *source);
    ~RegistrationConfirmationCodeRepository();
public:
    void add(const RegistrationConfirmationCode &entity, bool *ok = 0);
    void deleteExpired(bool *ok = 0);
    void deleteOneByUserId(quint64 userId, bool *ok = 0);
    QList<RegistrationConfirmationCode> findExpired(bool *ok = 0);
    RegistrationConfirmationCode findOneByCode(const BUuid &code, bool *ok = 0);
    DataSource *dataSource() const;
    bool isValid() const;
private:
    friend class RegistrationConfirmationCode;
    Q_DISABLE_COPY(RegistrationConfirmationCodeRepository)
};

#endif // REGISTRATIONCONFIRMATIONCODEREPOSITORY_H
