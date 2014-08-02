#include "transactionholder.h"

#include "datasource.h"

/*============================================================================
================================ TransactionHolder ===========================
============================================================================*/

/*============================== Public constructors =======================*/

TransactionHolder::TransactionHolder(DataSource *source) :
    Source(source)
{
    msuccess = false;
    if (Source && Source->isValid() && !Source->isTransactionActive())
        Source->transaction();
}

TransactionHolder::~TransactionHolder()
{
    if (isValid() && isTransactionActive())
        Source->endTransaction(msuccess);
}

/*============================== Public methods ============================*/

void TransactionHolder::setSuccess(bool success)
{
    msuccess = success;
}

void TransactionHolder::commit()
{
    msuccess = true;
}

bool TransactionHolder::doCommit()
{
    return endTransaction(true);
}

bool TransactionHolder::doRollback()
{
    return endTransaction(false);
}

bool TransactionHolder::endTransaction(bool success)
{
    if (!isValid() || !isTransactionActive())
        return false;
    return Source->endTransaction(success);
}

bool TransactionHolder::isTransactionActive() const
{
    return Source && Source->isValid() && Source->isTransactionActive();
}

bool TransactionHolder::isValid() const
{
    return Source && Source->isValid();
}

void TransactionHolder::rollback()
{
    msuccess = false;
}

DataSource *TransactionHolder::source() const
{
    return Source;
}

bool TransactionHolder::success() const
{
    return msuccess;
}
