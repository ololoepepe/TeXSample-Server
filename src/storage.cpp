#include "storage.h"
#include "database.h"
#include "sqlqueryresult.h"
#include "global.h"
#include "application.h"
#include "terminaliohandler.h"

#include <TOperationResult>
#include <TUserInfo>
#include <TAccessLevel>
#include <TSampleInfo>
#include <TeXSample>
#include <TInviteInfo>
#include <TCompilationResult>
#include <TProject>
#include <BSmtpSender>
#include <BEmail>
#include <BGenericSocket>

#include <BDirTools>
#include <BeQt>
#include <BTerminalIOHandler>

#include <QString>
#include <QFileInfo>
#include <QMutex>
#include <QMutexLocker>
#include <QVariantMap>
#include <QVariant>
#include <QDateTime>
#include <QByteArray>
#include <QUuid>
#include <QDir>
#include <QStringList>
#include <QRegExp>
#include <QSettings>

#include <QDebug>

/*============================================================================
================================ Storage =====================================
============================================================================*/

/*============================== Static public methods =====================*/

TOperationResult Storage::invalidInstanceResult()
{
    return TOperationResult(tr("Invalid Storage instance", "errorString"));
}

TOperationResult Storage::invalidParametersResult()
{
    return TOperationResult(tr("Invalid parameters", "errorString"));
}

TOperationResult Storage::databaseErrorResult()
{
    return TOperationResult(tr("Database error", "errorString"));
}

TOperationResult Storage::queryErrorResult()
{
    return TOperationResult(tr("Query error", "errorString"));
}

TOperationResult Storage::fileSystemErrorResult()
{
    return TOperationResult(tr("File system error", "errorString"));
}

bool Storage::initStorage(QString *errs)
{
    static QMutex mutex;
    static bool isInit = false;
    QMutexLocker locker(&mutex);
    if (isInit)
        return true;
    QString sty = BDirTools::readTextFile(BDirTools::findResource("texsample-framework/texsample.sty",
                                                                  BDirTools::GlobalOnly), "UTF-8");
    if (sty.isEmpty())
        return bRet(errs, tr("Failed to load texsample.sty", "errorString"), false);
    QString tex = BDirTools::readTextFile(BDirTools::findResource("texsample-framework/texsample.tex",
                                                                  BDirTools::GlobalOnly), "UTF-8");
    if (tex.isEmpty())
        return bRet(errs, tr("Failed to load texsample.tex", "errorString"), false);
    Database db(QUuid::createUuid().toString(), rootDir() + "/texsample.sqlite");
    if (!db.beginDBOperation())
        return bRet(errs, tr("Database error", "errorString"), false);
    QStringList list = BDirTools::readTextFile(BDirTools::findResource("db/texsample.schema",
                                                                       BDirTools::GlobalOnly), "UTF-8").split(";\n");
    foreach (int i, bRangeD(0, list.size() - 1))
    {
        list[i].replace('\n', ' ');
        list[i].replace(QRegExp("\\s+"), " ");
    }
    list.removeAll("");
    list.removeDuplicates();
    if (list.isEmpty())
        return bRet(errs, tr("Failed to parse database schema", "errorString"), false);
    bool users = db.tableExists("users");
    SqlQueryResult qr;
    users = users && db.execQuery("SELECT id FROM users WHERE login=:login", qr, ":login", QString("root"));
    users = users && qr.value("id").toULongLong();
    foreach (const QString &qs, list)
    {
        if (!db.execQuery(qs))
        {
            db.endDBOperation(false);
            return bRet(errs, tr("Query error", "errorString"), false);
        }
    }
    if (!db.endDBOperation())
        return bRet(errs, tr("Database error", "errorString"), false);
    Storage s;
    if (!s.isValid())
        return bRet(errs, tr("Invalid storage instance", "errorString"), false);
    if (!s.testInvites() || !s.testRecoveryCodes())
        return bRet(errs, tr("Failed to test invites or recovery codes", "errorString"), false);
    if (!users)
    {
        QString mail = BTerminalIOHandler::readLine("\n" + tr("Enter root e-mail: ", ""));
        QString e = tr("Operation cancelled", "errorString");
        if (mail.isEmpty())
            return bRet(errs, e, false);
        BTerminalIOHandler::setStdinEchoEnabled(false);
        QString pwd = BTerminalIOHandler::readLine(tr("Enter root password: ", ""));
        BTerminalIOHandler::setStdinEchoEnabled(true);
        BTerminalIOHandler::writeLine();
        if (pwd.isEmpty())
            return bRet(errs, e, false);

        TUserInfo info(TUserInfo::AddContext);
        info.setLogin("root");
        info.setEmail(mail);
        info.setPassword(pwd);
        info.setAccessLevel(TAccessLevel::RootLevel);
        TOperationResult r = s.addUser(info);
        if (!r)
            return bRet(errs, r.errorString(), false);
    }
    mtexsampleSty = sty;
    mtexsampleTex = tex;
    isInit = true;
    return true;
}

bool Storage::copyTexsample(const QString &path, const QString &codecName)
{
    if (!QDir(path).exists() || mtexsampleSty.isEmpty() || mtexsampleTex.isEmpty())
        return false;
    QString cn = (!codecName.isEmpty() ? codecName : QString("UTF-8")).toLower();
    return BDirTools::writeTextFile(path + "/texsample.sty", mtexsampleSty, cn)
            && BDirTools::writeTextFile(path + "/texsample.tex", mtexsampleTex, cn);
}

bool Storage::removeTexsample(const QString &path)
{
    return BDirTools::removeFilesInDir(path, QStringList() << "texsample.sty" << "texsample.tex");
}

/*============================== Public constructors =======================*/

Storage::Storage()
{
    mdb = new Database(QUuid::createUuid().toString(),
                       BDirTools::findResource("texsample.sqlite", BDirTools::UserOnly));
    mdb->open();
}

Storage::~Storage()
{
    delete mdb;
}

/*============================== Public methods ============================*/

TOperationResult Storage::addUser(const TUserInfo &info, const QUuid &invite)
{
    if (!info.isValid(TUserInfo::AddContext))
        return invalidParametersResult();
    if (!isValid())
        return invalidInstanceResult();
    if (!isUserUnique(info.login(), info.email()))
        return TOperationResult(tr("These login or password are already in use", "errorString"));
    if (!mdb->beginDBOperation())
        return databaseErrorResult();
    QString qs = "INSERT INTO users (email, login, password, access_level, real_name, created_dt, modified_dt) "
                 "VALUES (:email, :login, :pwd, :alvl, :rname, :cr_dt, :mod_dt)";
    QVariantMap bv;
    QDateTime dt = QDateTime::currentDateTimeUtc();
    bv.insert(":email", info.email());
    bv.insert(":login", info.login());
    bv.insert(":pwd", info.password());
    bv.insert(":alvl", (int) info.accessLevel());
    bv.insert(":rname", info.realName());
    bv.insert(":cr_dt", dt.toMSecsSinceEpoch());
    bv.insert(":mod_dt", dt.toMSecsSinceEpoch());
    SqlQueryResult qr = mdb->execQuery(qs, bv);
    if (!qr || (!invite.isNull() && !mdb->execQuery("DELETE FROM invites WHERE uuid = :uuid",
                                                    ":uuid", BeQt::pureUuidText(invite))))
    {
        mdb->endDBOperation(false);
        return queryErrorResult();
    }
    quint64 userId = qr.insertId().toULongLong();
    if(!saveUserAvatar(userId, info.avatar()))
        return fileSystemErrorResult();
    Global::Host h;
    h.address = bSettings->value("Mail/server_address").toString();
    h.port = bSettings->value("Mail/server_port", h.port).toUInt();
    Global::User u;
    u.login = bSettings->value("Mail/login").toString();
    u.password = TerminalIOHandler::mailPassword();
    Global::Mail m;
    m.from = "TeXSample Team";
    m.to << info.email();
    m.subject = "TeXSample registration";
    QString fn = BDirTools::findResource("templates/registration", BDirTools::GlobalOnly) + "/registration.txt";
    m.body = BDirTools::readTextFile(BDirTools::localeBasedFileName(fn), "UTF-8").replace("%username%", info.login());
    TOperationResult r = Global::sendMail(h, u, m, bSettings->value("Mail/ssl_required").toBool());
    if (!r)
    {
        BDirTools::rmdir(rootDir() + "/users/" + QString::number(userId));
        return r;
    }
    if (!mdb->endDBOperation(true))
    {
        BDirTools::rmdir(rootDir() + "/users/" + QString::number(userId));
        return databaseErrorResult();
    }
    return TOperationResult(true);
}

TOperationResult Storage::editUser(const TUserInfo &info)
{
    if (!info.isValid(TUserInfo::EditContext))
        return invalidParametersResult();
    if (!isValid())
        return invalidInstanceResult();
    if (!mdb->beginDBOperation())
        return databaseErrorResult();
    QString qs = "UPDATE users SET real_name = :rname, access_level = :alvl, modified_dt = :mod_dt";
    QVariantMap bv;
    bv.insert(":id", info.id());
    bv.insert(":rname", info.realName());
    bv.insert(":alvl", (int) info.accessLevel());
    bv.insert(":mod_dt", QDateTime::currentDateTimeUtc().toMSecsSinceEpoch());
    if (!info.password().isEmpty())
    {
        qs += ", password = :pwd";
        bv.insert(":pwd", info.password());
    }
    qs += " WHERE id = :id";
    if (!mdb->execQuery(qs, bv))
    {
        mdb->endDBOperation(false);
        return queryErrorResult();
    }
    if(!saveUserAvatar(info.id(), info.avatar()))
        return fileSystemErrorResult();
    if (!mdb->endDBOperation(true))
        return databaseErrorResult();
    return TOperationResult(true);
}

TOperationResult Storage::getUserInfo(quint64 userId, TUserInfo &info, QDateTime &updateDT, bool &cacheOk)
{
    if (!userId)
        return invalidParametersResult();
    if (!isValid())
        return invalidInstanceResult();
    if (!mdb->beginDBOperation())
        return databaseErrorResult();
    QString qs = "SELECT login, access_level, real_name, created_dt, modified_dt FROM users WHERE id = :id";
    SqlQueryResult r = mdb->execQuery(qs, ":id", userId);
    if (!r)
    {
        mdb->endDBOperation(false);
        return queryErrorResult();
    }
    if (!mdb->endDBOperation(true))
        return databaseErrorResult();
    QVariantMap m = r.value();
    cacheOk = m.value("modified_dt").toLongLong() <= updateDT.toUTC().toMSecsSinceEpoch();
    updateDT = QDateTime::currentDateTimeUtc();
    if (cacheOk)
        return TOperationResult(true);
    bool ok = false;
    QByteArray avatar = loadUserAvatar(userId, &ok);
    if (!ok)
        return fileSystemErrorResult();
    info.setId(userId);
    info.setLogin(m.value("login").toString());
    info.setAccessLevel(m.value("access_level").toInt());
    info.setRealName(m.value("real_name").toString());
    info.setAvatar(avatar);
    info.setCreationDateTime(QDateTime::fromMSecsSinceEpoch(m.value("created_dt").toULongLong()));
    info.setModificationDateTime(QDateTime::fromMSecsSinceEpoch(m.value("modified_dt").toULongLong()));
    return TOperationResult(true);
}

TOperationResult Storage::getShortUserInfo(quint64 userId, TUserInfo &info)
{
    if (!userId)
        return invalidParametersResult();
    if (!isValid())
        return invalidInstanceResult();
    if (!mdb->beginDBOperation())
        return databaseErrorResult();
    SqlQueryResult r = mdb->execQuery("SELECT login, real_name FROM users WHERE id = :id", ":id", userId);
    if (!r)
    {
        mdb->endDBOperation(false);
        return queryErrorResult();
    }
    if (!mdb->endDBOperation(true))
        return databaseErrorResult();
    QVariantMap m = r.value();
    info.setContext(TUserInfo::ShortInfoContext);
    info.setId(userId);
    info.setLogin(m.value("login").toString());
    info.setRealName(m.value("real_name").toString());
    return TOperationResult(true);
}

TCompilationResult Storage::addSample(quint64 userId, TProject project, const TSampleInfo &info)
{
    if (!userId || !project.isValid() || !info.isValid(TSampleInfo::AddContext))
        return invalidParametersResult();
    if (!isValid())
        return invalidInstanceResult();
    if (!mdb->beginDBOperation())
        return databaseErrorResult();
    QString qs = "INSERT INTO samples (sender_id, title, file_name, authors, tags, comment, created_dt, modified_dt) "
                 "VALUES (:sender_id, :title, :file_name, :authors, :tags, :comment, :created_dt, :modified_dt)";
    QVariantMap bv;
    qint64 dt = QDateTime::currentDateTimeUtc().toMSecsSinceEpoch();
    bv.insert(":sender_id", userId);
    bv.insert(":title", info.title());
    bv.insert(":file_name", info.fileName());
    bv.insert(":authors", info.authorsString());
    bv.insert(":tags", info.tagsString());
    bv.insert(":comment", info.comment());
    bv.insert(":created_dt", dt);
    bv.insert(":modified_dt", dt);
    SqlQueryResult qr = mdb->execQuery(qs, bv);
    if (!qr)
    {
        mdb->endDBOperation(false);
        return queryErrorResult();
    }
    project.rootFile()->setFileName(info.fileName());
    project.removeRestrictedCommands();
    QString tpath = QDir::tempPath() + "/texsample-server/samples/" + BeQt::pureUuidText(QUuid::createUuid());
    TCompilationResult cr = Global::compileProject(tpath, project);
    if (!cr)
    {
        mdb->endDBOperation(false);
        BDirTools::rmdir(tpath);
        return cr;
    }
    QString spath = rootDir() + "/samples/" + QString::number(qr.insertId().toULongLong());
    if (!BDirTools::renameDir(tpath, spath) && (!BDirTools::copyDir(tpath, spath, true) || !BDirTools::rmdir(tpath)))
    {
        mdb->endDBOperation(false);
        BDirTools::rmdir(tpath);
        return fileSystemErrorResult();
    }
    if (!mdb->endDBOperation(true))
        return databaseErrorResult();
    return cr;
}

TCompilationResult Storage::editSample(const TSampleInfo &info, TProject project)
{
    if (!info.isValid(TSampleInfo::EditContext) && !info.isValid(TSampleInfo::UpdateContext))
        return invalidParametersResult();
    if (!isValid())
        return invalidInstanceResult();
    QString pfn = sampleFileName(info.id());
    if (pfn.isEmpty())
        return databaseErrorResult();
    if (!mdb->beginDBOperation())
        return databaseErrorResult();
    QString qs = "UPDATE samples SET title = :title, authors = :authors, tags = :tags, comment = :comment, "
                 "modified_dt = :modified_dt";
    QVariantMap bv;
    bv.insert(":title", info.title());
    bv.insert(":authors", info.authorsString());
    bv.insert(":tags", info.tagsString());
    bv.insert(":comment", info.comment());
    bv.insert(":modified_dt", QDateTime::currentDateTimeUtc().toMSecsSinceEpoch());
    bv.insert(":id", info.id());
    if (info.context() == TSampleInfo::EditContext)
    {
        qs += ", type = :type, rating = :rating, admin_remark = :adm_rem";
        bv.insert(":type", info.type());
        bv.insert(":rating", info.rating());
        bv.insert(":adm_rem", info.adminRemark());
    }
    if (pfn != info.fileName())
    {
        qs += ", file_name = :fname";
        bv.insert(":fname", info.fileName());
    }
    qs += " WHERE id = :id";
    SqlQueryResult qr = mdb->execQuery(qs, bv);
    if (!qr)
    {
        mdb->endDBOperation(false);
        return queryErrorResult();
    }
    QString spath = rootDir() + "/samples/" + QString::number(info.id());
    if (pfn != info.fileName() && !project.isValid())
    {
        QDir sdir(spath);
        QString pbn = QFileInfo(pfn).baseName();
        QString bn = QFileInfo(info.fileName()).baseName();
        foreach (const QString &fn, sdir.entryList(QStringList() << (pbn + ".*"), QDir::Files))
        {
            if (!QFile::rename(sdir.absoluteFilePath(fn), spath + "/" + bn + "." + QFileInfo(fn).suffix()))
            {
                mdb->endDBOperation(false);
                return fileSystemErrorResult();
            }
        }
    }
    TCompilationResult cr(true);
    if (project.isValid())
    {
        project.rootFile()->setFileName(info.fileName());
        project.removeRestrictedCommands();
        if (!BDirTools::rmdir(spath))
        {
            mdb->endDBOperation(false);
            return fileSystemErrorResult();
        }
        project.removeRestrictedCommands();
        QString tpath = QDir::tempPath() + "/texsample-server/samples/" + BeQt::pureUuidText(QUuid::createUuid());
        cr = Global::compileProject(tpath, project);
        if (!cr)
        {
            mdb->endDBOperation(false);
            BDirTools::rmdir(tpath);
            return cr;
        }
        if (!BDirTools::renameDir(tpath, spath) && (!BDirTools::copyDir(tpath, spath, true)
                                                    || !BDirTools::rmdir(tpath)))
        {
            mdb->endDBOperation(false);
            BDirTools::rmdir(tpath);
            return fileSystemErrorResult();
        }
    }
    if (!mdb->endDBOperation(true))
        return databaseErrorResult();
    return cr;
}

TOperationResult Storage::deleteSample(quint64 id, const QString &reason)
{
    if (!id)
        return invalidParametersResult();
    if (!isValid())
        return invalidInstanceResult();
    quint64 sId = senderId(id);
    QDateTime createdDT = sampleCreationDateTime(id);
    if (!sId)
        return databaseErrorResult();
    if (!mdb->beginDBOperation())
        return databaseErrorResult();
    if (!mdb->execQuery("DELETE FROM samples WHERE id = :id", ":id", id))
    {
        mdb->endDBOperation(false);
        return queryErrorResult();
    }
    QString qs = "INSERT INTO deleted_samples (id, sender_id, reason, created_dt, deleted_dt) "
                 "VALUES (:id, :sender_id, :reason, :created_dt, :deleted_dt)";
    QVariantMap bv;
    bv.insert(":id", id);
    bv.insert(":sender_id", sId);
    bv.insert(":reason", reason);
    bv.insert(":created_dt", createdDT.toMSecsSinceEpoch());
    bv.insert(":deleted_dt", QDateTime::currentDateTimeUtc().toMSecsSinceEpoch());
    if (!mdb->execQuery(qs, bv))
    {
        mdb->endDBOperation(false);
        return queryErrorResult();
    }
    if (!BDirTools::rmdir(rootDir() + "/samples/" + QString::number(id)))
    {
        mdb->endDBOperation(false);
        return fileSystemErrorResult();
    }
    if (!mdb->endDBOperation(true))
        return databaseErrorResult();
    return TOperationResult(true);
}

TOperationResult Storage::getSampleSource(quint64 id, TProject &project, QDateTime &updateDT, bool &cacheOk)
{
    if (!id)
        return invalidParametersResult();
    if (!isValid())
        return invalidInstanceResult();
    if (updateDT.toUTC() >= sampleModificationDateTime(id))
    {
        cacheOk = true;
        return TOperationResult(true);
    }
    QString fn = sampleFileName(id);
    if (fn.isEmpty())
        return TOperationResult(tr("No such sample", "errorString")); //TODO
    updateDT = QDateTime::currentDateTimeUtc();
    return TOperationResult(project.load(rootDir() + "/samples/" + QString::number(id) + "/" + fn, "UTF-8"));
}

TOperationResult Storage::getSamplePreview(quint64 id, TProjectFile &file, QDateTime &updateDT, bool &cacheOk)
{
    if (!id)
        return invalidParametersResult();
    if (!isValid())
        return invalidInstanceResult();
    if (updateDT.toUTC() >= sampleModificationDateTime(id))
    {
        cacheOk = true;
        return TOperationResult(true);
    }
    QString fn = sampleFileName(id);
    if (fn.isEmpty())
        return TOperationResult(tr("No such sample", "errorString")); //TODO
    updateDT = QDateTime::currentDateTimeUtc();
    fn = rootDir() + "/samples/" + QString::number(id) + "/" + QFileInfo(fn).baseName() + ".pdf";
    return TOperationResult(file.loadAsBinary(fn, ""));
}

TOperationResult Storage::getSamplesList(TSampleInfo::SamplesList &newSamples, Texsample::IdList &deletedSamples,
                                         QDateTime &updateDT)
{
    if (!isValid())
        return invalidInstanceResult();
    if (!mdb->beginDBOperation())
        return databaseErrorResult();
    QString qsNew = "SELECT id, sender_id, authors, title, file_name, type, tags, rating, comment, "
                    "admin_remark, created_dt, modified_dt FROM samples WHERE modified_dt >= :update_dt";
    QString qsDeleted = "SELECT id, created_dt FROM deleted_samples "
                        "WHERE deleted_dt >= :update_dt AND created_dt < :update_dt_hack";
    SqlQueryResult rNew = mdb->execQuery(qsNew, ":update_dt", updateDT.toMSecsSinceEpoch());
    if (!rNew)
    {
        mdb->endDBOperation(false);
        return queryErrorResult();
    }
    SqlQueryResult rDeleted = mdb->execQuery(qsDeleted, ":update_dt", updateDT.toMSecsSinceEpoch(),
                                             ":update_dt_hack", updateDT.toMSecsSinceEpoch());
    if (!rDeleted)
    {
        mdb->endDBOperation(false);
        return queryErrorResult();
    }
    if (!mdb->endDBOperation(true))
        return databaseErrorResult();
    updateDT = QDateTime::currentDateTimeUtc();
    newSamples.clear();
    deletedSamples.clear();
    foreach (const QVariant &v, rNew.values())
    {
        QVariantMap m = v.toMap();
        TSampleInfo info;
        info.setId(m.value("id").toULongLong());
        TUserInfo uinfo;
        TOperationResult ur = getShortUserInfo(m.value("sender_id").toULongLong(), uinfo);
        if (!ur)
            return ur;
        info.setSender(uinfo);
        info.setAuthors(m.value("authors").toString());
        info.setTitle(m.value("title").toString());
        info.setFileName(m.value("file_name").toString());
        info.setType(m.value("type").toInt());
        info.setProjectSize(TProject::size(rootDir() + "/samples/" + info.idString() + "/" + info.fileName(),
                                           "UTF-8"));
        info.setTags(m.value("tags").toString());
        info.setRating(m.value("rating").toUInt());
        info.setComment(m.value("comment").toString());
        info.setAdminRemark(m.value("admin_remark").toString());
        info.setCreationDateTime(QDateTime::fromMSecsSinceEpoch(m.value("created_dt").toLongLong()));
        info.setModificationDateTime(QDateTime::fromMSecsSinceEpoch(m.value("modified_dt").toLongLong()));
        newSamples << info;
    }
    foreach (const QVariant &v, rDeleted.values())
        deletedSamples << v.toMap().value("id").toULongLong();
    return TOperationResult(true);
}

TOperationResult Storage::generateInvites(quint64 userId, const QDateTime &expiresDT, quint8 count,
                                          TInviteInfo::InvitesList &invites)
{
    if (!userId || !expiresDT.isValid() || !count || count > Texsample::MaximumInvitesCount)
        return invalidParametersResult();
    if (!isValid())
        return invalidInstanceResult();
    if (!mdb->beginDBOperation())
        return databaseErrorResult();
    invites.clear();
    QString qs = "INSERT INTO invites (uuid, creator_id, expires_dt, created_dt) "
                 "VALUES (:uuid, :user, :exp_dt, :cr_dt)";
    QVariantMap bv;
    bv.insert(":user", userId);
    bv.insert(":exp_dt", expiresDT.toUTC().toMSecsSinceEpoch());
    TInviteInfo info;
    info.setCreatorId(userId);
    info.setExpirationDateTime(expiresDT);
    quint8 i = 0;
    while (i < count)
    {
        QUuid uuid = QUuid::createUuid();
        QDateTime createdDT = QDateTime::currentDateTimeUtc();
        bv.insert(":uuid", BeQt::pureUuidText(uuid));
        bv.insert(":cr_dt", createdDT.toMSecsSinceEpoch());
        SqlQueryResult r = mdb->execQuery(qs, bv);
        if (!r)
        {
            mdb->endDBOperation(false);
            return queryErrorResult();
        }
        info.setId(r.insertId().toULongLong());
        info.setUuid(uuid);
        info.setCreationDateTime(createdDT);
        invites << info;
        ++i;
        bApp->scheduleInvitesAutoTest(info);
    }
    if (!mdb->endDBOperation(true))
        return databaseErrorResult();
    return TOperationResult(true);
}

TOperationResult Storage::getInvitesList(quint64 userId, TInviteInfo::InvitesList &invites)
{
    if (!userId)
        return invalidParametersResult();
    if (!isValid())
        return invalidInstanceResult();
    if (!mdb->beginDBOperation())
        return databaseErrorResult();
    QString qs = "SELECT id, uuid, creator_id, expires_dt, created_dt FROM invites WHERE creator_id = :cr_id";
    SqlQueryResult r = mdb->execQuery(qs, ":cr_id", userId);
    if (!r)
    {
        mdb->endDBOperation(false);
        return queryErrorResult();
    }
    if (!mdb->endDBOperation(true))
        return databaseErrorResult();
    foreach (const QVariant &v, r.values())
    {
        QVariantMap m = v.toMap();
        TInviteInfo info;
        info.setId(m.value("id").toULongLong());
        info.setUuid(m.value("uuid").toString());
        info.setCreatorId(userId);
        info.setExpirationDateTime(QDateTime::fromMSecsSinceEpoch(m.value("expires_dt").toULongLong()));
        info.setCreationDateTime(QDateTime::fromMSecsSinceEpoch(m.value("created_dt").toULongLong()));
        invites << info;
    }
    return TOperationResult(true);
}

TOperationResult Storage::getRecoveryCode(quint64 userId)
{
    if (!userId)
        return invalidParametersResult();
    if (!isValid())
        return invalidInstanceResult();
    if (!mdb->beginDBOperation())
        return databaseErrorResult();
    QString qs = "INSERT INTO recovery_codes (uuid, user_id, expires_dt, created_dt) "
                 "VALUES (:uuid, :user, :exp_dt, :cr_dt)";
    QVariantMap bv;
    QDateTime dt = QDateTime::currentDateTimeUtc();
    QString uuid = BeQt::pureUuidText(QUuid::createUuid());
    bv.insert(":user", userId);
    bv.insert(":exp_dt", dt.addDays(1).toMSecsSinceEpoch());
    bv.insert(":cr_dt", dt.toMSecsSinceEpoch());
    bv.insert(":uuid", uuid);
    SqlQueryResult r = mdb->execQuery(qs, bv);
    if (!r)
    {
        mdb->endDBOperation(false);
        return queryErrorResult();
    }
    bApp->scheduleRecoveryCodesAutoTest(dt);
    BSmtpSender smtp;
    smtp.setServer(bSettings->value("Mail/server_address").toString(),
                   bSettings->value("Mail/server_port", 25).toUInt());
    smtp.setLocalHostName("texsample-server.no-ip.org");
    smtp.setSocketType(bSettings->value("Mail/ssl_required").toBool() ? BGenericSocket::SslSocket :
                                                                        BGenericSocket::TcpSocket);
    smtp.setUser(bSettings->value("Mail/login").toString(), TerminalIOHandler::mailPassword());
    BEmail email;
    email.setSender("TeXSample Team");
    email.setReceiver(userEmail(userId));
    email.setSubject("TeXSample password recovery");
    QString fn = BDirTools::findResource("templates/recovery", BDirTools::GlobalOnly) + "/get.txt";
    email.setBody(BDirTools::readTextFile(BDirTools::localeBasedFileName(fn), "UTF-8").replace("%code%", uuid));
    smtp.waitForFinished();
    return TOperationResult(true);
}

TOperationResult Storage::recoverPassword(quint64 userId, const QUuid &code, const QByteArray &password)
{
    if (!userId || code.isNull() || password.isEmpty())
        return invalidParametersResult();
    if (!isValid())
        return invalidInstanceResult();
    if (!mdb->beginDBOperation())
        return databaseErrorResult();
    SqlQueryResult r = mdb->execQuery("SELECT id FROM recovery_codes WHERE user_id = :user AND uuid = :uuid",
                                      ":user", userId, ":uuid", BeQt::pureUuidText(code));
    quint64 rcid = r.value("id").toULongLong();
    if (!r || !rcid)
    {
        mdb->endDBOperation(false);
        return queryErrorResult();
    }
    if (!mdb->execQuery("UPDATE users SET password = :password WHERE id = :id", ":password", password, ":id", userId))
    {
        mdb->endDBOperation(false);
        return queryErrorResult();
    }
    if (!mdb->execQuery("DELETE FROM recovery_codes WHERE id = :id", ":id", rcid))
    {
        mdb->endDBOperation(false);
        return queryErrorResult();
    }
    if (!mdb->endDBOperation(true))
        return databaseErrorResult();
    BSmtpSender smtp;
    smtp.setServer(bSettings->value("Mail/server_address").toString(),
                   bSettings->value("Mail/server_port", 25).toUInt());
    smtp.setLocalHostName("texsample-server.no-ip.org");
    smtp.setSocketType(bSettings->value("Mail/ssl_required").toBool() ? BGenericSocket::SslSocket :
                                                                        BGenericSocket::TcpSocket);
    smtp.setUser(bSettings->value("Mail/login").toString(), TerminalIOHandler::mailPassword());
    BEmail email;
    email.setSender("TeXSample Team");
    email.setReceiver(userEmail(userId));
    email.setSubject("TeXSample password recovered");
    QString fn = BDirTools::findResource("templates/recovery", BDirTools::GlobalOnly) + "/recovered.txt";
    email.setBody(BDirTools::readTextFile(BDirTools::localeBasedFileName(fn), "UTF-8"));
    smtp.waitForFinished();
    return TOperationResult(true);
}

bool Storage::isUserUnique(const QString &login, const QString &email)
{
    if (login.isEmpty() || email.isEmpty() || !isValid())
        return false;
    SqlQueryResult r1 = mdb->execQuery("SELECT id FROM users WHERE login = :login", ":login", login);
    SqlQueryResult r2 = mdb->execQuery("SELECT id FROM users WHERE email = :email", ":email", login);
    return r1 && r2 && !r1.value("id").toULongLong() && !r2.value("id").toULongLong();
}

quint64 Storage::userId(const QString &login, const QByteArray &password)
{
    if (login.isEmpty() || !isValid())
        return 0;
    QString qs = "SELECT id FROM users WHERE login = :login";
    bool b = !password.isEmpty();
    if (b)
        qs += " AND password = :pwd";
    SqlQueryResult r = mdb->execQuery(qs, ":login", login, b ? ":pwd" : "", b ? QVariant(password) : QVariant());
    return r ? r.value("id").toULongLong() : 0;
}

quint64 Storage::senderId(quint64 sampleId)
{
    if (!sampleId || !isValid())
        return 0;
    SqlQueryResult r = mdb->execQuery("SELECT sender_id FROM samples WHERE id = :id", ":id", sampleId);
    return r.value("sender_id").toULongLong();
}

QString Storage::userEmail(quint64 userId)
{
    if (!userId || !isValid())
        return "";
    SqlQueryResult r = mdb->execQuery("SELECT email FROM users WHERE id = :id", ":id", userId);
    return r.value("email").toString();
}

TSampleInfo::Type Storage::sampleType(quint64 id)
{
    if (!id || !isValid())
        return TSampleInfo::Unverified;
    SqlQueryResult r = mdb->execQuery("SELECT type FROM samples WHERE id = :id", ":id", id);
    return TSampleInfo::typeFromInt(r.value("type").toInt());
}

QString Storage::sampleFileName(quint64 id)
{
    if (!id || !isValid())
        return "";
    SqlQueryResult r = mdb->execQuery("SELECT file_name FROM samples WHERE id = :id", ":id", id);
    return r.value("file_name").toString();
}

QDateTime Storage::sampleCreationDateTime(quint64 id, Qt::TimeSpec spec)
{
    if (!id || !isValid())
        return QDateTime().toUTC();
    SqlQueryResult r = mdb->execQuery("SELECT created_dt FROM samples WHERE id = :id", ":id", id);
    return QDateTime::fromMSecsSinceEpoch(r.value("created_dt").toLongLong()).toTimeSpec(spec);
}

QDateTime Storage::sampleModificationDateTime(quint64 id, Qt::TimeSpec spec)
{
    if (!id || !isValid())
        return QDateTime().toUTC();
    SqlQueryResult r = mdb->execQuery("SELECT modified_dt FROM samples WHERE id = :id", ":id", id);
    return QDateTime::fromMSecsSinceEpoch(r.value("modified_dt").toLongLong()).toTimeSpec(spec);
}

TAccessLevel Storage::userAccessLevel(quint64 userId)
{
    if (!userId || !isValid())
        return TAccessLevel();
    SqlQueryResult r = mdb->execQuery("SELECT access_level FROM users WHERE id = :id", ":id", userId);
    return r ? r.value("access_level").toInt() : 0;
}

quint64 Storage::inviteId(const QUuid &invite)
{
    if (invite.isNull() || !isValid())
        return 0;
    QString qs = "SELECT id FROM invites WHERE uuid = :uuid AND expires_dt > :exp_dt";
    QDateTime dt = QDateTime::currentDateTimeUtc();
    SqlQueryResult r = mdb->execQuery(qs, ":uuid", BeQt::pureUuidText(invite), ":exp_dt", dt.toMSecsSinceEpoch());
    return r ? r.value("id").toULongLong() : 0;
}

quint64 Storage::inviteId(const QString &inviteCode)
{
    return inviteId(BeQt::uuidFromText(inviteCode));
}

bool Storage::isValid() const
{
    return QFileInfo(rootDir()).isDir() && mdb && mdb->isOpen();
}

bool Storage::testInvites()
{
    if (!isValid() || !mdb->beginDBOperation())
        return false;
    bool b = mdb->execQuery("DELETE FROM invites WHERE expires_dt <= :current_dt", ":current_dt",
                            QDateTime::currentDateTimeUtc().toMSecsSinceEpoch());
    return mdb->endDBOperation(b) && b;
}

bool Storage::testRecoveryCodes()
{
    if (!isValid() || !mdb->beginDBOperation())
        return false;
    bool b = mdb->execQuery("DELETE FROM recovery_codes WHERE expires_dt <= :current_dt", ":current_dt",
                            QDateTime::currentDateTimeUtc().toMSecsSinceEpoch());
    return mdb->endDBOperation(b) && b;
}

/*============================== Static private methods ====================*/

QString Storage::rootDir()
{
    return BCoreApplication::location(BCoreApplication::DataPath, BCoreApplication::UserResources);
}

/*============================== Private methods ===========================*/

bool Storage::saveUserAvatar(quint64 userId, const QByteArray &data) const
{
    if (!isValid() || !userId)
        return false;
    return BDirTools::writeFile(rootDir() + "/users/" + QString::number(userId) + "/avatar.dat", data);
}

QByteArray Storage::loadUserAvatar(quint64 userId, bool *ok) const
{
    if (!isValid() || !userId)
        return bRet(ok, false, QByteArray());
    return BDirTools::readFile(rootDir() + "/users/" + QString::number(userId) + "/avatar.dat", -1, ok);
}

/*============================== Static private members ====================*/

QString Storage::mtexsampleSty;
QString Storage::mtexsampleTex;
