#include "accountrecoverycoderepository.h"

#include "datasource.h"
#include "entity/accountrecoverycode.h"
#include "transactionholder.h"

#include <TIdList>

#include <BSqlResult>
#include <BSqlWhere>

#include <QList>
#include <QString>

/*============================================================================
================================ AccountRecoveryCodeRepository ===============
============================================================================*/

/*============================== Public constructors =======================*/

AccountRecoveryCodeRepository::AccountRecoveryCodeRepository(DataSource *source) :
    Source(source)
{
    //
}

AccountRecoveryCodeRepository::~AccountRecoveryCodeRepository()
{
    //
}

/*============================== Public methods ============================*/

long AccountRecoveryCodeRepository::count()
{
    //
}

DataSource *AccountRecoveryCodeRepository::dataSource() const
{
    return Source;
}

bool AccountRecoveryCodeRepository::deleteAll()
{
    //
}

QList<AccountRecoveryCode> AccountRecoveryCodeRepository::findAll()
{
    //
}

bool AccountRecoveryCodeRepository::isValid() const
{
    return Source && Source->isValid();
}

bool AccountRecoveryCodeRepository::save(const AccountRecoveryCode &entity)
{
    //
}

bool AccountRecoveryCodeRepository::save(const QList<AccountRecoveryCode> &entities)
{
    //
}
