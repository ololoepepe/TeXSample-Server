/****************************************************************************
**
** Copyright (C) 2012-2014 Andrey Bogdanov
**
** This file is part of TeXSample Server.
**
** TeXSample Server is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** TeXSample Server is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with TeXSample Server.  If not, see <http://www.gnu.org/licenses/>.
**
****************************************************************************/

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
