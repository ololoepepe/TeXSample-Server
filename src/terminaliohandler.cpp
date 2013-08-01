#include "terminaliohandler.h"
#include "server.h"
#include "connection.h"
#include "storage.h"
#include "global.h"

#include <TeXSample>
#include <TOperationResult>
#include <TClientInfo>
#include <TAccessLevel>
#include <TUserInfo>

#include <BNetworkConnection>
#include <BGenericSocket>
#include <BeQt>
#include <BNetworkOperation>
#include <BGenericServer>
#include <BLogger>
#include <BCoreApplication>
#include <BSpamNotifier>
#include <BSettingsNode>
#include <BTranslation>
#include <BTranslateFunctor>
#include <BTranslator>

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
#include <QSettings>
#include <QDateTime>
#include <QTimer>
#include <QTcpSocket>
#include <QMutex>
#include <QMutexLocker>

#include <QDebug>

/*============================================================================
================================ TerminalIOHandler ===========================
============================================================================*/

/*============================== Static public methods =====================*/

TerminalIOHandler *TerminalIOHandler::instance()
{
    return static_cast<TerminalIOHandler *>(BTerminalIOHandler::instance());
}

/*============================== Public constructors =======================*/

TerminalIOHandler::TerminalIOHandler(QObject *parent) :
    BTerminalIOHandler(parent)
{
    installHandler(QuitCommand);
    installHandler(SetCommand);
    installHandler(HelpCommand);
    installHandler("user", (InternalHandler) &TerminalIOHandler::handleUser);
    installHandler("uptime", (InternalHandler) &TerminalIOHandler::handleUptime);
    installHandler("start", (InternalHandler) &TerminalIOHandler::handleStart);
    installHandler("stop", (InternalHandler) &TerminalIOHandler::handleStop);
    BSettingsNode *root = new BSettingsNode;
      BSettingsNode *n = new BSettingsNode("Mail", root);
        BSettingsNode *nn = new BSettingsNode("server_address", n);
          nn->setDescription(BTranslation::translate("BSettingsNode",
                                                     "E-mail server address used for e-mail delivery"));
        nn = new BSettingsNode(QVariant::UInt, "server_port", n);
          nn->setDescription(BTranslation::translate("BSettingsNode", "E-mail server port"));
        nn = new BSettingsNode("local_host_name", n);
          nn->setDescription(BTranslation::translate("BSettingsNode",
                                                     "Name of local host passed to the e-mail server"));
        nn = new BSettingsNode("ssl_required", n);
          nn->setDescription(BTranslation::translate("BSettingsNode",
                                                     "Determines wether the e-mail server requires SSL connection"));
        nn = new BSettingsNode("login", n);
          nn->setDescription(BTranslation::translate("BSettingsNode",
                                                     "Identifier used to log into the e-mail server"));
        nn = new BSettingsNode("password", n);
          nn->setUserSetFunction(&Global::setMailPassword);
          nn->setUserShowFunction(&Global::showMailPassword);
          nn->setDescription(BTranslation::translate("BSettingsNode", "Password used to log into the e-mail server"));
      createBeQtSettingsNode(root);
      n = new BSettingsNode("Log", root);
        nn = new BSettingsNode("mode", n);
          nn->setUserSetFunction(&Global::setLoggingMode);
          nn->setDescription(BTranslation::translate("BSettingsNode", "Logging mode. Possible values:\n"
                                                     "0 or less - don't log\n"
                                                     "1 - log to console only\n"
                                                     "2 - log to file only\n"
                                                     "3 and more - log to console and file\n"
                                                     "The default is 2"));
        nn = new BSettingsNode("noop", n);
          nn->setDescription(BTranslation::translate("BSettingsNode", "Logging the \"keep alive\" operations. "
                                                     "Possible values:\n"
                                                     "0 or less - don't log\n"
                                                     "1 - log locally\n"
                                                     "2 and more - log loaclly and remotely\n"
                                                     "The default is 0"));
    setRootSettingsNode(root);
    setHelpDescription(BTranslation::translate("BTerminalIOHandler", "This is TeXSample Server. "
                                               "Enter \"help --all\" to see full Help"));
    CommandHelpList chl;
    CommandHelp ch;
    ch.usage = "uptime";
    ch.description = BTranslation::translate("BTerminalIOHandler",
                                             "Show for how long the application has been running");
    setCommandHelp("uptime", ch);
    ch.usage = "user [--list]";
    ch.description = BTranslation::translate("BTerminalIOHandler", "Show connected user count or list them all");
    chl << ch;
    ch.usage = "user --connected-at|--info|--uptime <id|login>";
    ch.description = BTranslation::translate("BTerminalIOHandler", "Show information about the user. "
                                             "The user may be specified by id or by login. Options:\n"
                                             "  --connected-at - time when the user connected\n"
                                             "  --info - detailed user information\n"
                                             "  --uptime - for how long the user has been connected");
    chl << ch;
    ch.usage = "user --kick <id|login>";
    ch.description = BTranslation::translate("BTerminalIOHandler", "Disconnect the specified user. "
                                             "If login is specified, all connections of this user will be closed");
    chl << ch;
    setCommandHelp("user", chl);
    ch.usage = "start [address]";
    ch.description = BTranslation::translate("BTerminalIOHandler", "Start the server. "
                                             "If address is specified, the server will only listen on that address, "
                                             "otherwise it will listen on available all addresses.");
    setCommandHelp("start", ch);
    ch.usage = "stop";
    ch.description = BTranslation::translate("BTerminalIOHandler", "Stop the server. Users are NOT disconnected");
    setCommandHelp("stop", ch);
    melapsedTimer.start();
}

TerminalIOHandler::~TerminalIOHandler()
{
    //
}

/*============================== Public methods ============================*/

TOperationResult TerminalIOHandler::startServer(const QString &address)
{
    if (sServer->isListening())
        return TOperationResult(TMessage::ServerAlreadyRunningMessage);
    QString addr = (!address.isEmpty()) ? address : QString("0.0.0.0");
    if (!sServer->listen(addr, Texsample::MainPort))
        return TOperationResult(TMessage::FailedToStartServerError);
    return TOperationResult(true, TMessage::OkMessage);
}

TOperationResult TerminalIOHandler::stopServer()
{
    if (!sServer->isListening())
        return TOperationResult(TMessage::ServerIsNotRunningMessage);
    sServer->close();
    return TOperationResult(true, TMessage::OkMessage);
}

qint64 TerminalIOHandler::uptime() const
{
    return melapsedTimer.elapsed();
}

TOperationResult TerminalIOHandler::user(const QStringList &args, QString &text, BTranslator *t)
{
    BTranslateFunctor translate(t);
    if (args.isEmpty())
    {
        text = translate("TerminalIOHandler", "Connected user count:") + " "
                + QString::number(sServer->currentConnectionCount());
    }
    else if (args.first() == "--list")
    {
        int sz = sServer->currentConnectionCount();
        if (sz)
            text = translate("TerminalIOHandler", "Listing connected users") + " (" + QString::number(sz) + "):";
        else
            text = translate("TerminalIOHandler", "There are no connected users");
        sServer->lock();
        foreach (BNetworkConnection *c, sServer->connections())
        {
            Connection *cc = static_cast<Connection *>(c);
            text = "[" + cc->login() + "] [" + cc->peerAddress() + "] " + cc->uniqueId().toString();
        }
        sServer->unlock();
    }
    else if (args.size() == 2)
    {
        QList<Connection *> users;
        QUuid uuid = BeQt::uuidFromText(args.at(1));
        sServer->lock();
        if (uuid.isNull())
        {
            foreach (BNetworkConnection *c, sServer->connections())
            {
                Connection *cc = static_cast<Connection *>(c);
                if (cc->login() == args.at(1))
                    users << cc;
            }
        }
        else
        {
            foreach (BNetworkConnection *c, sServer->connections())
            {
                Connection *cc = static_cast<Connection *>(c);
                if (cc->uniqueId() == uuid)
                {
                    users << cc;
                    break;
                }
            }
        }
        if (args.first() == "--kick")
        {
            foreach (Connection *c, users)
                QMetaObject::invokeMethod(c, "abort", Qt::QueuedConnection);
        }
        else if (args.first() == "--info")
        {
            foreach (Connection *c, users)
                text = c->infoString();
        }
        else if (args.first() == "--uptime")
        {
            foreach (Connection *c, users)
                text = translate("TerminalIOHandler", "Uptime of") + " " + userPrefix(c) + " "
                        + msecsToString(c->uptime());
        }
        else if (args.first() == "--connected-at")
        {
            foreach (Connection *c, users)
                text = translate("TerminalIOHandler", "Connection time of") + " " + userPrefix(c) + " "
                        + c->connectedAt().toString(bLogger->dateTimeFormat());
        }
        sServer->unlock();
    }
    else
    {
        return TOperationResult(TMessage::InvalidCommandArgumentsError);
    }
    return TOperationResult(true);
}

/*============================== Protected methods =========================*/

bool TerminalIOHandler::handleCommand(const QString &, const QStringList &)
{
    writeLine(tr("Unknown command. Enter \"help --commands\" to see the list of available commands"));
    return false;
}

/*============================== Static private methods ====================*/

QString TerminalIOHandler::msecsToString(qint64 msecs)
{
    QString days = QString::number(msecs / (24 * BeQt::Hour));
    msecs %= (24 * BeQt::Hour);
    QString hours = QString::number(msecs / BeQt::Hour);
    hours.prepend(QString().fill('0', 2 - hours.length()));
    msecs %= BeQt::Hour;
    QString minutes = QString::number(msecs / BeQt::Minute);
    minutes.prepend(QString().fill('0', 2 - minutes.length()));
    msecs %= BeQt::Minute;
    QString seconds = QString::number(msecs / BeQt::Second);
    seconds.prepend(QString().fill('0', 2 - seconds.length()));
    return days + " " + tr("days") + " " + hours + ":" + minutes + ":" + seconds;
}

QString TerminalIOHandler::userPrefix(Connection *user)
{
    if (!user)
        return "";
    return "[" + user->login() + "] [" + user->peerAddress() + "] " + user->uniqueId().toString();
}

void TerminalIOHandler::writeHelpLine(const QString &command, const QString &description)
{
    QString s = "  " + command;
    if (s.length() > 28)
        s += "\n" + QString().fill(' ', 30);
    else
        s += QString().fill(' ', 30 - s.length());
    s += description;
    writeLine(s);
}

/*============================== Private methods ===========================*/

bool TerminalIOHandler::handleUser(const QString &, const QStringList &args)
{
    QString text;
    TOperationResult r = user(args, text, BCoreApplication::translator("texsample-server"));
    if (r)
        writeLine(text);
    else
        writeLine(r.messageString());
    return r;
}

bool TerminalIOHandler::handleUptime(const QString &, const QStringList &)
{
    writeLine(tr("Uptime:") + " " + msecsToString(melapsedTimer.elapsed()));
    return true;
}

bool TerminalIOHandler::handleStart(const QString &, const QStringList &args)
{
    TOperationResult r = startServer(!args.isEmpty() ? args.first() : QString());
    writeLine(r.messageString());
    return r;
}

bool TerminalIOHandler::handleStop(const QString &, const QStringList &)
{
    TOperationResult r = stopServer();
    writeLine(r.messageString());
    return r;
}
