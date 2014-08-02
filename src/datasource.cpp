#include "datasource.h"

#include "global.h"
#include "transactionholder.h"

#include <BDirTools>
#include <BSqlDatabase>
#include <BSqlQuery>
#include <BSqlResult>
#include <BSqlWhere>
#include <BUuid>

#include <QByteArray>
#include <QDebug>
#include <QFileInfo>
#include <QRegExp>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <QVariantList>
#include <QVariantMap>

/*============================================================================
================================ DataSource ==================================
============================================================================*/

/*============================== Public constructors =======================*/

DataSource::DataSource(const QString &location) :
    Location(location)
{
    trans = false;
    if (location.isEmpty() || !QFileInfo(location).isDir() || !BDirTools::touch(location + "/texsample.sqlite")) {
        db = 0;
        return;
    }
    db = new BSqlDatabase("QSQLITE", BUuid::createUuid().toString(true));
    db->setDatabaseName(location + "/texsample.sqlite");
    db->setOnOpenQuery("PRAGMA foreign_keys = ON");
    if (!db->open()) {
        delete db;
        db = 0;
    }
}

DataSource::~DataSource()
{
    delete db;
}

/*============================== Public methods ============================*/

bool DataSource::commit()
{
    return endTransaction(true);
}

bool DataSource::createFile(const QString &fileName, const QByteArray &data)
{
    //
}

bool DataSource::createTextFile(const QString &fileName, const QString &text)
{
    //
}

bool DataSource::deleteFile(const QString &fileName)
{
    //
}

BSqlResult DataSource::deleteFrom(const QString &table, const BSqlWhere &where)
{
    if (!isValid())
        return BSqlResult();
    return db->deleteFrom(table, where);
}

bool DataSource::endTransaction(bool success)
{
    if (!isValid() || !trans)
        return false;
    //TODO: FS transaction
    bool b = success ? db->commit() : db->rollback();
    if (!b) {
        //TODO: Do not cancel FS transaction
    }
    if (b)
        trans = false;
    return b;
}

BSqlResult DataSource::exec(const BSqlQuery &q)
{
    if (!isValid())
        return BSqlResult();
    return db->exec(q);
}

BSqlResult DataSource::exec(const QString &qs, const QString &placeholder1, const QVariant &boundValue1,
                            const QString &placeholder2, const QVariant &boundValue2)
{
    if (!isValid())
        return BSqlResult();
    return db->exec(qs, placeholder1, boundValue1, placeholder2, boundValue2);
}

BSqlResult DataSource::exec(const QString &qs, const QVariant &boundValue1, const QVariant &boundValue2)
{
    if (!isValid())
        return BSqlResult();
    return db->exec(qs, boundValue1, boundValue2);
}

BSqlResult DataSource::exec(const QString &qs, const QVariantList &boundValues)
{
    if (!isValid())
        return BSqlResult();
    return db->exec(qs, boundValues);
}

BSqlResult DataSource::exec(const QString &qs, const QVariantMap &boundValues)
{
    if (!isValid())
        return BSqlResult();
    return db->exec(qs, boundValues);
}

bool DataSource::fileExists(const QString &fileName)
{
    //
}

bool DataSource::initialize(QString *error)
{
    if (!isValid())
        return bRet(error, tr("Invalid DataSource instance", "error"), false);
    QString fn = BDirTools::findResource("db/texsample.schema", BDirTools::GlobalOnly);
    QStringList list = BSqlDatabase::schemaFromFile(fn, "UTF-8");
    if (list.isEmpty())
        return bRet(error, tr("Failed to load database schema", "error"), false);
    if (Global::readOnly()) {
        foreach (const QString &qs, list) {
            QString table;
            if (qs.startsWith("CREATE TABLE IF NOT EXISTS"))
                table = qs.section(QRegExp("\\s+"), 5, 5);
            else if (qs.startsWith("CREATE TABLE"))
                table = qs.section(QRegExp("\\s+"), 2, 2);
            if (table.isEmpty())
                continue;
            if (!db->tableExists(table))
                return bRet(error, tr("Can not create tables in read-only mode", "error"), false);
        }
    }
    TransactionHolder holder(this);
    if (!Global::readOnly() && !db->initializeFromSchema(list, false))
        return bRet(error, tr("Database initialization failed", "error"), false);
    holder.setSuccess(true);
    return bRet(error, QString(), true);
}

BSqlResult DataSource::insert(const QString &table, const QVariantMap &values, const BSqlWhere &where)
{
    if (!isValid())
        return BSqlResult();
    return db->insert(table, values, where);
}

BSqlResult DataSource::insert(const QString &table, const QString &field1, const QVariant &value1,
                              const QString &field2, const QVariant &value2, const BSqlWhere &where)
{
    if (!isValid())
        return BSqlResult();
    return db->insert(table, field1, value1, field2, value2, where);
}

bool DataSource::isTransactionActive() const
{
    return trans;
}

bool DataSource::isValid() const
{
    return db;
}

QString DataSource::location() const
{
    return Location;
}

QByteArray DataSource::readFile(const QString &fileName, bool *ok)
{
    //
}

QString DataSource::readTextFile(const QString &fileName, bool *ok)
{
    //
}

bool DataSource::rollback()
{
    return endTransaction(false);
}

BSqlResult DataSource::select(const QString &table, const QStringList &fields, const BSqlWhere &where)
{
    if (!isValid())
        return BSqlResult();
    return db->select(table, fields, where);
}

BSqlResult DataSource::select(const QString &table, const QString &field, const BSqlWhere &where)
{
    if (!isValid())
        return BSqlResult();
    return db->select(table, field, where);
}

bool DataSource::transaction()
{
    if (trans || !isValid() || !db->transaction())
        return false;
    //TODO: FS transaction
    trans = true;
    return true;
}

BSqlResult DataSource::update(const QString &table, const QVariantMap &values, const BSqlWhere &where)
{
    if (!isValid())
        return BSqlResult();
    return db->update(table, values, where);
}

BSqlResult DataSource::update(const QString &table, const QString &field1, const QVariant &value1,
                              const QString &field2, const QVariant &value2, const BSqlWhere &where)
{
    if (!isValid())
        return BSqlResult();
    return db->update(table, field1, value1, field2, value2, where);
}

bool DataSource::updateFile(const QString &fileName, const QByteArray &data)
{
    //
}

bool DataSource::updateTextFile(const QString &fileName, const QString &text)
{
    //
}
