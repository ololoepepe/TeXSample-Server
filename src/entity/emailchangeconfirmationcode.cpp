#include "emailchangeconfirmationcode.h"

#include "repository/emailchangeconfirmationcoderepository.h"

#include <TeXSample>

#include <QDateTime>
#include <QString>

/*============================================================================
================================ EmailChangeConfirmationCode =================
============================================================================*/

/*============================== Public constructors =======================*/

EmailChangeConfirmationCode::EmailChangeConfirmationCode()
{
    init();
}

EmailChangeConfirmationCode::EmailChangeConfirmationCode(const EmailChangeConfirmationCode &other)
{
    init();
    *this = other;
}

EmailChangeConfirmationCode::~EmailChangeConfirmationCode()
{
    //
}

/*============================== Protected constructors ====================*/

EmailChangeConfirmationCode::EmailChangeConfirmationCode(EmailChangeConfirmationCodeRepository *repo)
{
    init();
    this->repo = repo;
    createdByRepo = true;
}

/*============================== Public methods ============================*/

BUuid EmailChangeConfirmationCode::code() const
{
    return mcode;
}

void EmailChangeConfirmationCode::convertToCreatedByUser()
{
    if (!createdByRepo)
        return;
    createdByRepo = false;
    repo = 0;
    valid = false;
}

QString EmailChangeConfirmationCode::email() const
{
    return memail;
}

QDateTime EmailChangeConfirmationCode::expirationDateTime() const
{
    return mexpirationDateTime;
}

bool EmailChangeConfirmationCode::isCreatedByRepo() const
{
    return createdByRepo;
}

bool EmailChangeConfirmationCode::isValid() const
{
    if (createdByRepo)
        return valid;
    return !mcode.isNull() && !memail.isEmpty() && mexpirationDateTime.isValid() && muserId;
}

EmailChangeConfirmationCodeRepository *EmailChangeConfirmationCode::repository() const
{
    return repo;
}

void EmailChangeConfirmationCode::setCode(const BUuid &code)
{
    mcode = code;
}

void EmailChangeConfirmationCode::setEmail(const QString &email)
{
    memail = Texsample::testEmail(email) ? email : QString();
}

void EmailChangeConfirmationCode::setExpirationDateTime(const QDateTime &dt)
{
    mexpirationDateTime = dt.toUTC();
}

void EmailChangeConfirmationCode::setUserId(quint64 userId)
{
    muserId = userId;
}

quint64 EmailChangeConfirmationCode::userId() const
{
    return muserId;
}

/*============================== Public operators ==========================*/

EmailChangeConfirmationCode &EmailChangeConfirmationCode::operator =(const EmailChangeConfirmationCode &other)
{
    mcode = other.mcode;
    memail = other.memail;
    mexpirationDateTime = other.mexpirationDateTime;
    muserId = other.muserId;
    createdByRepo = other.createdByRepo;
    repo = other.repo;
    valid = other.valid;
    return *this;
}

/*============================== Private methods ===========================*/

void EmailChangeConfirmationCode::init()
{
    mexpirationDateTime = QDateTime().toUTC();
    muserId = 0;
    createdByRepo = false;
    repo = 0;
    valid = false;
}
