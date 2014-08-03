#ifndef ACCOUNTRECOVERYCODEREPOSITORY_H
#define ACCOUNTRECOVERYCODEREPOSITORY_H

class DataSource;

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
    long count();
    DataSource *dataSource() const;
    bool deleteAll();
    QList<AccountRecoveryCode> findAll();
    bool isValid() const;
    bool save(const AccountRecoveryCode &entity);
    bool save(const QList<AccountRecoveryCode> &entities);
private:
    friend class AccountRecoveryCode;
    Q_DISABLE_COPY(AccountRecoveryCodeRepository)
};

#endif // ACCOUNTRECOVERYCODEREPOSITORY_H
