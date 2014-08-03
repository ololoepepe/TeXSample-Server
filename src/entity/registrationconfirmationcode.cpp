#include "registrationconfirmationcode.h"

#include "repository/registrationconfirmationcoderepository.h"

#include <TeXSample>

#include <QDateTime>

/*============================================================================
================================ RegistrationConfirmationCode ================
============================================================================*/

/*============================== Public constructors =======================*/

RegistrationConfirmationCode::RegistrationConfirmationCode()
{
    init();
}

RegistrationConfirmationCode::RegistrationConfirmationCode(const RegistrationConfirmationCode &other)
{
    init();
    *this = other;
}

RegistrationConfirmationCode::~RegistrationConfirmationCode()
{
    //
}

/*============================== Protected constructors ====================*/

RegistrationConfirmationCode::RegistrationConfirmationCode(RegistrationConfirmationCodeRepository *repo)
{
    init();
    this->repo = repo;
    createdByRepo = true;
}

/*============================== Public methods ============================*/

BUuid RegistrationConfirmationCode::code() const
{
    return mcode;
}

QDateTime RegistrationConfirmationCode::expirationDateTime() const
{
    return mexpirationDateTime;
}

bool RegistrationConfirmationCode::isCreatedByRepo() const
{
    return createdByRepo;
}

bool RegistrationConfirmationCode::isValid() const
{
    if (createdByRepo)
        return valid;
    return !mcode.isNull() && mexpirationDateTime.isValid() && muserId;
}

RegistrationConfirmationCodeRepository *RegistrationConfirmationCode::repository() const
{
    return repo;
}

void RegistrationConfirmationCode::setCode(const BUuid &code)
{
    mcode = code;
}

void RegistrationConfirmationCode::setExpirationDateTime(const QDateTime &dt)
{
    mexpirationDateTime = dt.toUTC();
}

void RegistrationConfirmationCode::setUserId(quint64 userId)
{
    muserId = userId;
}

quint64 RegistrationConfirmationCode::userId() const
{
    return muserId;
}

/*============================== Public operators ==========================*/

RegistrationConfirmationCode &RegistrationConfirmationCode::operator =(const RegistrationConfirmationCode &other)
{
    mcode = other.mcode;
    mexpirationDateTime = other.mexpirationDateTime;
    muserId = other.muserId;
    repo = other.repo;
    return *this;
}

/*============================== Private methods ===========================*/

void RegistrationConfirmationCode::init()
{
    mexpirationDateTime = QDateTime().toUTC();
    muserId = 0;
    createdByRepo = false;
    repo = 0;
    valid = false;
}
