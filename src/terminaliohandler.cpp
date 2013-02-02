#include "terminaliohandler.h"
#include "server.h"
#include "connection.h"

#include <BNetworkConnection>

#include <QObject>
#include <QString>
#include <QStringList>
#include <QCoreApplication>
#include <QList>

/*============================================================================
================================ TerminalIOHandler ===========================
============================================================================*/

/*============================== Public constructors =======================*/

TerminalIOHandler::TerminalIOHandler(Server *server, QObject *parent) :
    BTerminalIOHandler(parent)
{
    mserver = server;
    installHandler("quit", (InternalHandler) &TerminalIOHandler::handleQuit);
    installHandler("exit", (InternalHandler) &TerminalIOHandler::handleQuit);
    installHandler("user", (InternalHandler) &TerminalIOHandler::handleUser);
}

/*============================== Private methods ===========================*/

void TerminalIOHandler::handleQuit(const QString &, const QStringList &)
{
    QCoreApplication::quit();
}

void TerminalIOHandler::handleUser(const QString &, const QStringList &)
{
    BTerminalIOHandler::writeLine( tr("Users:", "") + " " + QString::number( mserver->connections().size() ) );
}
