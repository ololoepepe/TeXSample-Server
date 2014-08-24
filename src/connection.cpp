#include "connection.h"

#include "application.h"
#include "datasource.h"
#include "server.h"
#include "service/requestin.h"
#include "service/requestout.h"
#include "service/userservice.h"
#include "translator.h"

#include <TAccessLevel>
#include <TAddGroupReplyData>
#include <TAddGroupRequestData>
#include <TAddUserReplyData>
#include <TAddUserRequestData>
#include <TAuthorizeReplyData>
#include <TAuthorizeRequestData>
#include <TConfirmRegistrationReplyData>
#include <TConfirmRegistrationRequestData>
#include <TGenerateInvitesReplyData>
#include <TGenerateInvitesRequestData>
#include <TGetInviteInfoListReplyData>
#include <TGetInviteInfoListRequestData>
#include <TGetSelfInfoReplyData>
#include <TGetSelfInfoRequestData>
#include <TGetUserInfoAdminReplyData>
#include <TGetUserInfoAdminRequestData>
#include <TGetUserInfoListAdminReplyData>
#include <TGetUserInfoListAdminRequestData>
#include <TGetUserInfoReplyData>
#include <TGetUserInfoRequestData>
#include <TGroupInfo>
#include <TGroupInfoList>
#include <TIdList>
#include <TLogRequestData>
#include <TOperation>
#include <TRegisterReplyData>
#include <TRegisterRequestData>
#include <TReply>
#include <TRequest>
#include <TServerState>
#include <TService>
#include <TServiceList>
#include <TUserConnectionInfo>
#include <TUserConnectionInfoList>
#include <TUserInfo>

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
    //
}

bool Connection::handleAddSampleRequest(BNetworkOperation *op)
{
    //
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
    //
}

bool Connection::handleChangePasswordRequest(BNetworkOperation *op)
{
    //
}

bool Connection::handleCheckEmailAvailabilityRequest(BNetworkOperation *op)
{
    //
}

bool Connection::handleCheckLoginAvailabilityRequest(BNetworkOperation *op)
{
    //
}

bool Connection::handleCompileTexProjectRequest(BNetworkOperation *op)
{
    //
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
    //
}

bool Connection::handleDeleteInvitesRequest(BNetworkOperation *op)
{
    //
}

bool Connection::handleDeleteLabRequest(BNetworkOperation *op)
{
    //
}

bool Connection::handleDeleteSampleRequest(BNetworkOperation *op)
{
    //
}

bool Connection::handleDeleteUserRequest(BNetworkOperation *op)
{
    //
}

bool Connection::handleEditGroupRequest(BNetworkOperation *op)
{
    //
}

bool Connection::handleEditLabRequest(BNetworkOperation *op)
{
    //
}

bool Connection::handleEditSampleRequest(BNetworkOperation *op)
{
    //
}

bool Connection::handleEditSampleAdminRequest(BNetworkOperation *op)
{
    //
}

bool Connection::handleEditSelfRequest(BNetworkOperation *op)
{
    //
}

bool Connection::handleEditUserRequest(BNetworkOperation *op)
{
    //
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
    if (!commonCheck(request.locale(), &error, TAccessLevel::AdminLevel))
        return sendReply(op, error);
    return sendReply(op, "dummy"); //TODO
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
    //
}

bool Connection::handleGetLabExtraFileRequest(BNetworkOperation *op)
{
    //
}

bool Connection::handleGetLabInfoListRequest(BNetworkOperation *op)
{
    //
}

bool Connection::handleGetLatestAppVersionRequest(BNetworkOperation *op)
{
    //
}

bool Connection::handleGetSampleInfoListRequest(BNetworkOperation *op)
{
    //
}

bool Connection::handleGetSamplePreviewRequest(BNetworkOperation *op)
{
    //
}

bool Connection::handleGetSampleSourceRequest(BNetworkOperation *op)
{
    //
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
    //
}

bool Connection::handleGetUserAvatarRequest(BNetworkOperation *op)
{
    //
}

bool Connection::handleGetUserConnectionInfoListRequest(BNetworkOperation *op)
{
    //
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
    //
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
    //
}

bool Connection::handleSetLatestAppVersionRequest(BNetworkOperation *op)
{
    //
}

bool Connection::handleSetServerStateRequest(BNetworkOperation *op)
{
    //
}

bool Connection::handleSubscribeRequest(BNetworkOperation *op)
{
    //
}

/*bool Connection::handleCheckEmailRequest(BNetworkOperation *op)
{
    QVariantMap in = op->variantData().toMap();
    QString email = in.value("email").toString();
    log("<" + email + "> Check e-mail request");
    bool free = !mstorage->userIdByEmail(email);
    QVariantMap out;
    out.insert("free", free);
    bool b = sendReply(op, out);
    op->waitForFinished();
    close();
    return b;
}

bool Connection::handleCheckLoginRequest(BNetworkOperation *op)
{
    QVariantMap in = op->variantData().toMap();
    QString login = in.value("login").toString();
    log("<" + login + "> Check login request");
    bool free = !mstorage->userId(login);
    QVariantMap out;
    out.insert("free", free);
    bool b = sendReply(op, out);
    op->waitForFinished();
    close();
    return b;
}

bool Connection::handleRegisterRequest(BNetworkOperation *op)
{
    QVariantMap in = op->variantData().toMap();
    TUserInfo info = in.value("user_info").value<TUserInfo>();
    QLocale l = in.value("locale").toLocale();
    QString prefix = "<" + info.login() + "> ";
    log(prefix + "Register request");
    //bool b = sendReply(op, mstorage->registerUser(info, l), LocalAndRemote, prefix);
    op->waitForFinished();
    close();
    //return b;
}

bool Connection::handleGetRecoveryCodeRequest(BNetworkOperation *op)
{
    QVariantMap in = op->variantData().toMap();
    QString email = in.value("email").toString();
    QLocale l = in.value("locale").toLocale();
    QString prefix = "<" + email + "> ";
    log(prefix + "Get recovery code request");
    //bool b = sendReply(op, mstorage->getRecoveryCode(email, l), LocalAndRemote, prefix);
    op->waitForFinished();
    close();
    //return b;
}

bool Connection::handleRecoverAccountRequest(BNetworkOperation *op)
{
    QVariantMap in = op->variantData().toMap();
    QString email = in.value("email").toString();
    QString code = in.value("recovery_code").toString();
    QByteArray password = in.value("password").toByteArray();
    QLocale l = in.value("locale").toLocale();
    QString prefix = "<" + email + "> ";
    log(prefix + "Recover account request");
    //bool b = sendReply(op, mstorage->recoverAccount(email, code, password, l), LocalAndRemote, prefix);
    op->waitForFinished();
    close();
    //return b;
}

bool Connection::handleGetLatestAppVersionRequest(BNetworkOperation *op)
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

bool Connection::handleAuthorizeRequest(BNetworkOperation *op)
{
    QVariantMap in = op->variantData().toMap();
    QString login = in.value("login").toString();
    QByteArray password = in.value("password").toByteArray();
    TClientInfo info = in.value("client_info").value<TClientInfo>();
    QString prefix = "<" + login + "> ";
    log(prefix + "Authorize request");
    if (muserId)
        return sendReply(op, TMessage::AlreadyAuthorizedMessage, true, LocalAndRemote, prefix);
    if (login.isEmpty())
        return sendReply(op, TMessage::InvalidLoginError, LocalAndRemote, prefix);
    if (password.isEmpty())
        return sendReply(op, TMessage::InvalidPasswordError, LocalAndRemote, prefix);
    quint64 id = mstorage->userId(login, password);
    if (!id)
        return sendReply(op, TMessage::NoSuchUserError, LocalAndRemote, prefix);
    if (!info.isValid())
        log(prefix + "Warning: invalid client info");
    mlogin = login;
    mclientInfo = info;
    mlocale = mclientInfo.locale();
    muserId = id;
    maccessLevel = mstorage->userAccessLevel(id);
    mservices = mstorage->userServices(muserId);
    setCriticalBufferSize(200 * BeQt::Megabyte);
    QString f = "Client info:\nUser ID: %d\nUnique ID: %i\nAccess level: %a\nOS: %o\nLocale: ";
    f += "%l\nClient: %c\nClient version: %v\nTeXSample version: %t\nBeQt version: %b\nQt version: %q";
    if (info.isValid())
        log(infoString(f) + "\nServices: " + mservices.toStringNoTr());
    QVariantMap out;
    out.insert("user_id", id);
    out.insert("access_level", maccessLevel);
    out.insert("services", mservices);
    restartTimer();
    bool b = sendReply(op, out);
    if (maccessLevel >= TAccessLevel::AdminLevel)
        msubscribed = in.value("subscription").toBool();
    return b;
}

bool Connection::handleAddUserRequest(BNetworkOperation *op)
{
    QVariantMap in = op->variantData().toMap();
    TUserInfo info = in.value("user_info").value<TUserInfo>();
    QStringList clabGroups = in.value("clab_groups").toStringList();
    log("Add user request: " + info.login());
    if (!muserId)
        return sendReply(op, TMessage::NotAuthorizedError);
    if (maccessLevel < TAccessLevel::AdminLevel)
        return sendReply(op, TMessage::NotEnoughRightsError);
    if (maccessLevel < TAccessLevel::SuperuserLevel && info.accessLevel() >= TAccessLevel::AdminLevel)
        return sendReply(op, TMessage::NotEnoughRightsError);
    foreach (const TService &s, info.services())
        if (!mservices.contains(s))
            return sendReply(op, TMessage::NotEnoughRightsError);
    if (!clabGroups.isEmpty() && !mservices.contains(TService::ClabService))
        return sendReply(op, TMessage::NotEnoughRightsError);
    return sendReply(op, mstorage->addUser(info, mlocale, clabGroups));
}

bool Connection::handleEditUserRequest(BNetworkOperation *op)
{
    QVariantMap in = op->variantData().toMap();
    TUserInfo info = in.value("user_info").value<TUserInfo>();
    bool editClab = in.value("edit_clab").toBool();
    QStringList clabGroups = in.value("clab_groups").toStringList();
    log("Edit user request: " + (info.id() ? info.idString() : info.login()));
    if (!muserId)
        return sendReply(op, TMessage::NotAuthorizedError);
    if (info.id() == muserId)
        return sendReply(op, TMessage::CantEditSelfError);
    if (maccessLevel < TAccessLevel::AdminLevel)
        return sendReply(op, TMessage::NotEnoughRightsError);
    TAccessLevel ualvl = mstorage->userAccessLevel(info.id());
    if (ualvl >= TAccessLevel::SuperuserLevel)
        return sendReply(op, TMessage::NotEnoughRightsError);
    if (maccessLevel < TAccessLevel::SuperuserLevel && ualvl >= TAccessLevel::AdminLevel)
        return sendReply(op, TMessage::NotEnoughRightsError);
    //One can't add a service to which he has no acces to a user
    foreach (const TService &s, info.services())
        if (!mservices.contains(s))
            return sendReply(op, TMessage::NotEnoughRightsError);
    //One can't remove a service to which he has no acces form a user
    foreach (const TService &s, mstorage->userServices(info.id()))
        if (!mservices.contains(s) && !info.services().contains(s))
            return sendReply(op, TMessage::NotEnoughRightsError);
    if (editClab && !mservices.contains(TService::ClabService))
        return sendReply(op, TMessage::NotEnoughRightsError);
    return sendReply(op, mstorage->editUser(info, editClab, clabGroups));
}

bool Connection::handleUpdateAccountRequest(BNetworkOperation *op)
{
    QVariantMap in = op->variantData().toMap();
    TUserInfo info = in.value("user_info").value<TUserInfo>();
    log("Update account request");
    if (!muserId)
        return sendReply(op, TMessage::NotAuthorizedError);
    if (info.id() != muserId)
        return sendReply(op, TMessage::NotOwnAccountError);
    if (maccessLevel < TAccessLevel::UserLevel)
        return sendReply(op, TMessage::NotEnoughRightsError);
    return sendReply(op, mstorage->updateUser(info));
}

bool Connection::handleGetUserInfoRequest(BNetworkOperation *op)
{
    QVariantMap in = op->variantData().toMap();
    quint64 id = in.value("user_id").toULongLong();
    QString login = in.value("user_login").toString();
    QDateTime updateDT = in.value("update_dt").toDateTime().toUTC();
    bool clabGroups = in.value("clab_groups").toBool();
    log("Get user info request: " + QString::number(id));
    if (!muserId)
        return sendReply(op, TMessage::NotAuthorizedError);
    if (maccessLevel < TAccessLevel::UserLevel)
        return sendReply(op, TMessage::NotEnoughRightsError);
    if (clabGroups && !mservices.contains(TService::ClabService))
        return sendReply(op, TMessage::NotEnoughRightsError);
    if (!id)
        id = mstorage->userId(login);
    TUserInfo info;
    bool cacheOk = false;
    TOperationResult r = mstorage->getUserInfo(id, info, updateDT, cacheOk);
    if (!r)
        return sendReply(op, r);
    QVariantMap out;
    out.insert("update_dt", updateDT);
    if (cacheOk)
    {
        out.insert("cache_ok", true);
    }
    else
    {
        out.insert("user_info", info);
        if (clabGroups)
            out.insert("clab_groups", mstorage->userClabGroups(id));
    }
    return sendReply(op, out, r);
}

bool Connection::handleGenerateInvitesRequest(BNetworkOperation *op)
{
    QVariantMap in = op->variantData().toMap();
    QDateTime expiresDT = in.value("expiration_dt").toDateTime().toUTC();
    quint8 count = in.value("count").toUInt();
    TServiceList services = in.value("services").value<TServiceList>();
    QStringList clabGroups = in.value("clab_groups").toStringList();
    log("Generate invites request (" + QString::number(count) + ")");
    if (!muserId)
        return sendReply(op, TMessage::NotAuthorizedError);
    if (maccessLevel < TAccessLevel::ModeratorLevel)
        return sendReply(op, TMessage::NotEnoughRightsError);
    foreach (const TService &s, services)
        if (!mservices.contains(s))
            return sendReply(op, TMessage::NotEnoughRightsError);
    if (!clabGroups.isEmpty() && !mservices.contains(TService::ClabService))
        return sendReply(op, TMessage::NotEnoughRightsError);
    TInviteInfoList invites;
    TOperationResult r = mstorage->generateInvites(muserId, expiresDT, count, services, clabGroups, invites);
    if (!r)
        return sendReply(op, r);
    QVariantMap out;
    out.insert("invite_infos", invites);
    return sendReply(op, out, r);
}

bool Connection::handleGetInvitesListRequest(BNetworkOperation *op)
{
    log("Get invites list request");
    if (!muserId)
        return sendReply(op, TMessage::NotAuthorizedError);
    if (maccessLevel < TAccessLevel::ModeratorLevel)
        return sendReply(op, TMessage::NotEnoughRightsError);
    if (!mservices.contains(TService::TexsampleService))
        return sendReply(op, TMessage::NotEnoughRightsError);
    TInviteInfoList invites;
    TOperationResult r = mstorage->getInvitesList(muserId, invites);
    if (!r)
        return sendReply(op, r);
    QVariantMap out;
    out.insert("invite_infos", invites);
    return sendReply(op, out, r);
}

bool Connection::handleSubscribeRequest(BNetworkOperation *op)
{
    bool b = op->variantData().toMap().value("subscription").toBool();
    log("Subscribe resuest: " + QString(msubscribed ? "true" : "false"));
    if (!muserId)
        return sendReply(op, TMessage::NotAuthorizedError);
    if (maccessLevel < TAccessLevel::AdminLevel)
        return sendReply(op, TMessage::NotEnoughRightsError);
    msubscribed = b;
    return sendReply(op);
}

bool Connection::handleChangeLocale(BNetworkOperation *op)
{
    QVariantMap in = op->variantData().toMap();
    QLocale l = in.value("locale").toLocale();
    logLocal("Change locale request: " + l.name());
    if (!muserId)
        return sendReply(op, TMessage::NotAuthorizedError, LocalOnly);
    if (maccessLevel < TAccessLevel::UserLevel)
        return sendReply(op, TMessage::NotEnoughRightsError, LocalOnly);
    mlocale = l;
    return sendReply(op, TOperationResult(true), LocalOnly);
}

bool Connection::handleStartServerRequest(BNetworkOperation *op)
{
    QVariantMap in = op->variantData().toMap();
    QString addr = in.value("address").toString();
    logLocal("Start server request: [" + addr + "]");
    if (!muserId)
        return sendReply(op, TMessage::NotAuthorizedError, LocalOnly);
    if (maccessLevel < TAccessLevel::AdminLevel)
        return sendReply(op, TMessage::NotEnoughRightsError, LocalOnly);
    return sendReply(op, TerminalIOHandler::instance()->startServer(addr), LocalOnly);
}

bool Connection::handleStopServerRequest(BNetworkOperation *op)
{
    logLocal("Stop server request");
    if (!muserId)
        return sendReply(op, TMessage::NotAuthorizedError, LocalOnly);
    if (maccessLevel < TAccessLevel::AdminLevel)
        return sendReply(op, TMessage::NotEnoughRightsError, LocalOnly);
    return sendReply(op, TerminalIOHandler::instance()->stopServer(), LocalOnly);
}

bool Connection::handleUptimeRequest(BNetworkOperation *op)
{
    logLocal("Uptime request");
    if (!muserId)
        return sendReply(op, TMessage::NotAuthorizedError, LocalOnly);
    if (maccessLevel < TAccessLevel::ModeratorLevel)
        return sendReply(op, TMessage::NotEnoughRightsError, LocalOnly);
    QVariantMap out;
    out.insert("msecs", TerminalIOHandler::instance()->uptime());
    return sendReply(op, out, LocalOnly);
}

bool Connection::handleUserRequest(BNetworkOperation *op)
{
    QVariantMap in = op->variantData().toMap();
    QStringList args = in.value("arguments").toStringList();
    logLocal("User request" + (args.size() ? (": " + args.join(" ")) : ""));
    if (!muserId)
        return sendReply(op, TMessage::NotAuthorizedError, LocalOnly);
    if (maccessLevel < TAccessLevel::AdminLevel)
        return sendReply(op, TMessage::NotEnoughRightsError, LocalOnly);
    QVariant result;
    TOperationResult r = TerminalIOHandler::instance()->user(args, result);
    QVariantMap out;
    if (r)
        out.insert("result", result);
    return sendReply(op, out, r, LocalOnly);
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

bool Connection::handleEditClabGroupsRequest(BNetworkOperation *op)
{
    QVariantMap in = op->variantData().toMap();
    QStringList newGroups = in.value("new_groups").toStringList();
    QStringList deletedGroups = in.value("deleted_groups").toStringList();
    log("Edit CloudLab groups list request: " + QString::number(newGroups.size()) + " new, "
        + QString::number(deletedGroups.size()) + " deleted");
    if (!muserId)
        return sendReply(op, TMessage::NotAuthorizedError);
    if (maccessLevel < TAccessLevel::AdminLevel)
        return sendReply(op, TMessage::NotEnoughRightsError);
    if (!mservices.contains(TService::ClabService))
        return sendReply(op, TMessage::NotEnoughRightsError);
    return sendReply(op, mstorage->editClabGroups(newGroups, deletedGroups));
}

bool Connection::handleGetClabGroupsListRequest(BNetworkOperation *op)
{
    log("Get CloudLab groups list request");
    if (!muserId)
        return sendReply(op, TMessage::NotAuthorizedError);
    if (maccessLevel < TAccessLevel::ModeratorLevel)
        return sendReply(op, TMessage::NotEnoughRightsError);
    if (!mservices.contains(TService::ClabService))
        return sendReply(op, TMessage::NotEnoughRightsError);
    QStringList groups;
    TOperationResult r = mstorage->getClabGroupsList(groups);
    if (!r)
        return sendReply(op, r);
    QVariantMap out;
    out.insert("groups_list", groups);
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
}

bool Connection::sendReply(BNetworkOperation *op, QVariantMap out, const TOperationResult &r, LogTarget lt,
                           const QString &prefix)
{
    out.insert("operation_result", r);
    op->reply(out);
    QString s = prefix + ((r.message() == TMessage::NoMessage) ? QString("Success!") : r.messageStringNoTr());
    switch (lt)
    {
    case LocalAndRemote:
        log(s);
        break;
    case LocalOnly:
        logLocal(s);
        break;
    case RemoteOnly:
        logRemote(s);
        break;
    case NoTarget:
    default:
        break;
    }
    return r;
}

bool Connection::sendReply(BNetworkOperation *op, QVariantMap out, const TCompilationResult &r, LogTarget lt,
                           const QString &prefix)
{
    out.insert("compilation_result", r);
    op->reply(out);
    QString s = prefix + ((r.message() == TMessage::NoMessage) ? QString("Success!") : r.messageStringNoTr());
    switch (lt)
    {
    case LocalAndRemote:
        log(s);
        break;
    case LocalOnly:
        logLocal(s);
        break;
    case RemoteOnly:
        logRemote(s);
        break;
    case NoTarget:
    default:
        break;
    }
    return r;
}

bool Connection::sendReply(BNetworkOperation *op, QVariantMap out, int msg, LogTarget lt, const QString &prefix)
{
    return sendReply(op, out, TOperationResult(msg), lt, prefix);
}

bool Connection::sendReply(BNetworkOperation *op, const TOperationResult &r, LogTarget lt, const QString &prefix)
{
    return sendReply(op, QVariantMap(), r, lt, prefix);
}

bool Connection::sendReply(BNetworkOperation *op, const TCompilationResult &r, LogTarget lt, const QString &prefix)
{
    return sendReply(op, QVariantMap(), r, lt, prefix);
}

bool Connection::sendReply(BNetworkOperation *op, int msg, LogTarget lt, const QString &prefix)
{
    return sendReply(op, TOperationResult(msg), lt, prefix);
}

bool Connection::sendReply(BNetworkOperation *op, int msg, bool success, LogTarget lt, const QString &prefix)
{
    return sendReply(op, TOperationResult(success, msg), lt, prefix);
}

bool Connection::sendReply(BNetworkOperation *op, QVariantMap out, LogTarget lt, const QString &prefix)
{
    return sendReply(op, out, TOperationResult(true), lt, prefix);
}

bool Connection::sendReply(BNetworkOperation *op, LogTarget lt, const QString &prefix)
{
    return sendReply(op, QVariantMap(), TOperationResult(true), lt, prefix);
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
