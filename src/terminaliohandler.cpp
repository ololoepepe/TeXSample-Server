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

/*void TerminalIOHandler::handleSetImplementation(const QString &, const QStringList &args, Connection *c)
{
    static QMutex mutex;
    QMutexLocker locker(&mutex);
    static TranslateFunctor translate;
    translate.setConnection(c);
    init_once(SettingsItem, Settings, SettingsItem())
    {
        SettingsItem mail("Mail");
          mail.addChildItem("server_address");
          mail.addChildItem("server_port", QVariant::UInt);
          mail.addChildItem("local_host_name");
          mail.addChildItem("login");
          SettingsItem i("password");
            i.setProperty("mail_password", true);
          mail.addChildItem(i);
          mail.addChildItem("ssl_required", QVariant::Bool);
        Settings.addChildItem(mail);
        SettingsItem beqt("BeQt");
          i.setKey("Core");
          i.addChildItem("locale", QVariant::Locale);
          beqt.addChildItem(i);
        Settings.addChildItem(beqt);
        SettingsItem l("Log");
          l.addChildItem("noop", QVariant::Int);
        Settings.addChildItem(l);
    }
    if (args.size() < 1 || args.size() > 2)
        return writeLine(translate("TerminalIOHandler", "Invalid parameters"), c);
    if (args.size() == 1 && c)
        return writeLine(translate("TerminalIOHandler", "Invalid parameters"), c);
    QString path = args.first();
    SettingsItem si = Settings.testPath(path);
    if (QVariant::Invalid == si.type())
        return writeLine(translate("TerminalIOHandler", "No such option"), c);
    path.replace('.', '/');
    if (si.property("mail_password").toBool() && args.size() == 1)
    {
        setStdinEchoEnabled(false);
        mmailPassword = readLine(translate("TerminalIOHandler", "Enter e-mail password:") + " ");
        setStdinEchoEnabled(true);
        writeLine(c);
    }
    else
    {
        QVariant v;
        if (args.size() == 2)
            v = args.last();
        else
            v = readLine(translate("TerminalIOHandler", "Enter value for") + " \"" + path.split("/").last() + "\": ");
        switch (si.type())
        {
        case QVariant::Locale:
            v = QLocale(v.toString());
            break;
        default:
            if (!v.convert(si.type()))
                return writeLine(translate("TerminalIOHandler", "Invalid value"), c);
            break;
        }
        bSettings->setValue(path, v);
    }
    writeLine(translate("TerminalIOHandler", "OK"), c);
}*/

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
    //TODO: Settings structute
    setRootSettingsNode(root);
    //TODO: Improve sescription
    setHelpDescription(QT_TRANSLATE_NOOP("TerminalIOHandler", "This is TeXSample Server"));
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
