#include "src/databaseinteractor.h"
#include "include/texsampleserver.h"

#include <QString>
#include <QReadWriteLock>
#include <QReadLocker>
#include <QWriteLocker>
#include <QSqlDatabase>
#include <QUuid>
#include <QSqlQuery>

QString adminLogin;
QString adminPassword;
QReadWriteLock adminInfoLock;

//

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
        delete db;
        QSqlDatabase::removeDatabase(cn);
        return 0;
    }
    return db;
}

void removeDatabase(QSqlDatabase *db)
{
    if (!db)
        return;
    QString cn = db->connectionName();
    delete db;
    QSqlDatabase::removeDatabase(cn);
}

//

void DatabaseInteractor::setAdminInfo(const QString &login, const QString &password)
{
    QWriteLocker locker(&adminInfoLock);
    adminLogin = login;
    adminPassword = password;
}

bool DatabaseInteractor::checkAdmin()
{
    QSqlDatabase *db = createDatabase();
    if (!db)
        return false;
    QSqlQuery *q = new QSqlQuery(*db);
    adminInfoLock.lockForRead();
    QString login = adminLogin;
    adminInfoLock.unlock();
    bool b = q->exec("CALL pCheckAdmin(\"" + login + "\")") && q->next(); //TODO: improve
    delete q;
    removeDatabase(db);
    return b;
}
