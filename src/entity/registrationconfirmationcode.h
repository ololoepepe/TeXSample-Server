#ifndef REGISTRATIONCONFIRMATIONCODE_H
#define REGISTRATIONCONFIRMATIONCODE_H

class RegistrationConfirmationCodeRepository;

#include <BUuid>

#include <QDateTime>

/*============================================================================
================================ RegistrationConfirmationCode ================
============================================================================*/

class RegistrationConfirmationCode
{
public:
    explicit RegistrationConfirmationCode();
    RegistrationConfirmationCode(const RegistrationConfirmationCode &other);
    ~RegistrationConfirmationCode();
protected:
    explicit RegistrationConfirmationCode(RegistrationConfirmationCodeRepository *repo);
public:
    BUuid code() const;
    QDateTime expirationDateTime() const;
    bool isValid() const;
    RegistrationConfirmationCodeRepository *repository() const;
    void setCode(const BUuid &code);
    void setExpirationDateTime(const QDateTime &dt);
    void setUserId(quint64 userId);
    quint64 userId() const;
public:
    RegistrationConfirmationCode &operator =(const RegistrationConfirmationCode &other);
private:
    void init();
private:
    BUuid mcode;
    QDateTime mexpirationDateTime;
    quint64 muserId;
    RegistrationConfirmationCodeRepository *repo;
private:
    friend class RegistrationConfirmationCodeRepository;
};

#endif // REGISTRATIONCONFIRMATIONCODE_H
