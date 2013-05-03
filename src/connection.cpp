#include "connection.h"
#include "database.h"
#include "sqlqueryresult.h"
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
    socket->tcpSocket()->setSocketOption(QTcpSocket::KeepAliveOption, 1);
    mstorage = new Storage(BCoreApplication::location(BCoreApplication::DataPath, BCoreApplication::UserResources));
    mauthorized = false;
    muserId = 0;
    mdb = new Database(uniqueId().toString(), BDirTools::findResource("texsample.sqlite", BDirTools::UserOnly));
    installRequestHandler(Texsample::AuthorizeRequest, (InternalHandler) &Connection::handleAuthorizeRequest);
    installRequestHandler(Texsample::AddUserRequest, (InternalHandler) &Connection::handleAddUserRequest);
    installRequestHandler(Texsample::EditUserRequest, (InternalHandler) &Connection::handleEditUserRequest);
    installRequestHandler(Texsample::UpdateAccountRequest, (InternalHandler) &Connection::handleUpdateAccountRequest);
    installRequestHandler(Texsample::GetUserInfoRequest, (InternalHandler) &Connection::handleGetUserInfoRequest);
    installRequestHandler(Texsample::AddSampleRequest, (InternalHandler) &Connection::handleAddSampleRequest);
    installRequestHandler(Texsample::EditSampleRequest, (InternalHandler) &Connection::handleEditSampleRequest);
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
    QTimer::singleShot(15 * BeQt::Second, this, SLOT(testAuthorization()));
}

Connection::~Connection()
{
    delete mstorage;
    delete mdb;
}

/*============================== Public methods ============================*/

QString Connection::login() const
{
    return mauthorized ? mlogin : QString();
}

TClientInfo Connection::clientInfo() const
{
    return mclientInfo;
}

QString Connection::infoString(const QString &format) const
{
    if (!mauthorized)
        return "";
    QString f = !format.isEmpty() ? format : QString("%l %p %u\n%a; %o\n%e; %t; %b; %q");
    QString s = mclientInfo.toString(f);
    s.replace("%l", "[" + mlogin + "]");
    s.replace("%p", "[" + peerAddress() + "]");
    s.replace("%u", uniqueId().toString());
    s.replace("%a", tr("Access level:", "info") + " " + maccessLevel.string());
    return s;
}

/*============================== Purotected methods ========================*/

void Connection::log(const QString &text, BLogger::Level lvl)
{
    BNetworkConnection::log((mauthorized ? ("[" + mlogin + "] ") : QString()) + text, lvl);
}

/*============================== Static private methods ====================*/

QString Connection::sampleSourceFileName(quint64 id)
{
    if (!id)
        return "";
    QString path = BDirTools::findResource( "samples/" + QString::number(id) );
    if ( path.isEmpty() )
        return "";
    QStringList files = QDir(path).entryList(QStringList() << "*.tex", QDir::Files);
    return (files.size() == 1) ? ( path + "/" + files.first() ) : QString();
}

QString Connection::samplePreviewFileName(quint64 id)
{
    if (!id)
        return "";
    QString tfn = sampleSourceFileName(id);
    if ( tfn.isEmpty() )
        return "";
    QFileInfo fi(tfn);
    QString fn = fi.path() + "/" + fi.baseName() + ".pdf";
    fi.setFile(fn);
    return ( fi.exists() && fi.isFile() ) ? fn : QString();
}

QStringList Connection::sampleAuxiliaryFileNames(quint64 id)
{
    QStringList list;
    if (!id)
        return list;
    QString tfn = sampleSourceFileName(id);
    if ( tfn.isEmpty() )
        return list;
    QFileInfo fi(tfn);
    QString path = fi.path();
    QStringList files = QDir(path).entryList(QDir::Files);
    static const QStringList Suffixes = QStringList() << "aux" << "idx" << "log" << "out" << "pdf" << "tex" << "toc";
    foreach (const QString &suff, Suffixes)
        files.removeAll(fi.baseName() + "." + suff);
    foreach (const QString &fn, files)
        list << path + "/" + fn;
    return list;
}

bool Connection::addFile(QVariantMap &target, const QString &fileName)
{
    if ( fileName.isEmpty() )
        return false;
    bool ok = false;
    QByteArray ba = BDirTools::readFile(fileName, -1, &ok);
    if (!ok)
        return false;
    target.insert( "file_name", QFileInfo(fileName).fileName() );
    target.insert("data", ba);
    return true;
}

bool Connection::addFile(QVariantList &target, const QString &fileName)
{
    QVariantMap m;
    if ( !addFile(m, fileName) )
        return false;
    target << m;
    return true;
}

int Connection::addFiles(QVariantMap &target, const QStringList &fileNames)
{
    if ( fileNames.isEmpty() )
        return 0;
    QVariantList list;
    int count = 0;
    foreach (const QString &fn, fileNames)
        if ( addFile(list, fn) )
            ++count;
    if ( !list.isEmpty() )
        target.insert("aux_files", list);
    return count;
}

bool Connection::addTextFile(QVariantMap &target, const QString &fileName)
{
    if ( fileName.isEmpty() )
        return false;
    bool ok = false;
    QString text = BDirTools::readTextFile(fileName, "UTF-8", &ok);
    if (!ok)
        return false;
    target.insert( "file_name", QFileInfo(fileName).fileName() );
    target.insert("text", text);
    return true;
}

QString Connection::tmpPath(const QUuid &uuid)
{
    if (uuid.isNull())
        return "";
    return BDirTools::findResource("tmp", BDirTools::UserOnly) + "/" + BeQt::pureUuidText(uuid);
}

bool Connection::testUserInfo(const QVariantMap &m, bool isNew)
{
    if (isNew)
    {
        if ( m.value("login").toString().isEmpty() )
            return false;
        bool ok = false;
        int lvl = m.value("access_level", TAccessLevel::NoLevel).toInt(&ok);
        if (!ok || !bRange(TAccessLevel::NoLevel, TAccessLevel::AdminLevel).contains(lvl))
            return false;
    }
    if ( m.value("password").toByteArray().isEmpty() )
        return false;
    if (m.value("real_name").toString().length() > 128)
        return false;
    if (m.value("avatar").toByteArray().size() > MaxAvatarSize)
        return false;
    return true;
}

TOperationResult Connection::notAuthorizedResult()
{
    return TOperationResult(tr("Not authorized", "errorString"));
}

/*============================== Private methods ===========================*/

void Connection::handleAuthorizeRequest(BNetworkOperation *op)
{
    if (muserId)
        return Global::sendReply(op, TOperationResult(true));
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
    mauthorized = true; //TODO: Use muserId instead
    setCriticalBufferSize(200 * BeQt::Megabyte);
    log(tr("Authorized", "log text"));
    TerminalIOHandler::writeLine(infoString("%u\n%a; %o\n%e; %t; %b; %q"));
    QVariantMap out;
    out.insert("user_id", id);
    out.insert("access_level", maccessLevel);
    Global::sendReply(op, out, TOperationResult(true));
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
    Global::sendReply(op, mstorage->editUser(info));
}

void Connection::handleUpdateAccountRequest(BNetworkOperation *op)
{
    if (!muserId)
        return Global::sendReply(op, notAuthorizedResult());
    QVariantMap in = op->variantData().toMap();
    TUserInfo info = in.value("user_info").value<TUserInfo>();
    log(tr("Update account request", "log text"));
    if (!info.isValid(TUserInfo::AddContext))
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
    quint64 authId = mstorage->authorId(info.id());
    if (authId != muserId && maccessLevel < TAccessLevel::AdminLevel)
        return Global::sendReply(op, TOperationResult("This is not your sample")); //TODO
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
    quint64 authId = mstorage->authorId(id);
    if (authId != muserId && maccessLevel < TAccessLevel::AdminLevel)
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
    qDebug() << project.size();
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
    return Global::sendReply(op, out, "compilation_result", r);
}

bool Connection::checkRights(TAccessLevel minLevel) const
{
    return mauthorized && maccessLevel >= minLevel;
}

quint64 Connection::userId(const QString &login)
{
    SqlQueryResult r;
    if (login.isEmpty() || !mdb->execQuery("SELECT id FROM users WHERE login = :login", r, ":login", login)
        || r.value().isEmpty())
        return 0;
    bool ok = false;
    quint64 id = r.value().value("id").toULongLong(&ok);
    return ok ? id : 0;
}

QString Connection::userLogin(quint64 id)
{
    SqlQueryResult r;
    if (!id || !mdb->execQuery("SELECT login FROM users WHERE id = :id", r, ":id", id) || r.value().isEmpty())
        return "";
    return r.value().value("login").toString();
}

void Connection::retOk(BNetworkOperation *op, const QVariantMap &out, const QString &msg)
{
    if (!op)
        return;
    mdb->endDBOperation();
    if ( !msg.isEmpty() )
        log(msg);
    sendReply(op, out);
}

void Connection::retOk(BNetworkOperation *op, QVariantMap &out, const QString &key, const QVariant &value,
                       const QString &msg)
{
    if ( !key.isEmpty() )
        out.insert(key, value);
    retOk(op, out, msg);
}

void Connection::retErr(BNetworkOperation *op, const QVariantMap &out, const QString &msg)
{
    if (!op)
        return;
    mdb->endDBOperation(false);
    if ( !msg.isEmpty() )
        log(msg);
    sendReply(op, out);
}

void Connection::retErr(BNetworkOperation *op, QVariantMap &out, const QString &key, const QVariant &value,
                        const QString &msg)
{
    QVariantMap m;
    if ( !key.isEmpty() )
        m.insert(key, value);
    retErr(op, out, msg);
}

/*============================== Private slots =============================*/

void Connection::testAuthorization()
{
    if (mauthorized)
        return;
    log("Authorization failed, closing connection");
    close();
}

/*============================== Static private constants ==================*/

const int Connection::MaxAvatarSize = BeQt::Megabyte;
