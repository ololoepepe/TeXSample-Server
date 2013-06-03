#include "terminaliohandler.h"
#include "server.h"
#include "connection.h"
#include "registrationserver.h"

#include <TeXSample>
#include <TOperationResult>
#include <TClientInfo>

#include <BNetworkConnection>
#include <BGenericSocket>
#include <BeQt>
#include <BNetworkOperation>
#include <BGenericServer>
#include <BLogger>
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
#include <QAbstractSocket>

#include <QDebug>

/*============================================================================
================================ TerminalIOHandler ===========================
============================================================================*/

/*============================== Static public methods =====================*/

void TerminalIOHandler::write(const QString &text)
{
    BTerminalIOHandler::write(text);
    Server::sendWriteRequest(text);
}

void TerminalIOHandler::writeLine(const QString &text)
{
    BTerminalIOHandler::writeLine(text);
    Server::sendWriteLineRequest(text);
}

QString TerminalIOHandler::mailPassword()
{
    return mmailPassword;
}

/*============================== Public constructors =======================*/

TerminalIOHandler::TerminalIOHandler(bool local, QObject *parent) :
    BTerminalIOHandler(parent)
{
    mserver = 0;
    mrserver = 0;
    mremote = 0;
    muserId = 0;
    installHandler("quit", &BeQt::handleQuit);
    installHandler("exit", &BeQt::handleQuit);
    installHandler("user", (InternalHandler) &TerminalIOHandler::handleUser);
    installHandler("uptime", (InternalHandler) &TerminalIOHandler::handleUptime);
    installHandler("connect", (InternalHandler) &TerminalIOHandler::handleConnect);
    installHandler("disconnect", (InternalHandler) &TerminalIOHandler::handleDisconnect);
    installHandler("remote", (InternalHandler) &TerminalIOHandler::handleRemote);
    installHandler("r", (InternalHandler) &TerminalIOHandler::handleRemote);
    melapsedTimer.start();
    if (local)
    {
        mserver = new Server(this);
        mrserver = new RegistrationServer(this);
        mserver->listen("0.0.0.0", Texsample::MainPort);
        mrserver->listen("0.0.0.0", Texsample::RegistrationPort);
        setStdinEchoEnabled(false);
        mmailPassword = readLine(tr("Enter e-mail password:", "") + " ");
        setStdinEchoEnabled(true);
        writeLine("");
    }
    else
    {
        mremote = new BNetworkConnection(BGenericSocket::TcpSocket, this);
        mremote->setLogger(new BLogger);
        mremote->logger()->setLogToConsoleEnabled(false);
        connect(mremote, SIGNAL(disconnected()), this, SLOT(disconnected()));
        connect(mremote, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(error(QAbstractSocket::SocketError)));
        connect(mremote, SIGNAL(requestReceived(BNetworkOperation *)), this, SLOT(remoteRequest(BNetworkOperation *)));
    }
}

TerminalIOHandler::~TerminalIOHandler()
{
    //
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

void TerminalIOHandler::connectToHost(const QString &hostName)
{
    if (hostName.isEmpty())
        return;
    writeLine(tr("Connecting to", "") + " " + hostName + "...");
    handleConnect("", QStringList() << hostName);
}

Server *TerminalIOHandler::server() const
{
    return mserver;
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
    if (!mremote)
        return;
    if (args.size() != 1)
        return writeLine(tr("Invalid usage", "") + "\nconnect <host>");
    QString login = readLine(tr("Enter login:", "") + " ");
    if (login.isEmpty())
        return;
    write(tr("Enter password:", "") + " ");
    setStdinEchoEnabled(false);
    QString password = readLine();
    setStdinEchoEnabled(true);
    writeLine("");
    if (password.isEmpty())
        return;
    QMetaObject::invokeMethod(this, "connectToHost", Qt::QueuedConnection, Q_ARG(QString, args.first()),
                              Q_ARG(QString, login), Q_ARG(QString, password));
}

void TerminalIOHandler::handleDisconnect(const QString &, const QStringList &)
{
    if (!mremote)
        return;
    QMetaObject::invokeMethod(this, "disconnectFromHost", Qt::QueuedConnection);
}

void TerminalIOHandler::handleRemote(const QString &, const QStringList &args)
{
    if (!mremote)
        return;
    QMetaObject::invokeMethod(this, "sendCommand", Qt::QueuedConnection, Q_ARG(QString, args.first()),
                              Q_ARG(QStringList, QStringList(args.mid(1))));
}

/*============================== Private slots =============================*/

void TerminalIOHandler::connectToHost(const QString &hostName, const QString &login, const QString &password)
{
    if (mremote->isConnected())
        return write(tr("Already connected to", "") + " " + mremote->peerAddress());
    mremote->connectToHost(hostName, 9041);
    if (!mremote->isConnected() && !mremote->waitForConnected())
    {
        mremote->close();
        writeLine(tr("Failed to connect to", "") + " " + hostName);
        return;
    }
    writeLine(tr("Connected to", "") + " " + hostName + ". " + tr("Authorizing...", ""));
    QVariantMap out;
    out.insert("login", login);
    out.insert("password", QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Sha1));
    out.insert("client_info", TClientInfo::createDefaultInfo());
    out.insert("subscription", true);
    BNetworkOperation *op = mremote->sendRequest(Texsample::AuthorizeRequest, out);
    if (!op->isFinished() && !op->isError() && !op->waitForFinished())
    {
        op->cancel();
        op->deleteLater();
        writeLine(tr("Authorization timed out", ""));
        return;
    }
    op->deleteLater();
    if (op->isError())
    {
        mremote->close();
        writeLine(tr("Operation error", ""));
        return;
    }
    QVariantMap in = op->variantData().toMap();
    TOperationResult r = in.value("operation_result").value<TOperationResult>();
    quint64 id = in.value("user_id").toULongLong();
    if (!r)
    {
        mremote->close();
        writeLine(tr("Authorization failed. The following error occured:", "") + " " + r.errorString());
        return;
    }
    muserId = id;
    writeLine(tr("Authorized successfully with user id", "") + " " + QString::number(id));
}

void TerminalIOHandler::disconnectFromHost()
{
    if (!mremote->isConnected())
        return;
    mremote->disconnectFromHost();
    if (mremote->isConnected() && !mremote->waitForDisconnected())
    {
        writeLine(tr("Disconnect timeout, closing socket", ""));
        mremote->close();
        return;
    }
    writeLine(tr("Disconnected", ""));
}

void TerminalIOHandler::sendCommand(const QString &cmd, const QStringList &args)
{
    if (!muserId)
        return writeLine(tr("Not authoized", ""));
    QVariantMap out;
    out.insert("command", cmd);
    out.insert("arguments", args);
    BNetworkOperation *op = mremote->sendRequest(Texsample::ExecuteCommandRequest, out);
    if (!op->isFinished() && !op->isError() && !op->waitForFinished())
    {
        op->deleteLater();
        writeLine(tr("Operation error", ""));
        return;
    }
    op->deleteLater();
    TOperationResult r = op->variantData().toMap().value("operation_result").value<TOperationResult>();
    if (!r)
        writeLine(tr("Failed to execute command. The following error occured:", "") + " " + r.errorString());
}

void TerminalIOHandler::disconnected()
{
    muserId = 0;
}

void TerminalIOHandler::error(QAbstractSocket::SocketError)
{
    muserId = 0;
}

void TerminalIOHandler::remoteRequest(BNetworkOperation *op)
{
    QVariantMap in = op->variantData().toMap();
    if (op->metaData().operation() == Texsample::LogRequest)
    {
        QString msg = in.value("log_text").toString();
        int ilvl = in.value("level").toInt();
        BLogger::Level lvl = bRangeD(BLogger::NoLevel, BLogger::CriticalLevel).contains(ilvl) ?
                    static_cast<BLogger::Level>(ilvl) : BLogger::NoLevel;
        BCoreApplication::log(msg, lvl);
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

/*============================== Private variables =========================*/

QString TerminalIOHandler::mmailPassword;
