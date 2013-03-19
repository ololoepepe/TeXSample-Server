#include "sqlqueryresult.h"

#include <QVariant>
#include <QVariantList>
#include <QVariantMap>

/*============================================================================
================================ SqlQueryResult ==============================
============================================================================*/

/*============================== Public constructors =======================*/

SqlQueryResult::SqlQueryResult()
{
    msuccess = false;
}

SqlQueryResult::SqlQueryResult(const SqlQueryResult &other)
{
    *this = other;
}

SqlQueryResult::SqlQueryResult(bool success, const QVariantList &values, const QVariant &insertId)
{
    msuccess = success;
    mvalues = values;
    minsertId = insertId;
}

/*============================== Public methods ============================*/

bool SqlQueryResult::success() const
{
    return msuccess;
}

QVariantList SqlQueryResult::values() const
{
    return mvalues;
}

QVariantMap SqlQueryResult::value() const
{
    return !mvalues.isEmpty() ? mvalues.first().toMap() : QVariantMap();
}

QVariant SqlQueryResult::insertId() const
{
    return minsertId;
}

/*============================== Public operators ==========================*/

SqlQueryResult &SqlQueryResult::operator =(const SqlQueryResult &other)
{
    msuccess = other.msuccess;
    mvalues = other.mvalues;
    minsertId = other.minsertId;
    return *this;
}

SqlQueryResult::operator bool() const
{
    return msuccess;
}
