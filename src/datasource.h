#ifndef DATASOURCE_H
#define DATASOURCE_H

class BSqlDatabase;
class BSqlQuery;
class BSqlResult;

#include <BSqlWhere>

#include <QByteArray>
#include <QCoreApplication>
#include <QString>
#include <QVariant>
#include <QVariantList>
#include <QVariantMap>

/*============================================================================
================================ DataSource ==================================
============================================================================*/

class DataSource
{
    Q_DECLARE_TR_FUNCTIONS(DataSource)
private:
    const QString Location;
private:
    BSqlDatabase *db;
    bool trans;
public:
    explicit DataSource(const QString &location);
    ~DataSource();
public:
    bool commit();
    BSqlResult deleteFrom(const QString &table, const BSqlWhere &where = BSqlWhere());
    bool endTransaction(bool success);
    BSqlResult exec(const BSqlQuery &q);
    BSqlResult exec(const QString &qs, const QString &placeholder1 = QString(),
                    const QVariant &boundValue1 = QVariant(), const QString &placeholder2 = QString(),
                    const QVariant &boundValue2 = QVariant());
    BSqlResult exec(const QString &qs, const QVariant &boundValue1, const QVariant &boundValue2 = QVariant());
    BSqlResult exec(const QString &qs, const QVariantList &boundValues);
    BSqlResult exec(const QString &qs, const QVariantMap &boundValues);
    bool initialize(QString *error = 0);
    BSqlResult insert(const QString &table, const QVariantMap &values, const BSqlWhere &where = BSqlWhere());
    BSqlResult insert(const QString &table, const QString &field1, const QVariant &value1,
                      const QString &field2 = QString(), const QVariant &value2 = QVariant(),
                      const BSqlWhere &where = BSqlWhere());
    bool isTransactionActive() const;
    bool isValid() const;
    QString location() const;
    bool rollback();
    BSqlResult select(const QString &table, const QStringList &fields, const BSqlWhere &where = BSqlWhere());
    BSqlResult select(const QString &table, const QString &field, const BSqlWhere &where = BSqlWhere());
    bool transaction();
    BSqlResult update(const QString &table, const QVariantMap &values, const BSqlWhere &where = BSqlWhere());
    BSqlResult update(const QString &table, const QString &field1, const QVariant &value1,
                      const BSqlWhere &where = BSqlWhere());
    BSqlResult update(const QString &table, const QString &field1, const QVariant &value1, const QString &field2,
                      const QVariant &value2, const BSqlWhere &where = BSqlWhere());
public:
    Q_DISABLE_COPY(DataSource)
};

#endif // DATASOURCE_H
