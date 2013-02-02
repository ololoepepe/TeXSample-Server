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
    installRequestHandler("authorize", (InternalHandler) &Connection::handleAuthorizeRequest);
    installRequestHandler("get_samples_list", (InternalHandler) &Connection::handleGetSamplesListRequest);
    installRequestHandler("get_sample_source", (InternalHandler) &Connection::handleGetSampleSourceRequest);
    installRequestHandler("get_sample_preview", (InternalHandler) &Connection::handleGetSamplePreviewRequest);
    installRequestHandler("add_sample", (InternalHandler) &Connection::handleAddSampleRequest);
    installRequestHandler("delete_sample", (InternalHandler) &Connection::handleDeleteSampleRequest);
    installRequestHandler("update_account", (InternalHandler) &Connection::handleUpdateAccountRequest);
    installRequestHandler("add_user", (InternalHandler) &Connection::handleAddUserRequest);
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
    static const QStringList Suffixes = QStringList() << "aux" << "idx" << "log" << "out" << "pdf" << "tex";
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

QString Connection::userTmpPath(const QString &login)
{
    if ( login.isEmpty() )
        return "";
    return BDirTools::findResource("tmp", BDirTools::UserOnly) + "/" + login;
}

bool Connection::compileSample(const QString &path, const QVariantMap &in, QString *log)
{
    QString fn = in.value("file_name").toString();
    QString text = in.value("text").toString();
    if ( path.isEmpty() || fn.isEmpty() || text.isEmpty() || !BDirTools::mkpath(path) )
        return false;
    if ( !BDirTools::writeTextFile(path + "/" + QFileInfo(fn).fileName(), text, "UTF-8") )
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
    args << ( "-jobname=" + QFileInfo(fn).baseName() );
    args << ("\\input texsample.tex \\input \"" + QFileInfo(fn).fileName() + "\" \\end{document}");
    proc.start("pdflatex", args);
    if ( !proc.waitForStarted(5 * BeQt::Second) || !proc.waitForFinished(2 * BeQt::Minute) )
    {
        proc.kill();
        BDirTools::rmdir(path);
        return false;
    }
    if (log)
        *log = BDirTools::readTextFile(path + "/" + QFileInfo(fn).baseName() + ".log"); //TODO: Maybe use UTF-8 codec?
    return true;
}

/*============================== Private methods ===========================*/

void Connection::handleAuthorizeRequest(BNetworkOperation *op)
{
    QVariantMap out;
    //TODO: Handle situations where the client is already authorized
    QVariantMap in = op->variantData().toMap();
    mlogin = in.value("login").toString();
    QByteArray pwd = in.value("password").toByteArray();
    log(tr("Authorize request:", "log text") + " " + mlogin);
    //TODO: Implement error notification
    if ( mlogin.isEmpty() || pwd.isEmpty() || !beginDBOperation() )
        return retErr(op, out, tr("Authorization failed", "log text") );
    QString qs = "SELECT password, access_level, real_name, avatar FROM users WHERE login=:login";
    QVariantMap q;
    if ( !execQuery(qs, q, ":login", mlogin) )
        return retErr(op, out, "Failed to authorize"); //TODO
    endDBOperation();
    mauthorized = (q.value("password").toByteArray() == pwd);
    out.insert("authorized", mauthorized);
    if (!mauthorized)
        return retOk( op, out, tr("Authorization failed", "log text") );
    maccessLevel = q.value("access_level", NoLevel).toInt(); //TODO: Check validity
    setCriticalBufferSize(200 * BeQt::Megabyte);
    out.insert("access_level", maccessLevel);
    out.insert( "real_name", q.value("real_name") );
    out.insert( "avatar", q.value("avatar") );
    log( tr("Authorized with access level:", "log text") + " " + QString::number(maccessLevel) );
    sendReply(op, out);
}

void Connection::handleGetSamplesListRequest(BNetworkOperation *op)
{
    QVariantMap out;
    log( tr("Get samples list request", "log text") );
    //TODO: Implement error notification
    if ( !mauthorized || maccessLevel < UserLevel || !beginDBOperation() )
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
    endDBOperation();
    log( tr("Last update time:", "log text") + " " + dt.toString() + " " +
         tr("New update time:", "log text") + " " + dtn.toString() );
    out.insert("update_dt", dtn);
    out.insert("samples", slist);
    out.insert("deleted_samples", dslist);
    log( tr("New samples:", "log text") + " " + QString::number( slist.size() ) + " " +
         tr("Deleted samples", "log text") + " " + QString::number( dslist.size() ) );
    sendReply(op, out);
}

void Connection::handleGetSampleSourceRequest(BNetworkOperation *op)
{
    QVariantMap out;
    log( tr("Get sample source request", "log text") );
    //TODO: Implement error notification
    if ( !mauthorized || maccessLevel < UserLevel || !beginDBOperation() )
        return retErr( op, out, tr("Getting sample source failed", "log text") );
    QVariantMap in = op->variantData().toMap();
    quint64 id = in.value("id").toULongLong();
    QDateTime dt = in.value("last_update_dt").toDateTime();
    QVariantMap bv;
    bv.insert(":id", id);
    QVariantMap q;
    QDateTime dtn = QDateTime::currentDateTimeUtc();
    if ( !execQuery("SELECT modified_dt FROM samples WHERE id = :id", q, ":id", id) || q.isEmpty() )
        return retErr( op, out, tr("Getting sample source failed", "log text") );
    endDBOperation();
    out.insert("update_dt", dtn);
    if ( dt.isValid() && dt.toMSecsSinceEpoch() > q.value("modified_dt").toLongLong() )
        return retOk( op, out, "cache_ok", true, tr("Cache is up to date", "log text") );
    if ( !addTextFile( out, sampleSourceFileName(id) ) )
        return retErr( op, out, tr("Getting sample source failed", "log text") );
    addFiles( out, sampleAuxiliaryFileNames(id) );
    sendReply(op, out);
}

void Connection::handleGetSamplePreviewRequest(BNetworkOperation *op)
{
    QVariantMap out;
    log( tr("Get sample preview request", "log text") );
    //TODO: Implement error notification
    if ( !mauthorized || maccessLevel < UserLevel || !beginDBOperation() )
        return retErr( op, out, tr("Getting sample preview failed", "log text") );
    QVariantMap in = op->variantData().toMap();
    quint64 id = in.value("id").toULongLong();
    QDateTime dt = in.value("last_update_dt").toDateTime();
    QVariantMap q;
    QDateTime dtn = QDateTime::currentDateTimeUtc();
    if ( !execQuery("SELECT id FROM samples WHERE id = :id", q, ":id", id) || q.isEmpty() )
        return retErr( op, out, tr("Getting sample preview failed", "log text") );
    endDBOperation();
    out.insert("update_dt", dtn);
    if ( dt.isValid() && dt > dtn)
        return retOk( op, out, "cache_ok", true, tr("Cache is up to date", "log text") );
    if ( !addFile( out, samplePreviewFileName(id) ) )
        return retErr( op, out, tr("Getting sample preview failed", "log text") );
    sendReply(op, out);
}

void Connection::handleAddSampleRequest(BNetworkOperation *op)
{
    QVariantMap out;
    log( tr("Add sample request", "log text") );
    //TODO: Implement error notification
    QVariantMap in = op->variantData().toMap();
    QString title = in.value("title").toString();
    QString tpath = userTmpPath(mlogin);
    QString log;
    if ( !mauthorized || maccessLevel < UserLevel || title.isEmpty() ||
         !beginDBOperation() || !compileSample(tpath, in, &log) )
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
    if ( !execQuery(qs, &iid, bv) || !execQuery("DELETE FROM deleted_samples WHERE id = :id", ":id", iid) )
    {
        BDirTools::rmdir(tpath);
        return retErr( op, out, tr("Adding sample failed", "log text") );
    }
    endDBOperation();
    QString sid = QString::number( iid.toLongLong() );
    QString npath = BDirTools::findResource("samples", BDirTools::UserOnly) + "/" + sid;
    if ( sid.isEmpty() || !BDirTools::renameDir(tpath, npath) )
        return retErr( op, out, tr("Adding sample failed", "log text") );
    out.insert("ok", true);
    sendReply(op, out);
}

void Connection::handleDeleteSampleRequest(BNetworkOperation *op)
{
    QVariantMap out;
    log( tr("Delete sample request", "log text") );
    //TODO: Implement error notification
    QVariantMap in = op->variantData().toMap();
    quint64 id = in.value("id").toULongLong();
    QVariantMap m;
    if ( !mauthorized || maccessLevel < UserLevel || !id || !beginDBOperation() ||
         !execQuery("SELECT author FROM samples WHERE id = :id", m, ":id", id) )
        return retErr( op, out, tr("Deleting sample failed", "log text") );
    if (m.value("author").toString() != mlogin && maccessLevel < AdminLevel)
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
    endDBOperation();
    out.insert("ok", true);
    sendReply(op, out);
}

void Connection::handleUpdateAccountRequest(BNetworkOperation *op)
{
    QVariantMap out;
    log( tr("Update account request", "log text") );
    //TODO: Implement error notification
    QVariantMap in = op->variantData().toMap();
    QByteArray pwd = in.value("password").toByteArray();
    QString qs = "UPDATE users SET password = :pwd, real_name = :rname, avatar = :avatar, modified_dt = :mod_dt "
                 "WHERE login = :login";
    QVariantMap bv;
    bv.insert(":login", mlogin);
    bv.insert(":pwd", pwd);
    bv.insert( ":rname", in.value("real_name") );
    bv.insert( ":avatar", in.value("avatar") );
    bv.insert( ":mod_dt", QDateTime::currentDateTimeUtc().toMSecsSinceEpoch() );
    if ( !mauthorized || maccessLevel < UserLevel || pwd.isEmpty() || !beginDBOperation() || !execQuery(qs, 0, bv) )
        return retErr( op, out, tr("Updating account failed", "log text") );
    endDBOperation();
    out.insert("ok", true);
    sendReply(op, out);
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
    if ( !mauthorized || maccessLevel < AdminLevel || login.isEmpty() || pwd.isEmpty() ||
         !bRange(NoLevel, AdminLevel).contains(lvl) || !beginDBOperation() || !execQuery(qs, 0, bv) )
        return retErr( op, out, tr("Adding user failed", "log text") );
    endDBOperation();
    out.insert("ok", true);
    sendReply(op, out);
}

void Connection::retOk(BNetworkOperation *op, const QVariantMap &out, const QString &msg)
{
    if (!op)
        return;
    endDBOperation(true);
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
