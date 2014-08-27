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

#include "datasource.h"

#include "application.h"
#include "settings.h"
#include "transactionholder.h"
#include "translator.h"

#include <BDirTools>
#include <BSqlDatabase>
#include <BSqlQuery>
#include <BSqlResult>
#include <BSqlWhere>
#include <BUuid>

#include <QByteArray>
#include <QDebug>
#include <QFileInfo>
#include <QLocale>
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
    if (trans)
        rollback();
    delete db;
}

/*============================== Public methods ============================*/

bool DataSource::commit()
{
    if (!isValid() || !trans)
        return false;
    if (!db->commit())
        return false;
    trans = false;
    return true;
}

BSqlResult DataSource::deleteFrom(const QString &table, const BSqlWhere &where)
{
    if (!isValid())
        return BSqlResult();
    return db->deleteFrom(table, where);
}

bool DataSource::endTransaction(bool success)
{
    return success ? commit() : rollback();
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

bool DataSource::initialize(QString *error)
{
    if (!isValid())
        return bRet(error, tr("Invalid DataSource instance", "error"), false);
    QString fn = BDirTools::findResource("db/texsample.schema", BDirTools::GlobalOnly);
    QStringList list = BSqlDatabase::schemaFromFile(fn, "UTF-8");
    if (list.isEmpty())
        return bRet(error, tr("Failed to load database schema", "error"), false);
    if (Settings::Server::readonly()) {
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
    if (!Settings::Server::readonly() && !db->initializeFromSchema(list, false))
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

BSqlResult DataSource::insert(const QString &table, const QString &field, const QVariant &value,
                              const BSqlWhere &where)
{
    if (!isValid())
        return BSqlResult();
    return db->insert(table, field, value, where);
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

bool DataSource::rollback()
{
    if (!isValid() || !trans)
        return false;
    if (!db->rollback())
        return false;
    trans = false;
    return true;
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

bool DataSource::shrinkDB(const Translator &t, QString *error)
{
    if (!isValid())
        return bRet(error, t.translate("DataSource", "Invalid DataSource instance (internal)", "error"), false);
    if (trans) {
        return bRet(error, t.translate("DataSource", "Unable to shrink DB while ther are active transactions",
                                       "error"), false);
    }
    BSqlResult result = exec(BSqlQuery("VACUUM"));
    if (!result.success())
        return bRet(error, t.translate("DataSource", "Failed to execute VACUUM command (internal)", "error"), false);
    return bRet(error, QString(), true);
}

bool DataSource::shrinkDB(QString *error)
{
    Translator t(Application::locale());
    return shrinkDB(t, error);
}

bool DataSource::transaction()
{
    if (trans || !isValid())
        return false;
    if (!db->transaction())
        return false;
    trans = true;
    return true;
}

BSqlResult DataSource::update(const QString &table, const QVariantMap &values, const BSqlWhere &where)
{
    if (!isValid())
        return BSqlResult();
    return db->update(table, values, where);
}

BSqlResult DataSource::update(const QString &table, const QString &field, const QVariant &value,
                              const BSqlWhere &where)
{
    if (!isValid())
        return BSqlResult();
    return db->update(table, field, value, where);
}

BSqlResult DataSource::update(const QString &table, const QString &field1, const QVariant &value1,
                              const QString &field2, const QVariant &value2, const BSqlWhere &where)
{
    if (!isValid())
        return BSqlResult();
    return db->update(table, field1, value1, field2, value2, where);
}
