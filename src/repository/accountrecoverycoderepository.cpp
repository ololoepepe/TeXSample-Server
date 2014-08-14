#include "accountrecoverycoderepository.h"

#include "datasource.h"
#include "entity/accountrecoverycode.h"
#include "transactionholder.h"

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

bool AccountRecoveryCodeRepository::add(const AccountRecoveryCode &entity)
{
    if (!isValid() || !entity.isValid() || entity.isCreatedByRepo())
        return false;
    QVariantMap values;
    values.insert("code", entity.code().toString(true));
    values.insert("expiration_date_time", entity.expirationDateTime().toUTC().toMSecsSinceEpoch());
    values.insert("user_id", entity.userId());
    TransactionHolder holder(Source);
    return Source->insert("account_recovery_codes", values).success() && holder.doCommit();
}

bool AccountRecoveryCodeRepository::deleteExpired()
{
    return isValid() && Source->deleteFrom("account_recovery_codes",
                                           BSqlWhere("expiration_date_time <= :date_time", ":date_time",
                                                     QDateTime::currentDateTimeUtc().toMSecsSinceEpoch()));
}

bool AccountRecoveryCodeRepository::deleteOneByUserId(quint64 userId)
{
    if (!isValid() || !userId)
        return false;
    TransactionHolder holder(Source);
    return Source->deleteFrom("account_recovery_codes", BSqlWhere("user_id = :user_id", ":user_id", userId))
            && holder.doCommit();
}

AccountRecoveryCode AccountRecoveryCodeRepository::findOneByCode(const BUuid &code)
{
    AccountRecoveryCode entity(this);
    if (!isValid() || code.isNull())
        return entity;
    BSqlResult result = Source->select("account_recovery_codes", QStringList() << "expiration_date_time" << "user_id",
                                       BSqlWhere("code = :code", ":code", code.toString(true)));
    if (!result.success() || result.value().isEmpty())
        return entity;
    entity.mcode = code;
    entity.mexpirationDateTime.setMSecsSinceEpoch(result.value("expiration_date_time").toLongLong());
    entity.muserId = result.value("user_id").toULongLong();
    entity.valid = true;
    return entity;
}

DataSource *AccountRecoveryCodeRepository::dataSource() const
{
    return Source;
}

bool AccountRecoveryCodeRepository::isValid() const
{
    return Source && Source->isValid();
}
