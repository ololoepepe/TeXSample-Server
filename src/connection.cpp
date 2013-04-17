#include "connection.h"
#include "database.h"
#include "sqlqueryresult.h"

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

#if (QT_VERSION < QT_VERSION_CHECK(5, 0, 0))
Q_DECLARE_METATYPE(QUuid)
#endif

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
    mauthorized = false;
    muserId = 0;
    maccessLevel = NoLevel;
    mdb = new Database(uniqueId().toString(), BDirTools::findResource("texsample.sqlite", BDirTools::UserOnly));
    installRequestHandler("register", (InternalHandler) &Connection::handleRegisterRequest);
    installRequestHandler("authorize", (InternalHandler) &Connection::handleAuthorizeRequest);
    installRequestHandler("get_samples_list", (InternalHandler) &Connection::handleGetSamplesListRequest);
    installRequestHandler("get_sample_source", (InternalHandler) &Connection::handleGetSampleSourceRequest);
    installRequestHandler("get_sample_preview", (InternalHandler) &Connection::handleGetSamplePreviewRequest);
    installRequestHandler("add_sample", (InternalHandler) &Connection::handleAddSampleRequest);
    installRequestHandler("update_sample", (InternalHandler) &Connection::handleUpdateSampleRequest);
    installRequestHandler("delete_sample", (InternalHandler) &Connection::handleDeleteSampleRequest);
    installRequestHandler("update_account", (InternalHandler) &Connection::handleUpdateAccountRequest);
    installRequestHandler("generate_invite", (InternalHandler) &Connection::handleGenerateInviteRequest);
    installRequestHandler("get_invites_list", (InternalHandler) &Connection::handleGetInvitesListRequest);
    installRequestHandler("add_user", (InternalHandler) &Connection::handleAddUserRequest);
    installRequestHandler("get_user_info", (InternalHandler) &Connection::handleGetUserInfoRequest);
    installRequestHandler("compile", (InternalHandler) &Connection::handleCompileRequest);
    QTimer::singleShot(15 * BeQt::Second, this, SLOT(testAuthorization()));
}

Connection::~Connection()
{
    delete mdb;
}

/*============================== Public methods ============================*/

QString Connection::login() const
{
    return mauthorized ? mlogin : QString();
}

QString Connection::info() const
{
    if (!mauthorized)
        return "";
    QString s = "[" + mlogin + "] [" + peerAddress() + "] " + uniqueId().toString() + "\n";
    s += tr("Access level:", "info") + " " + QString::number(maccessLevel) + "; "
            + tr("OS:", "info") + " " + minfo.osVersion + "\n";
    s += "TeX Creator: " + minfo.editorVersion + "; BeQt: " + minfo.beqtVersion + "; Qt: " + minfo.qtVersion;
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

bool Connection::compileSample(const QString &path, const QVariantMap &in, QString *log)
{
    QString fn = in.value("file_name").toString();
    QString text = in.value("text").toString();
    if ( path.isEmpty() || fn.isEmpty() || text.isEmpty() || !BDirTools::mkpath(path) )
        return false;
    QString bfn = QFileInfo(fn).baseName();
    if ( !BDirTools::writeTextFile(path + "/" + bfn + ".tex", text, "UTF-8") )
    {
        BDirTools::rmdir(path);
        return false;
    }
    foreach ( const QVariant &v, in.value("aux_files").toList() )
    {
        QVariantMap m = v.toMap();
        QString fn = m.value("file_name").toString();
        QByteArray ba = m.value("data").toByteArray();
        if ( !BDirTools::writeFile( path + "/" + QFileInfo(fn).fileName(), ba ) )
        {
            BDirTools::rmdir(path);
            return false;
        }
    }
    bool b = !execSampleCompiler(path, bfn, log);
    if (!b)
        BDirTools::rmdir(path);
    return b;
}

bool Connection::compile(const QString &path, const QVariantMap &in, int *exitCode, QString *log)
{
    QString cmd = in.value("compiler").toString();
    QString fn = QFileInfo(in.value("file_name").toString()).fileName();
    QString text = in.value("text").toString();
    static const QStringList compilers = QStringList() << "tex" << "latex" << "pdftex" << "pdflatex";
    if (path.isEmpty() || cmd.isEmpty() || !compilers.contains(cmd)
        || fn.isEmpty() || text.isEmpty() || !BDirTools::mkpath(path))
        return false;
    if (!BDirTools::writeTextFile(path + "/" + fn, text, "UTF-8"))
    {
        BDirTools::rmdir(path);
        return false;
    }
    foreach (const QVariant &v, in.value("aux_files").toList())
    {
        QVariantMap m = v.toMap();
        QString subpath = m.value("subpath").toString();
        if (!subpath.isEmpty() && !BDirTools::mkpath(path + "/" + subpath))
            return false;
        QString fn = QFileInfo(m.value("file_name").toString()).fileName();
        if (!subpath.isEmpty())
            fn.prepend(subpath + "/");
        if (!BDirTools::writeFile(path + "/" + fn, m.value("data").toByteArray()))
        {
            BDirTools::rmdir(path);
            return false;
        }
    }
    QStringList options = in.value("options").toStringList();
    QStringList commands = in.value("commands").toStringList();
    int code = execProjectCompiler(path, fn, cmd, options, commands, log);
    bool makeindex = text.contains("\\input texsample.tex") && in.value("makeindex").toBool(); //TODO: Improve
    if (!code && makeindex && !execTool(path, fn, "makeindex"))
        code = execProjectCompiler(path, fn, cmd, options, commands, log);
    if (!code && !cmd.contains("pdf") && in.value("dvips").toBool())
        execTool(path, fn, "dvips");
    if (code < 0)
    {
        BDirTools::rmdir(path);
        return bRet(exitCode, code, false);
    }
    return bRet(exitCode, code, code >= 0);
}

bool Connection::testUserInfo(const QVariantMap &m, bool isNew)
{
    if (isNew)
    {
        if ( m.value("login").toString().isEmpty() )
            return false;
        bool ok = false;
        int lvl = m.value("access_level", NoLevel).toInt(&ok);
        if (!ok || !bRange(NoLevel, AdminLevel).contains(lvl))
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

int Connection::execSampleCompiler(const QString &path, const QString &jobName, QString *log)
{
    QString tmpName = BeQt::pureUuidText(QUuid::createUuid()) + ".tex";
    if (!QFile::rename(path + "/" + jobName + ".tex", path + "/" + tmpName))
        return -3;
    QStringList args = QStringList() << "-interaction=nonstopmode" << ("-jobname=" + jobName)
                                     << ("\\input texsample.tex \\input " + tmpName + " \\end{document}");
    int code = BeQt::execProcess(path, "pdflatex", args, 5 * BeQt::Second, 2 * BeQt::Minute);
    if (!QFile::rename(path + "/" + tmpName, path + "/" + jobName + ".tex"))
        return -3;
    return bRet(log, (code >= 0) ? BDirTools::readTextFile(path + "/" + jobName + ".log") : QString(), code);
    //TODO: Maybe use UTF-8?
}

int Connection::execProjectCompiler(const QString &path, const QString &fileName, const QString &cmd,
                                    const QStringList &commands, const QStringList &options, QString *log)
{
    QStringList args = QStringList() << "-interaction=nonstopmode" << commands << (path + "/" + fileName) << options;
    args.removeAll("");
    return BeQt::execProcess(path, cmd, args, 5 * BeQt::Second, 5 * BeQt::Minute, log);
}

int Connection::execTool(const QString &path, const QString &fileName, const QString &tool)
{
    return BeQt::execProcess(path, tool, QStringList() << (path + "/" + QFileInfo(fileName).baseName()),
                             5 * BeQt::Second, BeQt::Minute);
}

bool Connection::saveUserAvatar(quint64 id, const QByteArray &avatar)
{
    if (!id)
        return false;
    QString fn = BDirTools::findResource("users", BDirTools::UserOnly) + "/" + QString::number(id) + "/avatar.dat";
    return BDirTools::writeFile(fn, avatar);
}

QByteArray Connection::loadUserAvatar(quint64 id, bool *ok)
{
    if (!id)
        return bRet(ok, false, QByteArray());
    QString fn = BDirTools::findResource("users/" + QString::number(id) + "/avatar.dat", BDirTools::UserOnly);
    return BDirTools::readFile(fn, -1, ok);
}

bool Connection::loadUserAvatar(quint64 id, QByteArray &avatar)
{
    bool ok = false;
    avatar = loadUserAvatar(id, &ok);
    return ok;
}

/*============================== Private methods ===========================*/

void Connection::handleRegisterRequest(BNetworkOperation *op)
{
    QVariantMap out;
    QVariantMap in = op->variantData().toMap();
    QUuid uuid = in.value("invite").value<QUuid>();
    QString login = in.value("login").toString();
    QByteArray pwd = in.value("password").toByteArray();
    log(tr("Register request:", "log text") + " " + login);
    //TODO: Implement error notification
    if (uuid.isNull() || login.isEmpty() || pwd.isEmpty() || !mdb->beginDBOperation())
        return retErr(op, out, tr("Registration failed", "log text") );
    qint64 dtn = QDateTime::currentDateTimeUtc().toMSecsSinceEpoch();
    QString qs = "SELECT id FROM invites WHERE uuid = :uuid AND expires_dt > :exp_dt";
    bool ok = false;
    SqlQueryResult r = mdb->execQuery(qs, ":uuid", uuid.toString(), ":exp_dt", dtn);
    if (!r.success() || r.value().value("id", 0).toLongLong(&ok) <= 0 || !ok)
        return retErr(op, out, "Registration failed");
    qint64 id = r.value().value("id", 0).toLongLong();
    qs = "SELECT id FROM users WHERE login = :login";
    if (!mdb->execQuery(qs, r, ":login", login) || r.value().value("id", 0).toLongLong() > 0
        || !mdb->execQuery("DELETE FROM invites WHERE id = :id", ":id", id))
        return retErr(op, out, "Registration failed");
    qs = "INSERT INTO users (login, password, access_level, created_dt, modified_dt) "
            "VALUES (:login, :pwd, :alvl, :cr_dt, :mod_dt)";
    QVariantMap bv;
    bv.insert(":login", login);
    bv.insert(":pwd", pwd);
    bv.insert(":alvl", UserLevel);
    bv.insert(":cr_dt", dtn);
    bv.insert(":mod_dt", dtn);
    if (!mdb->execQuery(qs, bv))
         return retErr(op, out, "Registration failed");
    retOk(op, out, "ok", true, tr("Registered", "log text"));
    op->waitForFinished();
    close();
}

void Connection::handleAuthorizeRequest(BNetworkOperation *op)
{
    QVariantMap out;
    //TODO: Handle situations where the client is already authorized
    QVariantMap in = op->variantData().toMap();
    mlogin = in.value("login").toString();
    QByteArray pwd = in.value("password").toByteArray();
    QDateTime dt = in.value("last_update_dt").toDateTime();
    log(tr("Authorize request:", "log text") + " " + mlogin);
    //TODO: Implement error notification
    if (mlogin.isEmpty() || pwd.isEmpty() || !mdb->beginDBOperation())
        return retErr(op, out, tr("Authorization failed", "log text"));
    QString qs = "SELECT password, access_level, real_name, modified_dt FROM users WHERE login = :login";
    QDateTime dtn = QDateTime::currentDateTimeUtc();
    SqlQueryResult r = mdb->execQuery(qs, ":login", mlogin);
    if (!r.success())
        return retErr(op, out, "Authorization failed");
    mauthorized = (r.value().value("password").toByteArray() == pwd);
    out.insert("authorized", mauthorized);
    if (!mauthorized)
        return retOk(op, out, tr("Authorization failed", "log text"));
    maccessLevel = r.value().value("access_level", NoLevel).toInt(); //TODO: Check validity
    muserId = userId(mlogin);
    setCriticalBufferSize(200 * BeQt::Megabyte);
    out.insert("update_dt", dtn);
    if (dt.isValid() && dt.toMSecsSinceEpoch() > r.value().value("modified_dt").toLongLong())
    {
        log( tr("Cache is up to date", "log text") );
        out.insert("cache_ok", true);
    }
    else
    {
        out.insert("access_level", maccessLevel);
        out.insert("real_name", r.value().value("real_name"));
        out.insert("avatar", loadUserAvatar(muserId));
    }
    minfo = Info(in);
    retOk(op, out, tr("Authorized:", "log text") + "\n" + info());
}

void Connection::handleGetSamplesListRequest(BNetworkOperation *op)
{
    QVariantMap out;
    log( tr("Get samples list request", "log text") );
    //TODO: Implement error notification
    if (!checkRights() || !mdb->beginDBOperation())
        return retErr(op, out, tr("Getting samples list failed", "log text"));
    QVariantMap in = op->variantData().toMap();
    QDateTime dt = in.value("last_update_dt").toDateTime();
    qint64 dtms = dt.toMSecsSinceEpoch();
    QString qs = "SELECT id, author_id, title, type, tags, rating, comment, admin_remark, created_dt, modified_dt "
            "FROM samples WHERE modified_dt >= :update_dt";
    QString qds = "SELECT id FROM deleted_samples WHERE deleted_dt >= :update_dt";
    SqlQueryResult r = mdb->execQuery(qs, ":update_dt", dtms);
    SqlQueryResult rd = mdb->execQuery(qds, ":update_dt", dtms);
    if (!r.success() || !rd.success())
        return retErr(op, out, tr("Getting samples list failed", "log text"));
    QDateTime dtn = QDateTime::currentDateTimeUtc();
    QVariantList slist = r.values();
    QVariantList dslist = rd.values();
    foreach (int i, bRangeD(0, slist.size() - 1))
    {
        QVariantMap m = slist.at(i).toMap();
        m.insert("author", userLogin(m.value("author_id").toULongLong()));
        slist[i] = m;
    }
    out.insert("update_dt", dtn);
    out.insert("samples", slist);
    out.insert("deleted_samples", dslist);
    retOk(op, out, tr("New samples:", "log text") + " " + QString::number( slist.size() ) + " "
          + tr("Deleted samples", "log text") + " " + QString::number( dslist.size() ));
}

void Connection::handleGetSampleSourceRequest(BNetworkOperation *op)
{
    QVariantMap out;
    log( tr("Get sample source request", "log text") );
    //TODO: Implement error notification
    if (!checkRights() || !mdb->beginDBOperation())
        return retErr( op, out, tr("Getting sample source failed", "log text") );
    QVariantMap in = op->variantData().toMap();
    quint64 id = in.value("id").toULongLong();
    QDateTime dt = in.value("last_update_dt").toDateTime();
    QDateTime dtn = QDateTime::currentDateTimeUtc();
    SqlQueryResult r = mdb->execQuery("SELECT modified_dt FROM samples WHERE id = :id", ":id", id);
    if (!r.success() || r.value().isEmpty())
        return retErr( op, out, tr("Getting sample source failed", "log text") );
    out.insert("update_dt", dtn);
    if (dt.isValid() && dt.toMSecsSinceEpoch() > r.value().value("modified_dt").toLongLong())
        return retOk(op, out, "cache_ok", true, tr("Cache is up to date", "log text"));
    if (!addTextFile(out, sampleSourceFileName(id)))
        return retErr(op, out, tr("Getting sample source failed", "log text"));
    addFiles(out, sampleAuxiliaryFileNames(id));
    retOk(op, out);
}

void Connection::handleGetSamplePreviewRequest(BNetworkOperation *op)
{
    QVariantMap out;
    log( tr("Get sample preview request", "log text") );
    //TODO: Implement error notification
    QVariantMap in = op->variantData().toMap();
    quint64 id = in.value("id").toULongLong();
    if (!id || !checkRights() || !mdb->beginDBOperation())
        return retErr( op, out, tr("Getting sample preview failed", "log text") );
    QDateTime dt = in.value("last_update_dt").toDateTime();
    QDateTime dtn = QDateTime::currentDateTimeUtc();
    SqlQueryResult r = mdb->execQuery("SELECT modified_dt FROM samples WHERE id = :id", ":id", id);
    if (!r.success() || r.value().isEmpty())
        return retErr(op, out, tr("Getting sample preview failed", "log text"));
    out.insert("update_dt", dtn);
    if (dt.isValid() && dt.toMSecsSinceEpoch() > r.value().value("modified_dt").toLongLong())
        return retOk(op, out, "cache_ok", true, tr("Cache is up to date", "log text"));
    if (!addFile(out, samplePreviewFileName(id)))
        return retErr(op, out, tr("Getting sample preview failed", "log text"));
    retOk(op, out);
}

void Connection::handleAddSampleRequest(BNetworkOperation *op)
{
    QVariantMap out;
    log( tr("Add sample request", "log text") );
    //TODO: Implement error notification
    QVariantMap in = op->variantData().toMap();
    QString title = in.value("title").toString();
    QString tpath = tmpPath(uniqueId());
    QString log;
    if (!checkRights() || title.isEmpty() || !mdb->beginDBOperation())
        return retErr(op, out, "log", log, tr("Adding sample failed", "log text"));
    out.insert("log", log);
    QString qs = "INSERT INTO samples (title, author_id, tags, comment, created_dt, modified_dt) "
                 "VALUES (:title, :author_id, :tags, :comment, :created_dt, :modified_dt)";
    QVariantMap bv;
    qint64 dt = QDateTime::currentDateTimeUtc().toMSecsSinceEpoch();
    bv.insert(":title", title);
    bv.insert(":author_id", muserId);
    bv.insert(":tags", in.value("tags").toStringList().join(", "));
    bv.insert(":comment", in.value("comment").toString());
    bv.insert(":created_dt", dt);
    bv.insert(":modified_dt", dt);
    SqlQueryResult r = mdb->execQuery(qs, bv);
    if (!r.success() || !compileSample(tpath, in, &log))
    {
        BDirTools::rmdir(tpath);
        return retErr(op, out, tr("Adding sample failed", "log text"));
    }
    QString sid = QString::number(r.insertId().toLongLong());
    QString npath = BDirTools::findResource("samples", BDirTools::UserOnly) + "/" + sid;
    if ( sid.isEmpty() || !BDirTools::renameDir(tpath, npath) )
        return retErr(op, out, tr("Adding sample failed", "log text"));
    retOk(op, out, "ok", true);
}

void Connection::handleUpdateSampleRequest(BNetworkOperation *op)
{
    QVariantMap out;
    log( tr("Update sample request", "log text") );
    //TODO: Implement error notification
    QVariantMap in = op->variantData().toMap();
    quint64 id = in.value("id").toULongLong();
    QString title = in.value("title").toString();
    bool isModer = maccessLevel >= ModeratorLevel;
    int type = 0;
    int rating = 0;
    if (isModer)
    {
        bool ok = false;
        type = in.value("type").toInt(&ok);
        bool ok2 = false;
        rating = in.value("rating").toInt(&ok2);
        if (!ok || !ok2 || !bRange(Unverified, Rejected).contains(type) || !bRange(0, 100).contains(rating))
            return retErr(op, out, tr("Updating sample failed", "log text"));
    }
    QString qs = "SELECT author_id, type FROM samples WHERE id = :id";
    if (!checkRights() || !id || title.isEmpty() || !mdb->beginDBOperation())
        return retErr(op, out, tr("Updating sample failed", "log text"));
    SqlQueryResult r = mdb->execQuery(qs, ":id", id);
    if (!r.success() || (r.value().value("author_id").toULongLong() != muserId && !isModer) ||
        (r.value().value("type").toInt() == Approved && !isModer))
        return retErr(op, out, tr("Updating sample failed", "log text"));
    qs = "UPDATE samples SET title = :title, tags = :tags, comment = :comment, modified_dt = :mod_dt";
    QVariantMap bv;
    bv.insert(":title", title);
    bv.insert(":tags", in.value("tags").toStringList().join(", "));
    bv.insert(":comment", in.value("comment"));
    bv.insert(":mod_dt", QDateTime::currentDateTimeUtc().toMSecsSinceEpoch());
    bv.insert(":id", id);
    if (isModer)
    {
        qs += ", type = :type, rating = :rating, admin_remark = :adm_rem";
        bv.insert(":type", type);
        bv.insert(":rating",  rating);
        bv.insert(":adm_rem", in.value("admin_remark"));
    }
    qs += " WHERE id = :id";
    if (!mdb->execQuery(qs, bv))
        return retErr(op, out, tr("Updating sample failed", "log text"));
    retOk(op, out, "ok", true);
}

void Connection::handleDeleteSampleRequest(BNetworkOperation *op)
{
    QVariantMap out;
    log( tr("Delete sample request", "log text") );
    //TODO: Implement error notification
    QVariantMap in = op->variantData().toMap();
    quint64 id = in.value("id").toULongLong();
    if (!checkRights() || !id || !mdb->beginDBOperation())
        return retErr(op, out, tr("Deleting sample failed", "log text"));
    SqlQueryResult r = mdb->execQuery("SELECT author_id, type FROM samples WHERE id = :id", ":id", id);
    if (!r.success())
        return retErr(op, out, tr("Deleting sample failed", "log text"));
    quint64 uid = r.value().value("author_id").toULongLong();
    if ((uid != muserId || r.value().value("type").toInt() == Approved) && maccessLevel < AdminLevel)
        return retErr(op, out, tr("Deleting sample failed", "log text"));
    if (!mdb->execQuery("DELETE FROM samples WHERE id = :id", ":id", id))
        return retErr(op, out, tr("Deleting sample failed", "log text"));
    QString qs = "INSERT INTO deleted_samples (id, creator_id, reason, deleted_dt) "
                 "VALUES (:id, :cr_id, :reason, :deleted_dt)";
    QVariantMap bv;
    bv.insert(":id", id);
    bv.insert(":cr_id", uid);
    bv.insert(":reason", in.value("reason"));
    bv.insert(":deleted_dt", QDateTime::currentDateTimeUtc().toMSecsSinceEpoch());
    if (!mdb->execQuery(qs, bv))
        return retErr(op, out, tr("Deleting sample failed", "log text"));
    if (!BDirTools::rmdir(BDirTools::findResource("samples/" + QString::number(id))))
        return retErr(op, out, tr("Deleting sample failed", "log text"));
    retOk(op, out, "ok", true);
}

void Connection::handleUpdateAccountRequest(BNetworkOperation *op)
{
    QVariantMap out;
    log( tr("Update account request", "log text") );
    //TODO: Implement error notification
    QVariantMap in = op->variantData().toMap();
    QByteArray pwd = in.value("password").toByteArray();
    QString qs = "UPDATE users SET password = :pwd, real_name = :rname, modified_dt = :mod_dt "
                 "WHERE login = :login";
    QVariantMap bv;
    bv.insert(":login", mlogin);
    bv.insert(":pwd", pwd);
    bv.insert(":rname", in.value("real_name"));
    bv.insert(":mod_dt", QDateTime::currentDateTimeUtc().toMSecsSinceEpoch());
    if (!checkRights() || !testUserInfo(in) || !mdb->beginDBOperation() || !mdb->execQuery(qs, bv)
        || !saveUserAvatar(muserId, in.value("avatar").toByteArray()))
        return retErr(op, out, tr("Updating account failed", "log text"));
    retOk(op, out, "ok", true);
}

void Connection::handleGenerateInviteRequest(BNetworkOperation *op)
{
    QVariantMap out;
    log( tr("Generate invite request", "log text") );
    //TODO: Implement error notification
    QVariantMap in = op->variantData().toMap();
    QDateTime expiresDT = in.value("expires_dt").toDateTime();
    if (expiresDT.isValid() && expiresDT.timeSpec() != Qt::UTC)
        expiresDT = expiresDT.toUTC();
    QDateTime dt = QDateTime::currentDateTimeUtc();
    QUuid uuid = QUuid::createUuid();
    QString qs = "INSERT INTO invites (uuid, creator_id, expires_dt, created_dt) "
            "VALUES (:uuid, :user, :exp_dt, :cr_dt)";
    QVariantMap bv;
    bv.insert(":uuid", uuid.toString());
    bv.insert(":creator_id", muserId);
    bv.insert(":exp_dt", expiresDT.isValid() ? expiresDT.toMSecsSinceEpoch() : dt.addDays(3).toMSecsSinceEpoch());
    bv.insert(":cr_dt", dt.toMSecsSinceEpoch());
    if (!checkRights(ModeratorLevel) || !mdb->beginDBOperation() || !mdb->execQuery(qs, bv))
        return retErr(op, out, tr("Generating invite failed", "log text"));
    retOk(op, out, "uuid", QVariant::fromValue(uuid));
}

void Connection::handleGetInvitesListRequest(BNetworkOperation *op)
{
    QVariantMap out;
    log( tr("Get invites list request", "log text") );
    //TODO: Implement error notification
    QString qs = "SELECT uuid, expires_dt FROM invites WHERE creator_id = :cr_id AND expires_dt > :exp_dt";
    QVariantMap bv;
    bv.insert(":cr_id", muserId);
    bv.insert(":exp_dt", QDateTime::currentDateTimeUtc().toMSecsSinceEpoch());
    SqlQueryResult r;
    if (!checkRights(ModeratorLevel) || !mdb->beginDBOperation() || !mdb->execQuery(qs, r, bv))
        return retErr(op, out, tr("Getting invites list failed", "log text"));
    QVariantList vl = r.values();
    foreach (int i, bRangeD(0, vl.size() - 1))
    {
        QVariantMap m = vl.at(i).toMap();
        m.insert("uuid", m.value("uuid"));
        QDateTime dt;
        dt.setTimeSpec(Qt::UTC);
        dt.setMSecsSinceEpoch(m.value("expires_dt").toLongLong());
        m.insert("expires_dt", dt);
        vl[i] = m;
    }
    out.insert("list", vl);
    out.insert("ok", true);
    retOk(op, out);
}

void Connection::handleAddUserRequest(BNetworkOperation *op)
{
    QVariantMap out;
    log( tr("Add user request", "log text") );
    //TODO: Implement error notification
    QVariantMap in = op->variantData().toMap();
    QString login = in.value("login").toString();
    QByteArray pwd = in.value("password").toByteArray();
    int lvl = in.value("access_level", NoLevel).toInt();
    QString qs = "INSERT INTO users (login, password, access_level, real_name, created_dt, modified_dt) "
                 "VALUES (:login, :pwd, :alvl, :rname, :cr_dt, :mod_dt)";
    QVariantMap bv;
    qint64 dt = QDateTime::currentDateTimeUtc().toMSecsSinceEpoch();
    bv.insert(":login", login);
    bv.insert(":pwd", pwd);
    bv.insert(":alvl", lvl);
    bv.insert(":rname", in.value("real_name"));
    bv.insert(":cr_dt", dt);
    bv.insert(":mod_dt", dt);
    QByteArray ava = in.value("avatar").toByteArray();
    SqlQueryResult r;
    if (!checkRights(AdminLevel) || !testUserInfo(in, true) || !mdb->beginDBOperation()
        || !(r = mdb->execQuery(qs, bv)) || !saveUserAvatar(r.insertId().toULongLong(), ava))
        return retErr(op, out, tr("Adding user failed", "log text"));
    retOk(op, out, "ok", true);
}

void Connection::handleGetUserInfoRequest(BNetworkOperation *op)
{
    QVariantMap out;
    log( tr("Get user info request", "log text") );
    //TODO: Implement error notification
    if (!checkRights() || !mdb->beginDBOperation())
        return retErr(op, out, tr("Getting user info failed", "log text"));
    QVariantMap in = op->variantData().toMap();
    QString login = in.value("login").toString();
    QDateTime dt = in.value("last_update_dt").toDateTime();
    QString qs = "SELECT access_level, real_name, modified_dt FROM users WHERE login = :login";
    QDateTime dtn = QDateTime::currentDateTimeUtc();
    SqlQueryResult r;
    if (login.isEmpty() || !mdb->execQuery(qs, r, ":login", login) || r.value().isEmpty())
        return retErr(op, out, tr("Getting user info failed", "log text"));
    out.insert("update_dt", dtn);
    if (dt.isValid() && dt.toMSecsSinceEpoch() > r.value().value("modified_dt").toLongLong())
        return retOk(op, out, "cache_ok", true, tr("Cache is up to date", "log text"));
    out.insert("access_level", r.value().value("access_level"));
    out.insert("real_name", r.value().value("real_name"));
    out.insert("avatar", loadUserAvatar(userId(login)));
    retOk(op, out);
}

void Connection::handleCompileRequest(BNetworkOperation *op)
{
    QVariantMap out;
    log( tr("Compile request", "log text") );
    //TODO: Implement error notification
    if (!checkRights())
        return retErr(op, out, tr("Compilation failed", "log text"));
    QVariantMap in = op->variantData().toMap();
    QString tpath = tmpPath(uniqueId());
    QString log;
    int exitCode = -1;
    bool b = compile(tpath, in, &exitCode, &log);
    out.insert("exit_code", exitCode);
    out.insert("log", log);
    if (!b)
        return retErr(op, out, tr("Compilation failed", "log text"));
    bool ok = false;
    QString bfn = QFileInfo(in.value("file_name").toString()).baseName();
    bool pdf = in.value("compiler").toString().contains("pdf");
    QStringList suffixes = QStringList() << (pdf ? "pdf" : "dvi");
    foreach (const QString &suff, QStringList() << "aux" << "idx" << "ilg" << "ind" << "log" << "out" << "toc")
        if (QFile(tpath + "/" + bfn + "." + suff).exists())
            suffixes << suff;
    if (!pdf && in.value("dvips").toBool())
        suffixes << "ps";
    foreach (const QString &suff, suffixes)
    {
        QByteArray ba = BDirTools::readFile(tpath + "/" + bfn + "." + suff, -1, &ok);
        if (!ok)
            break;
        out.insert(suff, ba);
    }
    BDirTools::rmdir(tpath);
    ok ? retOk(op, out, "ok", true) : retErr(op, out, tr("Compilation failed", "log text"));
}

bool Connection::checkRights(AccessLevel minLevel) const
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
