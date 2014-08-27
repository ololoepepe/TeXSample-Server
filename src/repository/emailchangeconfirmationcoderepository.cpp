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

#include "emailchangeconfirmationcoderepository.h"

#include "datasource.h"
#include "entity/emailchangeconfirmationcode.h"

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
================================ EmailChangeConfirmationCodeRepository =======
============================================================================*/

/*============================== Public constructors =======================*/

EmailChangeConfirmationCodeRepository::EmailChangeConfirmationCodeRepository(DataSource *source) :
    Source(source)
{
    //
}

EmailChangeConfirmationCodeRepository::~EmailChangeConfirmationCodeRepository()
{
    //
}

/*============================== Public methods ============================*/

void EmailChangeConfirmationCodeRepository::add(const EmailChangeConfirmationCode &entity, bool *ok)
{
    if (!isValid() || !entity.isValid() || entity.isCreatedByRepo())
        return bSet(ok, false);
    QVariantMap values;
    values.insert("code", entity.code().toString(true));
    values.insert("email", entity.email());
    values.insert("expiration_date_time", entity.expirationDateTime().toUTC().toMSecsSinceEpoch());
    values.insert("user_id", entity.userId());
    bSet(ok, Source->insert("email_change_confirmation_codes", values).success());
}

void EmailChangeConfirmationCodeRepository::deleteExpired(bool *ok)
{
    if (!isValid())
        return bSet(ok, false);
    QDateTime dt = QDateTime::currentDateTimeUtc();
    BSqlWhere where("expiration_date_time <= :date_time", ":date_time", dt.toMSecsSinceEpoch());
    bSet(ok, Source->deleteFrom("email_change_confirmation_codes", where).success());
}

void EmailChangeConfirmationCodeRepository::deleteOneByUserId(quint64 userId, bool *ok)
{
    if (!isValid() || !userId)
        return bSet(ok, false);
    BSqlWhere where("user_id = :user_id", ":user_id", userId);
    bSet(ok, Source->deleteFrom("email_change_confirmation_codes", where).success());
}

bool EmailChangeConfirmationCodeRepository::emailOccupied(const QString &email, bool *ok)
{
    if (!isValid() || email.isEmpty())
        return bRet(ok, false, false);
    BSqlWhere where("email = :email", ":email", email);
    BSqlResult result = Source->select("email_change_confirmation_codes", "COUNT(*)", where);
    if (!result.success())
        return bRet(ok, false, false);
    return bRet(ok, true, (result.value("COUNT(*)").toInt() > 0));
}

QList<EmailChangeConfirmationCode> EmailChangeConfirmationCodeRepository::findExpired(bool *ok)
{
    QList<EmailChangeConfirmationCode> list;
    if (!isValid())
        return bRet(ok, false, list);
    static const QStringList Fields = QStringList() << "user_id" << "code" << "email" << "expiration_date_time";
    QDateTime dt = QDateTime::currentDateTimeUtc();
    BSqlWhere where("expiration_date_time <= :date_time", ":date_time", dt.toMSecsSinceEpoch());
    BSqlResult result = Source->select("email_change_confirmation_codes", Fields, where);
    if (!result.success())
        return bRet(ok, false, list);
    foreach (const QVariantMap &m, result.values()) {
        EmailChangeConfirmationCode entity(this);
        entity.muserId = m.value("user_id").toULongLong();
        entity.mcode = BUuid(m.value("code").toString());
        entity.memail = m.value("email").toString();
        entity.mexpirationDateTime.setMSecsSinceEpoch(m.value("expiration_date_time").toLongLong());
        entity.valid = true;
        list << entity;
    }
    return bRet(ok, true, list);
}

EmailChangeConfirmationCode EmailChangeConfirmationCodeRepository::findOneByCode(const BUuid &code, bool *ok)
{
    EmailChangeConfirmationCode entity(this);
    if (!isValid() || code.isNull())
        return bRet(ok, false, entity);
    static const QStringList Fields = QStringList() << "email" << "expiration_date_time" << "user_id";
    BSqlWhere where("code = :code", ":code", code.toString(true));
    BSqlResult result = Source->select("email_change_confirmation_codes", Fields, where);
    if (!result.success())
        return bRet(ok, false, entity);
    if (result.value().isEmpty())
        return bRet(ok, true, entity);
    entity.mcode = code;
    entity.memail = result.value("email").toString();
    entity.mexpirationDateTime.setMSecsSinceEpoch(result.value("expiration_date_time").toLongLong());
    entity.muserId = result.value("user_id").toULongLong();
    entity.valid = true;
    return bRet(ok, true, entity);
}

DataSource *EmailChangeConfirmationCodeRepository::dataSource() const
{
    return Source;
}

bool EmailChangeConfirmationCodeRepository::isValid() const
{
    return Source && Source->isValid();
}
