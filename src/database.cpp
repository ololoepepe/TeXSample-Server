#include "database.h"
#include "sqlqueryresult.h"

#include <BeQtGlobal>

#include <QString>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QVariant>
#include <QVariantMap>
#include <QVariantList>
#include <QSqlRecord>

#include <QDebug>

/*============================================================================
================================ Database ====================================
============================================================================*/

/*============================== Static public methods =====================*/

SqlQueryResult Database::execQuery(const QSqlDatabase &db, const QString &query, const QVariantMap &boundValues)
{
    if (query.isEmpty() || !db.isOpen())
        return SqlQueryResult();
    QSqlQuery q(db);
    if (!q.prepare(query))
        return SqlQueryResult();
    foreach (const QString &key, boundValues.keys())
        q.bindValue(key, boundValues.value(key));
    if (!q.exec())
        return SqlQueryResult();
    QVariantList values;
    while (q.next())
    {
        QSqlRecord r = q.record();
        QVariantMap m;
        foreach (int i, bRangeD(0, r.count() - 1))
            m.insert(r.fieldName(i), r.value(i));
        if (!m.isEmpty())
            values << m;
    }
    return SqlQueryResult(true, values, q.lastInsertId());
}

bool Database::execQuery(const QSqlDatabase &db, const QString &query, SqlQueryResult &result,
                         const QVariantMap &boundValues)
{
    return result = execQuery(db, query, boundValues);
}

SqlQueryResult Database::execQuery(const QSqlDatabase &db, const QString &query,
                                   const QString &boundKey1, const QVariant &boundValue1,
                                   const QString &boundKey2, const QVariant &boundValue2,
                                   const QString &boundKey3, const QVariant &boundValue3)
{
    QVariantMap m;
    if (!boundKey1.isEmpty())
        m.insert(boundKey1, boundValue1);
    if (!boundKey2.isEmpty())
        m.insert(boundKey2, boundValue2);
    if (!boundKey3.isEmpty())
        m.insert(boundKey3, boundValue3);
    return execQuery(db, query, m);
}

bool Database::execQuery(const QSqlDatabase &db, const QString &query, SqlQueryResult &result,
                         const QString &boundKey1, const QVariant &boundValue1,
                         const QString &boundKey2, const QVariant &boundValue2,
                         const QString &boundKey3, const QVariant &boundValue3)
{
    return result = execQuery(db, query, boundKey1, boundValue1, boundKey2, boundValue2, boundKey3, boundValue3);
}

bool Database::tableExists(const QSqlDatabase &db, const QString &tableName)
{
    if (tableName.isEmpty() || !db.isOpen())
        return false;
    return execQuery(db, "SELECT count(*) FROM sqlite_master WHERE type='table' AND name='"
                     + tableName + "'").value("count(*)").toLongLong();
}

/*============================== Public constructors =======================*/

Database::Database(const QString &connectionName, const QString &dbName)
{
    mdb = new QSqlDatabase(QSqlDatabase::addDatabase("QSQLITE", connectionName));
    if (!dbName.isEmpty())
        setDatabaseName(dbName);
}

Database::~Database()
{
    endDBOperation(false);
    mdb->close();
    QString cn = mdb->connectionName();
    delete mdb;
    QSqlDatabase::removeDatabase(cn);
}

/*============================== Public methods ============================*/

void Database::setDatabaseName(const QString &name)
{
    mdb->setDatabaseName(name);
}

bool Database::open()
{
    return mdb->open();
}

void Database::close()
{
    mdb->close();
}

bool Database::beginDBOperation()
{
    if (!mdb->isOpen() && !mdb->open())
        return false;
    bool b = mdb->transaction() && execQuery("PRAGMA foreign_keys = ON").success();
    if (!b)
        endDBOperation(false);
    return b;
}

bool Database::endDBOperation(bool success)
{
    if (!mdb->isOpen())
        return true;
    bool b = success ? mdb->commit() : mdb->rollback();
    return b;
}

bool Database::isOpen() const
{
    return mdb->isOpen();
}

SqlQueryResult Database::execQuery(const QString &query, const QVariantMap &boundValues)
{
    return execQuery(*mdb, query, boundValues);
}

bool Database::execQuery(const QString &query, SqlQueryResult &result, const QVariantMap &boundValues)
{
    return execQuery(*mdb, query, result, boundValues);
}

SqlQueryResult Database::execQuery(const QString &query, const QString &boundKey1, const QVariant &boundValue1,
                                   const QString &boundKey2, const QVariant &boundValue2,
                                   const QString &boundKey3, const QVariant &boundValue3)
{
    return execQuery(*mdb, query, boundKey1, boundValue1, boundKey2, boundValue2, boundKey3, boundValue3);
}

bool Database::execQuery(const QString &query, SqlQueryResult &result,
                         const QString &boundKey1, const QVariant &boundValue1,
                         const QString &boundKey2, const QVariant &boundValue2,
                         const QString &boundKey3, const QVariant &boundValue3)
{
    return execQuery(*mdb, query, result, boundKey1, boundValue1, boundKey2, boundValue2, boundKey3, boundValue3);
}

bool Database::tableExists(const QString &tableName)
{
    return tableExists(*mdb, tableName);
}

QSqlDatabase *Database::innerDatabase() const
{
    return mdb;
}
