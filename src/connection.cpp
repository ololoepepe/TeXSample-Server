#include "connection.h"
#include "global.h"
#include "storage.h"
#include "terminaliohandler.h"
#include "server.h"

#include <TAccessLevel>
#include <TUserInfo>
#include <TSampleInfo>
#include <TeXSample>
#include <TOperationResult>
#include <TTexProject>
#include <TCompilerParameters>
#include <TCompiledProject>
#include <TCompilationResult>
#include <TInviteInfo>
#include <TMessage>
#include <TLabProject>
#include <TLabInfo>
#include <TLabInfoList>

#include <BNetworkConnection>
#include <BNetworkServer>
#include <BGenericSocket>
#include <BNetworkOperation>
#include <BDirTools>
#include <BTranslator>

#include <QObject>
#include <QByteArray>
#include <QString>
#include <QTimer>
#include <QUuid>
#include <QVariant>
#include <QVariantMap>
#include <QVariantList>
#include <QDateTime>
#include <QStringList>
#include <QRegExp>
#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QTcpSocket>
#include <QMetaObject>
#include <QSettings>
#include <QThread>

#include <QDebug>

/*============================================================================
================================ Connection ==================================
============================================================================*/

/*============================== Public constructors =======================*/

Connection::Connection(BNetworkServer *server, BGenericSocket *socket) :
    BNetworkConnection(server, socket)
{
    mstorage = new Storage;
    muserId = 0;
    msubscribed = false;
    if (!mstorage->isValid())
    {
        QString msg = tr("Invalid storage instance");
        logLocal(msg);
        logRemote(msg);
        close();
        return;
    }
    setCriticalBufferSize(2 * BeQt::Megabyte);
    setCloseOnCriticalBufferSize(true);
    mtimer.setInterval(5 * BeQt::Minute);
    connect(&mtimer, SIGNAL(timeout()), this, SLOT(keepAlive()));
    connect(this, SIGNAL(requestSent(BNetworkOperation *)), this, SLOT(restartTimer(BNetworkOperation *)));
    connect(this, SIGNAL(replyReceived(BNetworkOperation *)), this, SLOT(restartTimer(BNetworkOperation *)));
    connect(this, SIGNAL(incomingRequest(BNetworkOperation *)), this, SLOT(restartTimer(BNetworkOperation *)));
    connect(this, SIGNAL(requestReceived(BNetworkOperation *)), this, SLOT(restartTimer(BNetworkOperation *)));
    connect(this, SIGNAL(replySent(BNetworkOperation *)), this, SLOT(restartTimer(BNetworkOperation *)));
    installRequestHandler(Texsample::RegisterRequest, (InternalHandler) &Connection::handleRegisterRequest);
    installRequestHandler(Texsample::GetRecoveryCodeRequest,
                          (InternalHandler) &Connection::handleGetRecoveryCodeRequest);
    installRequestHandler(Texsample::RecoverAccountRequest,
                          (InternalHandler) &Connection::handleRecoverAccountRequest);
    installRequestHandler(Texsample::AuthorizeRequest, (InternalHandler) &Connection::handleAuthorizeRequest);
    installRequestHandler(Texsample::AddUserRequest, (InternalHandler) &Connection::handleAddUserRequest);
    installRequestHandler(Texsample::EditUserRequest, (InternalHandler) &Connection::handleEditUserRequest);
    installRequestHandler(Texsample::UpdateAccountRequest, (InternalHandler) &Connection::handleUpdateAccountRequest);
    installRequestHandler(Texsample::GetUserInfoRequest, (InternalHandler) &Connection::handleGetUserInfoRequest);
    installRequestHandler(Texsample::AddSampleRequest, (InternalHandler) &Connection::handleAddSampleRequest);
    installRequestHandler(Texsample::EditSampleRequest, (InternalHandler) &Connection::handleEditSampleRequest);
    installRequestHandler(Texsample::UpdateSampleRequest, (InternalHandler) &Connection::handleUpdateSampleRequest);
    installRequestHandler(Texsample::DeleteSampleRequest, (InternalHandler) &Connection::handleDeleteSampleRequest);
    installRequestHandler(Texsample::GetSamplesListRequest,
                          (InternalHandler) &Connection::handleGetSamplesListRequest);
    installRequestHandler(Texsample::GetSampleSourceRequest,
                          (InternalHandler) &Connection::handleGetSampleSourceRequest);
    installRequestHandler(Texsample::GetSamplePreviewRequest,
                          (InternalHandler) &Connection::handleGetSamplePreviewRequest);
    installRequestHandler(Texsample::GenerateInvitesRequest,
                          (InternalHandler) &Connection::handleGenerateInvitesRequest);
    installRequestHandler(Texsample::GetInvitesListRequest,
                          (InternalHandler) &Connection::handleGetInvitesListRequest);
    installRequestHandler(Texsample::CompileProjectRequest,
                          (InternalHandler) &Connection::handleCompileProjectRequest);
    installRequestHandler(Texsample::SubscribeRequest, (InternalHandler) &Connection::handleSubscribeRequest);
    installRequestHandler(Texsample::ChangeLocaleRequest, (InternalHandler) &Connection::handleChangeLocale);
    installRequestHandler(Texsample::StartServerRequest, (InternalHandler) &Connection::handleStartServerRequest);
    installRequestHandler(Texsample::StopServerRequest, (InternalHandler) &Connection::handleStopServerRequest);
    installRequestHandler(Texsample::UptimeRequest, (InternalHandler) &Connection::handleUptimeRequest);
    installRequestHandler(Texsample::UserRequest, (InternalHandler) &Connection::handleUserRequest);
    installRequestHandler(Texsample::EditClabGroupsRequest,
                          (InternalHandler) &Connection::handleEditClabGroupsRequest);
    installRequestHandler(Texsample::GetClabGroupsListRequest,
                          (InternalHandler) &Connection::handleGetClabGroupsListRequest);
    installRequestHandler(Texsample::AddLabRequest, (InternalHandler) &Connection::handleAddLabRequest);
    installRequestHandler(Texsample::EditLabRequest, (InternalHandler) &Connection::handleEditLabRequest);
    installRequestHandler(Texsample::DeleteLabRequest, (InternalHandler) &Connection::handleDeleteLabRequest);
    installRequestHandler(Texsample::GetLabRequest, (InternalHandler) &Connection::handleGetLabRequest);
    installRequestHandler(Texsample::GetLabsListRequest, (InternalHandler) &Connection::handleGetLabsListRequest);
    QTimer::singleShot(15 * BeQt::Second, this, SLOT(testAuthorization()));
    mconnectedAt = QDateTime::currentDateTimeUtc();
    muptimeTimer.start();
}

Connection::~Connection()
{
    delete mstorage;
}

/*============================== Public methods ============================*/

void Connection::sendLogRequest(const QString &text, BLogger::Level lvl)
{
    Qt::ConnectionType ct = (QThread::currentThread() == thread()) ? Qt::DirectConnection :
                                                                     Qt::BlockingQueuedConnection;
    QMetaObject::invokeMethod(this, "sendLogRequestInternal", ct, Q_ARG(QString, text), Q_ARG(int, lvl));
}

void Connection::sendMessageRequest(const TMessage &msg)
{
    Qt::ConnectionType ct = (QThread::currentThread() == thread()) ? Qt::DirectConnection :
                                                                     Qt::BlockingQueuedConnection;
    QMetaObject::invokeMethod(this, "sendMessageRequestInternal", ct, Q_ARG(int, msg));
}

QString Connection::login() const
{
    return muserId ? mlogin : QString();
}

TClientInfo Connection::clientInfo() const
{
    return mclientInfo;
}

quint64 Connection::userId() const
{
    return muserId;
}

QLocale Connection::locale() const
{
    return mlocale;
}

TAccessLevel Connection::accessLevel() const
{
    return maccessLevel;
}

TServiceList Connection::services() const
{
    return mservices;
}

QString Connection::infoString(const QString &format) const
{
    //%d - user id, "%u - login, %p - address, %i - id, %a - access level
    if (!muserId)
        return "";
    QString f = format;
    if (f.isEmpty())
        f = "[%u] [%p] [%i]\n%a; %o [%l]\nClient v%v; TeXSample v%t; BeQt v%b; Qt v%q";
    f.replace("%l", mlocale.name());
    QString s = mclientInfo.toString(f);
    s.replace("%d", QString::number(muserId));
    s.replace("%u", mlogin);
    s.replace("%p", peerAddress());
    s.replace("%i", BeQt::pureUuidText(uniqueId()));
    s.replace("%a", maccessLevel.toStringNoTr());
    return s;
}

QDateTime Connection::connectedAt(Qt::TimeSpec spec) const
{
    return mconnectedAt.toTimeSpec(spec);
}

bool Connection::isSubscribed() const
{
    return msubscribed;
}

qint64 Connection::uptime() const
{
    return muptimeTimer.elapsed();
}

/*============================== Purotected methods ========================*/

void Connection::log(const QString &text, BLogger::Level lvl)
{
    logLocal(text, lvl);
    logRemote(text, lvl);
}

void Connection::logLocal(const QString &text, BLogger::Level lvl)
{
    BNetworkConnection::log((muserId ? ("[" + mlogin + "] ") : QString()) + text, lvl);
}

void Connection::logRemote(const QString &text, BLogger::Level lvl)
{
    QString msg = (muserId ? ("[" + mlogin + "] ") : QString()) + text;
    Server::sendLogRequest("[" + peerAddress() + "] " + msg, lvl);
}

/*============================== Private methods ===========================*/

bool Connection::handleRegisterRequest(BNetworkOperation *op)
{
    QVariantMap in = op->variantData().toMap();
    TUserInfo info = in.value("user_info").value<TUserInfo>();
    QLocale l = in.value("locale").toLocale();
    QString prefix = "<" + info.login() + "> ";
    log(prefix + "Register request");
    bool b = sendReply(op, mstorage->registerUser(info, l), LocalAndRemote, prefix);
    op->waitForFinished();
    close();
    return b;
}

bool Connection::handleGetRecoveryCodeRequest(BNetworkOperation *op)
{
    QVariantMap in = op->variantData().toMap();
    QString email = in.value("email").toString();
    QLocale l = in.value("locale").toLocale();
    QString prefix = "<" + email + "> ";
    log(prefix + "Get recovery code request");
    bool b = sendReply(op, mstorage->getRecoveryCode(email, l), LocalAndRemote, prefix);
    op->waitForFinished();
    close();
    return b;
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
    bool b = sendReply(op, mstorage->recoverAccount(email, code, password, l), LocalAndRemote, prefix);
    op->waitForFinished();
    close();
    return b;
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
    log("Edit user request: " + info.idString());
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
    logLocal("User request: " + args.join(" "));
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
    log("Edit CLab groups list request: " + QString::number(newGroups.size()) + " new, "
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
    log("Get CLab groups list request");
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
    QString url = in.value("lab_url").toString();
    log("Add lab request: " + info.title());
    if (!muserId)
        return sendReply(op, TMessage::NotAuthorizedError);
    if (maccessLevel < TAccessLevel::ModeratorLevel)
        return sendReply(op, TMessage::NotEnoughRightsError);
    if (!mservices.contains(TService::ClabService))
        return sendReply(op, TMessage::NotEnoughRightsError);
    return sendReply(op, mstorage->addLab(muserId, info, webProject, linuxProject, macProject, winProject, url));
}

bool Connection::handleEditLabRequest(BNetworkOperation *op)
{
    QVariantMap in = op->variantData().toMap();
    TLabInfo info = in.value("lab_info").value<TLabInfo>();
    TLabProject webProject = in.value("web_project").value<TLabProject>();
    TLabProject linuxProject = in.value("linux_project").value<TLabProject>();
    TLabProject macProject = in.value("mac_project").value<TLabProject>();
    TLabProject winProject = in.value("win_project").value<TLabProject>();
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
    return sendReply(op, mstorage->editLab(info, webProject, linuxProject, macProject, winProject, url));
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
}

/*============================== Private slots =============================*/

void Connection::testAuthorization()
{
    if (muserId)
        return;
    log("Authorization failed, closing connection");
    close();
}

void Connection::restartTimer(BNetworkOperation *op)
{
    if (op && op->metaData().operation() == "noop")
        return;
    mtimer.stop();
    mtimer.start();
}

void Connection::keepAlive()
{
    if (!muserId || !isConnected())
        return;
    mtimer.stop();
    int l = bSettings->value("Log/noop").toInt();
    QString s = "Testing connection...";
    if (1 == l)
        logLocal(s);
    else if (l > 1)
        log(s);
    BNetworkOperation *op = sendRequest(operation(NoopOperation));
    bool b = op->waitForFinished(5 * BeQt::Minute);
    if (!b)
    {
        log("Connection response timeout");
        op->cancel();
    }
    op->deleteLater();
    if (b)
        mtimer.start();
}

void Connection::sendLogRequestInternal(const QString &text, int lvl)
{
    if (!muserId || !msubscribed || text.isEmpty() || !isConnected()
            || QAbstractSocket::UnknownSocketError != socket()->error())
        return;
    QVariantMap out;
    out.insert("text", text);
    out.insert("level", lvl);
    BNetworkOperation *op = sendRequest(Texsample::LogRequest, out);
    op->waitForFinished();
    op->deleteLater();
}

void Connection::sendMessageRequestInternal(int msg)
{
    if (!muserId || !msubscribed || msg < 0 || !isConnected()
            || QAbstractSocket::UnknownSocketError != socket()->error())
        return;
    QVariantMap out;
    out.insert("message", TMessage(msg));
    BNetworkOperation *op = sendRequest(Texsample::MessageRequest, out);
    op->waitForFinished();
    op->deleteLater();
}
