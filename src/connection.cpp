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
#include <TMessage>

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
    if (!mstorage->isValid())
    {
        QString msg = Global::string(Global::StorageError);
        logLocal(msg);
        logRemote(msg);
        close();
    }
    setCriticalBufferSize(BeQt::Kilobyte);
    setCloseOnCriticalBufferSize(true);
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
    installRequestHandler(Texsample::CompileProjectRequest,
                          (InternalHandler) &Connection::handleCompileProjectRequest);
    installRequestHandler(Texsample::SubscribeRequest, (InternalHandler) &Connection::handleSubscribeRequest);
    installRequestHandler(Texsample::ExecuteCommandRequest,
                          (InternalHandler) &Connection::handleExecuteCommandRequest);
    installRequestHandler("change_locale", (InternalHandler) &Connection::handleChangeLocale);
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

void Connection::sendWriteRequest(const QString &text)
{
    Qt::ConnectionType ct = (QThread::currentThread() == thread()) ? Qt::DirectConnection :
                                                                     Qt::BlockingQueuedConnection;
    QMetaObject::invokeMethod(this, "sendWriteRequestInternal", ct, Q_ARG(QString, text));
}

QString Connection::translate(const char *context, const char *sourceText, const char *disambiguation, int n)
{
    return mtranslator.translate(context, sourceText, disambiguation, n);
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
    //%d - user id, "%u - login, %p - address, %i - id, %a - access level
    if (!muserId)
        return "";
    QString f = format;
    if (f.isEmpty())
        f = "[%u] [%p] %i\n%a; %o [%l]\nClient v%v; TeXSample v%t; BeQt v%b; Qt v%q";
    f.replace("%l", mtranslator.locale().name());
    QString s = mclientInfo.toString(f);
    s.replace("%d", QString::number(muserId));
    s.replace("%u", mlogin);
    s.replace("%p", peerAddress());
    s.replace("%i", BeQt::pureUuidText(uniqueId()));
    s.replace("%a", QString::number(maccessLevel));
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
    QString msg = (muserId ? ("[" + mlogin + "] ") : QString()) + text;
    BNetworkConnection::log(msg, lvl);
}

void Connection::logRemote(const QString &text, BLogger::Level lvl)
{
    QString msg = (muserId ? ("[" + mlogin + "] ") : QString()) + text;
    TerminalIOHandler::sendLogRequest("[" + peerAddress() + "] " + msg, lvl);
}

/*============================== Private methods ===========================*/

bool Connection::handleRegisterRequest(BNetworkOperation *op)
{
    QVariantMap in = op->variantData().toMap();
    QUuid invite = BeQt::uuidFromText(in.value("invite").toString());
    TUserInfo info = in.value("user_info").value<TUserInfo>();
    Translator t(in.value("client_info").value<TClientInfo>().locale());
    log("Register request: " + info.login());
    if (invite.isNull() || !info.isValid(TUserInfo::RegisterContext))
    {
        Global::sendReply(op, Global::result(Global::InvalidParameters, &t));
        op->waitForFinished();
        return close();
    }
    info.setContext(TUserInfo::AddContext);
    info.setAccessLevel(TAccessLevel::UserLevel);
    quint64 id = mstorage->inviteId(invite);
    if (!id)
    {
        Global::sendReply(op, Global::result(Global::InvalidInvite, &t));
        op->waitForFinished();
        return close();
    }
    Global::sendReply(op, mstorage->addUser(info, &t, invite));
    op->waitForFinished();
    close();
}

bool Connection::handleGetRecoveryCodeRequest(BNetworkOperation *op)
{
    QVariantMap in = op->variantData().toMap();
    QString email = in.value("email").toString();
    Translator t(in.value("client_info").value<TClientInfo>().locale());
    if (email.isEmpty())
    {
        Global::sendReply(op, Global::result(Global::InvalidParameters, &t));
        op->waitForFinished();
        return close();
    }
    Global::sendReply(op, mstorage->getRecoveryCode(email, t));
    op->waitForFinished();
    close();
}

bool Connection::handleRecoverAccountRequest(BNetworkOperation *op)
{
    QVariantMap in = op->variantData().toMap();
    QString email = in.value("email").toString();
    QUuid code = BeQt::uuidFromText(in.value("recovery_code").toString());
    QByteArray password = in.value("password").toByteArray();
    Translator t(in.value("client_info").value<TClientInfo>().locale());
    if (email.isEmpty() || code.isNull() || password.isEmpty())
    {
        Global::sendReply(op, Global::result(Global::InvalidParameters, &t));
        op->waitForFinished();
        return close();
    }
    Global::sendReply(op, mstorage->recoverAccount(email, code, password, t));
    op->waitForFinished();
    close();
}

void Connection::handleAuthorizeRequest(BNetworkOperation *op)
{
    if (muserId)
        return Global::sendReply(op, TOperationResult(true));
    QVariantMap in = op->variantData().toMap();
    QString login = in.value("login").toString();
    QByteArray password = in.value("password").toByteArray();
    log("Authorize request: " + login);
    if (login.isEmpty() || password.isEmpty())
        return Global::sendReply(op, Global::result(Global::InvalidParameters, &mtranslator));
    quint64 id = mstorage->userId(login, password);
    if (!id)
        return Global::sendReply(op, Global::result(Global::NoSuchUser, &mtranslator));
    mlogin = login;
    mclientInfo = in.value("client_info").value<TClientInfo>();
    mlocale = mclientInfo.locale();
    muserId = id;
    maccessLevel = mstorage->userAccessLevel(id);
    if (maccessLevel >= TAccessLevel::AdminLevel)
        msubscribed = in.value("subscription").toBool();
    setCriticalBufferSize(200 * BeQt::Megabyte);
    QString f = "Authorized\nUser ID: %d\nUnique ID: %i\nAccess level: %a\nOS: %o\nLocale: ";
    f += "%l\nClient: %c";
    mtranslator.setLocale(mclientInfo.locale());
    mstorage->setTranslator(&mtranslator);
    f += "\nClient version: %v\nTeXSample version: %t\nBeQt version: %b\nQt version: %q";
    log(infoString(f));
    QVariantMap out;
    out.insert("user_id", id);
    out.insert("access_level", maccessLevel);
    Global::sendReply(op, out, TOperationResult(true));
    restartTimer();
}

void Connection::handleAddUserRequest(BNetworkOperation *op)
{
    if (!muserId)
        return Global::sendReply(op, Global::result(Global::NotAuthorized, &mtranslator));
    QVariantMap in = op->variantData().toMap();
    TUserInfo info = in.value("user_info").value<TUserInfo>();
    log("Add user request: " + info.login());
    if (!info.isValid(TUserInfo::AddContext))
        return Global::sendReply(op, Global::result(Global::InvalidParameters, &mtranslator));
    if (maccessLevel < TAccessLevel::AdminLevel)
        return Global::sendReply(op, Global::result(Global::NotEnoughRights, &mtranslator));
    Global::sendReply(op, mstorage->addUser(info, &mtranslator));
}

void Connection::handleEditUserRequest(BNetworkOperation *op)
{
    if (!muserId)
        return Global::sendReply(op, Global::result(Global::NotAuthorized, &mtranslator));
    QVariantMap in = op->variantData().toMap();
    TUserInfo info = in.value("user_info").value<TUserInfo>();
    log("Edit user request: " + info.idString());
    if (maccessLevel < TAccessLevel::AdminLevel)
        return Global::sendReply(op, Global::result(Global::NotEnoughRights, &mtranslator));
    if (maccessLevel >= TAccessLevel::AdminLevel)
        info.setAccessLevel(maccessLevel);
    Global::sendReply(op, mstorage->editUser(info));
}

void Connection::handleUpdateAccountRequest(BNetworkOperation *op)
{
    if (!muserId)
        return Global::sendReply(op, Global::result(Global::NotAuthorized, &mtranslator));
    QVariantMap in = op->variantData().toMap();
    TUserInfo info = in.value("user_info").value<TUserInfo>();
    log("Update account request");
    if (!info.isValid(TUserInfo::UpdateContext))
        return Global::sendReply(op, Global::result(Global::InvalidParameters, &mtranslator));
    if (info.id() != muserId)
        return Global::sendReply(op, Global::result(Global::NotYourAccount, &mtranslator));
    info.setContext(TUserInfo::EditContext);
    info.setAccessLevel(maccessLevel);
    Global::sendReply(op, mstorage->editUser(info));
}

void Connection::handleGetUserInfoRequest(BNetworkOperation *op)
{
    if (!muserId)
        return Global::sendReply(op, Global::result(Global::NotAuthorized, &mtranslator));
    QVariantMap in = op->variantData().toMap();
    quint64 id = in.value("user_id").toULongLong();
    QDateTime updateDT = in.value("update_dt").toDateTime().toUTC();
    log("Get user info request: " + QString::number(id));
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
        return Global::sendReply(op, Global::result(Global::NotAuthorized, &mtranslator));
    QVariantMap in = op->variantData().toMap();
    TProject project = in.value("project").value<TProject>();
    TSampleInfo info = in.value("sample_info").value<TSampleInfo>();
    log("Add sample request");
    Global::sendReply(op, "compilation_result", mstorage->addSample(muserId, project, info));
}

void Connection::handleEditSampleRequest(BNetworkOperation *op)
{
    if (!muserId)
        return Global::sendReply(op, Global::result(Global::NotAuthorized, &mtranslator));
    QVariantMap in = op->variantData().toMap();
    TSampleInfo info = in.value("sample_info").value<TSampleInfo>();
    TProject project = in.value("project").value<TProject>();
    log("Edit sample request: " + info.idString());
    if (maccessLevel < TAccessLevel::ModeratorLevel)
        return Global::sendReply(op, Global::result(Global::NotEnoughRights, &mtranslator));
    Global::sendReply(op, "compilation_result", mstorage->editSample(info, project));
}

void Connection::handleUpdateSampleRequest(BNetworkOperation *op)
{
    if (!muserId)
        return Global::sendReply(op, Global::result(Global::NotAuthorized, &mtranslator));
    QVariantMap in = op->variantData().toMap();
    TSampleInfo info = in.value("sample_info").value<TSampleInfo>();
    TProject project = in.value("project").value<TProject>();
    log("Update sample request: " + info.idString());
    quint64 sId = mstorage->senderId(info.id());
    if (sId != muserId)
        return Global::sendReply(op, Global::result(Global::NotYourSample, &mtranslator));
    if (mstorage->sampleType(info.type()) == TSampleInfo::Approved)
        return Global::sendReply(op, Global::result(Global::NotModifiableSample, &mtranslator));
    Global::sendReply(op, "compilation_result", mstorage->editSample(info, project));
}

void Connection::handleDeleteSampleRequest(BNetworkOperation *op)
{
    if (!muserId)
        return Global::sendReply(op, Global::result(Global::NotAuthorized, &mtranslator));
    QVariantMap in = op->variantData().toMap();
    quint64 id = in.value("sample_id").toULongLong();
    QString reason = in.value("reason").toString();
    log("Delete sample request: " + QString::number(id));
    quint64 sId = mstorage->senderId(id);
    if (sId != muserId && maccessLevel < TAccessLevel::AdminLevel)
        return Global::sendReply(op, Global::result(Global::NotYourSample, &mtranslator));
    Global::sendReply(op, mstorage->deleteSample(id, reason));
}

void Connection::handleGetSamplesListRequest(BNetworkOperation *op)
{
    if (!muserId)
        return Global::sendReply(op, Global::result(Global::NotAuthorized, &mtranslator));
    QVariantMap in = op->variantData().toMap();
    QDateTime updateDT = in.value("update_dt").toDateTime().toUTC();
    log("Get samples list request");
    TSampleInfoList newSamples;
    TIdList deletedSamples;
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
        return Global::sendReply(op, Global::result(Global::NotAuthorized, &mtranslator));
    QVariantMap in = op->variantData().toMap();
    quint64 id = in.value("sample_id").toULongLong();
    QDateTime updateDT = in.value("update_dt").toDateTime().toUTC();
    log("Get sample source request: " + QString::number(id));
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
        return Global::sendReply(op, Global::result(Global::NotAuthorized, &mtranslator));
    QVariantMap in = op->variantData().toMap();
    quint64 id = in.value("sample_id").toULongLong();
    QDateTime updateDT = in.value("update_dt").toDateTime().toUTC();
    log("Get sample preview request: " + QString::number(id));
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
        return Global::sendReply(op, Global::result(Global::NotAuthorized, &mtranslator));
    if (maccessLevel < TAccessLevel::ModeratorLevel)
        return Global::sendReply(op, Global::result(Global::NotEnoughRights, &mtranslator));
    QVariantMap in = op->variantData().toMap();
    QDateTime expiresDT = in.value("expiration_dt").toDateTime().toUTC();
    quint8 count = in.value("count").toUInt();
    log("Generate invites request (" + QString::number(count) + ")");
    TInviteInfoList invites;
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
        return Global::sendReply(op, Global::result(Global::NotAuthorized, &mtranslator));
    if (maccessLevel < TAccessLevel::ModeratorLevel)
        return Global::sendReply(op, Global::result(Global::NotEnoughRights, &mtranslator));
    log("Get invites list request");
    TInviteInfoList invites;
    TOperationResult r = mstorage->getInvitesList(muserId, invites);
    if (!r)
        Global::sendReply(op, r);
    QVariantMap out;
    out.insert("invite_infos", QVariant::fromValue(invites));
    Global::sendReply(op, out, r);
}

bool Connection::handleCompileProjectRequest(BNetworkOperation *op)
{
    if (!muserId)
        return Global::sendReply(op, TCompilationResult(TMessage())); //TODO
    Global::CompileParameters p;
    QVariantMap in = op->variantData().toMap();
    p.project = in.value("project").value<TProject>();
    p.param = in.value("parameters").value<TCompilerParameters>();
    log("Compile request");
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
    return Global::sendReply(op, out, r);
}

bool Connection::handleSubscribeRequest(BNetworkOperation *op)
{
    if (!muserId)
        return Global::sendReply(op, TMessage()); //TODO
    if (maccessLevel < TAccessLevel::AdminLevel)
        return Global::sendReply(op, TMessage()); //TODO
    msubscribed = op->variantData().toMap().value("subscription").toBool();
    log("Subscribe resuest: " + QString(msubscribed ? "true" : "false"));
    return Global::sendReply(op);
}

bool Connection::handleChangeLocale(BNetworkOperation *op)
{
    if (!muserId)
        return Global::sendReply(op, TMessage()); //TODO
    QVariantMap in = op->variantData().toMap();
    mlocale = in.value("locale").toLocale();
    logLocal("Change locale request: " + mlocale.name());
    Global::sendReply(op, TOperationResult(true));
    return true;
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
    if (!muserId || !msubscribed || text.isEmpty() || !isConnected())
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
    if (!muserId || !msubscribed || msg < 0 || !isConnected())
        return;
    QVariantMap out;
    out.insert("message", TMessage(msg));
    BNetworkOperation *op = sendRequest(Texsample::MessageRequest, out);
    op->waitForFinished();
    op->deleteLater();
}
