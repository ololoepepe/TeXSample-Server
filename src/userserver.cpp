#include "userserver.h"
#include "userworker.h"

#include <bnetworkserver.h>
#include <bgenericserver.h>
#include <bnetworkserverworker.h>
#include <bterminaliohandler.h>

#include <QString>
#include <QStringList>
#include <QMap>
#include <QCoreApplication>

UserServer::UserServer(QObject *parent) :
    BNetworkServer(BGenericServer::TcpServer, parent)
{
    //handlers
    mhandlers.insert("exit", &UserServer::handleExit);
    //
    connect( BTerminalIOHandler::instance(), SIGNAL( commandEntered(QString, QStringList) ),
             this, SLOT( commandEntered(QString, QStringList) ) );
}

//

BNetworkServerWorker *UserServer::createWorker()
{
    return new UserWorker;
}

//

void UserServer::handleExit(const QStringList &arguments)
{
    QCoreApplication::quit();
}

//

void UserServer::commandEntered(const QString &command, const QStringList &arguments)
{
    Handler h = mhandlers.value(command);
    if (h)
        (this->*h)(arguments);
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
const QString GroupServer = "server";
  const QString PrefixBanned = "banned";
    const QString KeyAddress = "address";

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
*/
