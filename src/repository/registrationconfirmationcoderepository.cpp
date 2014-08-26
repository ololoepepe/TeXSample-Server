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

#include "registrationconfirmationcoderepository.h"

#include "datasource.h"
#include "entity/registrationconfirmationcode.h"

#include <TIdList>

#include <BSqlResult>
#include <BSqlWhere>
#include <BUuid>

#include <QList>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <QVariantMap>

/*============================================================================
================================ RegistrationConfirmationCodeRepository ======
============================================================================*/

/*============================== Public constructors =======================*/

RegistrationConfirmationCodeRepository::RegistrationConfirmationCodeRepository(DataSource *source) :
    Source(source)
{
    //
}

RegistrationConfirmationCodeRepository::~RegistrationConfirmationCodeRepository()
{
    //
}

/*============================== Public methods ============================*/

void RegistrationConfirmationCodeRepository::add(const RegistrationConfirmationCode &entity, bool *ok)
{
    if (!isValid() || !entity.isValid() || entity.isCreatedByRepo())
        return bSet(ok, false);
    QVariantMap values;
    values.insert("code", entity.code().toString(true));
    values.insert("expiration_date_time", entity.expirationDateTime().toUTC().toMSecsSinceEpoch());
    values.insert("user_id", entity.userId());
    bSet(ok, Source->insert("registration_confirmation_codes", values).success());
}

void RegistrationConfirmationCodeRepository::deleteExpired(bool *ok)
{
    if (!isValid())
        return bSet(ok, false);
    QDateTime dt = QDateTime::currentDateTimeUtc();
    BSqlWhere where("expiration_date_time <= :date_time", ":date_time", dt.toMSecsSinceEpoch());
    bSet(ok, Source->deleteFrom("registration_confirmation_codes", where).success());
}

void RegistrationConfirmationCodeRepository::deleteOneByUserId(quint64 userId, bool *ok)
{
    if (!isValid() || !userId)
        return bSet(ok, false);
    BSqlWhere where("user_id = :user_id", ":user_id", userId);
    bSet(ok, Source->deleteFrom("registration_confirmation_codes", where).success());
}

QList<RegistrationConfirmationCode> RegistrationConfirmationCodeRepository::findExpired(bool *ok)
{
    QList<RegistrationConfirmationCode> list;
    if (!isValid())
        return bRet(ok, false, list);
    static const QStringList Fields = QStringList() << "user_id" << "code" << "expiration_date_time";
    QDateTime dt = QDateTime::currentDateTimeUtc();
    BSqlWhere where("expiration_date_time <= :date_time", ":date_time", dt.toMSecsSinceEpoch());
    BSqlResult result = Source->select("registration_confirmation_codes", Fields, where);
    if (!result.success())
        return bRet(ok, false, list);
    foreach (const QVariantMap &m, result.values()) {
        RegistrationConfirmationCode entity(this);
        entity.muserId = m.value("user_id").toULongLong();
        entity.mcode = BUuid(m.value("code").toString());
        entity.mexpirationDateTime.setMSecsSinceEpoch(m.value("expiration_date_time").toLongLong());
        list << entity;
    }
    return bRet(ok, true, list);
}

RegistrationConfirmationCode RegistrationConfirmationCodeRepository::findOneByCode(const BUuid &code, bool *ok)
{
    RegistrationConfirmationCode entity(this);
    if (!isValid() || code.isNull())
        return bRet(ok, false, entity);
    static const QStringList Fields = QStringList() << "expiration_date_time" << "user_id";
    BSqlWhere where("code = :code", ":code", code.toString(true));
    BSqlResult result = Source->select("registration_confirmation_codes", Fields, where);
    if (!result.success())
        return bRet(ok, false, entity);
    if (result.value().isEmpty())
        return bRet(ok, true, entity);
    entity.mcode = code;
    entity.mexpirationDateTime.setMSecsSinceEpoch(result.value("expiration_date_time").toLongLong());
    entity.muserId = result.value("user_id").toULongLong();
    entity.valid = true;
    return bRet(ok, true, entity);
}

DataSource *RegistrationConfirmationCodeRepository::dataSource() const
{
    return Source;
}

bool RegistrationConfirmationCodeRepository::isValid() const
{
    return Source && Source->isValid();
}
