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

#ifndef CONNECTION_H
#define CONNECTION_H

class ApplicationVersionService;
class DataSource;
class LabService;
class SampleService;
class Translator;
class UserService;

class TReply;

class BNetworkServer;
class BGenericSocket;
class BNetworkOperation;

class QString;

#include "application.h"

#include <TAccessLevel>
#include <TClientInfo>
#include <TIdList>
#include <TService>
#include <TServiceList>
#include <TUserInfo>

#include <BLogger>
#include <BNetworkConnection>

#include <QDateTime>
#include <QElapsedTimer>
#include <QLocale>
#include <QMap>
#include <QObject>
#include <QTimer>

/*============================================================================
================================ Connection ==================================
============================================================================*/

class Connection : public BNetworkConnection
{
    Q_OBJECT
public:
    enum Subscription
    {
        LogSubscription = 1
    };
private:
    enum LogTarget
    {
        NoTarget = 0,
        LocalOnly,
        RemoteOnly,
        LocalAndRemote
    };
private:
    DataSource * const Source;
    ApplicationVersionService * const ApplicationVersionServ;
    LabService * const LabServ;
    SampleService * const SampleServ;
    UserService * const UserServ;
private:
    TClientInfo mclientInfo;
    QDateTime mconnectionDT;
    QLocale mlocale;
    QMap<Subscription, bool> msubscriptions;
    QTimer mtimer;
    QElapsedTimer muptimeTimer;
    TUserInfo muserInfo;
public:
    explicit Connection(BNetworkServer *server, BGenericSocket *socket, const QString &location);
    ~Connection();
public:
    TClientInfo clientInfo() const;
    QDateTime connectionDateTime() const;
    bool isSubscribed(Subscription subscription) const;
    QLocale locale() const;
    void sendLogRequest(const QString &text, BLogger::Level lvl = BLogger::NoLevel);
    qint64 uptime() const;
    TUserInfo userInfo() const;
protected:
    void log(const QString &text, BLogger::Level lvl = BLogger::NoLevel);
    void logLocal(const QString &text, BLogger::Level lvl = BLogger::NoLevel);
    void logRemote(const QString &text, BLogger::Level lvl = BLogger::NoLevel);
private:
    bool accessCheck(const Translator &translator, QString *error = 0, const TServiceList &services = TServiceList(),
                     const TIdList &groups = TIdList());
    bool accessCheck(const QLocale &locale = Application::locale(), QString *error = 0,
                     const TServiceList &services = TServiceList(), const TIdList &groups = TIdList());
    bool commonCheck(const Translator &translator, QString *error = 0,
                     const TAccessLevel &accessLevel = TAccessLevel::NoLevel,
                     const TService &service = TService::NoService);
    bool commonCheck(const QLocale &locale = Application::locale(), QString *error = 0,
                     const TAccessLevel &accessLevel = TAccessLevel::NoLevel,
                     const TService &service = TService::NoService);
    bool handleAddGroupRequest(BNetworkOperation *op);
    bool handleAddLabRequest(BNetworkOperation *op);
    bool handleAddSampleRequest(BNetworkOperation *op);
    bool handleAddUserRequest(BNetworkOperation *op);
    bool handleAuthorizeRequest(BNetworkOperation *op);
    bool handleChangeEmailRequest(BNetworkOperation *op);
    bool handleChangePasswordRequest(BNetworkOperation *op);
    bool handleCheckEmailAvailabilityRequest(BNetworkOperation *op);
    bool handleCheckLoginAvailabilityRequest(BNetworkOperation *op);
    bool handleCompileTexProjectRequest(BNetworkOperation *op);
    bool handleConfirmEmailChangeRequest(BNetworkOperation *op);
    bool handleConfirmRegistrationRequest(BNetworkOperation *op);
    bool handleDeleteGroupRequest(BNetworkOperation *op);
    bool handleDeleteInvitesRequest(BNetworkOperation *op);
    bool handleDeleteLabRequest(BNetworkOperation *op);
    bool handleDeleteSampleRequest(BNetworkOperation *op);
    bool handleDeleteUserRequest(BNetworkOperation *op);
    bool handleEditGroupRequest(BNetworkOperation *op);
    bool handleEditLabRequest(BNetworkOperation *op);
    bool handleEditSampleRequest(BNetworkOperation *op);
    bool handleEditSampleAdminRequest(BNetworkOperation *op);
    bool handleEditSelfRequest(BNetworkOperation *op);
    bool handleEditUserRequest(BNetworkOperation *op);
    bool handleGenerateInvitesRequest(BNetworkOperation *op);
    bool handleGetGroupInfoListRequest(BNetworkOperation *op);
    bool handleGetInviteInfoListRequest(BNetworkOperation *op);
    bool handleGetLabDataRequest(BNetworkOperation *op);
    bool handleGetLabExtraFileRequest(BNetworkOperation *op);
    bool handleGetLabInfoListRequest(BNetworkOperation *op);
    bool handleGetLatestAppVersionRequest(BNetworkOperation *op);
    bool handleGetSampleInfoListRequest(BNetworkOperation *op);
    bool handleGetSamplePreviewRequest(BNetworkOperation *op);
    bool handleGetSampleSourceRequest(BNetworkOperation *op);
    bool handleGetSelfInfoRequest(BNetworkOperation *op);
    bool handleGetServerStateRequest(BNetworkOperation *op);
    bool handleGetUserConnectionInfoListRequest(BNetworkOperation *op);
    bool handleGetUserInfoAdminRequest(BNetworkOperation *op);
    bool handleGetUserInfoListAdminRequest(BNetworkOperation *op);
    bool handleGetUserInfoRequest(BNetworkOperation *op);
    bool handleNoopRequest(BNetworkOperation *op);
    bool handleRecoverAccountRequest(BNetworkOperation *op);
    bool handleRegisterRequest(BNetworkOperation *op);
    bool handleRequestRecoveryCodeRequest(BNetworkOperation *op);
    bool handleSetLatestAppVersionRequest(BNetworkOperation *op);
    bool handleSetServerStateRequest(BNetworkOperation *op);
    bool handleSubscribeRequest(BNetworkOperation *op);
    void initHandlers();
    bool sendReply(BNetworkOperation *op, const TReply &reply);
    bool sendReply(BNetworkOperation *op, const QString &message, bool success = false);
    bool sendReply(BNetworkOperation *op, const QString &message, const QVariant &data);
private slots:
    void ping();
    void restartTimer(BNetworkOperation *op = 0);
    void sendLogRequestInternal(const QString &text, int lvl);
    void testAuthorization();
};

#endif // CONNECTION_H
