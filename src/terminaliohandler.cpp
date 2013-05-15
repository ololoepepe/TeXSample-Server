#include "terminaliohandler.h"
#include "server.h"
#include "connection.h"
#include "registrationserver.h"
#include "logger.h"

#include <TeXSample>

#include <BNetworkConnection>
#include <BGenericSocket>
#include <BeQt>
#include <BNetworkOperation>
#include <BGenericServer>
#include <BRemoteLogger>
#include <BCoreApplication>

#include <QObject>
#include <QString>
#include <QStringList>
#include <QCoreApplication>
#include <QList>
#include <QUuid>
#include <QMetaObject>
#include <QElapsedTimer>
#include <QVariantMap>
#include <QVariant>
#include <QByteArray>
#include <QCryptographicHash>

#include <QDebug>

/*============================================================================
================================ TerminalIOHandler ===========================
============================================================================*/

/*============================== Static public methods =====================*/

void TerminalIOHandler::write(const QString &text)
{
    BTerminalIOHandler::write(text);
    Logger::sendWriteRequest(text);
}

void TerminalIOHandler::writeLine(const QString &text)
{
    BTerminalIOHandler::writeLine(text);
    Logger::sendWriteLineRequest(text);
}

/*============================== Public constructors =======================*/

TerminalIOHandler::TerminalIOHandler(bool local, QObject *parent) :
    BTerminalIOHandler(parent)
{
    mserver = 0;
    mrserver = 0;
    mremote = 0;
    installHandler("quit", &BeQt::handleQuit);
    installHandler("exit", &BeQt::handleQuit);
    installHandler("user", (InternalHandler) &TerminalIOHandler::handleUser);
    installHandler("uptime", (InternalHandler) &TerminalIOHandler::handleUptime);
    installHandler("connect", (InternalHandler) &TerminalIOHandler::handleConnect);
    installHandler("test", (InternalHandler) &TerminalIOHandler::handleTest);
    melapsedTimer.start();
    if (local)
    {
        mserver = new Server;
        mrserver = new RegistrationServer;
        mserver->listen("0.0.0.0", 9041);
        mrserver->listen("0.0.0.0", 9042);
    }
    else
    {
        mremote = new BNetworkConnection(BGenericSocket::TcpSocket);
        connect(mremote, SIGNAL(requestReceived(BNetworkOperation *)), this, SLOT(remoteRequest(BNetworkOperation *)));
    }
}

TerminalIOHandler::~TerminalIOHandler()
{
    delete mserver;
    delete mrserver;
    delete mremote;
}

/*============================== Public methods ============================*/

void TerminalIOHandler::executeCommand(const QString &cmd, const QStringList &args)
{
    if (cmd == "quit" || cmd == "exit")
        handleQuit(cmd, args);
    else if (cmd == "user")
        handleUser(cmd, args);
    else if (cmd == "uptime")
        handleUptime(cmd, args);
}

/*============================== Private methods ===========================*/

void TerminalIOHandler::handleQuit(const QString &, const QStringList &)
{
    QCoreApplication::quit();
}

void TerminalIOHandler::handleUser(const QString &, const QStringList &args)
{
    if (mremote)
        return;
    if (args.isEmpty())
    {
        writeLine(tr("Users:", "") + " " + QString::number(mserver->connections().size()));
    }
    else if (args.first() == "--list")
    {
        foreach (BNetworkConnection *c, mserver->connections())
        {
            Connection *cc = static_cast<Connection *>(c);
            writeLine("[" + cc->login() + "] [" + cc->peerAddress() + "] " + cc->uniqueId().toString());
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

void TerminalIOHandler::handleConnect(const QString &, const QStringList &args)
{
    if (args.size() != 1)
        return;
    if (args.first().isEmpty())
        return;
    QMetaObject::invokeMethod(this, "connectToHost", Qt::QueuedConnection, Q_ARG(QString, args.first()));
}

void TerminalIOHandler::handleTest(const QString &, const QStringList &args)
{
    QMetaObject::invokeMethod(this, "sendCommand", Qt::QueuedConnection, Q_ARG(QString, args.first()),
                              Q_ARG(QStringList, QStringList(args.mid(1))));
}

/*============================== Private slots =============================*/

void TerminalIOHandler::connectToHost(const QString &hostName)
{
    mremote->connectToHost(hostName, 9043);
    qDebug() << (mremote->isConnected() || mremote->waitForConnected());
    if (mremote->isConnected())
    {
        QVariantMap out;
        out.insert("login", "darkangel");
        out.insert("password", QCryptographicHash::hash("LinveInMyHeart", QCryptographicHash::Sha1));
        BNetworkOperation *op = mremote->sendRequest(Texsample::AuthorizeRequest, out);
        qDebug() << (op->isFinished() || op->waitForFinished());
    }
}

void TerminalIOHandler::sendCommand(const QString &cmd, const QStringList &args)
{
    QVariantMap out;
    out.insert("command", cmd);
    out.insert("arguments", args);
    BNetworkOperation *op = mremote->sendRequest(Texsample::ExecuteCommandRequest, out);
    qDebug() << (op->isFinished() || op->waitForFinished());
    op->deleteLater();
}

void TerminalIOHandler::remoteRequest(BNetworkOperation *op)
{
    QVariantMap in = op->variantData().toMap();
    if (op->metaData().operation() == Texsample::LogRequest)
    {
        QString msg = in.value("log_text").toString();
        bool stderrLevel = in.value("stderr_level").toBool();
        if (stderrLevel)
            writeErr(msg);
        else
            write(msg);
    }
    else if (op->metaData().operation() == Texsample::WriteRequest)
    {
        QString text = in.value("text").toString();
        BTerminalIOHandler::write(text);
    }
    mremote->sendReply(op, QByteArray());
    op->waitForFinished();
    op->deleteLater();
}
