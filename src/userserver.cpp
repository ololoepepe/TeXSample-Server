#include "src/userserver.h"
#include "src/userworker.h"

#include <bnetworkserver.h>
#include <bgenericserver.h>
#include <bnetworkserverworker.h>

UserServer::UserServer(QObject *parent) :
    BNetworkServer(BGenericServer::TcpServer, parent)
{
}

//

BNetworkServerWorker *UserServer::createWorker()
{
    return new UserWorker;
}

/*
#include <QObject>
#include <QTcpServer>
#include <QString>
#include <QHostAddress>
#include <QTcpSocket>
#include <QDir>
#include <QIODevice>
#include <QSqlDatabase>
#include <QStringList>
#include <QCoreApplication>
#include <QTimer>
#include <QDateTime>
#include <QSettings>
#include <QScopedPointer>
#include <QSqlQuery>

#include <QDebug>

const int AutosaveInterval = 6 * BCore::Hour;
//
const QHostAddress Address = QHostAddress::Any;
//
const QString GroupServer = "server";
  const QString PrefixBanned = "banned";
    const QString KeyAddress = "address";

//

void logger(const QString &text)
{
    BStdIO::writeLine(QDateTime::currentDateTime().toString("MM.dd-hh:mm:ss") + ": " + text);
}

//

bool Server::checkAdmin()
{
    QSqlDatabase *db = Connection::createDatabase("texsample-server");
    if (!db)
        return false;
    QSqlQuery *q = new QSqlQuery(*db);
    bool b = q->exec("CALL pCheckAdmin(\"" + Connection::adminLogin() + "\")") && q->next();
    delete q;
    Connection::removeDatabase(db);
    return b;
}

//

Server::Server(QObject *parent) :
    QTcpServer(parent)
{
    BStdIO::writeLine( tr("Starting server...", "stdout text") );
    connect( BStdIO::instance(), SIGNAL( read(QString) ), this, SLOT( read(QString) ) );
    mHandlerMap.insert("save-settings", &Server::handleSaveSettings);
    mHandlerMap.insert("banned", &Server::handleBanned);
    mHandlerMap.insert("ban", &Server::handleBan);
    mHandlerMap.insert("unban", &Server::handleUnban);
    mHandlerMap.insert("connections", &Server::handleConnections);
    mHandlerMap.insert("restart", &Server::handleRestart);
    mHandlerMap.insert("exit", &Server::handleExit);
    if ( !restart() )
    {
        BStdIO::writeLine( tr("Failed to start server. Will now quit.", "stdout text") );
        QTimer::singleShot( 1, QCoreApplication::instance(), SLOT( quit() ) );
        return;
    }
    loadSettings();
    mtimerAutosave = new QTimer(this);
    connect( mtimerAutosave, SIGNAL( timeout() ), this, SLOT( saveSettings() ) );
    mtimerAutosave->start(AutosaveInterval);
    BStdIO::writeLine( tr("Server succesfully started.", "stdout text") );
}

//

void Server::saveSettings()
{
    QScopedPointer<QSettings> s( BCore::newSettingsInstance() );
    if (!s)
        return;
    s->beginGroup(GroupServer);
      s->remove(PrefixBanned);
      s->beginWriteArray(PrefixBanned);
        for (int i = 0; i < mbanList.size(); ++i)
        {
            s->setArrayIndex(i);
            s->setValue( KeyAddress, mbanList.at(i) );
        }
      s->endArray();
    s->endGroup();
    s->sync();
}

//

void Server::incomingConnection(int handle)
{
    ConnectionThread *connection = new ConnectionThread(handle, this);
    connect( connection, SIGNAL( finished() ), this, SLOT( finished() ) );
    connect( connection, SIGNAL( updateNotify() ), this, SLOT( updateNotify() ) );
    connection->start();
    mConnections << connection;
    logger( tr("Added new connection. Connection count:", "log text") + " " + QString::number( mConnections.size() ) );
}

//

void Server::loadSettings()
{
    QScopedPointer<QSettings> s( BCore::newSettingsInstance() );
    if (!s)
        return;
    s->beginGroup(GroupServer);
      int size = s->beginReadArray(PrefixBanned);
        for (int i = 0; i < size; ++i)
        {
            s->setArrayIndex(i);
            QString ip = s->value(KeyAddress).toString();
            if ( !ip.isEmpty() )
                mbanList << ip;
        }
      s->endArray();
    s->endGroup();
}

bool Server::restart()
{
    close();
    return listen(Address, TexSampleServer::ServerPort);
}

bool Server::handleSaveSettings(const QStringList &arguments)
{
    saveSettings();
    BStdIO::writeLine( tr("Settings saved.", "log text") );
    return true;
}

bool Server::handleBanned(const QStringList &arguments)
{
    BStdIO::writeLine( tr("Banned count:", "log text") + " " + QString::number( mbanList.size() ) );
    for (int i = 0; i < mbanList.size(); ++i)
        BStdIO::writeLine( "#" + QString::number(i + 1) + ": " + mbanList.at(i) );
    return true;
}

bool Server::handleBan(const QStringList &arguments)
{
    if ( arguments.size() != 1 || arguments.first().isEmpty() )
    {
        BStdIO::writeLine( tr("Error: invalid arguments.", "log text") );
        return false;
    }
    const QString &ip = arguments.first();
    if ( mbanList.contains(ip) )
    {
        BStdIO::writeLine( tr("This address is already banned.", "log text") );
        return true;
    }
    mbanList << ip;
    BStdIO::writeLine( ip + " " + tr("has been banned.", "log text") );
    for (int i = 0; i < mConnections.size(); ++i)
    {
        Connection *c = mConnections.at(i)->connection();
        if (!c)
            continue;
        if (c->address() == ip)
            c->disconnectFromHost(-1);
    }
    return true;
}

bool Server::handleUnban(const QStringList &arguments)
{
    if ( arguments.size() != 1 || arguments.first().isEmpty() )
    {
        BStdIO::writeLine( tr("Error: invalid arguments.", "log text") );
        return false;
    }
    const QString &ip = arguments.first();
    if ( !mbanList.contains(ip) )
    {
        BStdIO::writeLine( tr("This address is not banned.", "log text") );
        return true;
    }
    mbanList.removeAll(ip);
    BStdIO::writeLine( ip + " " + tr("has been unbanned.", "log text") );
    return false;
}

bool Server::handleConnections(const QStringList &arguments)
{
    BStdIO::writeLine( tr("Connection count:", "log text") + " " + QString::number( mConnections.size() ) );
    if ( arguments.contains("-l") && !mConnections.isEmpty() )
    {
        int j = 1;
        for (int i = 0; i < mConnections.size(); ++i)
        {
            Connection *c = mConnections.at(i)->connection();
            if (!c)
                continue;
            BStdIO::writeLine( "#" + QString::number(j) + ": " + c->uniqueId() + "@" + c->address() );
            j++;
        }
    }
    return true;
}

bool Server::handleRestart(const QStringList &arguments)
{
    if ( restart() )
    {
        BStdIO::writeLine( tr("Ok: Server successfully started.", "log text") );
        return true;
    }
    else
    {
        BStdIO::writeLine( tr("Error: Failed to start server.", "log text") );
        return false;
    }
}

bool Server::handleExit(const QStringList &arguments)
{
    if ( !mConnections.isEmpty() )
    {
        if ( !arguments.contains("-f", Qt::CaseInsensitive) )
        {
            BStdIO::writeLine( tr("Error: There are some active connections. "
                                  "Use the -f parameter if you want to exit anyway.", "log text") );
            return false;
        }
        else
        {
            for (int i = mConnections.size() - 1; i >= 0; --i)
            {
                ConnectionThread *connection = mConnections.takeAt(i);
                connection->quit();
                connection->wait(5 * BCore::Second);
            }
            BStdIO::writeLine( tr("Warning: Server has active connections.", "log text") );
        }
    }
    close();
    QCoreApplication::instance()->quit();
    BStdIO::writeLine( tr("Ok: Exiting...", "log text") );
    return true;
}

//

void Server::read(const QString &text)
{
    QStringList list = BStdIO::splitCommand(text);
    if ( list.isEmpty() )
        return;
    Handler h = mHandlerMap.value( list.takeFirst() );
    if (h)
        (this->*h)(list);
    else
        BStdIO::writeLine( tr("Error: Invalid command.", "log text") );
}

void Server::finished()
{
    ConnectionThread *connection = qobject_cast<ConnectionThread *>( sender() );
    if ( !connection)
        return;
    mConnections.removeAll(connection);
    connection->deleteLater();
    logger( tr("Removed connection. Connection count:", "log text") + " " + QString::number( mConnections.size() ) );
}

void Server::updateNotify()
{
    for (int i = 0; i < mConnections.size(); ++i)
        mConnections.at(i)->requestUpdateNotify();
}
*/
