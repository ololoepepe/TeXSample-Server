#include "accountrecoverycode.h"

//#include "repository/accountrecoverycoderepository.h"

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
}

/*============================== Public methods ============================*/

BUuid AccountRecoveryCode::code() const
{
    return mcode;
}

QDateTime AccountRecoveryCode::expirationDateTime() const
{
    return mexpirationDateTime;
}

bool AccountRecoveryCode::isValid() const
{
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
    repo = other.repo;
    return *this;
}

/*============================== Private methods ===========================*/

void AccountRecoveryCode::init()
{
    mexpirationDateTime = QDateTime().toUTC();
    muserId = 0;
    repo = 0;
}
