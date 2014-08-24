#ifndef SETTINGS_H
#define SETTINGS_H

class BSettingsNode;

#include <QString>
#include <QVariant>

namespace Settings
{

namespace Email
{

const QString RootPath = "Email";
const QString LocalHostNameSubpath = "local_host_name";
const QString LoginSubpath = "login";
const QString PasswordSubpath = "password";
const QString ServerAddressSubpath = "server_address";
const QString ServerPortSubpath = "server_port";
const QString SslRequiredSubpath = "ssl_required";
const QString StorePasswordSubpath = "store_password";

bool hasLocalHostName();
bool hasServerPort();
bool hasSslRequired();
QString localHostName();
QString login();
QString password();
QString serverAddress();
quint16 serverPort();
void setLocalHostName(const QString &name);
void setLogin(const QString &login);
void setPassword(const QString &password);
bool setPassword(const BSettingsNode *n, const QVariant &v = QVariant());
void setServerAddress(const QString &address);
void setServerPort(quint16 port);
void setSslRequired(bool required);
void setStorePassword(bool store);
bool setStorePassword(const BSettingsNode *n, const QVariant &v = QVariant());
bool showPassword(const BSettingsNode *n, const QVariant &);
bool sslRequired();
bool storePassword();

}

namespace Log
{

const QString RootPath = "Log";
const QString LoggingModeSubpath = "mode";
const QString LogNoopSubpath = "noop";

int loggingMode();
int logNoop();
void setLoggingMode(int mode);
bool setLoggingMode(const BSettingsNode *n, const QVariant &v = QVariant());
void setLogNoop(int mode);

}

namespace Server
{

const QString RootPath = "Server";
const QString ReadonlySubpath = "readonly";

bool readonly();
void setReadonly(bool b);

}

}

#endif // SETTINGS_H
