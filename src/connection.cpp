#include "connection.h"

#include <BNetworkConnection>
#include <BNetworkServer>
#include <BGenericSocket>
#include <BNetworkOperation>
#include <BDirTools>

#include <QObject>
#include <QByteArray>
#include <QString>
#include <QTimer>
#include <QSqlDatabase>
#include <QUuid>
#include <QSqlQuery>
#include <QVariant>
#include <QVariantMap>
#include <QVariantList>
#include <QDateTime>
#include <QStringList>
#include <QRegExp>
#include <QDir>
#include <QFileInfo>
#include <QSqlRecord>
#include <QProcess>

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
    mauthorized = false;
    maccessLevel = NoLevel;
    mdb = new QSqlDatabase( QSqlDatabase::addDatabase( "QSQLITE", uniqueId().toString() ) );
    mdb->setDatabaseName( BDirTools::findResource("texsample.sqlite", BDirTools::UserOnly) );
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
    QTimer::singleShot( 15 * BeQt::Second, this, SLOT( testAuthorization() ) );
}

Connection::~Connection()
{
    endDBOperation(false);
    delete mdb;
    QSqlDatabase::removeDatabase( uniqueId().toString() );
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
    QProcess proc;
    proc.setWorkingDirectory(path);
    QStringList args;
    args << ("-jobname=" + bfn);
    args << ("\\input texsample.tex \\input \"" + bfn + ".tex\" \\end{document}");
    proc.start("pdflatex", args);
    if ( !proc.waitForStarted(5 * BeQt::Second) || !proc.waitForFinished(2 * BeQt::Minute) )
    {
        proc.kill();
        BDirTools::rmdir(path);
        return false;
    }
    if (log)
        *log = BDirTools::readTextFile(path + "/" + bfn + ".log"); //TODO: Maybe use UTF-8 codec?
    return true;
}

bool Connection::testUserInfo(const QVariantMap &m, bool isNew)
{
    if (isNew)
    {
        if ( m.value("login").toString().isEmpty() )
            return false;
        bool ok = false;
        int lvl = m.value("access_level", NoLevel).toInt(&ok);
        if ( !ok || !bRange(NoLevel, AdminLevel).contains(lvl) )
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

/*============================== Private methods ===========================*/

void Connection::handleRegisterRequest(BNetworkOperation *op)
{
    QVariantMap out;
    QVariantMap in = op->variantData().toMap();
    QUuid uuid = in.value("invite").toUuid();
    QString login = in.value("login").toString();
    QByteArray pwd = in.value("password").toByteArray();
    log(tr("Register request:", "log text") + " " + login);
    //TODO: Implement error notification
    if ( uuid.isNull() || login.isEmpty() || pwd.isEmpty() || !beginDBOperation() )
        return retErr(op, out, tr("Registration failed", "log text") );
    QDateTime dtn = QDateTime::currentDateTimeUtc();
    QString qs = "SELECT id FROM invites WHERE uuid = :uuid AND expires_dt > :exp_dt";
    QVariantMap q;
    QVariantMap bv;
    bv.insert(":uuid", uuid.toString());
    bv.insert(":exp_dt", dtn.toMSecsSinceEpoch());
    bool ok = false;
    if (!execQuery(qs, q, bv) || q.value("id", 0).toLongLong(&ok) <= 0 || !ok)
        return retErr(op, out, "Registration failed");
    qint64 id = q.value("id").toLongLong();
    qs = "SELECT id FROM users WHERE login = :login";
    q.clear();
    bv.clear();
    if (!execQuery(qs, q, ":login", login) || q.value("id", 0).toLongLong() > 0)
        return retErr(op, out, "Registration failed");
    if (!execQuery("DELETE FROM invites WHERE id = :id", ":id", id))
        return retErr(op, out, "Registration failed");
    qs = "INSERT INTO users (login, password, access_level, modified_dt) VALUES (:login, :pwd, :alvl, :mod_dt)";
    bv.insert(":login", login);
    bv.insert(":pwd", pwd);
    bv.insert(":alvl", UserLevel);
    bv.insert(":mod_dt", dtn.toMSecsSinceEpoch());
    if (!execQuery(qs, 0, bv))
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
    if ( mlogin.isEmpty() || pwd.isEmpty() || !beginDBOperation() )
        return retErr(op, out, tr("Authorization failed", "log text") );
    QString qs = "SELECT password, access_level, real_name, avatar, modified_dt FROM users WHERE login = :login";
    QVariantMap q;
    QDateTime dtn = QDateTime::currentDateTimeUtc();
    if ( !execQuery(qs, q, ":login", mlogin) )
        return retErr(op, out, "Authorization failed");
    mauthorized = (q.value("password").toByteArray() == pwd);
    out.insert("authorized", mauthorized);
    if (!mauthorized)
        return retOk( op, out, tr("Authorization failed", "log text") );
    maccessLevel = q.value("access_level", NoLevel).toInt(); //TODO: Check validity
    setCriticalBufferSize(200 * BeQt::Megabyte);
    out.insert("update_dt", dtn);
    if ( dt.isValid() && dt.toMSecsSinceEpoch() > q.value("modified_dt").toLongLong() )
    {
        log( tr("Cache is up to date", "log text") );
        out.insert("cache_ok", true);
    }
    else
    {
        out.insert("access_level", maccessLevel);
        out.insert( "real_name", q.value("real_name") );
        out.insert( "avatar", q.value("avatar") );
    }
    retOk(op, out, tr("Authorized with access level:", "log text") + " " + QString::number(maccessLevel));
}

void Connection::handleGetSamplesListRequest(BNetworkOperation *op)
{
    QVariantMap out;
    log( tr("Get samples list request", "log text") );
    //TODO: Implement error notification
    if ( !checkRights() || !beginDBOperation() )
        return retErr( op, out, tr("Getting samples list failed", "log text") );
    QVariantMap in = op->variantData().toMap();
    QDateTime dt = in.value("last_update_dt").toDateTime();
    qint64 dtms = dt.toMSecsSinceEpoch();
    QString qs = "SELECT id, title, author, type, tags, rating, comment, admin_remark, modified_dt "
                 "FROM samples WHERE modified_dt >= :update_dt";
    QString qds = "SELECT id FROM deleted_samples WHERE deleted_dt >= :update_dt";
    QVariantList slist;
    QVariantList dslist;
    QDateTime dtn = QDateTime::currentDateTimeUtc();
    if ( !execQuery(qs, slist, ":update_dt", dtms) || !execQuery(qds, dslist, ":update_dt", dtms) )
        return retErr( op, out, tr("Getting samples list failed", "log text") );
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
    if ( !checkRights() || !beginDBOperation() )
        return retErr( op, out, tr("Getting sample source failed", "log text") );
    QVariantMap in = op->variantData().toMap();
    quint64 id = in.value("id").toULongLong();
    QDateTime dt = in.value("last_update_dt").toDateTime();
    QVariantMap q;
    QDateTime dtn = QDateTime::currentDateTimeUtc();
    if ( !execQuery("SELECT modified_dt FROM samples WHERE id = :id", q, ":id", id) || q.isEmpty() )
        return retErr( op, out, tr("Getting sample source failed", "log text") );
    out.insert("update_dt", dtn);
    if ( dt.isValid() && dt.toMSecsSinceEpoch() > q.value("modified_dt").toLongLong() )
        return retOk( op, out, "cache_ok", true, tr("Cache is up to date", "log text") );
    if ( !addTextFile( out, sampleSourceFileName(id) ) )
        return retErr( op, out, tr("Getting sample source failed", "log text") );
    addFiles( out, sampleAuxiliaryFileNames(id) );
    retOk(op, out);
}

void Connection::handleGetSamplePreviewRequest(BNetworkOperation *op)
{
    QVariantMap out;
    log( tr("Get sample preview request", "log text") );
    //TODO: Implement error notification
    QVariantMap in = op->variantData().toMap();
    quint64 id = in.value("id").toULongLong();
    if ( !id || !checkRights() || !beginDBOperation() )
        return retErr( op, out, tr("Getting sample preview failed", "log text") );
    QDateTime dt = in.value("last_update_dt").toDateTime();
    QVariantMap q;
    QDateTime dtn = QDateTime::currentDateTimeUtc();
    if ( !execQuery("SELECT modified_dt FROM samples WHERE id = :id", q, ":id", id) || q.isEmpty() )
        return retErr( op, out, tr("Getting sample preview failed", "log text") );
    out.insert("update_dt", dtn);
    if ( dt.isValid() && dt.toMSecsSinceEpoch() > q.value("modified_dt").toLongLong() )
        return retOk( op, out, "cache_ok", true, tr("Cache is up to date", "log text") );
    if ( !addFile( out, samplePreviewFileName(id) ) )
        return retErr( op, out, tr("Getting sample preview failed", "log text") );
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
    if ( !checkRights() || title.isEmpty() || !beginDBOperation() )
        return retErr( op, out, "log", log, tr("Adding sample failed", "log text") );
    out.insert("log", log);
    QString qs = "INSERT INTO samples (title, author, tags, comment, modified_dt) "
                 "VALUES (:title, :author, :tags, :comment, :modified_dt)";
    QVariantMap bv;
    bv.insert(":title", title);
    bv.insert(":author", mlogin);
    bv.insert( ":tags", in.value("tags").toStringList().join(", ") );
    bv.insert( ":comment", in.value("comment").toString() );
    bv.insert( ":modified_dt", QDateTime::currentDateTimeUtc().toMSecsSinceEpoch() );
    QVariant iid;
    if ( !execQuery(qs, &iid, bv) || !compileSample(tpath, in, &log) )
    {
        BDirTools::rmdir(tpath);
        return retErr( op, out, tr("Adding sample failed", "log text") );
    }
    QString sid = QString::number( iid.toLongLong() );
    QString npath = BDirTools::findResource("samples", BDirTools::UserOnly) + "/" + sid;
    if ( sid.isEmpty() || !BDirTools::renameDir(tpath, npath) )
        return retErr( op, out, tr("Adding sample failed", "log text") );
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
        if ( !ok || !ok2 || !bRange(Unverified, Rejected).contains(type) || !bRange(0, 100).contains(rating) )
            return retErr( op, out, tr("Updating sample failed", "log text") );
    }
    QString qs = "SELECT author, type FROM samples WHERE id = :id";
    QVariantMap q;
    if (!checkRights() || !id || title.isEmpty() || !beginDBOperation() || !execQuery(qs, ":id", id)
        || (q.value("author").toString() != mlogin && !isModer) || (q.value("type").toInt() == Approved && !isModer))
        return retErr( op, out, tr("Updating sample failed", "log text") );
    qs = "UPDATE samples SET title = :title, tags = :tags, comment = :comment, modified_dt = :mod_dt";
    QVariantMap bv;
    bv.insert(":title", title);
    bv.insert( ":tags", in.value("tags").toStringList().join(", ") );
    bv.insert( ":comment", in.value("comment") );
    bv.insert( ":mod_dt", QDateTime::currentDateTimeUtc().toMSecsSinceEpoch() );
    bv.insert(":id", id);
    if (isModer)
    {
        qs += ", type = :type, rating = :rating, admin_remark = :adm_rem";
        bv.insert(":type", type);
        bv.insert(":rating",  rating);
        bv.insert( ":adm_rem", in.value("admin_remark") );
    }
    qs += " WHERE id = :id";
    if ( !execQuery(qs, 0, bv) )
        return retErr( op, out, tr("Updating sample failed", "log text") );
    retOk(op, out, "ok", true);
}

void Connection::handleDeleteSampleRequest(BNetworkOperation *op)
{
    QVariantMap out;
    log( tr("Delete sample request", "log text") );
    //TODO: Implement error notification
    QVariantMap in = op->variantData().toMap();
    quint64 id = in.value("id").toULongLong();
    QVariantMap m;
    if ( !checkRights() || !id || !beginDBOperation() ||
         !execQuery("SELECT author, type FROM samples WHERE id = :id", m, ":id", id) )
        return retErr( op, out, tr("Deleting sample failed", "log text") );
    if ((m.value("author").toString() != mlogin || m.value("type").toInt() == Approved) && maccessLevel < AdminLevel)
        return retErr( op, out, tr("Deleting sample failed", "log text") );
    if ( !execQuery("DELETE FROM samples WHERE id = :id", ":id", id) )
        return retErr( op, out, tr("Deleting sample failed", "log text") );
    QString qs = "INSERT INTO deleted_samples (id, user, reason, deleted_dt) "
                 "VALUES (:id, :user, :reason, :deleted_dt)";
    QVariantMap bv;
    bv.insert(":id", id);
    bv.insert(":user", mlogin);
    bv.insert( ":reason", in.value("reason") );
    bv.insert( ":deleted_dt", QDateTime::currentDateTimeUtc().toMSecsSinceEpoch() );
    if ( !execQuery(qs, 0, bv) )
        return retErr( op, out, tr("Deleting sample failed", "log text") );
    if ( !BDirTools::rmdir( BDirTools::findResource( "samples/" + QString::number(id) ) ) )
        return retErr( op, out, tr("Deleting sample failed", "log text") );
    retOk(op, out, "ok", true);
}

void Connection::handleUpdateAccountRequest(BNetworkOperation *op)
{
    QVariantMap out;
    log( tr("Update account request", "log text") );
    //TODO: Implement error notification
    QVariantMap in = op->variantData().toMap();
    QByteArray pwd = in.value("password").toByteArray();
    QByteArray ava = in.value("avatar").toByteArray();
    QString qs = "UPDATE users SET password = :pwd, real_name = :rname, avatar = :avatar, modified_dt = :mod_dt "
                 "WHERE login = :login";
    QVariantMap bv;
    bv.insert(":login", mlogin);
    bv.insert(":pwd", pwd);
    bv.insert( ":rname", in.value("real_name") );
    bv.insert(":avatar", ava);
    bv.insert( ":mod_dt", QDateTime::currentDateTimeUtc().toMSecsSinceEpoch() );
    if ( !checkRights() || !testUserInfo(in) || !beginDBOperation() || !execQuery(qs, 0, bv) )
        return retErr( op, out, tr("Updating account failed", "log text") );
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
    QString qs = "INSERT INTO invites (uuid, user, expires_dt, created_dt) VALUES (:uuid, :user, :exp_dt, :cr_dt)";
    QVariantMap bv;
    bv.insert(":uuid", uuid.toString());
    bv.insert(":user", mlogin);
    bv.insert(":exp_dt", expiresDT.isValid() ? expiresDT.toMSecsSinceEpoch() : dt.addDays(3).toMSecsSinceEpoch());
    bv.insert(":cr_dt", dt.toMSecsSinceEpoch());
    if (!checkRights(ModeratorLevel) || !beginDBOperation() || !execQuery(qs, 0, bv))
        return retErr( op, out, tr("Generating invite failed", "log text") );
    retOk(op, out, "uuid", uuid);
}

void Connection::handleGetInvitesListRequest(BNetworkOperation *op)
{
    QVariantMap out;
    log( tr("Get invites list request", "log text") );
    //TODO: Implement error notification
    QString qs = "SELECT uuid, expires_dt FROM invites WHERE user = :user AND expires_dt > :exp_dt";
    QVariantList vl;
    QVariantMap bv;
    bv.insert(":user", mlogin);
    bv.insert(":exp_dt", QDateTime::currentDateTimeUtc().toMSecsSinceEpoch());
    if (!checkRights(ModeratorLevel) || !beginDBOperation() || !execQuery(qs, vl, bv))
        return retErr( op, out, tr("Getting invites list failed", "log text") );
    if (!vl.isEmpty())
    {
        foreach ( int i, bRange(0, vl.size() - 1) )
        {
            QVariantMap m = vl.at(i).toMap();
            m.insert("uuid", QUuid(m.value("uuid").toString()));
            QDateTime dt;
            dt.setTimeSpec(Qt::UTC);
            dt.setMSecsSinceEpoch(m.value("expires_dt").toLongLong());
            m.insert("expires_dt", dt);
            vl[i] = m;
        }
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
    QString qs = "INSERT INTO users (login, password, access_level, real_name, avatar, modified_dt) "
                 "VALUES (:login, :pwd, :alvl, :rname, :avatar, :mod_dt)";
    QVariantMap bv;
    bv.insert(":login", login);
    bv.insert(":pwd", pwd);
    bv.insert(":alvl", lvl);
    bv.insert( ":rname", in.value("real_name") );
    bv.insert( ":avatar", in.value("avatar") );
    bv.insert( ":mod_dt", QDateTime::currentDateTimeUtc().toMSecsSinceEpoch() );
    if ( !checkRights(AdminLevel) || testUserInfo(in, true) || !beginDBOperation() || !execQuery(qs, 0, bv) )
        return retErr( op, out, tr("Adding user failed", "log text") );
    retOk(op, out, "ok", true);
}

void Connection::handleGetUserInfoRequest(BNetworkOperation *op)
{
    QVariantMap out;
    log( tr("Get user info request", "log text") );
    //TODO: Implement error notification
    if ( !checkRights() || !beginDBOperation() )
        return retErr( op, out, tr("Getting user info failed", "log text") );
    QVariantMap in = op->variantData().toMap();
    QString login = in.value("login").toString();
    QDateTime dt = in.value("last_update_dt").toDateTime();
    QString qs = "SELECT access_level, real_name, avatar, modified_dt FROM users WHERE login = :login";
    QVariantMap q;
    QDateTime dtn = QDateTime::currentDateTimeUtc();
    if ( login.isEmpty() || !execQuery(qs, q, ":login", login) || q.isEmpty() )
        return retErr( op, out, tr("Getting user info failed", "log text") );
    out.insert("update_dt", dtn);
    if ( dt.isValid() && dt.toMSecsSinceEpoch() > q.value("modified_dt").toLongLong() )
        return retOk( op, out, "cache_ok", true, tr("Cache is up to date", "log text") );
    out.insert( "access_level", q.value("access_level") );
    out.insert( "real_name", q.value("real_name") );
    out.insert( "avatar", q.value("avatar") );
    retOk(op, out);
}

bool Connection::checkRights(AccessLevel minLevel) const
{
    return mauthorized && maccessLevel >= minLevel;
}

void Connection::retOk(BNetworkOperation *op, const QVariantMap &out, const QString &msg)
{
    if (!op)
        return;
    endDBOperation();
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
    endDBOperation(false);
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

bool Connection::beginDBOperation()
{
    if ( mdb->isOpen() || !mdb->open() || !mdb->transaction() )
        return false;
    QSqlQuery q("PRAGMA foreign_keys = ON", *mdb);
    bool b = q.exec();
    q.finish();
    if (!b)
    {
        mdb->rollback();
        mdb->close();
    }
    return b;
}

bool Connection::endDBOperation(bool success)
{
    bool b = !mdb->isOpen() || ( success ? mdb->commit() : mdb->rollback() );
    if (b)
        mdb->close();
    return b;
}

bool Connection::execQuery(const QString &query, QVariantMap &values, const QVariantMap &boundValues,
                           QVariant *insertId)
{
    if ( query.isEmpty() )
        return false;
    QSqlQuery q(*mdb);
    if ( !q.prepare(query) )
        return false;
    foreach ( const QString &key, boundValues.keys() )
        q.bindValue( key, boundValues.value(key) );
    if ( !q.exec() )
        return false;
    if (insertId)
        *insertId = q.lastInsertId();
    if ( !q.next() )
        return true;
    QSqlRecord r = q.record();
    if ( r.isEmpty() )
        return true;
    foreach ( int i, bRange(0, r.count() - 1) )
        values.insert( r.fieldName(i), r.value(i) );
    return true;
}

bool Connection::execQuery(const QString &query, QVariantList &values, const QVariantMap &boundValues,
                           QVariant *insertId)
{
    if ( query.isEmpty() )
        return false;
    QSqlQuery q(*mdb);
    if ( !q.prepare(query) )
        return false;
    foreach ( const QString &key, boundValues.keys() )
        q.bindValue( key, boundValues.value(key) );
    if ( !q.exec() )
        return false;
    if (insertId)
        *insertId = q.lastInsertId();
    while ( q.next() )
    {
        QSqlRecord r = q.record();
        QVariantMap m;
        if ( !r.isEmpty() )
            foreach ( int i, bRange(0, r.count() - 1) )
                m.insert( r.fieldName(i), r.value(i) );
        values << m;
    }
    return true;
}

bool Connection::execQuery(const QString &query, QVariantMap &values, const QString &boundKey,
                           const QVariant &boundValue, QVariant *insertId)
{
    QVariantMap m;
    if ( !boundKey.isEmpty() )
        m.insert(boundKey, boundValue);
    return execQuery(query, values, m, insertId);
}

bool Connection::execQuery(const QString &query, QVariantList &values, const QString &boundKey,
                           const QVariant &boundValue, QVariant *insertId)
{
    QVariantMap m;
    if ( !boundKey.isEmpty() )
        m.insert(boundKey, boundValue);
    return execQuery(query, values, m, insertId);
}

bool Connection::execQuery(const QString &query, QVariant *insertId, const QVariantMap &boundValues)
{
    QVariantMap m;
    return execQuery(query, m, boundValues, insertId);
}

bool Connection::execQuery(const QString &query, const QString &boundKey,
                           const QVariant &boundValue, QVariant *insertId)
{
    QVariantMap m;
    return execQuery(query, m, boundKey, boundValue, insertId);
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
