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

#include "connection.h"

#include "application.h"
#include "datasource.h"
#include "server.h"
#include "service/applicationversionservice.h"
#include "service/labservice.h"
#include "service/requestin.h"
#include "service/requestout.h"
#include "service/sampleservice.h"
#include "service/userservice.h"
#include "translator.h"

#include <TeXSample/TeXSampleCore>

#include <BGenericSocket>
#include <BNetworkConnection>
#include <BNetworkOperation>
#include <BNetworkOperationMetaData>
#include <BUuid>

#include <QAbstractSocket>
#include <QDateTime>
#include <QDebug>
#include <QLocale>
#include <QMetaObject>
#include <QObject>
#include <QSettings>
#include <QString>
#include <QThread>
#include <QTimer>
#include <QVariant>

/*============================================================================
================================ Connection ==================================
============================================================================*/

/*============================== Public constructors =======================*/

Connection::Connection(BNetworkServer *server, BGenericSocket *socket, const QString &location) :
    BNetworkConnection(server, socket), Source(new DataSource(location)),
    ApplicationVersionServ(new ApplicationVersionService(Source)), LabServ(new LabService(Source)),
    SampleServ(new SampleService(Source)), UserServ(new UserService(Source))
{
    if (!Source->isValid()) {
        QString msg = translationsEnabled() ? tr("Invalid storage instance") : QString("Invalid storage instance");
        logLocal(msg);
        logRemote(msg);
        close();
        return;
    }
    setTranslationsEnabled(false);
    setCriticalBufferSize(3 * BeQt::Megabyte);
    setCloseOnCriticalBufferSize(true);
    mtimer.setInterval(5 * BeQt::Minute);
    connect(&mtimer, SIGNAL(timeout()), this, SLOT(ping()));
    connect(this, SIGNAL(requestSent(BNetworkOperation *)), this, SLOT(restartTimer(BNetworkOperation *)));
    connect(this, SIGNAL(replyReceived(BNetworkOperation *)), this, SLOT(restartTimer(BNetworkOperation *)));
    connect(this, SIGNAL(incomingRequest(BNetworkOperation *)), this, SLOT(restartTimer(BNetworkOperation *)));
    connect(this, SIGNAL(requestReceived(BNetworkOperation *)), this, SLOT(restartTimer(BNetworkOperation *)));
    connect(this, SIGNAL(replySent(BNetworkOperation *)), this, SLOT(restartTimer(BNetworkOperation *)));
    initHandlers();
    QTimer::singleShot(BeQt::Minute, this, SLOT(testAuthorization()));
    mconnectionDT = QDateTime::currentDateTimeUtc();
    muptimeTimer.start();
}

Connection::~Connection()
{
    delete ApplicationVersionServ;
    delete LabServ;
    delete SampleServ;
    delete UserServ;
    delete Source;
}

/*============================== Public methods ============================*/

TClientInfo Connection::clientInfo() const
{
    return mclientInfo;
}

QDateTime Connection::connectionDateTime() const
{
    return mconnectionDT;
}

bool Connection::isSubscribed(Subscription subscription) const
{
    return msubscriptions.value(subscription);
}

QLocale Connection::locale() const
{
    return mlocale;
}

void Connection::sendLogRequest(const QString &text, BLogger::Level lvl)
{
    Qt::ConnectionType ct = (QThread::currentThread() == thread()) ? Qt::DirectConnection : Qt::QueuedConnection;
    QMetaObject::invokeMethod(this, "sendLogRequestInternal", ct, Q_ARG(QString, text), Q_ARG(int, lvl));
}

qint64 Connection::uptime() const
{
    return muptimeTimer.elapsed();
}

TUserInfo Connection::userInfo() const
{
    return muserInfo;
}

/*============================== Purotected methods ========================*/

void Connection::log(const QString &text, BLogger::Level lvl)
{
    logLocal(text, lvl);
    logRemote(text, lvl);
}

void Connection::logLocal(const QString &text, BLogger::Level lvl)
{
    BNetworkConnection::log((muserInfo.id() ? ("[" + muserInfo.login() + "] ") : QString()) + text, lvl);
}

void Connection::logRemote(const QString &text, BLogger::Level lvl)
{
    QString msg = "[" + peerAddress() + "] " + (muserInfo.id() ? ("[" + muserInfo.login() + "] ") : QString()) + text;
    server()->lock();
    foreach (BNetworkConnection *c, server()->connections()) {
        if (this == c)
            continue;
        static_cast<Connection *>(c)->sendLogRequest(msg, lvl);
    }
    server()->unlock();
}

/*============================== Private methods ===========================*/

bool Connection::accessCheck(const Translator &translator, QString *error, const TServiceList &services,
                             const TIdList &groups)
{
    foreach (const TService &s, services) {
        if (!muserInfo.availableServices().contains(s))
            return bRet(error, translator.translate("UserService", "No access to service", "error"), false);
    }
    foreach (quint64 groupId, groups) {
        if (!muserInfo.groups().contains(groupId))
            return bRet(error, translator.translate("UserService", "No access to group", "error"), false);
    }
    return bRet(error, QString(), true);
}

bool Connection::accessCheck(const QLocale &locale, QString *error, const TServiceList &services, const
                             TIdList &groups)
{
    Translator t(locale);
    return accessCheck(t, error, services, groups);
}

bool Connection::commonCheck(const Translator &translator, QString *error, const TAccessLevel &accessLevel,
                             const TService &service)
{
    if (!isValid())
        return bRet(error, translator.translate("Connection", "Invalid Connection instance (internal)", "error"), false);
    bool alvl = (TAccessLevel(TAccessLevel::NoLevel) != accessLevel);
    bool srv = (TService(TService::NoService) != service);
    if (alvl && srv && !muserInfo.isValid())
        return bRet(error, translator.translate("Connection", "Not authorized", "error"), false);
    if (alvl && muserInfo.accessLevel() < accessLevel)
        return bRet(error, translator.translate("Connection", "Not enough rights", "error"), false);
    if (srv && !muserInfo.availableServices().contains(service))
        return bRet(error, translator.translate("Connection", "No access to service", "error"), false);
    return bRet(error, QString(), true);
}

bool Connection::commonCheck(const QLocale &locale, QString *error, const TAccessLevel &accessLevel,
                             const TService &service)
{
    Translator t(locale);
    return commonCheck(t, error, accessLevel, service);
}

bool Connection::handleAddGroupRequest(BNetworkOperation *op)
{
    log("Request: " + op->metaData().operation());
    TRequest request = op->variantData().value<TRequest>();
    QString error;
    if (!commonCheck(request.locale(), &error, TAccessLevel::AdminLevel))
        return sendReply(op, error);
    RequestIn<TAddGroupRequestData> in(request);
    return sendReply(op, UserServ->addGroup(in, muserInfo.id()).createReply());
}

bool Connection::handleAddLabRequest(BNetworkOperation *op)
{
    log("Request: " + op->metaData().operation());
    TRequest request = op->variantData().value<TRequest>();
    QString error;
    if (!commonCheck(request.locale(), &error, TAccessLevel::ModeratorLevel, TService::CloudlabService))
        return sendReply(op, error);
    RequestIn<TAddLabRequestData> in(request);
    return sendReply(op, LabServ->addLab(in, muserInfo.id()).createReply());
}

bool Connection::handleAddSampleRequest(BNetworkOperation *op)
{
    log("Request: " + op->metaData().operation());
    TRequest request = op->variantData().value<TRequest>();
    QString error;
    if (!commonCheck(request.locale(), &error, TAccessLevel::UserLevel, TService::TexsampleService))
        return sendReply(op, error);
    RequestIn<TAddSampleRequestData> in(request);
    return sendReply(op, SampleServ->addSample(in, muserInfo.id()).createReply());
}

bool Connection::handleAddUserRequest(BNetworkOperation *op)
{
    log("Request: " + op->metaData().operation());
    TRequest request = op->variantData().value<TRequest>();
    Translator t(request.locale());
    QString error;
    TAddUserRequestData requestData = request.data().value<TAddUserRequestData>();
    if (!commonCheck(t, &error, qMin(TAccessLevel::AdminLevel, requestData.accessLevel().level())))
        return sendReply(op, error);
    if (!accessCheck(t, &error, requestData.availableServices(), requestData.groups()))
        return sendReply(op, error);
    RequestIn<TAddUserRequestData> in(request);
    return sendReply(op, UserServ->addUser(in).createReply());
}

bool Connection::handleAuthorizeRequest(BNetworkOperation *op)
{
    log("Request: " + op->metaData().operation());
    TRequest request = op->variantData().value<TRequest>();
    Translator t(request.locale());
    QString error;
    if (!commonCheck(t, &error))
        return sendReply(op, error);
    if (muserInfo.isValid()) {
        TAuthorizeReplyData data;
        data.setUserInfo(muserInfo);
        return sendReply(op, t.translate("Connection", "Already authorized", "message"), data);
    }
    RequestIn<TAuthorizeRequestData> in(request);
    RequestOut<TAuthorizeReplyData> out = UserServ->authorize(in);
    if (!out.success())
        return sendReply(op, out.createReply());
    setCriticalBufferSize(200 * BeQt::Megabyte);
    muserInfo = out.data().userInfo();
    mclientInfo = in.data().clientInfo();
    QString s = "Client info:\n";
    s += "User ID: " + QString::number(muserInfo.id()) + "\n";
    s += "User login: " + muserInfo.login() + "\n";
    s += "Unique ID: " + uniqueId().toString(true) + "\n";
    s += "Access level: " + muserInfo.accessLevel().toStringNoTr() + "\n";
    s += mclientInfo.toString("Client: %n v%v (%p)\nTeXSample version: %t\nBeQt version: %b\nQt version: %q\n");
    s += mclientInfo.toString("OS: %o (%a)");
    log(s);
    return sendReply(op, out.createReply());
}

bool Connection::handleChangeEmailRequest(BNetworkOperation *op)
{
    log("Request: " + op->metaData().operation());
    TRequest request = op->variantData().value<TRequest>();
    Translator t(request.locale());
    QString error;
    if (!commonCheck(t, &error, TAccessLevel::UserLevel))
        return sendReply(op, error);
    RequestIn<TChangeEmailRequestData> in(request);
    return sendReply(op, UserServ->changeEmail(in, muserInfo.id()).createReply());
}

bool Connection::handleChangePasswordRequest(BNetworkOperation *op)
{
    log("Request: " + op->metaData().operation());
    TRequest request = op->variantData().value<TRequest>();
    Translator t(request.locale());
    QString error;
    if (!commonCheck(t, &error, TAccessLevel::UserLevel))
        return sendReply(op, error);
    RequestIn<TChangePasswordRequestData> in(request);
    return sendReply(op, UserServ->changePassword(in, muserInfo.id()).createReply());
}

bool Connection::handleCheckEmailAvailabilityRequest(BNetworkOperation *op)
{
    log("Request: " + op->metaData().operation());
    TRequest request = op->variantData().value<TRequest>();
    Translator t(request.locale());
    QString error;
    if (!commonCheck(t, &error))
        return sendReply(op, error);
    RequestIn<TCheckEmailAvailabilityRequestData> in(request);
    return sendReply(op, UserServ->checkEmailAvailability(in).createReply());
}

bool Connection::handleCheckLoginAvailabilityRequest(BNetworkOperation *op)
{
    log("Request: " + op->metaData().operation());
    TRequest request = op->variantData().value<TRequest>();
    Translator t(request.locale());
    QString error;
    if (!commonCheck(t, &error))
        return sendReply(op, error);
    RequestIn<TCheckLoginAvailabilityRequestData> in(request);
    return sendReply(op, UserServ->checkLoginAvailability(in).createReply());
}

bool Connection::handleCompileTexProjectRequest(BNetworkOperation *op)
{
    log("Request: " + op->metaData().operation());
    TRequest request = op->variantData().value<TRequest>();
    QString error;
    if (!commonCheck(request.locale(), &error, TAccessLevel::UserLevel, TService::TexsampleService))
        return sendReply(op, error);
    RequestIn<TCompileTexProjectRequestData> in(request);
    return sendReply(op, SampleServ->compileTexProject(in).createReply());
}

bool Connection::handleConfirmEmailChangeRequest(BNetworkOperation *op)
{
    log("Request: " + op->metaData().operation());
    TRequest request = op->variantData().value<TRequest>();
    QString error;
    if (!commonCheck(request.locale(), &error))
        return sendReply(op, error);
    RequestIn<TConfirmEmailChangeRequestData> in(request);
    return sendReply(op, UserServ->confirmEmailChange(in).createReply());
}

bool Connection::handleConfirmRegistrationRequest(BNetworkOperation *op)
{
    log("Request: " + op->metaData().operation());
    TRequest request = op->variantData().value<TRequest>();
    QString error;
    if (!commonCheck(request.locale(), &error))
        return sendReply(op, error);
    RequestIn<TConfirmRegistrationRequestData> in(request);
    return sendReply(op, UserServ->confirmRegistration(in).createReply());
}

bool Connection::handleDeleteGroupRequest(BNetworkOperation *op)
{
    log("Request: " + op->metaData().operation());
    TRequest request = op->variantData().value<TRequest>();
    QString error;
    if (!commonCheck(request.locale(), &error, TAccessLevel::ModeratorLevel))
        return sendReply(op, error);
    RequestIn<TDeleteGroupRequestData> in(request);
    return sendReply(op, UserServ->deleteGroup(in, muserInfo.id()).createReply());
}

bool Connection::handleDeleteInvitesRequest(BNetworkOperation *op)
{
    log("Request: " + op->metaData().operation());
    TRequest request = op->variantData().value<TRequest>();
    QString error;
    if (!commonCheck(request.locale(), &error, TAccessLevel::ModeratorLevel))
        return sendReply(op, error);
    RequestIn<TDeleteInvitesRequestData> in(request);
    return sendReply(op, UserServ->deleteInvites(in, muserInfo.id()).createReply());
}

bool Connection::handleDeleteLabRequest(BNetworkOperation *op)
{
    log("Request: " + op->metaData().operation());
    TRequest request = op->variantData().value<TRequest>();
    QString error;
    if (!commonCheck(request.locale(), &error, TAccessLevel::ModeratorLevel, TService::CloudlabService))
        return sendReply(op, error);
    RequestIn<TDeleteLabRequestData> in(request);
    return sendReply(op, LabServ->deleteLab(in, muserInfo.id()).createReply());
}

bool Connection::handleDeleteSampleRequest(BNetworkOperation *op)
{
    log("Request: " + op->metaData().operation());
    TRequest request = op->variantData().value<TRequest>();
    QString error;
    if (!commonCheck(request.locale(), &error, TAccessLevel::UserLevel, TService::TexsampleService))
        return sendReply(op, error);
    RequestIn<TDeleteSampleRequestData> in(request);
    return sendReply(op, SampleServ->deleteSample(in, muserInfo.id()).createReply());
}

bool Connection::handleDeleteUserRequest(BNetworkOperation *op)
{
    log("Request: " + op->metaData().operation());
    TRequest request = op->variantData().value<TRequest>();
    QString error;
    if (!commonCheck(request.locale(), &error, TAccessLevel::SuperuserLevel))
        return sendReply(op, error);
    RequestIn<TDeleteUserRequestData> in(request);
    return sendReply(op, UserServ->deleteUser(in).createReply());
}

bool Connection::handleEditGroupRequest(BNetworkOperation *op)
{
    log("Request: " + op->metaData().operation());
    TRequest request = op->variantData().value<TRequest>();
    QString error;
    if (!commonCheck(request.locale(), &error, TAccessLevel::ModeratorLevel))
        return sendReply(op, error);
    RequestIn<TEditGroupRequestData> in(request);
    return sendReply(op, UserServ->editGroup(in, muserInfo.id()).createReply());
}

bool Connection::handleEditLabRequest(BNetworkOperation *op)
{
    log("Request: " + op->metaData().operation());
    TRequest request = op->variantData().value<TRequest>();
    QString error;
    if (!commonCheck(request.locale(), &error, TAccessLevel::ModeratorLevel, TService::CloudlabService))
        return sendReply(op, error);
    RequestIn<TEditLabRequestData> in(request);
    return sendReply(op, LabServ->editLab(in, muserInfo.id()).createReply());
}

bool Connection::handleEditSampleRequest(BNetworkOperation *op)
{
    log("Request: " + op->metaData().operation());
    TRequest request = op->variantData().value<TRequest>();
    QString error;
    if (!commonCheck(request.locale(), &error, TAccessLevel::UserLevel, TService::TexsampleService))
        return sendReply(op, error);
    RequestIn<TEditSampleRequestData> in(request);
    return sendReply(op, SampleServ->editSample(in, muserInfo.id()).createReply());
}

bool Connection::handleEditSampleAdminRequest(BNetworkOperation *op)
{
    log("Request: " + op->metaData().operation());
    TRequest request = op->variantData().value<TRequest>();
    QString error;
    if (!commonCheck(request.locale(), &error, TAccessLevel::ModeratorLevel, TService::TexsampleService))
        return sendReply(op, error);
    RequestIn<TEditSampleAdminRequestData> in(request);
    return sendReply(op, SampleServ->editSampleAdmin(in, muserInfo.id()).createReply());
}

bool Connection::handleEditSelfRequest(BNetworkOperation *op)
{
    log("Request: " + op->metaData().operation());
    TRequest request = op->variantData().value<TRequest>();
    QString error;
    if (!commonCheck(request.locale(), &error, TAccessLevel::UserLevel))
        return sendReply(op, error);
    RequestIn<TEditSelfRequestData> in(request);
    return sendReply(op, UserServ->editSelf(in, muserInfo.id()).createReply());
}

bool Connection::handleEditUserRequest(BNetworkOperation *op)
{
    log("Request: " + op->metaData().operation());
    TRequest request = op->variantData().value<TRequest>();
    QString error;
    if (!commonCheck(request.locale(), &error, TAccessLevel::AdminLevel))
        return sendReply(op, error);
    RequestIn<TEditUserRequestData> in(request);
    return sendReply(op, UserServ->editUser(in, muserInfo.id()).createReply());
}

bool Connection::handleGenerateInvitesRequest(BNetworkOperation *op)
{
    log("Request: " + op->metaData().operation());
    TRequest request = op->variantData().value<TRequest>();
    Translator t(request.locale());
    QString error;
    TGenerateInvitesRequestData requestData = request.data().value<TGenerateInvitesRequestData>();
    if (!commonCheck(t, &error, qMin(TAccessLevel::AdminLevel, requestData.accessLevel().level())))
        return sendReply(op, error);
    if (!accessCheck(t, &error, requestData.services(), requestData.groups()))
        return sendReply(op, error);
    RequestIn<TGenerateInvitesRequestData> in(request);
    return sendReply(op, UserServ->generateInvites(in, muserInfo.id()).createReply());
}

bool Connection::handleGetGroupInfoListRequest(BNetworkOperation *op)
{
    log("Request: " + op->metaData().operation());
    TRequest request = op->variantData().value<TRequest>();
    QString error;
    if (!commonCheck(request.locale(), &error, TAccessLevel::ModeratorLevel))
        return sendReply(op, error);
    RequestIn<TGetGroupInfoListRequestData> in(request);
    return sendReply(op, UserServ->getGroupInfoList(in, muserInfo.id()).createReply());
}

bool Connection::handleGetInviteInfoListRequest(BNetworkOperation *op)
{
    log("Request: " + op->metaData().operation());
    TRequest request = op->variantData().value<TRequest>();
    QString error;
    if (!commonCheck(request.locale(), &error, TAccessLevel::AdminLevel))
        return sendReply(op, error);
    RequestIn<TGetInviteInfoListRequestData> in(request);
    return sendReply(op, UserServ->getInviteInfoList(in, muserInfo.id()).createReply());
}

bool Connection::handleGetLabDataRequest(BNetworkOperation *op)
{
    log("Request: " + op->metaData().operation());
    TRequest request = op->variantData().value<TRequest>();
    QString error;
    if (!commonCheck(request.locale(), &error, TAccessLevel::UserLevel, TService::CloudlabService))
        return sendReply(op, error);
    RequestIn<TGetLabDataRequestData> in(request);
    return sendReply(op, LabServ->getLabData(in).createReply());
}

bool Connection::handleGetLabExtraFileRequest(BNetworkOperation *op)
{
    log("Request: " + op->metaData().operation());
    TRequest request = op->variantData().value<TRequest>();
    QString error;
    if (!commonCheck(request.locale(), &error, TAccessLevel::UserLevel, TService::CloudlabService))
        return sendReply(op, error);
    RequestIn<TGetLabExtraFileRequestData> in(request);
    return sendReply(op, LabServ->getLabExtraFile(in).createReply());
}

bool Connection::handleGetLabInfoListRequest(BNetworkOperation *op)
{
    log("Request: " + op->metaData().operation());
    TRequest request = op->variantData().value<TRequest>();
    QString error;
    if (!commonCheck(request.locale(), &error, TAccessLevel::UserLevel, TService::CloudlabService))
        return sendReply(op, error);
    RequestIn<TGetLabInfoListRequestData> in(request);
    return sendReply(op, LabServ->getLabInfoList(in, muserInfo.id()).createReply());
}

bool Connection::handleGetLatestAppVersionRequest(BNetworkOperation *op)
{
    log("Request: " + op->metaData().operation());
    TRequest request = op->variantData().value<TRequest>();
    QString error;
    if (!commonCheck(request.locale(), &error))
        return sendReply(op, error);
    RequestIn<TGetLatestAppVersionRequestData> in(request);
    return sendReply(op, ApplicationVersionServ->getLatestAppVersion(in).createReply());
}

bool Connection::handleGetSampleInfoListRequest(BNetworkOperation *op)
{
    log("Request: " + op->metaData().operation());
    TRequest request = op->variantData().value<TRequest>();
    QString error;
    if (!commonCheck(request.locale(), &error, TAccessLevel::UserLevel, TService::TexsampleService))
        return sendReply(op, error);
    RequestIn<TGetSampleInfoListRequestData> in(request);
    return sendReply(op, SampleServ->getSampleInfoList(in).createReply());
}

bool Connection::handleGetSamplePreviewRequest(BNetworkOperation *op)
{
    log("Request: " + op->metaData().operation());
    TRequest request = op->variantData().value<TRequest>();
    QString error;
    if (!commonCheck(request.locale(), &error, TAccessLevel::UserLevel, TService::TexsampleService))
        return sendReply(op, error);
    RequestIn<TGetSamplePreviewRequestData> in(request);
    return sendReply(op, SampleServ->getSamplePreview(in).createReply());
}

bool Connection::handleGetSampleSourceRequest(BNetworkOperation *op)
{
    log("Request: " + op->metaData().operation());
    TRequest request = op->variantData().value<TRequest>();
    QString error;
    if (!commonCheck(request.locale(), &error, TAccessLevel::UserLevel, TService::TexsampleService))
        return sendReply(op, error);
    RequestIn<TGetSampleSourceRequestData> in(request);
    return sendReply(op, SampleServ->getSampleSource(in).createReply());
}

bool Connection::handleGetSelfInfoRequest(BNetworkOperation *op)
{
    log("Request: " + op->metaData().operation());
    TRequest request = op->variantData().value<TRequest>();
    QString error;
    if (!commonCheck(request.locale(), &error, TAccessLevel::UserLevel))
        return sendReply(op, error);
    RequestIn<TGetSelfInfoRequestData> in(request);
    return sendReply(op, UserServ->getSelfInfo(in, muserInfo.id()).createReply());
}

bool Connection::handleGetServerStateRequest(BNetworkOperation *op)
{
    log("Request: " + op->metaData().operation());
    TRequest request = op->variantData().value<TRequest>();
    QString error;
    if (!commonCheck(request.locale(), &error, TAccessLevel::ModeratorLevel))
        return sendReply(op, error);
    TGetServerStateReplyData replyData;
    TServerState state;
    state.setListening(server()->isListening());
    state.setUptime(bApp->uptime());
    replyData.setServerState(state);
    return sendReply(op, "", replyData);
}

bool Connection::handleGetUserConnectionInfoListRequest(BNetworkOperation *op)
{
    log("Request: " + op->metaData().operation());
    TRequest request = op->variantData().value<TRequest>();
    QString error;
    if (!commonCheck(request.locale(), &error, TAccessLevel::ModeratorLevel))
        return sendReply(op, error);
    TGetUserConnectionInfoListRequestData requestData = request.data().value<TGetUserConnectionInfoListRequestData>();
    TGetUserConnectionInfoListReplyData replyData;
    TUserConnectionInfoList list = bApp->server()->userConnections(requestData.matchPattern(),
                                                                   requestData.matchType());
    replyData.setConnectionInfoList(list);
    return sendReply(op, "", replyData);
}

bool Connection::handleGetUserInfoAdminRequest(BNetworkOperation *op)
{
    log("Request: " + op->metaData().operation());
    TRequest request = op->variantData().value<TRequest>();
    QString error;
    if (!commonCheck(request.locale(), &error, TAccessLevel::AdminLevel))
        return sendReply(op, error);
    RequestIn<TGetUserInfoAdminRequestData> in(request);
    return sendReply(op, UserServ->getUserInfoAdmin(in).createReply());
}

bool Connection::handleGetUserInfoListAdminRequest(BNetworkOperation *op)
{
    log("Request: " + op->metaData().operation());
    TRequest request = op->variantData().value<TRequest>();
    QString error;
    if (!commonCheck(request.locale(), &error, TAccessLevel::AdminLevel))
        return sendReply(op, error);
    RequestIn<TGetUserInfoListAdminRequestData> in(request);
    return sendReply(op, UserServ->getUserInfoListAdmin(in).createReply());
}

bool Connection::handleGetUserInfoRequest(BNetworkOperation *op)
{
    log("Request: " + op->metaData().operation());
    TRequest request = op->variantData().value<TRequest>();
    QString error;
    if (!commonCheck(request.locale(), &error, TAccessLevel::UserLevel))
        return sendReply(op, error);
    RequestIn<TGetUserInfoRequestData> in(request);
    return sendReply(op, UserServ->getUserInfo(in).createReply());
}

bool Connection::handleNoopRequest(BNetworkOperation *op)
{
    int l = bSettings->value("Log/noop").toInt();
    QString s = "Replying to connection test";
    if (1 == l)
        logLocal(s);
    else if (l > 1)
        log(s);
    op->reply();
    return true;
}

bool Connection::handleRecoverAccountRequest(BNetworkOperation *op)
{
    log("Request: " + op->metaData().operation());
    TRequest request = op->variantData().value<TRequest>();
    QString error;
    if (!commonCheck(request.locale(), &error))
        return sendReply(op, error);
    RequestIn<TRecoverAccountRequestData> in(request);
    return sendReply(op, UserServ->recoverAccount(in).createReply());
}

bool Connection::handleRegisterRequest(BNetworkOperation *op)
{
    log("Request: " + op->metaData().operation());
    TRequest request = op->variantData().value<TRequest>();
    QString error;
    if (!commonCheck(request.locale(), &error))
        return sendReply(op, error);
    RequestIn<TRegisterRequestData> in(request);
    return sendReply(op, UserServ->registerUser(in).createReply());
}

bool Connection::handleRequestRecoveryCodeRequest(BNetworkOperation *op)
{
    log("Request: " + op->metaData().operation());
    TRequest request = op->variantData().value<TRequest>();
    QString error;
    if (!commonCheck(request.locale(), &error))
        return sendReply(op, error);
    RequestIn<TRequestRecoveryCodeRequestData> in(request);
    return sendReply(op, UserServ->requestRecoveryCode(in).createReply());
}

bool Connection::handleSetLatestAppVersionRequest(BNetworkOperation *op)
{
    log("Request: " + op->metaData().operation());
    TRequest request = op->variantData().value<TRequest>();
    QString error;
    if (!commonCheck(request.locale(), &error, TAccessLevel::AdminLevel))
        return sendReply(op, error);
    RequestIn<TSetLatestAppVersionRequestData> in(request);
    return sendReply(op, ApplicationVersionServ->setLatestAppVersion(in).createReply());
}

bool Connection::handleSetServerStateRequest(BNetworkOperation *op)
{
    log("Request: " + op->metaData().operation());
    TRequest request = op->variantData().value<TRequest>();
    Translator t(request.locale());
    QString error;
    if (!commonCheck(t, &error, TAccessLevel::AdminLevel))
        return sendReply(op, error);
    TSetServerStateRequestData requestData = request.data().value<TSetServerStateRequestData>();
    if (requestData.listening()) {
        if (server()->isListening())
            return sendReply(op, t.translate("Connection", "Server is already listening", "error"), false);
        QString address = requestData.address();
        if (address.isEmpty())
            address = "0.0.0.0";
        bool b = false;
        QMetaObject::invokeMethod(server(), "listenSlot", Qt::BlockingQueuedConnection, Q_RETURN_ARG(bool, b),
                                  Q_ARG(QString, address), Q_ARG(quint16, Texsample::MainPort));
        if (!b)
            return sendReply(op, t.translate("Connection", "Failed to start server", "error"), false);
    } else {
        server()->close();
    }
    TServerState state;
    state.setListening(server()->isListening());
    state.setUptime(bApp->uptime());
    TSetServerStateReplyData replyData;
    replyData.setServerState(state);
    return sendReply(op, "", replyData);
}

bool Connection::handleSubscribeRequest(BNetworkOperation *op)
{
    log("Request: " + op->metaData().operation());
    TRequest request = op->variantData().value<TRequest>();
    QString error;
    if (!commonCheck(request.locale(), &error, TAccessLevel::ModeratorLevel))
        return sendReply(op, error);
    TSubscribeRequestData requestData = request.data().value<TSubscribeRequestData>();
    if (requestData.subscribedToLog())
        msubscriptions.insert(LogSubscription, true);
    else
        msubscriptions.remove(LogSubscription);
    TSubscribeReplyData replyData;
    return sendReply(op, "", replyData);
}

void Connection::initHandlers()
{
    installRequestHandler(TOperation::AddGroup, InternalHandler(&Connection::handleAddGroupRequest));
    installRequestHandler(TOperation::AddLab, InternalHandler(&Connection::handleAddLabRequest));
    installRequestHandler(TOperation::AddSample, InternalHandler(&Connection::handleAddSampleRequest));
    installRequestHandler(TOperation::AddUser, InternalHandler(&Connection::handleAddUserRequest));
    installRequestHandler(TOperation::Authorize, InternalHandler(&Connection::handleAuthorizeRequest));
    installRequestHandler(TOperation::ChangeEmail, InternalHandler(&Connection::handleChangeEmailRequest));
    installRequestHandler(TOperation::ChangePassword, InternalHandler(&Connection::handleChangePasswordRequest));
    installRequestHandler(TOperation::CheckEmailAvailability,
                          InternalHandler(&Connection::handleCheckEmailAvailabilityRequest));
    installRequestHandler(TOperation::CheckLoginAvailability,
                          InternalHandler(&Connection::handleCheckLoginAvailabilityRequest));
    installRequestHandler(TOperation::CompileTexProject, InternalHandler(&Connection::handleCompileTexProjectRequest));
    installRequestHandler(TOperation::ConfirmEmailChange,
                          InternalHandler(&Connection::handleConfirmEmailChangeRequest));
    installRequestHandler(TOperation::ConfirmRegistration,
                          InternalHandler(&Connection::handleConfirmRegistrationRequest));
    installRequestHandler(TOperation::DeleteGroup, InternalHandler(&Connection::handleDeleteGroupRequest));
    installRequestHandler(TOperation::DeleteInvites, InternalHandler(&Connection::handleDeleteInvitesRequest));
    installRequestHandler(TOperation::DeleteLab, InternalHandler(&Connection::handleDeleteLabRequest));
    installRequestHandler(TOperation::DeleteSample, InternalHandler(&Connection::handleDeleteSampleRequest));
    installRequestHandler(TOperation::DeleteUser, InternalHandler(&Connection::handleDeleteUserRequest));
    installRequestHandler(TOperation::EditGroup, InternalHandler(&Connection::handleEditGroupRequest));
    installRequestHandler(TOperation::EditLab, InternalHandler(&Connection::handleEditLabRequest));
    installRequestHandler(TOperation::EditSample, InternalHandler(&Connection::handleEditSampleRequest));
    installRequestHandler(TOperation::EditSampleAdmin, InternalHandler(&Connection::handleEditSampleAdminRequest));
    installRequestHandler(TOperation::EditSelf, InternalHandler(&Connection::handleEditSelfRequest));
    installRequestHandler(TOperation::EditUser, InternalHandler(&Connection::handleEditUserRequest));
    installRequestHandler(TOperation::GenerateInvites, InternalHandler(&Connection::handleGenerateInvitesRequest));
    installRequestHandler(TOperation::GetGroupInfoList, InternalHandler(&Connection::handleGetGroupInfoListRequest));
    installRequestHandler(TOperation::GetInviteInfoList, InternalHandler(&Connection::handleGetInviteInfoListRequest));
    installRequestHandler(TOperation::GetLabData, InternalHandler(&Connection::handleGetLabDataRequest));
    installRequestHandler(TOperation::GetLabExtraFile, InternalHandler(&Connection::handleGetLabExtraFileRequest));
    installRequestHandler(TOperation::GetLabInfoList, InternalHandler(&Connection::handleGetLabInfoListRequest));
    installRequestHandler(TOperation::GetLatestAppVersion,
                          InternalHandler(&Connection::handleGetLatestAppVersionRequest));
    installRequestHandler(TOperation::GetSampleInfoList, InternalHandler(&Connection::handleGetSampleInfoListRequest));
    installRequestHandler(TOperation::GetSamplePreview, InternalHandler(&Connection::handleGetSamplePreviewRequest));
    installRequestHandler(TOperation::GetSampleSource, InternalHandler(&Connection::handleGetSampleSourceRequest));
    installRequestHandler(TOperation::GetSelfInfo, InternalHandler(&Connection::handleGetSelfInfoRequest));
    installRequestHandler(TOperation::GetServerState, InternalHandler(&Connection::handleGetServerStateRequest));
    installRequestHandler(TOperation::GetUserConnectionInfoList,
                          InternalHandler(&Connection::handleGetUserConnectionInfoListRequest));
    installRequestHandler(TOperation::GetUserInfo, InternalHandler(&Connection::handleGetUserInfoRequest));
    installRequestHandler(TOperation::GetUserInfoAdmin, InternalHandler(&Connection::handleGetUserInfoAdminRequest));
    installRequestHandler(TOperation::GetUserInfoListAdmin,
                          InternalHandler(&Connection::handleGetUserInfoListAdminRequest));
    installRequestHandler(operation(NoopOperation),InternalHandler(&Connection::handleNoopRequest));
    installRequestHandler(TOperation::RecoverAccount, InternalHandler(&Connection::handleRecoverAccountRequest));
    installRequestHandler(TOperation::Register, InternalHandler(&Connection::handleRegisterRequest));
    installRequestHandler(TOperation::RequestRecoveryCode,
                          InternalHandler(&Connection::handleRequestRecoveryCodeRequest));
    installRequestHandler(TOperation::SetLatestAppVersion,
                          InternalHandler(&Connection::handleSetLatestAppVersionRequest));
    installRequestHandler(TOperation::SetServerState, InternalHandler(&Connection::handleSetServerStateRequest));
    installRequestHandler(TOperation::Subscribe, InternalHandler(&Connection::handleSubscribeRequest));
}

bool Connection::sendReply(BNetworkOperation *op, const TReply &reply)
{
    log("Request: " + op->metaData().operation() + " - " + (reply.success() ? "success" : "fail"));
    return op->reply(QVariant::fromValue(reply)) && reply.success();
}

bool Connection::sendReply(BNetworkOperation *op, const QString &message, bool success)
{
    TReply reply(message);
    reply.setSuccess(success);
    return sendReply(op, reply);
}

bool Connection::sendReply(BNetworkOperation *op, const QString &message, const QVariant &data)
{
    TReply reply(message);
    reply.setSuccess(true);
    reply.setData(data);
    return sendReply(op, reply);
}

/*============================== Private slots =============================*/

void Connection::ping()
{
    if (!muserInfo.isValid() || !isConnected())
        return;
    mtimer.stop();
    int l = bSettings->value("Log/noop").toInt();
    QString s = "Testing connection...";
    if (1 == l)
        logLocal(s);
    else if (l > 1)
        log(s);
    BNetworkOperation *op = sendRequest(operation(NoopOperation));
    op->setAutoDelete(true);
    bool b = op->waitForFinished(5 * BeQt::Minute);
    if (!b) {
        log("Connection response timeout");
        op->cancel();
    }
    if (b)
        mtimer.start();
}

void Connection::restartTimer(BNetworkOperation *op)
{
    if (op && op->metaData().operation() == operation(NoopOperation))
        return;
    mtimer.stop();
    mtimer.start();
}

void Connection::sendLogRequestInternal(const QString &text, int lvl)
{
    if (!muserInfo.isValid() || !isSubscribed(LogSubscription) || text.isEmpty() || !isConnected()
            || QAbstractSocket::UnknownSocketError != socket()->error())
        return;
    TLogRequestData requestData;
    requestData.setLevel(enum_cast<BLogger::Level>(lvl, BLogger::NoLevel, BLogger::FatalLevel));
    requestData.setText(text);
    BNetworkOperation *op = sendRequest(TOperation::Log, TRequest(requestData));
    op->waitForFinished();
    op->deleteLater();
}

void Connection::testAuthorization()
{
    if (muserInfo.isValid())
        return;
    log("Authorization failed, closing connection");
    close();
}
