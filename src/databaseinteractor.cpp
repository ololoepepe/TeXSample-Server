#include "src/databaseinteractor.h"
#include "include/texsampleserver.h"

#include <QString>
#include <QReadWriteLock>
#include <QReadLocker>
#include <QWriteLocker>
#include <QSqlDatabase>
#include <QUuid>
#include <QSqlQuery>
#include <QStringList>
#include <QVariantList>
#include <QVariant>

#include <QDebug>

QString adminLogin;
QString adminPassword;
QReadWriteLock adminInfoLock;

//

QSqlDatabase *createDatabase();
void removeDatabase(QSqlDatabase *db);

QSqlDatabase *createDatabase()
{
    QString cn = QUuid::createUuid().toString();
    QSqlDatabase *db = new QSqlDatabase( QSqlDatabase::addDatabase(TexSampleServer::DBType, cn) );
    db->setHostName("127.0.0.1");
    db->setPort(TexSampleServer::DBPort);
    db->setDatabaseName(TexSampleServer::DBName);
    adminInfoLock.lockForRead();
    db->setUserName(adminLogin);
    db->setPassword(adminPassword);
    adminInfoLock.unlock();
    if ( !db->open() )
    {
        removeDatabase(db);
        return 0;
    }
    return db;
}

void removeDatabase(QSqlDatabase *db)
{
    if (!db)
        return;
    if ( db->isOpen() )
        db->close();
    QString cn = db->connectionName();
    delete db;
    QSqlDatabase::removeDatabase(cn);
}

bool callStoredProcedure(const QString &procedure, const QStringList &parameters, QVariantList &results)
{
    if ( procedure.isEmpty() )
        return false;
    QSqlDatabase *db = createDatabase();
    if (!db)
        return false;
    QSqlQuery *q = new QSqlQuery(*db);
    QString qs = "CALL " + procedure + "(";
    for (int i = 0; i < parameters.size(); ++i)
        qs += "\'" + parameters.at(i) + "\'" +( (i < parameters.size() - 1) ? ", " : "" );
    qs += ")";
    bool b = q->exec(qs);
    while ( q->next() )
        results << q->value(0);
    delete q;
    removeDatabase(db);
    return b;
}

//

void DatabaseInteractor::setAdminInfo(const QString &login, const QString &password)
{
    QWriteLocker locker(&adminInfoLock);
    adminLogin = login;
    adminPassword = password;
}

bool DatabaseInteractor::checkAdmin() //TODO: improve
{
    adminInfoLock.lockForRead();
    QString login = adminLogin;
    adminInfoLock.unlock();
    if ( login.isEmpty() )
        return false;
    QVariantList results;
    bool b = callStoredProcedure("pCheckAdmin", QStringList() << login, results);
    return b && !results.isEmpty();
}

bool DatabaseInteractor::checkUser(const QString &login, const QString &password)
{
    if ( login.isEmpty() || password.isEmpty() )
        return false;
    QVariantList results;
    bool b = callStoredProcedure("pCheckUser", QStringList() << login << password, results);
    return b && !results.isEmpty();
}

QString DatabaseInteractor::insertSample(const QString &title, const QString &tags,
                                         const QString &comment, const QString &user)
{
    //TODO: Needs to be replaced by a stored procedure execution, but procedures seem not work properly
    QString id;
    if ( title.isEmpty() || user.isEmpty() )
        return id;
    QSqlDatabase *db = createDatabase();
    if (!db)
        return id;
    QSqlQuery *q = new QSqlQuery(*db);
    q->prepare("INSERT INTO " + TexSampleServer::DBTable + " (title, tags, comment, user) "
               "VALUES (:title, :tags, :comment, :user)");
    q->bindValue(":title", title);
    q->bindValue(":tags", tags);
    q->bindValue(":comment", comment);
    q->bindValue(":user", user);
    q->exec();
    id = q->lastInsertId().toString();
    delete q;
    removeDatabase(db);
    return id;
}

bool DatabaseInteractor::deleteSample(const QString &id)
{
    if ( id.isEmpty() )
        return false;
    QSqlDatabase *db = createDatabase();
    if (!db)
        return false;
    QSqlQuery *q = new QSqlQuery(*db);
    bool b = q->exec("DELETE FROM " + TexSampleServer::DBTable + " WHERE id=\"" + id + "\"");
    delete q;
    removeDatabase(db);
    return b;
}
