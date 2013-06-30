#include "connection.h"
#include "global.h"
#include "storage.h"
#include "terminaliohandler.h"

#include <TAccessLevel>
#include <TUserInfo>
#include <TSampleInfo>
#include <TeXSample>
#include <TOperationResult>
#include <TProject>
#include <TCompilerParameters>
#include <TCompiledProject>
#include <TCompilationResult>
#include <TInviteInfo>

#include <BNetworkConnection>
#include <BNetworkServer>
#include <BGenericSocket>
#include <BNetworkOperation>
#include <BDirTools>

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

#include <QDebug>

/*============================================================================
================================ Connection ==================================
============================================================================*/

/*============================== Public constructors =======================*/

Connection::Connection(BNetworkServer *server, BGenericSocket *socket) :
    BNetworkConnection(server, socket)
{
    setCriticalBufferSize(BeQt::Kilobyte);
    setCloseOnCriticalBufferSize(true);
    mstorage = new Storage;
    muserId = 0;
    msubscribed = false;
    mtimer.setInterval(5 * BeQt::Minute);
    connect(&mtimer, SIGNAL(timeout()), this, SLOT(keepAlive()));
    connect(this, SIGNAL(requestSent(BNetworkOperation *)), this, SLOT(restartTimer(BNetworkOperation *)));
    connect(this, SIGNAL(replyReceived(BNetworkOperation *)), this, SLOT(restartTimer(BNetworkOperation *)));
    connect(this, SIGNAL(incomingRequest(BNetworkOperation *)), this, SLOT(restartTimer(BNetworkOperation *)));
    connect(this, SIGNAL(requestReceived(BNetworkOperation *)), this, SLOT(restartTimer(BNetworkOperation *)));
    connect(this, SIGNAL(replySent(BNetworkOperation *)), this, SLOT(restartTimer(BNetworkOperation *)));
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
    installRequestHandler(Texsample::GetRecoveryCodeRequest, (InternalHandler) &Connection::handleGetRecoveryCode);
    installRequestHandler(Texsample::RecoverPasswordRequest, (InternalHandler) &Connection::handleRecoverPassword);
    installRequestHandler(Texsample::CompileProjectRequest,
                          (InternalHandler) &Connection::handleCompileProjectRequest);
    installRequestHandler(Texsample::SubscribeRequest, (InternalHandler) &Connection::handleSubscribeRequest);
    installRequestHandler(Texsample::ExecuteCommandRequest,
                          (InternalHandler) &Connection::handleExecuteCommandRequest);
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
    QMetaObject::invokeMethod(this, "sendLogRequestInternal", Qt::QueuedConnection, Q_ARG(QString, text),
                              Q_ARG(int, lvl));
}

void Connection::sendWriteRequest(const QString &text)
{
    QMetaObject::invokeMethod(this, "sendWriteRequestInternal", Qt::QueuedConnection, Q_ARG(QString, text));
}

QString Connection::login() const
{
    return muserId ? mlogin : QString();
}

TClientInfo Connection::clientInfo() const
{
    return mclientInfo;
}

QString Connection::infoString(const QString &format) const
{
    //"%u - login, %p - address, %i - id, %a - access level
    if (!muserId)
        return "";
    QString f = !format.isEmpty() ? format :
                                    QString("[%u] [%p] %i\n%a; %o [%l]\n%c v%v; TeXSample v%t; BeQt v%b; Qt v%q");
    QString s = mclientInfo.toString(f);
    s.replace("%u", mlogin);
    s.replace("%p", peerAddress());
    s.replace("%i", uniqueId().toString());
    s.replace("%a", maccessLevel.string());
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
    QString msg = (muserId ? ("[" + mlogin + "] ") : QString()) + text;
    if (isConnected())
        TerminalIOHandler::sendLogRequest("[" + peerAddress() + "] " + msg, lvl);
    BNetworkConnection::log(msg, lvl);
}

void Connection::logLocal(const QString &text, BLogger::Level lvl)
{
    BNetworkConnection::log("[" + mlogin + "] " + text, lvl);
}

/*============================== Static private methods ====================*/

TOperationResult Connection::notAuthorizedResult()
{
    return TOperationResult(tr("Not authorized", "errorString"));
}

/*============================== Private methods ===========================*/

void Connection::handleAuthorizeRequest(BNetworkOperation *op)
{
    if (muserId)
        return Global::sendReply(op, TOperationResult(true));
    qDebug() << op->data().size();
    QVariantMap in = op->variantData().toMap();
    QString login = in.value("login").toString();
    QByteArray password = in.value("password").toByteArray();
    log(tr("Authorize request:", "log text") + " " + login);
    if (login.isEmpty() || password.isEmpty())
        return Global::sendReply(op, Storage::invalidParametersResult());
    quint64 id = mstorage->userId(login, password);
    if (!id)
        return Global::sendReply(op, TOperationResult(tr("No such user", "errorString")));
    mlogin = login;
    mclientInfo = in.value("client_info").value<TClientInfo>();
    muserId = id;
    maccessLevel = mstorage->userAccessLevel(id);
    if (maccessLevel >= TAccessLevel::AdminLevel)
        msubscribed = in.value("subscription").toBool();
    setCriticalBufferSize(200 * BeQt::Megabyte);
    log(tr("Authorized:", "log text") + " " + uniqueId().toString());
    log(infoString("%a\n%o [%l]\n%c v%v; TeXSample v%t; BeQt v%b; Qt v%q"));
    QVariantMap out;
    out.insert("user_id", id);
    out.insert("access_level", maccessLevel);
    Global::sendReply(op, out, TOperationResult(true));
    restartTimer();
}

void Connection::handleAddUserRequest(BNetworkOperation *op)
{
    if (!muserId)
        return Global::sendReply(op, notAuthorizedResult());
    QVariantMap in = op->variantData().toMap();
    TUserInfo info = in.value("user_info").value<TUserInfo>();
    log(tr("Add user request:", "log text") + " " + info.login());
    if (!info.isValid(TUserInfo::AddContext))
        return Global::sendReply(op, Storage::invalidParametersResult());
    if (maccessLevel < TAccessLevel::AdminLevel)
        return Global::sendReply(op, TOperationResult("Only admin can do this")); //TODO
    Global::sendReply(op, mstorage->addUser(info));
}

void Connection::handleEditUserRequest(BNetworkOperation *op)
{
    if (!muserId)
        return Global::sendReply(op, notAuthorizedResult());
    QVariantMap in = op->variantData().toMap();
    TUserInfo info = in.value("user_info").value<TUserInfo>();
    log(tr("Edit user request", "log text"));
    if (maccessLevel < TAccessLevel::AdminLevel)
        return Global::sendReply(op, TOperationResult("Only admin can do this")); //TODO
    if (maccessLevel >= TAccessLevel::AdminLevel)
        info.setAccessLevel(maccessLevel);
    Global::sendReply(op, mstorage->editUser(info));
}

void Connection::handleUpdateAccountRequest(BNetworkOperation *op)
{
    if (!muserId)
        return Global::sendReply(op, notAuthorizedResult());
    QVariantMap in = op->variantData().toMap();
    TUserInfo info = in.value("user_info").value<TUserInfo>();
    log(tr("Update account request", "log text"));
    if (!info.isValid(TUserInfo::UpdateContext))
        return Global::sendReply(op, Storage::invalidParametersResult());
    if (info.id() != muserId)
        return Global::sendReply(op, TOperationResult("This is not your account")); //TODO
    info.setContext(TUserInfo::EditContext);
    info.setAccessLevel(maccessLevel);
    Global::sendReply(op, mstorage->editUser(info));
}

void Connection::handleGetUserInfoRequest(BNetworkOperation *op)
{
    if (!muserId)
        return Global::sendReply(op, notAuthorizedResult());
    QVariantMap in = op->variantData().toMap();
    quint64 id = in.value("user_id").toULongLong();
    QDateTime updateDT = in.value("update_dt").toDateTime().toUTC();
    log(tr("Get user info request", "log text"));
    TUserInfo info;
    bool cacheOk = false;
    TOperationResult r = mstorage->getUserInfo(id, info, updateDT, cacheOk);
    if (!r)
        Global::sendReply(op, r);
    QVariantMap out;
    out.insert("update_dt", updateDT);
    if (cacheOk)
        out.insert("cache_ok", true);
    else
        out.insert("user_info", info);
    Global::sendReply(op, out, r);
}

void Connection::handleAddSampleRequest(BNetworkOperation *op)
{
    if (!muserId)
        return Global::sendReply(op, notAuthorizedResult());
    QVariantMap in = op->variantData().toMap();
    TProject project = in.value("project").value<TProject>();
    TSampleInfo info = in.value("sample_info").value<TSampleInfo>();
    log(tr("Add sample request", "log text"));
    Global::sendReply(op, "compilation_result", mstorage->addSample(muserId, project, info));
}

void Connection::handleEditSampleRequest(BNetworkOperation *op)
{
    if (!muserId)
        return Global::sendReply(op, notAuthorizedResult());
    QVariantMap in = op->variantData().toMap();
    TSampleInfo info = in.value("sample_info").value<TSampleInfo>();
    TProject project = in.value("project").value<TProject>();
    log(tr("Edit sample request", "log text"));
    if (maccessLevel < TAccessLevel::ModeratorLevel)
        return Global::sendReply(op, TOperationResult("Only admin can do this")); //TODO
    Global::sendReply(op, "compilation_result", mstorage->editSample(info, project));
}

void Connection::handleUpdateSampleRequest(BNetworkOperation *op)
{
    if (!muserId)
        return Global::sendReply(op, notAuthorizedResult());
    QVariantMap in = op->variantData().toMap();
    TSampleInfo info = in.value("sample_info").value<TSampleInfo>();
    TProject project = in.value("project").value<TProject>();
    log(tr("Update sample request", "log text"));
    quint64 sId = mstorage->senderId(info.id());
    if (sId != muserId)
        return Global::sendReply(op, TOperationResult("This is not your sample")); //TODO
    if (mstorage->sampleType(info.type()) == TSampleInfo::Approved)
        return Global::sendReply(op, TOperationResult("You can not modify approved sample")); //TODO
    Global::sendReply(op, "compilation_result", mstorage->editSample(info, project));
}

void Connection::handleDeleteSampleRequest(BNetworkOperation *op)
{
    if (!muserId)
        return Global::sendReply(op, notAuthorizedResult());
    QVariantMap in = op->variantData().toMap();
    quint64 id = in.value("sample_id").toULongLong();
    QString reason = in.value("reason").toString();
    log(tr("Delete sample request", "log text"));
    quint64 sId = mstorage->senderId(id);
    if (sId != muserId && maccessLevel < TAccessLevel::AdminLevel)
        return Global::sendReply(op, TOperationResult("This is not your sample")); //TODO
    Global::sendReply(op, mstorage->deleteSample(id, reason));
}

void Connection::handleGetSamplesListRequest(BNetworkOperation *op)
{
    if (!muserId)
        return Global::sendReply(op, notAuthorizedResult());
    QVariantMap in = op->variantData().toMap();
    QDateTime updateDT = in.value("update_dt").toDateTime().toUTC();
    log(tr("Get samples list request", "log text"));
    TSampleInfo::SamplesList newSamples;
    Texsample::IdList deletedSamples;
    TOperationResult r = mstorage->getSamplesList(newSamples, deletedSamples, updateDT);
    if (!r)
        Global::sendReply(op, r);
    QVariantMap out;
    out.insert("update_dt", updateDT);
    if (!newSamples.isEmpty())
        out.insert("new_sample_infos", QVariant::fromValue(newSamples));
    if (!deletedSamples.isEmpty())
        out.insert("deleted_sample_infos", QVariant::fromValue(deletedSamples));
    Global::sendReply(op, out, r);
}

void Connection::handleGetSampleSourceRequest(BNetworkOperation *op)
{
    if (!muserId)
        return Global::sendReply(op, notAuthorizedResult());
    QVariantMap in = op->variantData().toMap();
    quint64 id = in.value("sample_id").toULongLong();
    QDateTime updateDT = in.value("update_dt").toDateTime().toUTC();
    log(tr("Get sample source request", "log text"));
    TProject project;
    bool cacheOk = false;
    TOperationResult r = mstorage->getSampleSource(id, project, updateDT, cacheOk);
    if (!r)
        Global::sendReply(op, r);
    QVariantMap out;
    out.insert("update_dt", updateDT);
    if (cacheOk)
        out.insert("cache_ok", true);
    else
        out.insert("project", project);
    Global::sendReply(op, out, r);
}

void Connection::handleGetSamplePreviewRequest(BNetworkOperation *op)
{
    if (!muserId)
        return Global::sendReply(op, notAuthorizedResult());
    QVariantMap in = op->variantData().toMap();
    quint64 id = in.value("sample_id").toULongLong();
    QDateTime updateDT = in.value("update_dt").toDateTime().toUTC();
    log(tr("Get sample preview request", "log text"));
    TProjectFile file;
    bool cacheOk = false;
    TOperationResult r = mstorage->getSamplePreview(id, file, updateDT, cacheOk);
    if (!r)
        Global::sendReply(op, r);
    QVariantMap out;
    out.insert("update_dt", updateDT);
    if (cacheOk)
        out.insert("cache_ok", true);
    else
        out.insert("project_file", file);
    Global::sendReply(op, out, r);
}

void Connection::handleGenerateInvitesRequest(BNetworkOperation *op)
{
    if (!muserId)
        return Global::sendReply(op, notAuthorizedResult());
    if (maccessLevel < TAccessLevel::ModeratorLevel)
        return Global::sendReply(op, TOperationResult("Only moderator can do this")); //TODO
    QVariantMap in = op->variantData().toMap();
    QDateTime expiresDT = in.value("expiration_dt").toDateTime().toUTC();
    quint8 count = in.value("count").toUInt();
    log(tr("Generate invites request", "log text"));
    TInviteInfo::InvitesList invites;
    TOperationResult r = mstorage->generateInvites(muserId, expiresDT, count, invites);
    if (!r)
        Global::sendReply(op, r);
    QVariantMap out;
    out.insert("invite_infos", QVariant::fromValue(invites));
    Global::sendReply(op, out, r);
}

void Connection::handleGetInvitesListRequest(BNetworkOperation *op)
{
    if (!muserId)
        return Global::sendReply(op, notAuthorizedResult());
    if (maccessLevel < TAccessLevel::ModeratorLevel)
        return Global::sendReply(op, TOperationResult("Only moderator can do this")); //TODO
    log(tr("Get invites list request", "log text"));
    TInviteInfo::InvitesList invites;
    TOperationResult r = mstorage->getInvitesList(muserId, invites);
    if (!r)
        Global::sendReply(op, r);
    QVariantMap out;
    out.insert("invite_infos", QVariant::fromValue(invites));
    Global::sendReply(op, out, r);
}

void Connection::handleGetRecoveryCode(BNetworkOperation *op)
{
    if (!muserId)
        return Global::sendReply(op, notAuthorizedResult());
    Global::sendReply(op, mstorage->getRecoveryCode(muserId));
}

void Connection::handleRecoverPassword(BNetworkOperation *op)
{
    if (!muserId)
        return Global::sendReply(op, notAuthorizedResult());
    QVariantMap in = op->variantData().toMap();
    QUuid code = BeQt::uuidFromText(in.value("recovery_code").toString());
    QByteArray password = in.value("password").toByteArray();
    if (code.isNull() || password.isEmpty())
        return Global::sendReply(op, Storage::invalidParametersResult());
    Global::sendReply(op, mstorage->recoverPassword(muserId, code, password));
}

void Connection::handleCompileProjectRequest(BNetworkOperation *op)
{
    if (!muserId)
        return Global::sendReply(op, "compilation_result", TCompilationResult(notAuthorizedResult()));
    QVariantMap in = op->variantData().toMap();
    TProject project = in.value("project").value<TProject>();
    TCompilerParameters parameters = in.value("parameters").value<TCompilerParameters>();
    log(tr("Compile request", "log text"));
    QString path = QDir::tempPath() + "/texsample-server/compiler/" + BeQt::pureUuidText(uniqueId());
    TCompiledProject compiledProject;
    TCompilationResult *mr = parameters.makeindexEnabled() ? new TCompilationResult : 0;
    TCompilationResult *dr = parameters.dvipsEnabled() ? new TCompilationResult : 0;
    TCompilationResult r = Global::compileProject(path, project, parameters, &compiledProject, mr, dr);
    QVariantMap out;
    if (mr)
        out.insert("makeindex_result", *mr);
    if (dr)
        out.insert("dvips_result", *dr);
    if (r)
        out.insert("compiled_project", compiledProject);
    Global::sendReply(op, out, "compilation_result", r);
}

void Connection::handleSubscribeRequest(BNetworkOperation *op)
{
    if (!muserId)
        return Global::sendReply(op, notAuthorizedResult());
    if (maccessLevel < TAccessLevel::AdminLevel)
        return Global::sendReply(op, TOperationResult("Only admin can do this")); //TODO
    msubscribed = op->variantData().toMap().value("subscription").toBool();
    Global::sendReply(op, TOperationResult(true));
}

void Connection::handleExecuteCommandRequest(BNetworkOperation *op)
{
    if (!muserId)
        return Global::sendReply(op, Global::notAuthorizedResult());
    if (maccessLevel < TAccessLevel::AdminLevel)
        return Global::sendReply(op, TOperationResult("Only admin can do this")); //TODO
    QVariantMap in = op->variantData().toMap();
    QString command = in.value("command").toString();
    QStringList args = in.value("arguments").toStringList();
    logLocal(tr("Execute command request:", "log text") + " " + command);
    if (command.isEmpty())
        return Global::sendReply(op, Storage::invalidParametersResult());
    TerminalIOHandler::executeCommand(command, args, this);
    Global::sendReply(op, TOperationResult(true));
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
    logLocal(tr("Testing connection...", "log"));
    BNetworkOperation *op = sendRequest("noop");
    bool b = op->waitForFinished(5 * BeQt::Minute);
    if (!b)
    {
        log(tr("Connection response timeout", "log"));
        op->cancel();
    }
    op->deleteLater();
    if (b)
        mtimer.start();
}

void Connection::sendLogRequestInternal(const QString &text, int lvl)
{
    if (!muserId || !msubscribed || text.isEmpty() || !isConnected())
        return;
    QVariantMap out;
    out.insert("log_text", text);
    out.insert("level", lvl);
    BNetworkOperation *op = sendRequest(Texsample::LogRequest, out);
    op->waitForFinished();
    op->deleteLater();
}

void Connection::sendWriteRequestInternal(const QString &text)
{
    if (!muserId || !msubscribed || text.isEmpty() || !isConnected())
        return;
    QVariantMap out;
    out.insert("text", text);
    BNetworkOperation *op = sendRequest(Texsample::WriteRequest, out);
    op->waitForFinished();
    op->deleteLater();
}
