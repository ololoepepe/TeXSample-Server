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

bool EmailChangeConfirmationCodeRepository::add(const EmailChangeConfirmationCode &entity)
{
    if (!isValid() || !entity.isValid() || entity.isCreatedByRepo())
        return false;
    QVariantMap values;
    values.insert("code", entity.code().toString(true));
    values.insert("email", entity.email());
    values.insert("expiration_date_time", entity.expirationDateTime().toUTC().toMSecsSinceEpoch());
    values.insert("user_id", entity.userId());
    return Source->insert("email_change_confirmation_codes", values).success();
}

bool EmailChangeConfirmationCodeRepository::deleteExpired()
{
    return isValid() && Source->deleteFrom("email_change_confirmation_codes",
                                           BSqlWhere("expiration_date_time <= :date_time", ":date_time",
                                                     QDateTime::currentDateTimeUtc().toMSecsSinceEpoch()));
}

bool EmailChangeConfirmationCodeRepository::deleteOneByUserId(quint64 userId)
{
    if (!isValid() || !userId)
        return false;
    return Source->deleteFrom("email_change_confirmation_codes",
                              BSqlWhere("user_id = :user_id", ":user_id", userId)).success();
}

bool EmailChangeConfirmationCodeRepository::emailOccupied(const QString &email)
{
    if (!isValid() || email.isEmpty())
        return false;
    BSqlResult result = Source->select("email_change_confirmation_codes", "COUNT(*)",
                                       BSqlWhere("email = :email", ":email", email));
    return result.success() && result.value("COUNT(*)").toInt() > 0;
}

QList<EmailChangeConfirmationCode> EmailChangeConfirmationCodeRepository::findExpired()
{
    QList<EmailChangeConfirmationCode> list;
    if (!isValid())
        return list;
    BSqlResult result = Source->select("email_change_confirmation_codes",
                                       QStringList() << "user_id" << "code" << "email" << "expiration_date_time",
                                           BSqlWhere("expiration_date_time <= :date_time", ":date_time",
                                                     QDateTime::currentDateTimeUtc().toMSecsSinceEpoch()));
    if (!result.success())
        return list;
    foreach (const QVariantMap &m, result.values()) {
        EmailChangeConfirmationCode entity(this);
        entity.muserId = m.value("user_id").toULongLong();
        entity.mcode = BUuid(m.value("code").toString());
        entity.memail = m.value("email").toString();
        entity.mexpirationDateTime.setMSecsSinceEpoch(m.value("expiration_date_time").toLongLong());
        list << entity;
    }
    return list;
}

EmailChangeConfirmationCode EmailChangeConfirmationCodeRepository::findOneByCode(const BUuid &code)
{
    EmailChangeConfirmationCode entity(this);
    if (!isValid() || code.isNull())
        return entity;
    BSqlResult result = Source->select("email_change_confirmation_codes",
                                       QStringList() << "email" << "expiration_date_time" << "user_id",
                                       BSqlWhere("code = :code", ":code", code.toString(true)));
    if (!result.success() || result.value().isEmpty())
        return entity;
    entity.mcode = code;
    entity.memail = result.value("email").toString();
    entity.mexpirationDateTime.setMSecsSinceEpoch(result.value("expiration_date_time").toLongLong());
    entity.muserId = result.value("user_id").toULongLong();
    entity.valid = true;
    return entity;
}

DataSource *EmailChangeConfirmationCodeRepository::dataSource() const
{
    return Source;
}

bool EmailChangeConfirmationCodeRepository::isValid() const
{
    return Source && Source->isValid();
}
