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
    //TODO: Improve settings description
    BSettingsNode *root = new BSettingsNode;
      BSettingsNode *n = new BSettingsNode("Mail", root);
        BSettingsNode *nn = new BSettingsNode("server_address", n);
          nn->setDescription(QT_TRANSLATE_NOOP("TerminalIOHandler", "E-mail server address used for e-mail delivery"));
        nn = new BSettingsNode(QVariant::UInt, "server_port", n);
          nn->setDescription(QT_TRANSLATE_NOOP("TerminalIOHandler", "E-mail server port"));
        nn = new BSettingsNode("local_host_name", n);
          nn->setDescription(QT_TRANSLATE_NOOP("TerminalIOHandler", "Name of local host passed to the e-mail server"));
        nn = new BSettingsNode("ssl_required", n);
          nn->setDescription(QT_TRANSLATE_NOOP("TerminalIOHandler", "Determines wether the e-mail server requires SSL "
                                               "connection"));
        nn = new BSettingsNode("login", n);
          nn->setDescription(QT_TRANSLATE_NOOP("TerminalIOHandler", "Identifier used to log into the e-mail server"));
        nn = new BSettingsNode("password", n);
          nn->setUserSetFunction(&Global::setMailPassword);
          nn->setUserShowFunction(&Global::showMailPassword);
          nn->setDescription(QT_TRANSLATE_NOOP("TerminalIOHandler", "Password used to log into the e-mail server"));
      n = new BSettingsNode("BeQt", root);
        nn = new BSettingsNode("Core", n);
          BSettingsNode *nnn = new BSettingsNode(QVariant::Locale, "locale", nn);
            nnn->setDescription(QT_TRANSLATE_NOOP("TerminalIOHandler", "Locale for the whole application"));
      n = new BSettingsNode("Log", root);
        nn = new BSettingsNode("noop", n);
          nn->setDescription(QT_TRANSLATE_NOOP("TerminalIOHandler", "Logging the \"keep alive\" operations: [0|1|2]"));
    setRootSettingsNode(root);
    //TODO: Improve sescription
    setHelpDescription(QT_TRANSLATE_NOOP("TerminalIOHandler", "This is TeXSample Server.\n"
                                         "Enter \"help --all\" to see full Help"));
    CommandHelp ch;
    ch.usage = "help [--all|--commands|--settings]\nhelp <command>";
    ch.description = QT_TRANSLATE_NOOP("TerminalIOHandler", "Show application help");
    setCommandHelp("help", ch);
    ch.usage = "quit";
    ch.description = QT_TRANSLATE_NOOP("TerminalIOHandler", "Quit the application");
    setCommandHelp("quit", ch);
    ch.usage = "uptime";
    ch.description = QT_TRANSLATE_NOOP("TerminalIOHandler", "Show for how long the application has been running");
    setCommandHelp("uptime", ch);
    ch.usage = "user [--list]\nuser [--connected-at|--info|--kick|--uptime] <id|login>";
    ch.description = QT_TRANSLATE_NOOP("TerminalIOHandler", "Show information about the connected users");
    setCommandHelp("user", ch);
    ch.usage = "set <key> [value]\nset --tree\nset --show|--description <key>";
    ch.description = QT_TRANSLATE_NOOP("TerminalIOHandler", "Set configuration variable");
    setCommandHelp("set", ch);
    ch.usage = "start [port]";
    ch.description = QT_TRANSLATE_NOOP("TerminalIOHandler", "Start the server");
    setCommandHelp("start", ch);
    ch.usage = "stop";
    ch.description = QT_TRANSLATE_NOOP("TerminalIOHandler", "Stop the server");
    setCommandHelp("stop", ch);
    melapsedTimer.start();
}

TerminalIOHandler::~TerminalIOHandler()
{
    //
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
    if (args.isEmpty())
    {
        writeLine(tr("Connected user count:") + " " + QString::number(sServer->currentConnectionCount()));
    }
    else if (args.first() == "--list")
    {
        int sz = sServer->currentConnectionCount();
        if (sz)
            writeLine(tr("Listing connected users") + " (" + QString::number(sz) + "):");
        else
            writeLine(tr("There are no connected users"));
        sServer->lock();
        foreach (BNetworkConnection *c, sServer->connections())
        {
            Connection *cc = static_cast<Connection *>(c);
            writeLine("[" + cc->login() + "] [" + cc->peerAddress() + "] " + cc->uniqueId().toString());
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
                writeLine(c->infoString());
        }
        else if (args.first() == "--uptime")
        {
            foreach (Connection *c, users)
                writeLine(tr("Uptime of") + " " + userPrefix(c) + " " + msecsToString(c->uptime()));
        }
        else if (args.first() == "--connected-at")
        {
            foreach (Connection *c, users)
                writeLine(tr("Connection time of") + " " + userPrefix(c) + " "
                          + c->connectedAt().toString(bLogger->dateTimeFormat()));
        }
        sServer->unlock();
    }
    else
    {
        writeLine(tr("Invalid parameters"));
        return false;
    }
    return true;
}

bool TerminalIOHandler::handleUptime(const QString &, const QStringList &)
{
    writeLine(tr("Uptime:") + " " + msecsToString(melapsedTimer.elapsed()));
    return true;
}

bool TerminalIOHandler::handleStart(const QString &, const QStringList &args)
{
    if (sServer->isListening())
    {
        writeLine(tr("The server is already running"));
        return false;
    }
    QString addr = (args.size() >= 1) ? args.first() : QString("0.0.0.0");
    if (!sServer->listen(addr, Texsample::MainPort))
    {
        writeLine(tr("Failed to start server"));
        return false;
    }
    writeLine(tr("OK"));
    return true;
}

bool TerminalIOHandler::handleStop(const QString &, const QStringList &)
{
    if (!sServer->isListening())
    {
        writeLine(tr("The server is not running"));
        return false;
    }
    sServer->close();
    writeLine(tr("OK"));
    return false;
}
