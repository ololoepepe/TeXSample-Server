#include "registrationconfirmationcoderepository.h"

#include "datasource.h"
#include "entity/registrationconfirmationcode.h"
#include "transactionholder.h"

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

bool RegistrationConfirmationCodeRepository::add(const RegistrationConfirmationCode &entity)
{
    if (!isValid() || !entity.isValid() || entity.isCreatedByRepo())
        return false;
    QVariantMap values;
    values.insert("code", entity.code().toString(true));
    values.insert("expiration_date_time", entity.expirationDateTime().toUTC().toMSecsSinceEpoch());
    values.insert("user_id", entity.userId());
    TransactionHolder holder(Source);
    return Source->insert("registration_confirmation_codes_codes", values).success() && holder.doCommit();
}

bool RegistrationConfirmationCodeRepository::deleteExpired()
{
    return isValid() && Source->deleteFrom("registration_confirmation_codes",
                                           BSqlWhere("expiration_date_time <= :date_time", ":date_time",
                                                     QDateTime::currentDateTimeUtc().toMSecsSinceEpoch()));
}

bool RegistrationConfirmationCodeRepository::deleteOneByUserId(quint64 userId)
{
    if (!isValid() || !userId)
        return false;
    TransactionHolder holder(Source);
    return Source->deleteFrom("registration_confirmation_codes_codes",
                              BSqlWhere("user_id = :user_id", ":user_id", userId)).success() && holder.doCommit();
}

RegistrationConfirmationCode RegistrationConfirmationCodeRepository::findOneByCode(const BUuid &code)
{
    RegistrationConfirmationCode entity(this);
    if (!isValid() || code.isNull())
        return entity;
    BSqlResult result = Source->select("registration_confirmation_codes_codes",
                                       QStringList() << "expiration_date_time" << "user_id",
                                       BSqlWhere("code = :code", ":code", code.toString(true)));
    if (!result.success() || result.value().isEmpty())
        return entity;
    entity.mcode = code;
    entity.mexpirationDateTime.setMSecsSinceEpoch(result.value("expiration_date_time").toLongLong());
    entity.muserId = result.value("user_id").toULongLong();
    entity.valid = true;
    return entity;
}

DataSource *RegistrationConfirmationCodeRepository::dataSource() const
{
    return Source;
}

bool RegistrationConfirmationCodeRepository::isValid() const
{
    return Source && Source->isValid();
}
