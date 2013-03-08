#include "terminaliohandler.h"
#include "server.h"
#include "connection.h"

#include <BNetworkConnection>

#include <QObject>
#include <QString>
#include <QStringList>
#include <QCoreApplication>
#include <QList>
#include <QUuid>
#include <QMetaObject>

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

void TerminalIOHandler::handleUser(const QString &, const QStringList &args)
{
    if (args.isEmpty())
    {
        BTerminalIOHandler::writeLine(tr("Users:", "") + " " + QString::number(mserver->connections().size()));
    }
    else if (args.first() == "--list")
    {
        foreach (BNetworkConnection *c, mserver->connections())
        {
            Connection *cc = static_cast<Connection *>(c);
            BTerminalIOHandler::writeLine("[" + cc->login() + "] [" + cc->peerAddress()
                                          + "] " + cc->uniqueId().toString());
        }
    }
    else if (args.first() == "--kick")
    {
        if (args.size() < 2)
            return;
        QUuid uuid = BeQt::uuidFromText(args.at(1));
        if (uuid.isNull())
        {
            foreach (BNetworkConnection *c, mserver->connections())
            {
                Connection *cc = static_cast<Connection *>(c);
                if (cc->login() == args.at(1))
                    QMetaObject::invokeMethod(cc, "abort", Qt::QueuedConnection);
            }
        }
        else
        {
            foreach (BNetworkConnection *c, mserver->connections())
            {
                if (c->uniqueId() == uuid)
                {
                    QMetaObject::invokeMethod(c, "abort", Qt::QueuedConnection);
                    break;
                }
            }
        }
    }
}
