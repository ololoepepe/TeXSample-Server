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
#include <QSettings>

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

TerminalIOHandler::TerminalIOHandler(QObject *parent) :
    BTerminalIOHandler(parent)
{
    mserver = new Server(this);
    mrserver = new RegistrationServer(this);
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

/*============================== Public methods ============================*/

void TerminalIOHandler::executeCommand(const QString &cmd, const QStringList &args)
{
    if (cmd == "quit" || cmd == "exit")
        BeQt::handleQuit(cmd, args);
    else if (cmd == "user")
        handleUser(cmd, args);
    else if (cmd == "uptime")
        handleUptime(cmd, args);
    else if (cmd == "set")
        handleSet(cmd, args);
    else if (cmd == "start")
        handleStart(cmd, args);
    else if (cmd == "stop")
        handleStop(cmd, args);
}

Server *TerminalIOHandler::server() const
{
    return mserver;
}

/*============================== Private methods ===========================*/

void TerminalIOHandler::handleUser(const QString &, const QStringList &args)
{
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

void TerminalIOHandler::handleSet(const QString &, const QStringList &args)
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
        return writeLine(tr("Invalid parameters", ""));
    QString path = args.first();
    SettingsItem si = Settings.testPath(path);
    if (QVariant::Invalid == si.type())
        return writeLine(tr("No such option", ""));
    path.replace('.', '/');
    if (si.property("mail_password").toBool())
    {
        setStdinEchoEnabled(false);
        mmailPassword = readLine(tr("Enter e-mail password:", "") + " ");
        setStdinEchoEnabled(true);
        writeLine("");
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
                return writeLine(tr("Invalid value", ""));
            break;
        }
        bSettings->setValue(path, v);
    }
    writeLine(tr("OK", ""));
}

void TerminalIOHandler::handleStart(const QString &, const QStringList &args)
{
    if (mserver->isListening())
        return writeLine(tr("The server is already running", ""));
    QString addr = (args.size() >= 1) ? args.first() : QString("0.0.0.0");
    if (!mserver->listen(addr, Texsample::MainPort) || !mrserver->listen(addr, Texsample::RegistrationPort))
    {
        mserver->close();
        mrserver->close();
        return writeLine(tr("Failed to start server", ""));
    }
    writeLine(tr("OK", ""));
}

void TerminalIOHandler::handleStop(const QString &, const QStringList &)
{
    if (!mserver->isListening())
        return writeLine(tr("The server is not running", ""));
    mserver->close();
    mrserver->close();
    writeLine(tr("OK", ""));
}

void TerminalIOHandler::handleHelp(const QString &, const QStringList &)
{
    //
}

/*============================== Private variables =========================*/

QString TerminalIOHandler::mmailPassword;
