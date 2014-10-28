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

#ifndef APPLICATION_H
#define APPLICATION_H

class ApplicationVersionService;
class DataSource;
class Server;
class UserService;

class QStringList;
class QTimerEvent;

#include <TCoreApplication>

#include <BTextTools>

#include <QElapsedTimer>
#include <QMutex>
#include <QObject>
#include <QString>

#if defined(bApp)
#   undef bApp
#endif
#define bApp static_cast<Application *>(BApplicationBase::binstance())

/*============================================================================
================================ Application =================================
============================================================================*/

class Application : public TCoreApplication
{
    Q_OBJECT
private:
    static QMutex serverMutex;
private:
    DataSource * const Source;
    ApplicationVersionService * const ApplicationVersionServ;
    UserService * const UserServ;
private:
    QElapsedTimer melapsedTimer;
    Server *mserver;
    int timerId;
public:
    explicit Application(int &argc, char **argv, const QString &applicationName, const QString &organizationName);
    ~Application();
public:
    static bool copyTexsample(const QString &path, const QString &codecName = QString("UTF-8"));
public:
    bool initializeEmail();
    bool initializeStorage();
    void initializeTerminal();
    Server *server();
    void updateLoggingMode();
    void updateReadonly();
    qint64 uptime() const;
protected:
    void timerEvent(QTimerEvent *e);
private:
    static bool checkParsingError(BTextTools::OptionsParsingError error, const QString &errorData);
    static bool handleSetAppVersionCommand(const QString &cmd, const QStringList &args);
    static bool handleShrinkDBCommand(const QString &cmd, const QStringList &args);
    static bool handleStartCommand(const QString &cmd, const QStringList &args);
    static bool handleStopCommand(const QString &cmd, const QStringList &args);
    static bool handleUnknownCommand(const QString &command, const QStringList &args);
    static bool handleUptimeCommand(const QString &cmd, const QStringList &args);
    static bool handleUserInfoCommand(const QString &cmd, const QStringList &args);
    static QString msecsToString(qint64 msecs);
private:
    void compatibility();
private:
    Q_DISABLE_COPY(Application)
};

#endif // APPLICATION_H
