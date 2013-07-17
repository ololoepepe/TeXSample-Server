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

void TerminalIOHandler::write(const QString &text, Connection *c)
{
    BTerminalIOHandler::write(text);
    if (c)
        c->sendWriteRequest(text);
}

void TerminalIOHandler::writeLine(const QString &text, Connection *c)
{
    write(text + "\n", c);
}

void TerminalIOHandler::writeLine(Connection *c)
{
    writeLine(QString(), c);
}

void TerminalIOHandler::sendLogRequest(const QString &text, BLogger::Level lvl)
{
    Server *s = TerminalIOHandler::server();
    if (!s)
        return;
    s->lock();
    foreach (BNetworkConnection *c, s->connections())
    {
        Connection *cc = static_cast<Connection *>(c);
        cc->sendLogRequest(text, lvl);
    }
    s->unlock();
}

void TerminalIOHandler::executeCommand(const QString &cmd, const QStringList &args, Connection *c)
{
    static QMutex mutex;
    QMutexLocker locker(&mutex);
    TerminalIOHandler *inst = instance();
    if (!inst)
        return;
    if (cmd == "quit" || cmd == "exit")
        inst->handleQuit(cmd, args);
    else if (cmd == "user")
        inst->handleUserImplementation(cmd, args, c);
    else if (cmd == "uptime")
        inst->handleUptimeImplementation(cmd, args, c);
    else if (cmd == "set")
        inst->handleSetImplementation(cmd, args, c);
    else if (cmd == "start")
        inst->handleStartImplementation(cmd, args, c);
    else if (cmd == "stop")
        inst->handleStopImplementation(cmd, args, c);
}

TerminalIOHandler *TerminalIOHandler::instance()
{
    return static_cast<TerminalIOHandler *>(BTerminalIOHandler::instance());
}

Server *TerminalIOHandler::server()
{
    TerminalIOHandler *inst = instance();
    return inst ? inst->mserver : 0;
}

QString TerminalIOHandler::mailPassword()
{
    return mmailPassword;
}

/*============================== Public constructors =======================*/

TerminalIOHandler::TerminalIOHandler(QObject *parent) :
    BTerminalIOHandler(parent)
{
    mserver = new Server(this);
    installHandler(QuitCommand);
    installHandler("user", (InternalHandler) &TerminalIOHandler::handleUser);
    installHandler("uptime", (InternalHandler) &TerminalIOHandler::handleUptime);
    installHandler("set", (InternalHandler) &TerminalIOHandler::handleSet);
    installHandler("start", (InternalHandler) &TerminalIOHandler::handleStart);
    installHandler("stop", (InternalHandler) &TerminalIOHandler::handleStop);
    installHandler("help", (InternalHandler) &TerminalIOHandler::handleHelp);
    melapsedTimer.start();
}

TerminalIOHandler::~TerminalIOHandler()
{
    //
}

/*============================== Protected methods =========================*/

bool TerminalIOHandler::handleCommand(const QString &, const QStringList &)
{
    writeLine(tr("Unknown command. Enter \"help\" to see the list of available commands"));
}

/*============================== Static private methods ====================*/

QString TerminalIOHandler::msecsToString(qint64 msecs, Connection *c)
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
    static TranslateFunctor translate;
    translate.setConnection(c);
    return days + " " + translate("TerminalIOHandler", "days") + " " + hours + ":" + minutes + ":" + seconds;
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

void TerminalIOHandler::handleUser(const QString &cmd, const QStringList &args)
{
    handleUserImplementation(cmd, args);
}

void TerminalIOHandler::handleUptime(const QString &cmd, const QStringList &args)
{
    handleUptimeImplementation(cmd, args);
}

void TerminalIOHandler::handleSet(const QString &cmd, const QStringList &args)
{
    handleSetImplementation(cmd, args);
}

void TerminalIOHandler::handleStart(const QString &cmd, const QStringList &args)
{
    handleStartImplementation(cmd, args);
}

void TerminalIOHandler::handleStop(const QString &cmd, const QStringList &args)
{
    handleStopImplementation(cmd, args);
}

void TerminalIOHandler::handleHelp(const QString &, const QStringList &)
{
    writeLine(tr("The following commands are available:", "help"));
    writeHelpLine("help", tr("Show this Help", "help"));
    writeHelpLine("quit, exit", tr("Quit the application", "help"));
    writeHelpLine("uptime", tr("Show for how long the application has been running", "help"));
    writeHelpLine("user [--list], user [--connected-at|--info|--kick|--uptime] <id|login>",
                  tr("Show information about the user(s) connected", "help"));
    writeHelpLine("set <key> [value]", tr("Set configuration variable", "help"));
    writeHelpLine("start", tr("Start the main server and the auxiliary servers", "help"));
    writeHelpLine("stop", tr("Stop the main server and the auxiliary servers", "help"));
}

void TerminalIOHandler::handleUserImplementation(const QString &, const QStringList &args, Connection *c)
{
    static QMutex mutex;
    QMutexLocker locker(&mutex);
    static TranslateFunctor translate;
    translate.setConnection(c);
    if (args.isEmpty())
    {
        writeLine(translate("TerminalIOHandler", "Connected user count:") + " "
                  + QString::number(mserver->currentConnectionCount()), c);
    }
    else if (args.first() == "--list")
    {
        int sz = mserver->currentConnectionCount();
        if (sz)
            writeLine(translate("TerminalIOHandler", "Listing connected users") + " (" + QString::number(sz) + "):", c);
        else
            writeLine(translate("TerminalIOHandler", "There are no connected users"), c);
        mserver->lock();
        foreach (BNetworkConnection *cc, mserver->connections())
        {
            Connection *ccc = static_cast<Connection *>(cc);
            writeLine("[" + ccc->login() + "] [" + ccc->peerAddress() + "] " + ccc->uniqueId().toString(), c);
        }
        mserver->unlock();
    }
    else
    {
        if (args.size() < 2)
            return writeLine(translate("TerminalIOHandler", "Invalid parameters"));
        QList<Connection *> users;
        QUuid uuid = BeQt::uuidFromText(args.at(1));
        mserver->lock();
        if (uuid.isNull())
        {
            foreach (BNetworkConnection *c, mserver->connections())
            {
                Connection *cc = static_cast<Connection *>(c);
                if (cc->login() == args.at(1))
                    users << cc;
            }
        }
        else
        {
            foreach (BNetworkConnection *c, mserver->connections())
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
            foreach (Connection *cc, users)
                QMetaObject::invokeMethod(cc, "abort", Qt::QueuedConnection);
        }
        else if (args.first() == "--info")
        {
            foreach (Connection *cc, users)
                writeLine(cc->infoString(), c);
        }
        else if (args.first() == "--uptime")
        {
            foreach (Connection *cc, users)
                writeLine(translate("TerminalIOHandler", "Uptime of") + " " + userPrefix(cc) + " "
                          + msecsToString(cc->uptime(), c), c);
        }
        else if (args.first() == "--connected-at")
        {
            foreach (Connection *cc, users)
                writeLine(translate("TerminalIOHandler", "Connection time of") + " " + userPrefix(cc) + " "
                          + cc->connectedAt().toString(bLogger->dateTimeFormat()), c);
        }
        mserver->unlock();
    }
}

void TerminalIOHandler::handleUptimeImplementation(const QString &, const QStringList &, Connection *c)
{
    static QMutex mutex;
    QMutexLocker locker(&mutex);
    static TranslateFunctor translate;
    translate.setConnection(c);
    writeLine(translate("TerminalIOHandler", "Uptime:") + " " + msecsToString(melapsedTimer.elapsed(), c), c);
}

void TerminalIOHandler::handleSetImplementation(const QString &, const QStringList &args, Connection *c)
{
    /*static QMutex mutex;
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
    writeLine(translate("TerminalIOHandler", "OK"), c);*/
}

void TerminalIOHandler::handleStartImplementation(const QString &, const QStringList &args, Connection *c)
{
    static QMutex mutex;
    QMutexLocker locker(&mutex);
    static TranslateFunctor translate;
    translate.setConnection(c);
    if (mserver->isListening())
        return writeLine(translate("TerminalIOHandler", "The server is already running"), c);
    QString addr = (args.size() >= 1) ? args.first() : QString("0.0.0.0");
    if (!mserver->listen(addr, Texsample::MainPort) /*|| !mregistrationServer->listen(addr, Texsample::RegistrationPort)
            || !mrecoveryServer->listen(addr, Texsample::RecoveryPort)*/)
    {
        mserver->close();
        mregistrationServer->close();
        mrecoveryServer->close();
        return writeLine(translate("TerminalIOHandler", "Failed to start server"), c);
    }
    writeLine(translate("TerminalIOHandler", "OK"), c);
}

void TerminalIOHandler::handleStopImplementation(const QString &, const QStringList &, Connection *c)
{
    static QMutex mutex;
    QMutexLocker locker(&mutex);
    static TranslateFunctor translate;
    translate.setConnection(c);
    if (!mserver->isListening())
        return writeLine(translate("TerminalIOHandler", "The server is not running"), c);
    mserver->close();
    mregistrationServer->close();
    mrecoveryServer->close();
    writeLine(translate("TerminalIOHandler", "OK"), c);
}

/*============================== Private variables =========================*/

QString TerminalIOHandler::mmailPassword;
