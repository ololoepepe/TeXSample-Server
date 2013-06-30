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

#include <QDebug>

/*============================================================================
================================ SettingsItem ================================
============================================================================*/

class SettingsItem
{
public:
    explicit SettingsItem();
    explicit SettingsItem(const QString &key, QVariant::Type t = QVariant::String);
    SettingsItem(const SettingsItem &other);
public:
    void setKey(const QString &key);
    void setType(const QVariant::Type t);
    void setProperty(const QString &name, const QVariant &value = QVariant());
    void addChildItem(const SettingsItem &item);
    void addChildItem(const QString &key, QVariant::Type t = QVariant::String);
    void removeChildItem(const QString &key);
    QString key() const;
    QVariant::Type type() const;
    QVariant property(const QString &name) const;
    QList<SettingsItem> childItems() const;
    SettingsItem testPath(const QString &path, const QChar &separator = '.') const;
public:
    SettingsItem &operator =(const SettingsItem &other);
    bool operator ==(const SettingsItem &other) const;
private:
    QString mkey;
    QVariant::Type mtype;
    QVariantMap mproperties;
    QList<SettingsItem> mchildren;
};

/*============================================================================
================================ RegistrationConnection ======================
============================================================================*/

class RegistrationConnection : public BNetworkConnection
{
public:
    explicit RegistrationConnection(BNetworkServer *server, BGenericSocket *socket);
    ~RegistrationConnection();
protected:
    void log(const QString &text, BLogger::Level lvl = BLogger::NoLevel);
private:
    void handleRegisterRequest(BNetworkOperation *op);
private:
    Storage *mstorage;
};

/*============================================================================
================================ RecoveryConnection ==========================
============================================================================*/

class RecoveryConnection : public BNetworkConnection
{
public:
    explicit RecoveryConnection(BNetworkServer *server, BGenericSocket *socket);
    ~RecoveryConnection();
protected:
    void log(const QString &text, BLogger::Level lvl = BLogger::NoLevel);
private:
    void handleGetRecoveryCode(BNetworkOperation *op);
    void handleRecoverAccount(BNetworkOperation *op);
private:
    Storage *mstorage;
};

/*============================================================================
================================ RegistrationServer ==========================
============================================================================*/

class RegistrationServer : public BNetworkServer
{
public:
    explicit RegistrationServer(QObject *parent = 0);
protected:
    BNetworkConnection *createConnection(BGenericSocket *socket);
};

/*============================================================================
================================ RecoveryServer ==============================
============================================================================*/

class RecoveryServer : public BNetworkServer
{
public:
    explicit RecoveryServer(QObject *parent = 0);
protected:
    BNetworkConnection *createConnection(BGenericSocket *socket);
};

/*============================================================================
================================ SettingsItem ================================
============================================================================*/

/*============================== Public constructors =======================*/

SettingsItem::SettingsItem()
{
    mtype = QVariant::Invalid;
}

SettingsItem::SettingsItem(const QString &key, QVariant::Type t)
{
    mkey = key;
    mtype = t;
}

SettingsItem::SettingsItem(const SettingsItem &other)
{
    *this = other;
}

/*============================== Public methods ============================*/

void SettingsItem::setKey(const QString &key)
{
    mkey = key;
}

void SettingsItem::setType(const QVariant::Type t)
{
    mtype = t;
}

void SettingsItem::setProperty(const QString &name, const QVariant &value)
{
    if (name.isEmpty())
        return;
    if (value.isValid())
        mproperties[name] = value;
    else
        mproperties.remove(name);
}

void SettingsItem::addChildItem(const SettingsItem &item)
{
    if (item.key().isEmpty() || QVariant::Invalid == item.type() || mchildren.contains(item))
        return;
    mchildren << item;
}

void SettingsItem::addChildItem(const QString &key, QVariant::Type t)
{
    addChildItem(SettingsItem(key, t));
}

void SettingsItem::removeChildItem(const QString &key)
{
    if (key.isEmpty())
        return;
    mchildren.removeAll(SettingsItem(key));
}

QString SettingsItem::key() const
{
    return mkey;
}

QVariant::Type SettingsItem::type() const
{
    return mtype;
}

QVariant SettingsItem::property(const QString &name) const
{
    return mproperties.value(name);
}

QList<SettingsItem> SettingsItem::childItems() const
{
    return mchildren;
}

SettingsItem SettingsItem::testPath(const QString &path, const QChar &separator) const
{
    if (path.isEmpty())
        return SettingsItem();
    if (mkey.isEmpty())
    {
        foreach (const SettingsItem &i, mchildren)
        {
            SettingsItem si = i.testPath(path, separator);
            if (QVariant::Invalid != si.type())
                return si;
        }
    }
    else
    {
        QStringList sl = path.split(!separator.isNull() ? separator : QChar('.'));
        if (sl.takeFirst() != mkey)
            return SettingsItem();
        QString spath = sl.join(QString(separator));
        if (spath.isEmpty())
            return *this;
        foreach (const SettingsItem &i, mchildren)
        {
            SettingsItem si = i.testPath(spath, separator);
            if (QVariant::Invalid != si.type())
                return si;
        }
    }
    return SettingsItem();
}

/*============================== Public operators ==========================*/

SettingsItem &SettingsItem::operator =(const SettingsItem &other)
{
    mkey = other.mkey;
    mtype = other.mtype;
    mproperties = other.mproperties;
    mchildren = other.mchildren;
    return *this;
}

bool SettingsItem::operator ==(const SettingsItem &other) const
{
    return mkey == other.mkey; //TODO
}

/*============================================================================
================================ RegistrationConnection ======================
============================================================================*/

/*============================== Public constructors =======================*/

RegistrationConnection::RegistrationConnection(BNetworkServer *server, BGenericSocket *socket) :
    BNetworkConnection(server, socket)
{
    setCriticalBufferSize(BeQt::Megabyte + 500 * BeQt::Kilobyte);
    setCloseOnCriticalBufferSize(true);
    socket->tcpSocket()->setSocketOption(QTcpSocket::KeepAliveOption, 1);
    mstorage = new Storage;
    installRequestHandler(Texsample::RegisterRequest,
                          (InternalHandler) &RegistrationConnection::handleRegisterRequest);
    QTimer::singleShot(BeQt::Minute, this, SLOT(close()));
}

RegistrationConnection::~RegistrationConnection()
{
    delete mstorage;
}

/*============================== Protected methods =========================*/

void RegistrationConnection::log(const QString &text, BLogger::Level lvl)
{
    TerminalIOHandler::sendLogRequest("[" + peerAddress() + "] " + text, lvl);
    BNetworkConnection::log(text, lvl);
}

/*============================== Private methods ===========================*/

void RegistrationConnection::handleRegisterRequest(BNetworkOperation *op)
{
    QVariantMap in = op->variantData().toMap();
    QUuid invite = BeQt::uuidFromText(in.value("invite").toString());
    TUserInfo info = in.value("user_info").value<TUserInfo>();
    log(tr("Register request:", "log text") + " " + info.login());
    if (invite.isNull() || !info.isValid(TUserInfo::RegisterContext))
        return Global::sendReply(op, Storage::invalidParametersResult());
    info.setContext(TUserInfo::AddContext);
    info.setAccessLevel(TAccessLevel::UserLevel);
    quint64 id = mstorage->inviteId(invite);
    if (!id)
        return Global::sendReply(op, TOperationResult(tr("Invalid invite", "errorString")));
    Global::sendReply(op, mstorage->addUser(info, invite));
    op->waitForFinished();
    close();
}

/*============================================================================
================================ RecoveryConnection ==========================
============================================================================*/

/*============================== Public constructors =======================*/

RecoveryConnection::RecoveryConnection(BNetworkServer *server, BGenericSocket *socket) :
    BNetworkConnection(server, socket)
{
    setCriticalBufferSize(BeQt::Megabyte + 500 * BeQt::Kilobyte);
    setCloseOnCriticalBufferSize(true);
    socket->tcpSocket()->setSocketOption(QTcpSocket::KeepAliveOption, 1);
    mstorage = new Storage;
    installRequestHandler(Texsample::GetRecoveryCodeRequest,
                          (InternalHandler) &RecoveryConnection::handleGetRecoveryCode);
    installRequestHandler(Texsample::RecoverAccountRequest,
                          (InternalHandler) &RecoveryConnection::handleRecoverAccount);
    QTimer::singleShot(BeQt::Minute, this, SLOT(close()));
}

RecoveryConnection::~RecoveryConnection()
{
    delete mstorage;
}

/*============================== Protected methods =========================*/

void RecoveryConnection::log(const QString &text, BLogger::Level lvl)
{
    TerminalIOHandler::sendLogRequest("[" + peerAddress() + "] " + text, lvl);
    BNetworkConnection::log(text, lvl);
}

/*============================== Private methods ===========================*/

void RecoveryConnection::handleGetRecoveryCode(BNetworkOperation *op)
{
    QString email = op->variantData().toMap().value("email").toString();
    if (email.isEmpty())
        return Global::sendReply(op, Storage::invalidParametersResult());
    Global::sendReply(op, mstorage->getRecoveryCode(email));
}

void RecoveryConnection::handleRecoverAccount(BNetworkOperation *op)
{
    QVariantMap in = op->variantData().toMap();
    QString email = in.value("email").toString();
    QUuid code = BeQt::uuidFromText(in.value("recovery_code").toString());
    QByteArray password = in.value("password").toByteArray();
    if (email.isEmpty() || code.isNull() || password.isEmpty())
        return Global::sendReply(op, Storage::invalidParametersResult());
    Global::sendReply(op, mstorage->recoverAccount(email, code, password));
}

/*============================================================================
================================ RegistrationServer ==========================
============================================================================*/

/*============================== Public constructors =======================*/

RegistrationServer::RegistrationServer(QObject *parent) :
    BNetworkServer(BGenericServer::TcpServer, parent)
{
    spamNotifier()->setCheckInterval(BeQt::Second);
    spamNotifier()->setEventLimit(5);
    spamNotifier()->setEnabled(true);
}

/*============================== Protected methods =========================*/

BNetworkConnection *RegistrationServer::createConnection(BGenericSocket *socket)
{
    return new RegistrationConnection(this, socket);
}

/*============================================================================
================================ RecoveryServer ==============================
============================================================================*/

/*============================== Public constructors =======================*/

RecoveryServer::RecoveryServer(QObject *parent) :
    BNetworkServer(BGenericServer::TcpServer, parent)
{
    spamNotifier()->setCheckInterval(BeQt::Second);
    spamNotifier()->setEventLimit(5);
    spamNotifier()->setEnabled(true);
}

/*============================== Protected methods =========================*/

BNetworkConnection *RecoveryServer::createConnection(BGenericSocket *socket)
{
    return new RecoveryConnection(this, socket);
}

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
    foreach (BNetworkConnection *c, s->connections())
    {
        Connection *cc = static_cast<Connection *>(c);
        cc->sendLogRequest(text, lvl);
    }
}

void TerminalIOHandler::executeCommand(const QString &cmd, const QStringList &args, Connection *c)
{
    TerminalIOHandler *inst = instance();
    if (!inst)
        return;
    if (cmd == "quit" || cmd == "exit")
        BeQt::handleQuit(cmd, args);
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
    mregistrationServer = new RegistrationServer(this);
    mrecoveryServer = new RecoveryServer(this);
    installHandler("quit", &BeQt::handleQuit);
    installHandler("exit", &BeQt::handleQuit);
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
    return days + " " + tr("days", "") + " " + hours + ":" + minutes + ":" + seconds;
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
    writeHelpLine("stop", tr("Stop the main server and the registration server", "help"));
}

void TerminalIOHandler::handleUserImplementation(const QString &, const QStringList &args, Connection *c)
{
    if (args.isEmpty())
    {
        writeLine(tr("Users:", "") + " " + QString::number(mserver->connections().size()), c);
    }
    else if (args.first() == "--list")
    {
        int sz = mserver->connections().size();
        if (sz)
            writeLine(tr("Listing users", "") + " (" + QString::number(sz) + "):", c);
        else
            writeLine(tr("There are no connected users", ""), c);
        foreach (BNetworkConnection *cc, mserver->connections())
        {
            Connection *ccc = static_cast<Connection *>(cc);
            writeLine("[" + ccc->login() + "] [" + ccc->peerAddress() + "] " + ccc->uniqueId().toString(), c);
        }
    }
    else
    {
        if (args.size() < 2)
            return;
        QList<Connection *> users;
        QUuid uuid = BeQt::uuidFromText(args.at(1));
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
                writeLine(tr("Uptime for", "") + " " + userPrefix(cc) + " " + msecsToString(cc->uptime()), c);
        }
        else if (args.first() == "--connected-at")
        {
            foreach (Connection *cc, users)
                writeLine(tr("Connection time for", "") + " " + userPrefix(cc) + " "
                          + cc->connectedAt().toString(bLogger->dateTimeFormat()), c);
        }
    }
}

void TerminalIOHandler::handleUptimeImplementation(const QString &, const QStringList &, Connection *c)
{
    writeLine(tr("Uptime:", "") + " " + msecsToString(melapsedTimer.elapsed()), c);
}

void TerminalIOHandler::handleSetImplementation(const QString &, const QStringList &args, Connection *c)
{
    init_once(SettingsItem, Settings, SettingsItem())
    {
        SettingsItem mail("Mail");
          mail.addChildItem("server_address");
          mail.addChildItem("server_port", QVariant::UInt);
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
    }
    if (args.size() < 1 || args.size() > 2)
        return writeLine(tr("Invalid parameters", ""), c);
    if (args.size() == 1 && c)
        return writeLine(tr("Invalid parameters", ""), c);
    QString path = args.first();
    SettingsItem si = Settings.testPath(path);
    if (QVariant::Invalid == si.type())
        return writeLine(tr("No such option", ""), c);
    path.replace('.', '/');
    if (si.property("mail_password").toBool() && args.size() == 1)
    {
        setStdinEchoEnabled(false);
        mmailPassword = readLine(tr("Enter e-mail password:", "") + " ");
        setStdinEchoEnabled(true);
        writeLine(c);
    }
    else
    {
        QVariant v;
        if (args.size() == 2)
            v = args.last();
        else
            v = readLine(tr("Enter value for", "") + " \"" + path.split("/").last() + "\": ");
        switch (si.type())
        {
        case QVariant::Locale:
            v = QLocale(v.toString());
            break;
        default:
            if (!v.convert(si.type()))
                return writeLine(tr("Invalid value", ""), c);
            break;
        }
        bSettings->setValue(path, v);
    }
    writeLine(tr("OK", ""), c);
}

void TerminalIOHandler::handleStartImplementation(const QString &, const QStringList &args, Connection *c)
{
    if (mserver->isListening())
        return writeLine(tr("The server is already running", ""), c);
    QString addr = (args.size() >= 1) ? args.first() : QString("0.0.0.0");
    if (!mserver->listen(addr, Texsample::MainPort) || !mregistrationServer->listen(addr, Texsample::RegistrationPort)
            || !mrecoveryServer->listen(addr, Texsample::RecoveryPort))
    {
        mserver->close();
        mregistrationServer->close();
        mrecoveryServer->close();
        return writeLine(tr("Failed to start server", ""), c);
    }
    writeLine(tr("OK", ""), c);
}

void TerminalIOHandler::handleStopImplementation(const QString &, const QStringList &, Connection *c)
{
    if (!mserver->isListening())
        return writeLine(tr("The server is not running", ""), c);
    mserver->close();
    mregistrationServer->close();
    mrecoveryServer->close();
    writeLine(tr("OK", ""), c);
}

/*============================== Private variables =========================*/

QString TerminalIOHandler::mmailPassword;
