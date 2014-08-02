#ifndef TRANSACTIONHOLDER_H
#define TRANSACTIONHOLDER_H

class DataSource;

#include <QtGlobal>

/*============================================================================
================================ TransactionHolder ===========================
============================================================================*/

class TransactionHolder
{
private:
    DataSource * const Source;
private:
    bool msuccess;
public:
    explicit TransactionHolder(DataSource *source);
    ~TransactionHolder();
public:
    void setSuccess(bool success);
    void commit();
    bool doCommit();
    bool doRollback();
    bool endTransaction(bool success);
    bool isTransactionActive() const;
    bool isValid() const;
    void rollback();
    DataSource *source() const;
    bool success() const;
public:
    Q_DISABLE_COPY(TransactionHolder)
};

#endif // TRANSACTIONHOLDER_H
