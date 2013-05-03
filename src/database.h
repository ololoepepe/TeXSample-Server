#ifndef DATABASE_H
#define DATABASE_H

class SqlQueryResult;

class QSqlDatabase;
class QVariant;

#include <QString>
#include <QVariantMap>

/*============================================================================
================================ Database ====================================
============================================================================*/

class Database
{
public:
    SqlQueryResult execQuery(const QSqlDatabase &db, const QString &query, const QVariantMap &boundValues);
    bool execQuery(const QSqlDatabase &db, const QString &query, SqlQueryResult &result,
                   const QVariantMap &boundValues);
    SqlQueryResult execQuery(const QSqlDatabase &db, const QString &query,
                             const QString &boundKey1 = QString(), const QVariant &boundValue1 = QVariant(),
                             const QString &boundKey2 = QString(), const QVariant &boundValue2 = QVariant(),
                             const QString &boundKey3 = QString(), const QVariant &boundValue3 = QVariant());
    bool execQuery(const QSqlDatabase &db, const QString &query, SqlQueryResult &result,
                   const QString &boundKey1 = QString(), const QVariant &boundValue1 = QVariant(),
                   const QString &boundKey2 = QString(), const QVariant &boundValue2 = QVariant(),
                   const QString &boundKey3 = QString(), const QVariant &boundValue3 = QVariant());
public:
    explicit Database(const QString &connectionName = QString(), const QString &dbName = QString());
    virtual ~Database();
public:
    void setDatabaseName(const QString &name);
    bool beginDBOperation();
    bool endDBOperation(bool success = true);
    SqlQueryResult execQuery(const QString &query, const QVariantMap &boundValues);
    bool execQuery(const QString &query, SqlQueryResult &result, const QVariantMap &boundValues);
    SqlQueryResult execQuery(const QString &query,
                             const QString &boundKey1 = QString(), const QVariant &boundValue1 = QVariant(),
                             const QString &boundKey2 = QString(), const QVariant &boundValue2 = QVariant(),
                             const QString &boundKey3 = QString(), const QVariant &boundValue3 = QVariant());
    bool execQuery(const QString &query, SqlQueryResult &result,
                   const QString &boundKey1 = QString(), const QVariant &boundValue1 = QVariant(),
                   const QString &boundKey2 = QString(), const QVariant &boundValue2 = QVariant(),
                   const QString &boundKey3 = QString(), const QVariant &boundValue3 = QVariant());
    QSqlDatabase *innerDatabase() const;
private:
    QSqlDatabase *mdb;
private:
    Q_DISABLE_COPY(Database)
};

#endif // DATABASE_H
