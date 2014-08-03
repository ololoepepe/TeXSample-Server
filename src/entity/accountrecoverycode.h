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
private:
    quint64 muserId;
    BUuid mcode;
    QDateTime mexpirationDateTime;
    bool createdByRepo;
    AccountRecoveryCodeRepository *repo;
    bool valid;
public:
    explicit AccountRecoveryCode();
    AccountRecoveryCode(const AccountRecoveryCode &other);
    ~AccountRecoveryCode();
protected:
    explicit AccountRecoveryCode(AccountRecoveryCodeRepository *repo);
public:
    BUuid code() const;
    bool isCreatedByRepo() const;
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
    friend class AccountRecoveryCodeRepository;
};

#endif // ACCOUNTRECOVERYCODE_H
