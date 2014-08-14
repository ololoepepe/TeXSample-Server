#ifndef ACCOUNTRECOVERYCODEREPOSITORY_H
#define ACCOUNTRECOVERYCODEREPOSITORY_H

class DataSource;

class BUuid;

#include "entity/accountrecoverycode.h"

#include <TIdList>

#include <QList>

/*============================================================================
================================ AccountRecoveryCodeRepository ===============
============================================================================*/

class AccountRecoveryCodeRepository
{
private:
    DataSource * const Source;
public:
    explicit AccountRecoveryCodeRepository(DataSource *source);
    ~AccountRecoveryCodeRepository();
public:
    bool add(const AccountRecoveryCode &entity);
    bool deleteExpired();
    bool deleteOneByUserId(quint64 userId);
    AccountRecoveryCode findOneByCode(const BUuid &code);
    DataSource *dataSource() const;
    bool isValid() const;
private:
    friend class AccountRecoveryCode;
    Q_DISABLE_COPY(AccountRecoveryCodeRepository)
};

#endif // ACCOUNTRECOVERYCODEREPOSITORY_H
