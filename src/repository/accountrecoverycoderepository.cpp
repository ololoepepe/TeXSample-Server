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

#include "accountrecoverycoderepository.h"

#include "datasource.h"
#include "entity/accountrecoverycode.h"

#include <TIdList>

#include <BSqlResult>
#include <BSqlWhere>
#include <BUuid>

#include <QDateTime>
#include <QList>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <QVariantMap>

/*============================================================================
================================ AccountRecoveryCodeRepository ===============
============================================================================*/

/*============================== Public constructors =======================*/

AccountRecoveryCodeRepository::AccountRecoveryCodeRepository(DataSource *source) :
    Source(source)
{
    //
}

AccountRecoveryCodeRepository::~AccountRecoveryCodeRepository()
{
    //
}

/*============================== Public methods ============================*/

void AccountRecoveryCodeRepository::add(const AccountRecoveryCode &entity, bool *ok)
{
    if (!isValid() || !entity.isValid() || entity.isCreatedByRepo())
        return bSet(ok, false);
    QVariantMap values;
    values.insert("code", entity.code().toString(true));
    values.insert("expiration_date_time", entity.expirationDateTime().toUTC().toMSecsSinceEpoch());
    values.insert("user_id", entity.userId());
    bSet(ok, Source->insert("account_recovery_codes", values).success());
}

void AccountRecoveryCodeRepository::deleteExpired(bool *ok)
{
    if (!isValid())
        return bSet(ok, false);
    QDateTime dt = QDateTime::currentDateTimeUtc();
    BSqlWhere where("expiration_date_time <= :date_time", ":date_time", dt.toMSecsSinceEpoch());
    bSet(ok, Source->deleteFrom("account_recovery_codes", where).success());
}

void AccountRecoveryCodeRepository::deleteOneByUserId(quint64 userId, bool *ok)
{
    if (!isValid() || !userId)
        return bSet(ok, false);
    BSqlWhere where("user_id = :user_id", ":user_id", userId);
    bSet(ok, Source->deleteFrom("account_recovery_codes", where).success());
}

AccountRecoveryCode AccountRecoveryCodeRepository::findOneByCode(const BUuid &code, bool *ok)
{
    AccountRecoveryCode entity(this);
    if (!isValid() || code.isNull())
        return bRet(ok, false, entity);
    static const QStringList Fields = QStringList() << "expiration_date_time" << "user_id";
    BSqlWhere where("code = :code", ":code", code.toString(true));
    BSqlResult result = Source->select("account_recovery_codes", Fields, where);
    if (!result.success())
        return bRet(ok, false, entity);
    if (result.values().isEmpty())
        return bRet(ok, true, entity);
    entity.mcode = code;
    entity.mexpirationDateTime.setMSecsSinceEpoch(result.value("expiration_date_time").toLongLong());
    entity.muserId = result.value("user_id").toULongLong();
    entity.valid = true;
    return bRet(ok, true, entity);
}

DataSource *AccountRecoveryCodeRepository::dataSource() const
{
    return Source;
}

bool AccountRecoveryCodeRepository::isValid() const
{
    return Source && Source->isValid();
}
