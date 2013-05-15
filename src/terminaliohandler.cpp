#include "terminaliohandler.h"
#include "server.h"
#include "connection.h"
#include "remotecontrolserver.h"

#include <BNetworkConnection>
#include <BeQt>

#include <QObject>
#include <QString>
#include <QStringList>
#include <QCoreApplication>
#include <QList>
#include <QUuid>
#include <QMetaObject>
#include <QElapsedTimer>

/*============================================================================
================================ TerminalIOHandler ===========================
============================================================================*/

/*============================== Public constructors =======================*/

TerminalIOHandler::TerminalIOHandler(Server *server, RemoteControlServer *rcserver, QObject *parent) :
    BTerminalIOHandler(parent)
{
    mserver = server;
    mrcserver = rcserver;
    melapsedTimer.start();
    installHandler("quit", &BeQt::handleQuit);
    installHandler("exit", &BeQt::handleQuit);
    installHandler("user", (InternalHandler) &TerminalIOHandler::handleUser);
    installHandler("uptime", (InternalHandler) &TerminalIOHandler::handleUptime);
}

/*============================== Public methods ============================*/

void TerminalIOHandler::executeCommand(const QString &command, const QStringList &args)
{
    if (command == "quit" || command == "exit")
    {
        //
    }
    else if (command == "user")
    {
        if (args.isEmpty())
        {
            //
        }
    }
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
    else if (args.first() == "--info")
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
                    writeLine(cc->infoString());
            }
        }
        else
        {
            foreach (BNetworkConnection *c, mserver->connections())
            {
                Connection *cc = static_cast<Connection *>(c);
                if (cc->uniqueId() == uuid)
                    writeLine(cc->infoString());
            }
        }
    }
}

void TerminalIOHandler::handleUptime(const QString &, const QStringList &)
{
    qint64 elapsed = melapsedTimer.elapsed();
    QString days = QString::number(elapsed / (24 * BeQt::Hour));
    elapsed %= (24 * BeQt::Hour);
    QString hours = QString::number(elapsed / BeQt::Hour);
    hours.prepend(QString().fill('0', 2 - hours.length()));
    elapsed %= BeQt::Hour;
    QString minutes = QString::number(elapsed / BeQt::Minute);
    minutes.prepend(QString().fill('0', 2 - minutes.length()));
    elapsed %= BeQt::Minute;
    QString seconds = QString::number(elapsed / BeQt::Second);
    seconds.prepend(QString().fill('0', 2 - seconds.length()));
    writeLine(tr("Uptime:", "") + " " + days + " " + tr("days", "") + " " + hours + ":" + minutes + ":" + seconds);
}
