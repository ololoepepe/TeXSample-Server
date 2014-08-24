class BSettingsNode;

#include "settings.h"

#include "application.h"

#include <BTerminal>

#include <QDebug>
#include <QSettings>
#include <QString>
#include <QVariant>

B_DECLARE_TRANSLATE_FUNCTION

namespace Settings
{

namespace Email
{

static const QString LocalHostNamePath = RootPath + "/" + LocalHostNameSubpath;
static const QString LoginPath = RootPath + "/" + LoginSubpath;
static const QString PasswordPath = RootPath + "/" + PasswordSubpath;
static const QString ServerAddressPath = RootPath + "/" + ServerAddressSubpath;
static const QString ServerPortPath = RootPath + "/" + ServerPortSubpath;
static const QString SslRequiredPath = RootPath + "/" + SslRequiredSubpath;
static const QString StorePasswordPath = RootPath + "/" + StorePasswordSubpath;
QString mpassword;

bool hasLocalHostName()
{
    return bSettings->contains(LocalHostNamePath);
}

bool hasServerPort()
{
    return bSettings->contains(ServerPortPath);
}

bool hasSslRequired()
{
    return bSettings->contains(SslRequiredPath);
}

QString localHostName()
{
    return bSettings->value(LocalHostNamePath).toString();
}

QString login()
{
    return bSettings->value(LoginPath).toString();
}

QString password()
{
    return storePassword() ? bSettings->value(PasswordPath).toString() : mpassword;
}

QString serverAddress()
{
    return bSettings->value(ServerAddressPath).toString();
}

quint16 serverPort()
{
    return bSettings->value(ServerPortPath, quint16(25)).toUInt();
}

void setLocalHostName(const QString &name)
{
    bSettings->setValue(LocalHostNamePath, name);
}

void setLogin(const QString &login)
{
    bSettings->setValue(LoginPath, login);
}

void setPassword(const QString &password)
{
    if (storePassword())
        bSettings->setValue(PasswordPath, password);
    else
        mpassword = password;
}

bool setPassword(const BSettingsNode *, const QVariant &v)
{
    mpassword = !v.isNull() ? v.toString() :
                              bReadLineSecure(translate("Settings::Email", "Enter e-mail password:") + " ");
    return !mpassword.isEmpty();
}

void setServerAddress(const QString &address)
{
    bSettings->setValue(ServerAddressPath, address);
}

void setServerPort(quint16 port)
{
    bSettings->setValue(ServerPortPath, port);
}

void setSslRequired(bool required)
{
    bSettings->setValue(SslRequiredPath, required);
}

void setStorePassword(bool store)
{
    bool bstore = storePassword();
    bSettings->setValue(StorePasswordPath, store);
    if (bstore != store) {
        if (store) {
            bSettings->setValue(PasswordPath, mpassword);
        } else {
            mpassword = bSettings->value(PasswordPath).toString();
            bSettings->remove(PasswordPath);
        }
    }
}

bool setStorePassword(const BSettingsNode *, const QVariant &v)
{
    bool store = false;
    if (!v.isNull())
        store = v.toBool();
    else
        store = (bReadLine(translate("Settings::Email", "Enter value for \"store_password\" [true|false]:") + " ")
                 == "true");
    setStorePassword(store);
    return true;
}

bool showPassword(const BSettingsNode *, const QVariant &)
{
    if (!QVariant(bReadLine(translate("Settings::Email", "Printing password is unsecure! Do you want to continue?")
                            + " [yes|No] ")).toBool())
        return false;
    bWriteLine(translate("Settings::Email", "Value for") + " \"password\": " + mpassword);
    return true;
}

bool sslRequired()
{
    return bSettings->value(SslRequiredPath, true).toBool();
}

bool storePassword()
{
    return bSettings->value(StorePasswordPath, false).toBool();
}

}

namespace Log
{

static const QString LoggingModePath = RootPath + "/" + LoggingModeSubpath;
static const QString LogNoopPath = RootPath + "/" + LogNoopSubpath;

int loggingMode()
{
    return bSettings->value(LoggingModePath, 2).toInt();
}

int logNoop()
{
    return bSettings->value(LogNoopPath, 0).toInt();
}

void setLoggingMode(int mode)
{
    bSettings->setValue(LoggingModePath, mode);
}

bool setLoggingMode(const BSettingsNode *, const QVariant &v)
{
    QString s = !v.isNull() ? v.toString() : bReadLine(translate("Settings::Email", "Enter logging mode:") + " ");
    if (s.isEmpty())
        return false;
    bool ok = false;
    int m = s.toInt(&ok);
    if (!ok)
        return false;
    setLoggingMode(m);
    bApp->updateLoggingMode();
    return true;
}

void setLogNoop(int mode)
{
    bSettings->setValue(LogNoopPath, mode);
}

}

namespace Server
{

static const QString ReadonlyPath = RootPath + "/" + ReadonlySubpath;

bool readonly()
{
    return bSettings->value(ReadonlyPath).toBool();
}

void setReadonly(bool b)
{
    bSettings->setValue(ReadonlyPath, b);
    bApp->updateReadonly();
}

}

}
