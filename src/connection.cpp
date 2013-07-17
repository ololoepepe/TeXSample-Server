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
        QString msg = TMessage().messageStringNoTr(); //TODO
        logLocal(msg);
        logRemote(msg);
        close();
    }
    setCriticalBufferSize(2 * BeQt::Megabyte);
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
    installRequestHandler(Texsample::ChangeLocaleRequest, (InternalHandler) &Connection::handleChangeLocale);
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

QString Connection::infoString(const QString &format) const
{
    //%d - user id, "%u - login, %p - address, %i - id, %a - access level
    if (!muserId)
        return "";
    QString f = format;
    if (f.isEmpty())
        f = "[%u] [%p] %i\n%a; %o [%l]\nClient v%v; TeXSample v%t; BeQt v%b; Qt v%q";
    f.replace("%l", mlocale.name());
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
    QString code = in.value("invite").toString();
    TUserInfo info = in.value("user_info").value<TUserInfo>();
    QLocale l = in.value("locale").toLocale();
    log("Register request: " + info.login());
    if (BeQt::uuidFromText(code).isNull() || !info.isValid(TUserInfo::RegisterContext))
    {
        Global::sendReply(op, TMessage()); //TODO
        op->waitForFinished();
        close();
        return false;
    }
    info.setContext(TUserInfo::AddContext);
    info.setAccessLevel(TAccessLevel::UserLevel);
    bool b = Global::sendReply(op, mstorage->addUser(info, l, code));
    op->waitForFinished();
    close();
    return b;
}

bool Connection::handleGetRecoveryCodeRequest(BNetworkOperation *op)
{
    QVariantMap in = op->variantData().toMap();
    QString email = in.value("email").toString();
    QLocale l = in.value("locale").toLocale();
    bool b = Global::sendReply(op, mstorage->getRecoveryCode(email, l));
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
    bool b = Global::sendReply(op, mstorage->recoverAccount(email, code, password, l));
    op->waitForFinished();
    close();
    return b;
}

bool Connection::handleAuthorizeRequest(BNetworkOperation *op)
{
    if (muserId)
        return Global::sendReply(op);
    QVariantMap in = op->variantData().toMap();
    QString login = in.value("login").toString();
    QByteArray password = in.value("password").toByteArray();
    log("Authorize request: " + login);
    if (login.isEmpty() || password.isEmpty())
        return Global::sendReply(op, TMessage()); //TODO
    quint64 id = mstorage->userId(login, password);
    if (!id)
        return Global::sendReply(op, TMessage()); //TODO
    mlogin = login;
    mclientInfo = in.value("client_info").value<TClientInfo>();
    mlocale = mclientInfo.locale();
    muserId = id;
    maccessLevel = mstorage->userAccessLevel(id);
    if (maccessLevel >= TAccessLevel::AdminLevel)
        msubscribed = in.value("subscription").toBool();
    setCriticalBufferSize(200 * BeQt::Megabyte);
    QString f = "Authorized\nUser ID: %d\nUnique ID: %i\nAccess level: %a\nOS: %o\nLocale: ";
    f += "%l\nClient: %c\nClient version: %v\nTeXSample version: %t\nBeQt version: %b\nQt version: %q";
    log(infoString(f));
    QVariantMap out;
    out.insert("user_id", id);
    out.insert("access_level", maccessLevel);
    restartTimer();
    return Global::sendReply(op, out, TOperationResult(true));
}

bool Connection::handleAddUserRequest(BNetworkOperation *op)
{
    if (!muserId)
        return Global::sendReply(op, TMessage()); //TODO
    QVariantMap in = op->variantData().toMap();
    TUserInfo info = in.value("user_info").value<TUserInfo>();
    log("Add user request: " + info.login());
    if (!info.isValid(TUserInfo::AddContext))
        return Global::sendReply(op, TMessage()); //TODO
    if (maccessLevel < TAccessLevel::AdminLevel)
        return Global::sendReply(op, TMessage()); //TODO
    return Global::sendReply(op, mstorage->addUser(info, mlocale));
}

bool Connection::handleEditUserRequest(BNetworkOperation *op)
{
    if (!muserId)
        return Global::sendReply(op, TMessage()); //TODO
    QVariantMap in = op->variantData().toMap();
    TUserInfo info = in.value("user_info").value<TUserInfo>();
    log("Edit user request: " + info.idString());
    if (maccessLevel < TAccessLevel::AdminLevel)
        return Global::sendReply(op, TMessage()); //TODO
    if (maccessLevel >= TAccessLevel::AdminLevel)
        info.setAccessLevel(maccessLevel);
    return Global::sendReply(op, mstorage->editUser(info));
}

bool Connection::handleUpdateAccountRequest(BNetworkOperation *op)
{
    if (!muserId)
        return Global::sendReply(op, TMessage()); //TODO
    QVariantMap in = op->variantData().toMap();
    TUserInfo info = in.value("user_info").value<TUserInfo>();
    log("Update account request");
    if (!info.isValid(TUserInfo::UpdateContext))
        return Global::sendReply(op, TMessage()); //TODO
    if (info.id() != muserId)
        return Global::sendReply(op, TMessage()); //TODO
    info.setContext(TUserInfo::EditContext);
    info.setAccessLevel(maccessLevel);
    return Global::sendReply(op, mstorage->editUser(info));
}

bool Connection::handleGetUserInfoRequest(BNetworkOperation *op)
{
    if (!muserId)
        return Global::sendReply(op, TMessage()); //TODO
    QVariantMap in = op->variantData().toMap();
    quint64 id = in.value("user_id").toULongLong();
    QDateTime updateDT = in.value("update_dt").toDateTime().toUTC();
    log("Get user info request: " + QString::number(id));
    TUserInfo info;
    bool cacheOk = false;
    TOperationResult r = mstorage->getUserInfo(id, info, updateDT, cacheOk);
    if (!r)
        return Global::sendReply(op, r);
    QVariantMap out;
    out.insert("update_dt", updateDT);
    if (cacheOk)
        out.insert("cache_ok", true);
    else
        out.insert("user_info", info);
    return Global::sendReply(op, out, r);
}

bool Connection::handleAddSampleRequest(BNetworkOperation *op)
{
    if (!muserId)
        return Global::sendReply(op, TMessage()); //TODO
    QVariantMap in = op->variantData().toMap();
    TProject project = in.value("project").value<TProject>();
    TSampleInfo info = in.value("sample_info").value<TSampleInfo>();
    log("Add sample request");
    return Global::sendReply(op, mstorage->addSample(muserId, project, info));
}

bool Connection::handleEditSampleRequest(BNetworkOperation *op)
{
    if (!muserId)
        return Global::sendReply(op, TMessage()); //TODO
    QVariantMap in = op->variantData().toMap();
    TSampleInfo info = in.value("sample_info").value<TSampleInfo>();
    TProject project = in.value("project").value<TProject>();
    log("Edit sample request: " + info.idString());
    if (maccessLevel < TAccessLevel::ModeratorLevel)
        return Global::sendReply(op, TMessage()); //TODO
    return Global::sendReply(op, mstorage->editSample(info, project));
}

bool Connection::handleUpdateSampleRequest(BNetworkOperation *op)
{
    if (!muserId)
        return Global::sendReply(op, TMessage()); //TODO
    QVariantMap in = op->variantData().toMap();
    TSampleInfo info = in.value("sample_info").value<TSampleInfo>();
    TProject project = in.value("project").value<TProject>();
    log("Update sample request: " + info.idString());
    quint64 sId = mstorage->senderId(info.id());
    if (sId != muserId)
        return Global::sendReply(op, TMessage()); //TODO
    if (mstorage->sampleType(info.type()) == TSampleInfo::Approved)
        return Global::sendReply(op, TMessage()); //TODO
    return Global::sendReply(op, mstorage->editSample(info, project));
}

bool Connection::handleDeleteSampleRequest(BNetworkOperation *op)
{
    if (!muserId)
        return Global::sendReply(op, TMessage()); //TODO
    QVariantMap in = op->variantData().toMap();
    quint64 id = in.value("sample_id").toULongLong();
    QString reason = in.value("reason").toString();
    log("Delete sample request: " + QString::number(id));
    quint64 sId = mstorage->senderId(id);
    if (sId != muserId && maccessLevel < TAccessLevel::AdminLevel)
        return Global::sendReply(op, TMessage()); //TODO
    return Global::sendReply(op, mstorage->deleteSample(id, reason));
}

bool Connection::handleGetSamplesListRequest(BNetworkOperation *op)
{
    if (!muserId)
        return Global::sendReply(op, TMessage()); //TODO
    QVariantMap in = op->variantData().toMap();
    QDateTime updateDT = in.value("update_dt").toDateTime().toUTC();
    log("Get samples list request");
    TSampleInfoList newSamples;
    TIdList deletedSamples;
    TOperationResult r = mstorage->getSamplesList(newSamples, deletedSamples, updateDT);
    if (!r)
        return Global::sendReply(op, r);
    QVariantMap out;
    out.insert("update_dt", updateDT);
    if (!newSamples.isEmpty())
        out.insert("new_sample_infos", QVariant::fromValue(newSamples));
    if (!deletedSamples.isEmpty())
        out.insert("deleted_sample_infos", QVariant::fromValue(deletedSamples));
    return Global::sendReply(op, out, r);
}

bool Connection::handleGetSampleSourceRequest(BNetworkOperation *op)
{
    if (!muserId)
        return Global::sendReply(op, TMessage()); //TODO
    QVariantMap in = op->variantData().toMap();
    quint64 id = in.value("sample_id").toULongLong();
    QDateTime updateDT = in.value("update_dt").toDateTime().toUTC();
    log("Get sample source request: " + QString::number(id));
    TProject project;
    bool cacheOk = false;
    TOperationResult r = mstorage->getSampleSource(id, project, updateDT, cacheOk);
    if (!r)
        return Global::sendReply(op, r);
    QVariantMap out;
    out.insert("update_dt", updateDT);
    if (cacheOk)
        out.insert("cache_ok", true);
    else
        out.insert("project", project);
    return Global::sendReply(op, out, r);
}

bool Connection::handleGetSamplePreviewRequest(BNetworkOperation *op)
{
    if (!muserId)
        return Global::sendReply(op, TMessage()); //TODO
    QVariantMap in = op->variantData().toMap();
    quint64 id = in.value("sample_id").toULongLong();
    QDateTime updateDT = in.value("update_dt").toDateTime().toUTC();
    log("Get sample preview request: " + QString::number(id));
    TProjectFile file;
    bool cacheOk = false;
    TOperationResult r = mstorage->getSamplePreview(id, file, updateDT, cacheOk);
    if (!r)
        return Global::sendReply(op, r);
    QVariantMap out;
    out.insert("update_dt", updateDT);
    if (cacheOk)
        out.insert("cache_ok", true);
    else
        out.insert("project_file", file);
    return Global::sendReply(op, out, r);
}

bool Connection::handleGenerateInvitesRequest(BNetworkOperation *op)
{
    if (!muserId)
        return Global::sendReply(op, TMessage()); //TODO
    if (maccessLevel < TAccessLevel::ModeratorLevel)
        return Global::sendReply(op, TMessage()); //TODO
    QVariantMap in = op->variantData().toMap();
    QDateTime expiresDT = in.value("expiration_dt").toDateTime().toUTC();
    quint8 count = in.value("count").toUInt();
    log("Generate invites request (" + QString::number(count) + ")");
    TInviteInfoList invites;
    TOperationResult r = mstorage->generateInvites(muserId, expiresDT, count, invites);
    if (!r)
        return Global::sendReply(op, r);
    QVariantMap out;
    out.insert("invite_infos", invites);
    return Global::sendReply(op, out, r);
}

bool Connection::handleGetInvitesListRequest(BNetworkOperation *op)
{
    if (!muserId)
        return Global::sendReply(op, TMessage()); //TODO
    if (maccessLevel < TAccessLevel::ModeratorLevel)
        return Global::sendReply(op, TMessage()); //TODO
    log("Get invites list request");
    TInviteInfoList invites;
    TOperationResult r = mstorage->getInvitesList(muserId, invites);
    if (!r)
        return Global::sendReply(op, r);
    QVariantMap out;
    out.insert("invite_infos", invites);
    return Global::sendReply(op, out, r);
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
