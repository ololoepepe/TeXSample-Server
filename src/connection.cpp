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
#include "service/requestin.h"
#include "service/requestout.h"
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
    BNetworkConnection(server, socket), Source(new DataSource(location)), UserServ(new UserService(Source))
{
    if (!Source->isValid()) {
        QString msg = translationsEnabled() ? tr("Invalid storage instance") : QString("Invalid storage instance");
        logLocal(msg);
        logRemote(msg);
        close();
        return;
    }
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
    TRequest request = op->variantData().value<TRequest>();
    QString error;
    if (!commonCheck(request.locale(), &error, TAccessLevel::AdminLevel))
        return sendReply(op, error);
    RequestIn<TAddGroupRequestData> in(request);
    return sendReply(op, UserServ->addGroup(in, muserInfo.id()).createReply());
}

bool Connection::handleAddLabRequest(BNetworkOperation *op)
{
    //TODO
}

bool Connection::handleAddSampleRequest(BNetworkOperation *op)
{
    //TODO
}

bool Connection::handleAddUserRequest(BNetworkOperation *op)
{
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
    muserInfo = out.data().userInfo();
    mclientInfo = in.data().clientInfo();
    return sendReply(op, out.createReply());
}

bool Connection::handleChangeEmailRequest(BNetworkOperation *op)
{
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
    //TODO
}

bool Connection::handleConfirmEmailChangeRequest(BNetworkOperation *op)
{
    TRequest request = op->variantData().value<TRequest>();
    QString error;
    if (!commonCheck(request.locale(), &error))
        return sendReply(op, error);
    RequestIn<TConfirmEmailChangeRequestData> in(request);
    return sendReply(op, UserServ->confirmEmailChange(in).createReply());
}

bool Connection::handleConfirmRegistrationRequest(BNetworkOperation *op)
{
    TRequest request = op->variantData().value<TRequest>();
    QString error;
    if (!commonCheck(request.locale(), &error))
        return sendReply(op, error);
    RequestIn<TConfirmRegistrationRequestData> in(request);
    return sendReply(op, UserServ->confirmRegistration(in).createReply());
}

bool Connection::handleDeleteGroupRequest(BNetworkOperation *op)
{
    TRequest request = op->variantData().value<TRequest>();
    QString error;
    if (!commonCheck(request.locale(), &error, TAccessLevel::ModeratorLevel))
        return sendReply(op, error);
    RequestIn<TDeleteGroupRequestData> in(request);
    return sendReply(op, UserServ->deleteGroup(in, muserInfo.id()).createReply());
}

bool Connection::handleDeleteInvitesRequest(BNetworkOperation *op)
{
    TRequest request = op->variantData().value<TRequest>();
    QString error;
    if (!commonCheck(request.locale(), &error, TAccessLevel::ModeratorLevel))
        return sendReply(op, error);
    RequestIn<TDeleteInvitesRequestData> in(request);
    return sendReply(op, UserServ->deleteInvites(in, muserInfo.id()).createReply());
}

bool Connection::handleDeleteLabRequest(BNetworkOperation *op)
{
    //TODO
}

bool Connection::handleDeleteSampleRequest(BNetworkOperation *op)
{
    //TODO
}

bool Connection::handleDeleteUserRequest(BNetworkOperation *op)
{
    //TODO
    TRequest request = op->variantData().value<TRequest>();
    Translator t(request.locale());
    return sendReply(op, t.translate("Connection", "This operation is not supported yet", "message"));
}

bool Connection::handleEditGroupRequest(BNetworkOperation *op)
{
    TRequest request = op->variantData().value<TRequest>();
    QString error;
    if (!commonCheck(request.locale(), &error, TAccessLevel::ModeratorLevel))
        return sendReply(op, error);
    RequestIn<TEditGroupRequestData> in(request);
    return sendReply(op, UserServ->editGroup(in, muserInfo.id()).createReply());
}

bool Connection::handleEditLabRequest(BNetworkOperation *op)
{
    //TODO
}

bool Connection::handleEditSampleRequest(BNetworkOperation *op)
{
    //TODO
}

bool Connection::handleEditSampleAdminRequest(BNetworkOperation *op)
{
    //TODO
}

bool Connection::handleEditSelfRequest(BNetworkOperation *op)
{
    TRequest request = op->variantData().value<TRequest>();
    QString error;
    if (!commonCheck(request.locale(), &error, TAccessLevel::UserLevel))
        return sendReply(op, error);
    RequestIn<TEditSelfRequestData> in(request);
    return sendReply(op, UserServ->editSelf(in, muserInfo.id()).createReply());
}

bool Connection::handleEditUserRequest(BNetworkOperation *op)
{
    TRequest request = op->variantData().value<TRequest>();
    QString error;
    if (!commonCheck(request.locale(), &error, TAccessLevel::AdminLevel))
        return sendReply(op, error);
    RequestIn<TEditUserRequestData> in(request);
    return sendReply(op, UserServ->editUser(in, muserInfo.id()).createReply());
}

bool Connection::handleGenerateInvitesRequest(BNetworkOperation *op)
{
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
    TRequest request = op->variantData().value<TRequest>();
    QString error;
    if (!commonCheck(request.locale(), &error, TAccessLevel::ModeratorLevel))
        return sendReply(op, error);
    RequestIn<TGetGroupInfoListRequestData> in(request);
    return sendReply(op, UserServ->getGroupInfoList(in, muserInfo.id()).createReply());
}

bool Connection::handleGetInviteInfoListRequest(BNetworkOperation *op)
{
    TRequest request = op->variantData().value<TRequest>();
    QString error;
    if (!commonCheck(request.locale(), &error, TAccessLevel::AdminLevel))
        return sendReply(op, error);
    RequestIn<TGetInviteInfoListRequestData> in(request);
    return sendReply(op, UserServ->getInviteInfoList(in, muserInfo.id()).createReply());
}

bool Connection::handleGetLabDataRequest(BNetworkOperation *op)
{
    //TODO
}

bool Connection::handleGetLabExtraFileRequest(BNetworkOperation *op)
{
    //TODO
}

bool Connection::handleGetLabInfoListRequest(BNetworkOperation *op)
{
    //TODO
}

bool Connection::handleGetLatestAppVersionRequest(BNetworkOperation *op)
{
    //TODO
}

bool Connection::handleGetSampleInfoListRequest(BNetworkOperation *op)
{
    //TODO
}

bool Connection::handleGetSamplePreviewRequest(BNetworkOperation *op)
{
    //TODO
}

bool Connection::handleGetSampleSourceRequest(BNetworkOperation *op)
{
    //TODO
}

bool Connection::handleGetSelfInfoRequest(BNetworkOperation *op)
{
    TRequest request = op->variantData().value<TRequest>();
    QString error;
    if (!commonCheck(request.locale(), &error, TAccessLevel::UserLevel))
        return sendReply(op, error);
    RequestIn<TGetSelfInfoRequestData> in(request);
    return sendReply(op, UserServ->getSelfInfo(in, muserInfo.id()).createReply());
}

bool Connection::handleGetServerStateRequest(BNetworkOperation *op)
{
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

bool Connection::handleGetUserAvatarRequest(BNetworkOperation *op)
{
    TRequest request = op->variantData().value<TRequest>();
    QString error;
    if (!commonCheck(request.locale(), &error, TAccessLevel::UserLevel))
        return sendReply(op, error);
    RequestIn<TGetUserAvatarRequestData> in(request);
    return sendReply(op, UserServ->getUserAvatar(in).createReply());
}

bool Connection::handleGetUserConnectionInfoListRequest(BNetworkOperation *op)
{
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
    TRequest request = op->variantData().value<TRequest>();
    QString error;
    if (!commonCheck(request.locale(), &error, TAccessLevel::AdminLevel))
        return sendReply(op, error);
    RequestIn<TGetUserInfoAdminRequestData> in(request);
    return sendReply(op, UserServ->getUserInfoAdmin(in).createReply());
}

bool Connection::handleGetUserInfoListAdminRequest(BNetworkOperation *op)
{
    TRequest request = op->variantData().value<TRequest>();
    QString error;
    if (!commonCheck(request.locale(), &error, TAccessLevel::AdminLevel))
        return sendReply(op, error);
    RequestIn<TGetUserInfoListAdminRequestData> in(request);
    return sendReply(op, UserServ->getUserInfoListAdmin(in).createReply());
}

bool Connection::handleGetUserInfoRequest(BNetworkOperation *op)
{
    TRequest request = op->variantData().value<TRequest>();
    QString error;
    if (!commonCheck(request.locale(), &error, TAccessLevel::UserLevel))
        return sendReply(op, error);
    RequestIn<TGetUserInfoRequestData> in(request);
    return sendReply(op, UserServ->getUserInfo(in).createReply());
}

bool Connection::handleNoopRequest(BNetworkOperation *op)
{
    op->reply();
}

bool Connection::handleRecoverAccountRequest(BNetworkOperation *op)
{
    TRequest request = op->variantData().value<TRequest>();
    QString error;
    if (!commonCheck(request.locale(), &error))
        return sendReply(op, error);
    RequestIn<TRecoverAccountRequestData> in(request);
    return sendReply(op, UserServ->recoverAccount(in).createReply());
}

bool Connection::handleRegisterRequest(BNetworkOperation *op)
{
    TRequest request = op->variantData().value<TRequest>();
    QString error;
    if (!commonCheck(request.locale(), &error))
        return sendReply(op, error);
    RequestIn<TRegisterRequestData> in(request);
    return sendReply(op, UserServ->registerUser(in).createReply());
}

bool Connection::handleRequestRecoveryCodeRequest(BNetworkOperation *op)
{
    TRequest request = op->variantData().value<TRequest>();
    QString error;
    if (!commonCheck(request.locale(), &error))
        return sendReply(op, error);
    RequestIn<TRequestRecoveryCodeRequestData> in(request);
    return sendReply(op, UserServ->requestRecoveryCode(in).createReply());
}

bool Connection::handleSetLatestAppVersionRequest(BNetworkOperation *op)
{
    //TODO
}

bool Connection::handleSetServerStateRequest(BNetworkOperation *op)
{
    TRequest request = op->variantData().value<TRequest>();
    Translator t(request.locale());
    QString error;
    if (!commonCheck(t, &error, TAccessLevel::AdminLevel))
        return sendReply(op, error);
    TSetServerStateRequestData requestData = request.data().value<TSetServerStateRequestData>();
    if (requestData.listening()) {
        if (server()->isListening())
            return sendReply(op, t.translate("Connection", "Server is already listening", "error"), false);
        if (!server()->listen(requestData.address(), Texsample::MainPort))
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

/*bool Connection::handleGetLatestAppVersionRequest(BNetworkOperation *op)
{
    TClientInfo ci = op->variantData().toMap().value("client_info").value<TClientInfo>();
    QString name = ci.client().toLower().replace(QRegExp("\\s"), "-");
    QString ls = "Get latest app version request: " + name + " v" + ci.clientVersion().toString();
    if (ci.isClientPortable())
        ls += " (portable)";
    ls += "@" + ci.os();
    log(ls);
    QVariantMap out;
    QString s = "AppVersion/" + name + "/" + ci.os().left(3).toLower() + "/" + (ci.isClientPortable() ?
                                                                                    "portable" : "normal") + "/";
    out.insert("version", BCoreApplication::settingsInstance()->value(s + "/version"));
    out.insert("url", BCoreApplication::settingsInstance()->value(s + "/url"));
    sendReply(op, out);
    return true;
}

bool Connection::handleSetLatestAppVersionRequest(BNetworkOperation *op)
{
    QVariantMap in = op->variantData().toMap();
    QStringList args = in.value("arguments").toStringList();
    logLocal("Set latest app version request" + (args.size() ? (": " + args.join(" ")) : ""));
    if (!muserId)
        return sendReply(op, TMessage::NotAuthorizedError, LocalOnly);
    if (maccessLevel < TAccessLevel::SuperuserLevel)
        return sendReply(op, TMessage::NotEnoughRightsError, LocalOnly);
    return sendReply(op, TerminalIOHandler::instance()->setAppVersion(args), LocalOnly);
}

bool Connection::handleAddSampleRequest(BNetworkOperation *op)
{
    QVariantMap in = op->variantData().toMap();
    TTexProject project = in.value("project").value<TTexProject>();
    TSampleInfo info = in.value("sample_info").value<TSampleInfo>();
    log("Add sample request: " + info.title());
    if (!muserId)
        return sendReply(op, TMessage::NotAuthorizedError);
    if (maccessLevel < TAccessLevel::UserLevel)
        return sendReply(op, TMessage::NotEnoughRightsError);
    if (!mservices.contains(TService::TexsampleService))
        return sendReply(op, TMessage::NotEnoughRightsError);
    return sendReply(op, mstorage->addSample(muserId, project, info));
}

bool Connection::handleEditSampleRequest(BNetworkOperation *op)
{
    QVariantMap in = op->variantData().toMap();
    TSampleInfo info = in.value("sample_info").value<TSampleInfo>();
    TTexProject project = in.value("project").value<TTexProject>();
    log("Edit sample request: " + info.idString());
    if (!muserId)
        return sendReply(op, TMessage::NotAuthorizedError);
    if (maccessLevel < TAccessLevel::ModeratorLevel)
        return sendReply(op, TMessage::NotEnoughRightsError);
    if (!mservices.contains(TService::TexsampleService))
        return sendReply(op, TMessage::NotEnoughRightsError);
    return sendReply(op, mstorage->editSample(info, project));
}

bool Connection::handleUpdateSampleRequest(BNetworkOperation *op)
{
    QVariantMap in = op->variantData().toMap();
    TSampleInfo info = in.value("sample_info").value<TSampleInfo>();
    TTexProject project = in.value("project").value<TTexProject>();
    log("Update sample request: " + info.idString());
    if (!muserId)
        return sendReply(op, TMessage::NotAuthorizedError);
    if (maccessLevel < TAccessLevel::UserLevel)
        return sendReply(op, TMessage::NotEnoughRightsError);
    quint64 sId = mstorage->sampleSenderId(info.id());
    if (sId != muserId)
        return sendReply(op, TMessage::NotOwnSampleError);
    if (mstorage->sampleType(info.type()) == TSampleInfo::Approved)
        return sendReply(op, TMessage::NotModifiableSampleError);
    if (!mservices.contains(TService::TexsampleService))
        return sendReply(op, TMessage::NotEnoughRightsError);
    return sendReply(op, mstorage->editSample(info, project));
}

bool Connection::handleDeleteSampleRequest(BNetworkOperation *op)
{
    QVariantMap in = op->variantData().toMap();
    quint64 id = in.value("sample_id").toULongLong();
    QString reason = in.value("reason").toString();
    log("Delete sample request: " + QString::number(id) + (!reason.isEmpty() ? (" (" + reason + ")") : QString()));
    if (!muserId)
        return sendReply(op, TMessage::NotAuthorizedError);
    if (maccessLevel < TAccessLevel::UserLevel)
        return sendReply(op, TMessage::NotEnoughRightsError);
    quint64 sId = mstorage->sampleSenderId(id);
    if (maccessLevel < TAccessLevel::AdminLevel)
    {
        if (sId != muserId)
            return sendReply(op, TMessage::NotOwnSampleError);
        else if (mstorage->sampleType(id) == TSampleInfo::Approved)
            return sendReply(op, TMessage::NotModifiableSampleError);
    }
    if (!mservices.contains(TService::TexsampleService))
        return sendReply(op, TMessage::NotEnoughRightsError);
    return sendReply(op, mstorage->deleteSample(id, reason));
}

bool Connection::handleGetSamplesListRequest(BNetworkOperation *op)
{
    QVariantMap in = op->variantData().toMap();
    QDateTime updateDT = in.value("update_dt").toDateTime().toUTC();
    log("Get samples list request");
    if (!muserId)
        return sendReply(op, TMessage::NotAuthorizedError);
    if (maccessLevel < TAccessLevel::UserLevel)
        return sendReply(op, TMessage::NotEnoughRightsError);
    if (!mservices.contains(TService::TexsampleService))
        return sendReply(op, TMessage::NotEnoughRightsError);
    TSampleInfoList newSamples;
    TIdList deletedSamples;
    TOperationResult r = mstorage->getSamplesList(newSamples, deletedSamples, updateDT);
    if (!r)
        return sendReply(op, r);
    QVariantMap out;
    out.insert("update_dt", updateDT);
    if (!newSamples.isEmpty())
        out.insert("new_sample_infos", newSamples);
    if (!deletedSamples.isEmpty())
        out.insert("deleted_sample_infos", deletedSamples);
    return sendReply(op, out, r);
}

bool Connection::handleGetSampleSourceRequest(BNetworkOperation *op)
{
    QVariantMap in = op->variantData().toMap();
    quint64 id = in.value("sample_id").toULongLong();
    QDateTime updateDT = in.value("update_dt").toDateTime().toUTC();
    log("Get sample source request: " + QString::number(id));
    if (!muserId)
        return sendReply(op, TMessage::NotAuthorizedError);
    if (maccessLevel < TAccessLevel::UserLevel)
        return sendReply(op, TMessage::NotEnoughRightsError);
    if (!mservices.contains(TService::TexsampleService))
        return sendReply(op, TMessage::NotEnoughRightsError);
    TTexProject project;
    bool cacheOk = false;
    TOperationResult r = mstorage->getSampleSource(id, project, updateDT, cacheOk);
    if (!r)
        return sendReply(op, r);
    QVariantMap out;
    out.insert("update_dt", updateDT);
    if (cacheOk)
        out.insert("cache_ok", true);
    else
        out.insert("project", project);
    return sendReply(op, out, r);
}

bool Connection::handleGetSamplePreviewRequest(BNetworkOperation *op)
{
    QVariantMap in = op->variantData().toMap();
    quint64 id = in.value("sample_id").toULongLong();
    QDateTime updateDT = in.value("update_dt").toDateTime().toUTC();
    log("Get sample preview request: " + QString::number(id));
    if (!muserId)
        return sendReply(op, TMessage::NotAuthorizedError);
    if (maccessLevel < TAccessLevel::UserLevel)
        return sendReply(op, TMessage::NotEnoughRightsError);
    if (!mservices.contains(TService::TexsampleService))
        return sendReply(op, TMessage::NotEnoughRightsError);
    TProjectFile file;
    bool cacheOk = false;
    TOperationResult r = mstorage->getSamplePreview(id, file, updateDT, cacheOk);
    if (!r)
        return sendReply(op, r);
    QVariantMap out;
    out.insert("update_dt", updateDT);
    if (cacheOk)
        out.insert("cache_ok", true);
    else
        out.insert("project_file", file);
    return sendReply(op, out, r);
}

bool Connection::handleCompileProjectRequest(BNetworkOperation *op)
{
    log("Compile request");
    if (!muserId)
        return sendReply(op, TMessage::NotAuthorizedError);
    if (maccessLevel < TAccessLevel::UserLevel)
        return sendReply(op, TMessage::NotEnoughRightsError);
    if (!mservices.contains(TService::TexsampleService))
        return sendReply(op, TMessage::NotEnoughRightsError);
    Global::CompileParameters p;
    QVariantMap in = op->variantData().toMap();
    p.project = in.value("project").value<TTexProject>();
    p.param = in.value("parameters").value<TCompilerParameters>();
    p.path = QDir::tempPath() + "/texsample-server/compiler/" + BeQt::pureUuidText(uniqueId());
    p.compiledProject = new TCompiledProject;
    p.makeindexResult = p.param.makeindexEnabled() ? new TCompilationResult : 0;
    p.dvipsResult = p.param.dvipsEnabled() ? new TCompilationResult : 0;
    TCompilationResult r = Global::compileProject(p);
    QVariantMap out;
    if (p.makeindexResult)
        out.insert("makeindex_result", *p.makeindexResult);
    if (p.dvipsResult)
        out.insert("dvips_result", *p.dvipsResult);
    if (r)
        out.insert("compiled_project", *p.compiledProject);
    return sendReply(op, out, r);
}

bool Connection::handleAddLabRequest(BNetworkOperation *op)
{
    QVariantMap in = op->variantData().toMap();
    TLabInfo info = in.value("lab_info").value<TLabInfo>();
    TLabProject webProject = in.value("web_project").value<TLabProject>();
    TLabProject linuxProject = in.value("linux_project").value<TLabProject>();
    TLabProject macProject = in.value("mac_project").value<TLabProject>();
    TLabProject winProject = in.value("win_project").value<TLabProject>();
    TProjectFileList extraFiles = in.value("extra_files").value<TProjectFileList>();
    QString url = in.value("lab_url").toString();
    log("Add lab request: " + info.title());
    if (!muserId)
        return sendReply(op, TMessage::NotAuthorizedError);
    if (maccessLevel < TAccessLevel::ModeratorLevel)
        return sendReply(op, TMessage::NotEnoughRightsError);
    if (!mservices.contains(TService::ClabService))
        return sendReply(op, TMessage::NotEnoughRightsError);
    return sendReply(op, mstorage->addLab(muserId, info, webProject, linuxProject, macProject, winProject, url,
                                          extraFiles));
}

bool Connection::handleEditLabRequest(BNetworkOperation *op)
{
    QVariantMap in = op->variantData().toMap();
    TLabInfo info = in.value("lab_info").value<TLabInfo>();
    TLabProject webProject = in.value("web_project").value<TLabProject>();
    TLabProject linuxProject = in.value("linux_project").value<TLabProject>();
    TLabProject macProject = in.value("mac_project").value<TLabProject>();
    TLabProject winProject = in.value("win_project").value<TLabProject>();
    QStringList deletedExtraFiles = in.value("deleted_extra_files").toStringList();
    TProjectFileList newExtraFiles = in.value("new_extra_files").value<TProjectFileList>();
    QString url = in.value("lab_url").toString();
    log("Edit lab request: " + info.title());
    if (!muserId)
        return sendReply(op, TMessage::NotAuthorizedError);
    if (maccessLevel < TAccessLevel::ModeratorLevel)
        return sendReply(op, TMessage::NotEnoughRightsError);
    if (maccessLevel < TAccessLevel::ModeratorLevel && mstorage->labSenderId(info.id()) != muserId)
        return sendReply(op, TMessage::NotOwnLabError);
    if (!mservices.contains(TService::ClabService))
        return sendReply(op, TMessage::NotEnoughRightsError);
    return sendReply(op, mstorage->editLab(info, webProject, linuxProject, macProject, winProject, url,
                                           deletedExtraFiles, newExtraFiles));
}

bool Connection::handleDeleteLabRequest(BNetworkOperation *op)
{
    QVariantMap in = op->variantData().toMap();
    quint64 id = in.value("lab_id").toULongLong();
    QString reason = in.value("reason").toString();
    log("Delete lab request: " + QString::number(id) + (!reason.isEmpty() ? (" (" + reason + ")") : QString()));
    if (!muserId)
        return sendReply(op, TMessage::NotAuthorizedError);
    if (maccessLevel < TAccessLevel::ModeratorLevel)
        return sendReply(op, TMessage::NotEnoughRightsError);
    if (maccessLevel < TAccessLevel::AdminLevel && mstorage->labSenderId(id) != muserId)
        return sendReply(op, TMessage::NotOwnLabError);
    if (!mservices.contains(TService::ClabService))
        return sendReply(op, TMessage::NotEnoughRightsError);
    return sendReply(op, mstorage->deleteLab(id, reason));
}

bool Connection::handleGetLabsListRequest(BNetworkOperation *op)
{
    QVariantMap in = op->variantData().toMap();
    QDateTime updateDT = in.value("update_dt").toDateTime().toUTC();
    log("Get labs list request");
    if (!muserId)
        return sendReply(op, TMessage::NotAuthorizedError);
    if (maccessLevel < TAccessLevel::UserLevel)
        return sendReply(op, TMessage::NotEnoughRightsError);
    if (!mservices.contains(TService::ClabService))
        return sendReply(op, TMessage::NotEnoughRightsError);
    TLabInfoList newLabs;
    TIdList deletedLabs;
    TOperationResult r = mstorage->getLabsList(muserId, mclientInfo.osType(), newLabs, deletedLabs, updateDT);
    if (!r)
        return sendReply(op, r);
    QVariantMap out;
    out.insert("update_dt", updateDT);
    if (!newLabs.isEmpty())
        out.insert("new_lab_infos", newLabs);
    if (!deletedLabs.isEmpty())
        out.insert("deleted_lab_infos", deletedLabs);
    return sendReply(op, out, r);
}

bool Connection::handleGetLabRequest(BNetworkOperation *op)
{
    QVariantMap in = op->variantData().toMap();
    quint64 id = in.value("lab_id").toULongLong();
    log("Get lab request: " + QString::number(id));
    if (!muserId)
        return sendReply(op, TMessage::NotAuthorizedError);
    if (maccessLevel < TAccessLevel::UserLevel)
        return sendReply(op, TMessage::NotEnoughRightsError);
    if (!mservices.contains(TService::ClabService))
        return sendReply(op, TMessage::NotEnoughRightsError);
    TLabProject project;
    QString url;
    TLabInfo::Type t;
    TOperationResult r = mstorage->getLab(id, mclientInfo.osType(), project, t, url);
    if (!r)
        return sendReply(op, r);
    QVariantMap out;
    out.insert("type", (int) t);
    if (project.isValid())
        out.insert("project", project);
    else
        out.insert("url", url);
    return sendReply(op, out, r);
}

bool Connection::handleGetLabExtraAttachedFileRequest(BNetworkOperation *op)
{
    QVariantMap in = op->variantData().toMap();
    quint64 id = in.value("lab_id").toULongLong();
    QString fn = in.value("file_name").toString();
    log("Get lab extra file request: " + QString::number(id) + "/" + fn);
    if (!muserId)
        return sendReply(op, TMessage::NotAuthorizedError);
    if (maccessLevel < TAccessLevel::UserLevel)
        return sendReply(op, TMessage::NotEnoughRightsError);
    if (!mservices.contains(TService::ClabService))
        return sendReply(op, TMessage::NotEnoughRightsError);
    TProjectFile file;
    TOperationResult r = mstorage->getLabExtraAttachedFile(id, fn, file);
    if (!r)
        return sendReply(op, r);
    QVariantMap out;
    out.insert("file", file);
    return sendReply(op, out, r);
}*/

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
    installRequestHandler(TOperation::GetUserAvatar, InternalHandler(&Connection::handleGetUserAvatarRequest));
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
