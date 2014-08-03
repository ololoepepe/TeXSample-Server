#ifndef ACCOUNTRECOVERYCODE_H
#define ACCOUNTRECOVERYCODE_H

class AccountRecoveryCodeRepository;

#include <BUuid>

#include <QDateTime>

/*============================================================================
================================ AccountRecoveryCode =========================
============================================================================*/

class AccountRecoveryCode
{
public:
    explicit AccountRecoveryCode();
    AccountRecoveryCode(const AccountRecoveryCode &other);
    ~AccountRecoveryCode();
protected:
    explicit AccountRecoveryCode(AccountRecoveryCodeRepository *repo);
public:
    BUuid code() const;
    QDateTime expirationDateTime() const;
    bool isValid() const;
    AccountRecoveryCodeRepository *repository() const;
    void setCode(const BUuid &code);
    void setExpirationDateTime(const QDateTime &dt);
    void setUserId(quint64 userId);
    quint64 userId() const;
public:
    AccountRecoveryCode &operator =(const AccountRecoveryCode &other);
private:
    void init();
private:
    BUuid mcode;
    QDateTime mexpirationDateTime;
    quint64 muserId;
    AccountRecoveryCodeRepository *repo;
private:
    friend class AccountRecoveryCodeRepository;
};

#endif // ACCOUNTRECOVERYCODE_H
