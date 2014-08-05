#include "accountrecoverycode.h"

#include "repository/accountrecoverycoderepository.h"

#include <TeXSample>

#include <QDateTime>

/*============================================================================
================================ AccountRecoveryCode =========================
============================================================================*/

/*============================== Public constructors =======================*/

AccountRecoveryCode::AccountRecoveryCode()
{
    init();
}

AccountRecoveryCode::AccountRecoveryCode(const AccountRecoveryCode &other)
{
    init();
    *this = other;
}

AccountRecoveryCode::~AccountRecoveryCode()
{
    //
}

/*============================== Protected constructors ====================*/

AccountRecoveryCode::AccountRecoveryCode(AccountRecoveryCodeRepository *repo)
{
    init();
    this->repo = repo;
    createdByRepo = true;
}

/*============================== Public methods ============================*/

BUuid AccountRecoveryCode::code() const
{
    return mcode;
}

void AccountRecoveryCode::convertToCreatedByUser()
{
    if (!createdByRepo)
        return;
    createdByRepo = false;
    repo = 0;
    valid = false;
}

QDateTime AccountRecoveryCode::expirationDateTime() const
{
    return mexpirationDateTime;
}

bool AccountRecoveryCode::isCreatedByRepo() const
{
    return createdByRepo;
}

bool AccountRecoveryCode::isValid() const
{
    if (createdByRepo)
        return valid;
    return !mcode.isNull() && mexpirationDateTime.isValid() && muserId;
}

AccountRecoveryCodeRepository *AccountRecoveryCode::repository() const
{
    return repo;
}

void AccountRecoveryCode::setCode(const BUuid &code)
{
    mcode = code;
}

void AccountRecoveryCode::setExpirationDateTime(const QDateTime &dt)
{
    mexpirationDateTime = dt.toUTC();
}

void AccountRecoveryCode::setUserId(quint64 userId)
{
    muserId = userId;
}

quint64 AccountRecoveryCode::userId() const
{
    return muserId;
}

/*============================== Public operators ==========================*/

AccountRecoveryCode &AccountRecoveryCode::operator =(const AccountRecoveryCode &other)
{
    mcode = other.mcode;
    mexpirationDateTime = other.mexpirationDateTime;
    muserId = other.muserId;
    createdByRepo = other.createdByRepo;
    repo = other.repo;
    valid = other.valid;
    return *this;
}

/*============================== Private methods ===========================*/

void AccountRecoveryCode::init()
{
    mexpirationDateTime = QDateTime().toUTC();
    muserId = 0;
    createdByRepo = false;
    repo = 0;
    valid = false;
}
