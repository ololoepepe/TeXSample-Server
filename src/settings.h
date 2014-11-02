/****************************************************************************
**
** Copyright (C) 2012-2014 Andrey Bogdanov
**
** This file is part of TeXSample Server.
**
** TeXSample Server is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** TeXSample Server is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with TeXSample Server.  If not, see <http://www.gnu.org/licenses/>.
**
****************************************************************************/

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

namespace Texsample
{

const QString RootPath = "Texsample";
const QString PathSubpath = "path";

QString path();
void setPath(const QString &path);

}

}

#endif // SETTINGS_H
