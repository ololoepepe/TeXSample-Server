#ifndef SQLQUERYRESULT_H
#define SQLQUERYRESULT_H

#include <QVariant>
#include <QVariantList>
#include <QVariantMap>

/*============================================================================
================================ SqlQueryResult ==============================
============================================================================*/

class SqlQueryResult
{
public:
    explicit SqlQueryResult();
    SqlQueryResult(const SqlQueryResult &other);
    explicit SqlQueryResult(bool success, const QVariantList &values, const QVariant &insertId);
public:
    bool success() const;
    QVariantList values() const;
    QVariantMap value() const;
    QVariant value(const QString &id) const;
    QVariant insertId() const;
public:
    SqlQueryResult &operator =(const SqlQueryResult &other);
    operator bool() const;
private:
    bool msuccess;
    QVariantList mvalues;
    QVariant minsertId;
};

#endif // SQLQUERYRESULT_H
