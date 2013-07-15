#include "storage.h"
#include "global.h"
#include "application.h"
#include "terminaliohandler.h"
#include "translator.h"

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
#include <BSqlDatabase>
#include <BSqlQuery>
#include <BSqlWhere>
#include <BSqlResult>

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

bool Storage::initStorage(QString *errs)
{
    static QMutex mutex;
    static bool isInit = false;
    QMutexLocker locker(&mutex);
    if (isInit)
        return true;
    Translator t(BCoreApplication::locale());
    QString sty = BDirTools::readTextFile(BDirTools::findResource("texsample-framework/texsample.sty",
                                                                  BDirTools::GlobalOnly), "UTF-8");
    if (sty.isEmpty())
        return bRet(errs, tr("Failed to load texsample.sty", "error"), false);
    QString tex = BDirTools::readTextFile(BDirTools::findResource("texsample-framework/texsample.tex",
                                                                  BDirTools::GlobalOnly), "UTF-8");
    if (tex.isEmpty())
        return bRet(errs, tr("Failed to load texsample.tex", "error"), false);
    BSqlDatabase db("QSQLITE", QUuid::createUuid().toString());
    db.setDatabaseName(rootDir() + "/texsample.sqlite");
    db.setOnOpenQuery("PRAGMA foreign_keys = ON");
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
        return bRet(errs, tr("Failed to parse database schema", "error"), false);
    bool users = !db.select("users", "id", BSqlWhere("login = :login", ":login", QString("root"))).values().isEmpty();
    foreach (const QString &qs, list)
    {
        if (!db.transaction())
            return bRet(errs, Global::string(Global::DatabaseError, &t), false);
        if (!db.exec(qs))
        {
            db.rollback();
            return bRet(errs, Global::string(Global::QueryError, &t), false);
        }
        if (!db.commit())
            return bRet(errs, Global::string(Global::DatabaseError, &t), false);
    }
    Storage s;
    if (!s.isValid())
        return bRet(errs, Global::string(Global::StorageError, &t), false);
    if (!s.testInvites() || !s.testRecoveryCodes())
        return bRet(errs, tr("Failed to test invites or recovery codes", "error"), false);
    if (!users)
    {
        QString mail = BTerminalIOHandler::readLine("\n" + tr("Enter root e-mail:") + " ");
        QString e = tr("Operation cancelled", "error");
        if (mail.isEmpty())
            return bRet(errs, e, false);
        BTerminalIOHandler::setStdinEchoEnabled(false);
        QString pwd = BTerminalIOHandler::readLine(tr("Enter root password:") + " ");
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

Storage::Storage(Translator *t)
{
    mdb = new BSqlDatabase("QSQLITE", QUuid::createUuid().toString());
    mdb->setDatabaseName(BDirTools::findResource("texsample.sqlite", BDirTools::UserOnly));
    mdb->setOnOpenQuery("PRAGMA foreign_keys = ON");
    mdb->open();
    mtranslator = t;
}

Storage::~Storage()
{
    delete mdb;
}

/*============================== Public methods ============================*/

void Storage::setTranslator(Translator *t)
{
    mtranslator = t;
}

TOperationResult Storage::addUser(const TUserInfo &info, Translator *t, const QUuid &invite)
{
    if (!info.isValid(TUserInfo::AddContext))
        return Global::result(Global::InvalidParameters, mtranslator);
    if (!isValid())
        return Global::result(Global::StorageError, mtranslator);
    if (!isUserUnique(info.login(), info.email()))
        return Global::result(Global::LoginOrEmailOccupied, mtranslator);
    if (!mdb->transaction())
        return Global::result(Global::DatabaseError, mtranslator);
    qint64 msecs = QDateTime::currentDateTimeUtc().toMSecsSinceEpoch();
    QVariantMap m;
    m.insert("email", info.email());
    m.insert("login", info.login());
    m.insert("password", info.password());
    m.insert("access_level", (int) info.accessLevel());
    m.insert("real_name", info.realName());
    m.insert("created_dt", msecs);
    m.insert("modified_dt", msecs);
    BSqlResult qr = mdb->insert("users", m);
    if (!qr || (!invite.isNull() && !mdb->deleteFrom("invites", BSqlWhere("uuid = :uuid", ":uuid",
                                                                          BeQt::pureUuidText(invite)))))
    {
        mdb->rollback();
        return Global::result(Global::QueryError, mtranslator);
    }
    quint64 userId = qr.lastInsertId().toULongLong();
    if(!saveUserAvatar(userId, info.avatar()))
        return Global::result(Global::FileSystemError, mtranslator);
    if (!mdb->commit())
    {
        BDirTools::rmdir(rootDir() + "/users/" + QString::number(userId));
        return Global::result(Global::DatabaseError, mtranslator);
    }
    if (info.login() != "root")
    {
        QString fn = BDirTools::findResource("templates/registration", BDirTools::GlobalOnly) + "/registration.txt";
        QString text = BDirTools::readTextFile(BDirTools::localeBasedFileName(fn), "UTF-8");
        text.replace("%username%", info.login());
        QString s = t ? t->translate("Storage", "TeXSample registration") : QString("TeXSample registration");
        Global::sendEmail(info.email(), s, text, mtranslator);
    }
    return TOperationResult(true);
}

TOperationResult Storage::editUser(const TUserInfo &info)
{
    if (!info.isValid(TUserInfo::EditContext))
        return Global::result(Global::InvalidParameters, mtranslator);
    if (!isValid())
        return Global::result(Global::StorageError, mtranslator);
    if (!mdb->transaction())
        return Global::result(Global::DatabaseError, mtranslator);
    QVariantMap m;
    m.insert("real_name", info.realName());
    m.insert("access_level", (int) info.accessLevel());
    m.insert("modified_dt", QDateTime::currentDateTimeUtc().toMSecsSinceEpoch());
    if (!info.password().isEmpty())
        m.insert("password", info.password());
    if (!mdb->update("users", m, BSqlWhere("id = :id", ":id", info.id())))
    {
        mdb->rollback();
        return Global::result(Global::QueryError, mtranslator);
    }
    if(!saveUserAvatar(info.id(), info.avatar()))
        return Global::result(Global::FileSystemError, mtranslator);
    if (!mdb->commit())
        return Global::result(Global::DatabaseError, mtranslator);
    return TOperationResult(true);
}

TOperationResult Storage::getUserInfo(quint64 userId, TUserInfo &info, QDateTime &updateDT, bool &cacheOk)
{
    if (!userId)
        return Global::result(Global::InvalidParameters, mtranslator);
    if (!isValid())
        return Global::result(Global::StorageError, mtranslator);
    QStringList sl = QStringList() << "login" << "access_level" << "real_name" << "created_dt" << "modified_dt";
    BSqlResult r = mdb->select("users", sl, BSqlWhere("id = :id", ":id", userId));
    if (!r)
        return Global::result(Global::QueryError, mtranslator);
    cacheOk = r.value("modified_dt").toLongLong() <= updateDT.toUTC().toMSecsSinceEpoch();
    updateDT = QDateTime::currentDateTimeUtc();
    if (cacheOk)
        return TOperationResult(true);
    bool ok = false;
    QByteArray avatar = loadUserAvatar(userId, &ok);
    if (!ok)
        return Global::result(Global::FileSystemError, mtranslator);
    info.setId(userId);
    info.setLogin(r.value("login").toString());
    info.setAccessLevel(r.value("access_level").toInt());
    info.setRealName(r.value("real_name").toString());
    info.setAvatar(avatar);
    info.setCreationDateTime(QDateTime::fromMSecsSinceEpoch(r.value("created_dt").toULongLong()));
    info.setModificationDateTime(QDateTime::fromMSecsSinceEpoch(r.value("modified_dt").toULongLong()));
    return TOperationResult(true);
}

TOperationResult Storage::getShortUserInfo(quint64 userId, TUserInfo &info)
{
    if (!userId)
        return Global::result(Global::InvalidParameters, mtranslator);
    if (!isValid())
        return Global::result(Global::StorageError, mtranslator);
    BSqlResult r = mdb->select("users", QStringList() << "login" << "real_name", BSqlWhere("id = :id", ":id", userId));
    if (!r)
        return Global::result(Global::QueryError, mtranslator);
    info.setContext(TUserInfo::ShortInfoContext);
    info.setId(userId);
    info.setLogin(r.value("login").toString());
    info.setRealName(r.value("real_name").toString());
    return TOperationResult(true);
}

TCompilationResult Storage::addSample(quint64 userId, TProject project, const TSampleInfo &info)
{
    if (!userId || !project.isValid() || !info.isValid(TSampleInfo::AddContext))
        return Global::result(Global::InvalidParameters, mtranslator);
    if (!isValid())
        return Global::result(Global::StorageError, mtranslator);
    if (!mdb->transaction())
        return Global::result(Global::DatabaseError, mtranslator);
    qint64 msecs = QDateTime::currentDateTimeUtc().toMSecsSinceEpoch();
    QVariantMap m;
    m.insert("sender_id", userId);
    m.insert("title", info.title());
    m.insert("file_name", info.fileName());
    m.insert("authors", info.authorsString());
    m.insert("tags", info.tagsString());
    m.insert("comment", info.comment());
    m.insert("created_dt", msecs);
    m.insert("modified_dt", msecs);
    BSqlResult qr = mdb->insert("samples", m);
    if (!qr)
    {
        mdb->rollback();
        return Global::result(Global::QueryError, mtranslator);
    }
    project.rootFile()->setFileName(info.fileName());
    project.removeRestrictedCommands();
    Global::CompileParameters p;
    p.project = project;
    p.path = QDir::tempPath() + "/texsample-server/samples/" + BeQt::pureUuidText(QUuid::createUuid());
    TCompilationResult cr = Global::compileProject(p);
    if (!cr)
    {
        mdb->rollback();
        BDirTools::rmdir(p.path);
        return cr;
    }
    QString spath = rootDir() + "/samples/" + QString::number(qr.lastInsertId().toULongLong());
    if (!BDirTools::renameDir(p.path, spath) && (!BDirTools::copyDir(p.path, spath, true)
                                                 || !BDirTools::rmdir(p.path)))
    {
        mdb->rollback();
        BDirTools::rmdir(p.path);
        return Global::result(Global::FileSystemError, mtranslator);
    }
    if (!mdb->commit())
        return Global::result(Global::DatabaseError, mtranslator);
    return cr;
}

TCompilationResult Storage::editSample(const TSampleInfo &info, TProject project)
{
    if (!info.isValid(TSampleInfo::EditContext) && !info.isValid(TSampleInfo::UpdateContext))
        return Global::result(Global::InvalidParameters, mtranslator);
    if (!isValid())
        return Global::result(Global::StorageError, mtranslator);
    QString pfn = sampleFileName(info.id());
    if (pfn.isEmpty() || !mdb->transaction())
        return Global::result(Global::DatabaseError, mtranslator);
    QVariantMap m;
    m.insert("title", info.title());
    m.insert("authors", info.authorsString());
    m.insert("tags", info.tagsString());
    m.insert("comment", info.comment());
    m.insert("modified_dt", QDateTime::currentDateTimeUtc().toMSecsSinceEpoch());
    if (info.context() == TSampleInfo::EditContext)
    {
        m.insert("type", info.type());
        m.insert("rating", info.rating());
        m.insert("admin_remark", info.adminRemark());
    }
    if (pfn != info.fileName())
        m.insert("file_name", info.fileName());
    BSqlResult qr = mdb->update("samples", m, BSqlWhere("id = :id", ":id", info.id()));
    if (!qr)
    {
        mdb->rollback();
        return Global::result(Global::QueryError, mtranslator);
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
                mdb->rollback();
                return Global::result(Global::FileSystemError, mtranslator);
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
            mdb->rollback();
            return Global::result(Global::FileSystemError, mtranslator);
        }
        project.removeRestrictedCommands();
        Global::CompileParameters p;
        p.project = project;
        p.path = QDir::tempPath() + "/texsample-server/samples/" + BeQt::pureUuidText(QUuid::createUuid());
        cr = Global::compileProject(p);
        if (!cr)
        {
            mdb->rollback();
            BDirTools::rmdir(p.path);
            return cr;
        }
        if (!BDirTools::renameDir(p.path, spath) && (!BDirTools::copyDir(p.path, spath, true)
                                                    || !BDirTools::rmdir(p.path)))
        {
            mdb->rollback();
            BDirTools::rmdir(p.path);
            return Global::result(Global::FileSystemError, mtranslator);
        }
    }
    if (!mdb->commit())
        return Global::result(Global::DatabaseError, mtranslator);
    return cr;
}

TOperationResult Storage::deleteSample(quint64 sampleId, const QString &reason)
{
    if (!sampleId)
        return Global::result(Global::InvalidParameters, mtranslator);
    if (!isValid())
        return Global::result(Global::StorageError, mtranslator);
    quint64 userId = senderId(sampleId);
    qint64 createdMsecs = sampleCreationDateTime(sampleId).toMSecsSinceEpoch();
    if (!userId || !mdb->transaction())
        return Global::result(Global::DatabaseError, mtranslator);
    if (!mdb->deleteFrom("samples", BSqlWhere("id = :id", ":id", sampleId)))
    {
        mdb->rollback();
        return Global::result(Global::QueryError, mtranslator);
    }
    QVariantMap m;
    m.insert("id", sampleId);
    m.insert("sender_id", userId);
    m.insert("reason", reason);
    m.insert("created_dt", createdMsecs);
    m.insert("deleted_dt", QDateTime::currentDateTimeUtc().toMSecsSinceEpoch());
    if (!mdb->insert("deleted_samples", m))
    {
        mdb->rollback();
        return Global::result(Global::QueryError, mtranslator);
    }
    if (!BDirTools::rmdir(rootDir() + "/samples/" + QString::number(sampleId)))
    {
        mdb->rollback();
        return Global::result(Global::FileSystemError, mtranslator);
    }
    if (!mdb->commit())
        return Global::result(Global::DatabaseError, mtranslator);
    return TOperationResult(true);
}

TOperationResult Storage::getSampleSource(quint64 sampleId, TProject &project, QDateTime &updateDT, bool &cacheOk)
{
    if (!sampleId)
        return Global::result(Global::InvalidParameters, mtranslator);
    if (!isValid())
        return Global::result(Global::StorageError, mtranslator);
    if (updateDT.toUTC() >= sampleModificationDateTime(sampleId))
    {
        cacheOk = true;
        return TOperationResult(true);
    }
    QString fn = sampleFileName(sampleId);
    if (fn.isEmpty())
        return Global::result(Global::NoSuchSample, mtranslator);
    updateDT = QDateTime::currentDateTimeUtc();
    return TOperationResult(project.load(rootDir() + "/samples/" + QString::number(sampleId) + "/" + fn, "UTF-8"));
}

TOperationResult Storage::getSamplePreview(quint64 sampleId, TProjectFile &file, QDateTime &updateDT, bool &cacheOk)
{
    if (!sampleId)
        return Global::result(Global::InvalidParameters, mtranslator);
    if (!isValid())
        return Global::result(Global::StorageError, mtranslator);
    if (updateDT.toUTC() >= sampleModificationDateTime(sampleId))
    {
        cacheOk = true;
        return TOperationResult(true);
    }
    QString fn = sampleFileName(sampleId);
    if (fn.isEmpty())
        return Global::result(Global::NoSuchSample, mtranslator);
    updateDT = QDateTime::currentDateTimeUtc();
    fn = rootDir() + "/samples/" + QString::number(sampleId) + "/" + QFileInfo(fn).baseName() + ".pdf";
    return TOperationResult(file.loadAsBinary(fn, ""));
}

TOperationResult Storage::getSamplesList(TSampleInfo::SamplesList &newSamples, Texsample::IdList &deletedSamples,
                                         QDateTime &updateDT, bool hack)
{
    if (!isValid())
        return Global::result(Global::InvalidParameters, mtranslator);
    qint64 updateMsecs = updateDT.toUTC().toMSecsSinceEpoch();
    QStringList sl1 = QStringList() << "id" << "sender_id" << "authors" << "title" << "file_name" << "type" << "tags"
                                    << "rating" << "admin_remark" << "created_dt" << "modified_dt";
    BSqlResult r1 = mdb->select("samples", sl1, BSqlWhere("modified_dt >= :update_dt", ":update_dt", updateMsecs));
    if (!r1)
        return Global::result(Global::QueryError, mtranslator);
    BSqlWhere w2("deleted_dt >= :update_dt AND created_dt < :update_dt_hack", ":update_dt", updateMsecs,
                 ":update_dt_hack", updateMsecs);
    BSqlResult r2 = mdb->select("deleted_samples", QStringList() << "id" << "created_dt", w2);
    if (!r2)
        return Global::result(Global::QueryError, mtranslator);
    updateDT = QDateTime::currentDateTimeUtc();
    newSamples.clear();
    deletedSamples.clear();
    foreach (const QVariantMap &m, r1.values())
    {
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
        int sz = -1; //HACK: Must fix
        if (!hack)
            sz = TProject::size(rootDir() + "/samples/" + info.idString() + "/" + info.fileName(), "UTF-8");
        info.setProjectSize(sz);
        info.setTags(m.value("tags").toString());
        info.setRating(m.value("rating").toUInt());
        info.setComment(m.value("comment").toString());
        info.setAdminRemark(m.value("admin_remark").toString());
        info.setCreationDateTime(QDateTime::fromMSecsSinceEpoch(m.value("created_dt").toLongLong()));
        info.setModificationDateTime(QDateTime::fromMSecsSinceEpoch(m.value("modified_dt").toLongLong()));
        newSamples << info;
    }
    foreach (const QVariant &v, r2.values())
        deletedSamples << v.toMap().value("id").toULongLong();
    return TOperationResult(true);
}

TOperationResult Storage::generateInvites(quint64 userId, const QDateTime &expiresDT, quint8 count,
                                          TInviteInfo::InvitesList &invites)
{
    if (!userId || !expiresDT.isValid() || !count || count > Texsample::MaximumInvitesCount)
        return Global::result(Global::InvalidParameters, mtranslator);
    if (!isValid())
        return Global::result(Global::StorageError, mtranslator);
    if (!mdb->transaction())
        return Global::result(Global::DatabaseError, mtranslator);
    invites.clear();
    QVariantMap m;
    m.insert("creator_id", userId);
    m.insert("expires_dt", expiresDT.toUTC().toMSecsSinceEpoch());
    TInviteInfo info;
    info.setCreatorId(userId);
    info.setExpirationDateTime(expiresDT.toUTC());
    quint8 i = 0;
    while (i < count)
    {
        QUuid uuid = QUuid::createUuid();
        QDateTime createdDT = QDateTime::currentDateTimeUtc();
        m.insert("uuid", BeQt::pureUuidText(uuid));
        m.insert("created_dt", createdDT.toMSecsSinceEpoch());
        BSqlResult r = mdb->insert("invites", m);
        if (!r)
        {
            mdb->rollback();
            return Global::result(Global::QueryError, mtranslator);
        }
        info.setId(r.lastInsertId().toULongLong());
        info.setUuid(uuid);
        info.setCreationDateTime(createdDT);
        invites << info;
        ++i;
        bApp->scheduleInvitesAutoTest(info);
    }
    if (!mdb->commit())
        return Global::result(Global::DatabaseError, mtranslator);
    return TOperationResult(true);
}

TOperationResult Storage::getInvitesList(quint64 userId, TInviteInfo::InvitesList &invites)
{
    if (!userId)
        return Global::result(Global::InvalidParameters, mtranslator);
    if (!isValid())
        return Global::result(Global::StorageError, mtranslator);
    QStringList fields = QStringList() << "id" << "uuid" << "creator_id" << "expires_dt" << "created_dt";
    BSqlResult r = mdb->select("invites", fields, BSqlWhere("creator_id = :creator_id", ":creator_id", userId));
    if (!r)
        return Global::result(Global::QueryError, mtranslator);
    foreach (const QVariantMap &m, r.values())
    {
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

TOperationResult Storage::getRecoveryCode(const QString &email, const Translator &t)
{
    if (email.isEmpty())
        return Global::result(Global::InvalidParameters, mtranslator);
    if (!isValid())
        return Global::result(Global::StorageError, mtranslator);
    quint64 userId = userIdByEmail(email);
    if (!userId || !mdb->transaction())
        return Global::result(Global::DatabaseError, mtranslator);
    QString code = BeQt::pureUuidText(QUuid::createUuid());
    QDateTime crDt = QDateTime::currentDateTimeUtc();
    QDateTime expDt = crDt.addDays(1);
    QVariantMap m;
    m.insert("uuid", code);
    m.insert("user_id", userId);
    m.insert("expires_dt", expDt.toMSecsSinceEpoch());
    m.insert("created_dt", crDt.toMSecsSinceEpoch());
    BSqlResult r = mdb->insert("recovery_codes", m);
    if (!r)
    {
        mdb->rollback();
        return Global::result(Global::QueryError, mtranslator);
    }
    if (!mdb->commit())
        return Global::result(Global::DatabaseError, mtranslator);
    bApp->scheduleRecoveryCodesAutoTest(expDt);
    QString fn = BDirTools::findResource("templates/recovery", BDirTools::GlobalOnly) + "/get.txt";
    QString text = BDirTools::readTextFile(BDirTools::localeBasedFileName(fn), "UTF-8");
    text.replace("%code%", code).replace("%username%", userLogin(userId));
    Global::sendEmail(email, t.translate("Storage", "TeXSample account recovering"), text);
    return TOperationResult(true);
}

TOperationResult Storage::recoverAccount(const QString &email, const QUuid &code, const QByteArray &password,
                                         const Translator &t)
{
    if (email.isEmpty() || code.isNull() || password.isEmpty())
        return Global::result(Global::InvalidParameters, mtranslator);
    if (!isValid())
        return Global::result(Global::StorageError, mtranslator);
    quint64 userId = userIdByEmail(email);
    if (!userId)
        return Global::result(Global::DatabaseError, mtranslator);
    BSqlWhere w("user_id = :user_id AND uuid = :uuid", ":user_id", userId, ":uuid", BeQt::pureUuidText(code));
    BSqlResult r = mdb->select("recovery_codes", "id", w);
    quint64 codeId = r.value("id").toULongLong();
    if (!r || !codeId)
        return TOperationResult("Invalid recovery code");
    if (!mdb->transaction())
        return Global::result(Global::DatabaseError, mtranslator);
    if (!mdb->update("users", "password", password, "", QVariant(), BSqlWhere("id = :id", ":id", userId)))
    {
        mdb->rollback();
        return Global::result(Global::QueryError, mtranslator);
    }
    if (!mdb->deleteFrom("recovery_codes", BSqlWhere("id = :id", ":id", codeId)))
    {
        mdb->rollback();
        return Global::result(Global::QueryError, mtranslator);
    }
    if (!mdb->commit())
        return Global::result(Global::DatabaseError, mtranslator);
    QString fn = BDirTools::findResource("templates/recovery", BDirTools::GlobalOnly) + "/recovered.txt";
    QString text = BDirTools::readTextFile(BDirTools::localeBasedFileName(fn), "UTF-8");
    Global::sendEmail(email, t.translate("Storage", "TeXSample account recovered"), text);
    return TOperationResult(true);
}

bool Storage::isUserUnique(const QString &login, const QString &email)
{
    if (login.isEmpty() || email.isEmpty() || !isValid())
        return false;
    BSqlWhere w("login = :login OR email = :email", ":login", login, ":email", email);
    BSqlResult r = mdb->select("users", "id", w);
    return r && !r.value("id").toULongLong();
}

quint64 Storage::userId(const QString &login, const QByteArray &password)
{
    if (login.isEmpty() || !isValid())
        return 0;
    BSqlWhere w("login = :login", ":login", login);
    if (!password.isEmpty())
    {
        w.setString(w.string() + " AND password = :password");
        QVariantMap m;
        m.insert(":password", password);
        w.setBoundValues(w.boundValues().unite(m));
    }
    return mdb->select("users", "id", w).value("id").toULongLong();
}

quint64 Storage::userIdByEmail(const QString &email)
{
    if (email.isEmpty() || !isValid())
        return 0;
    BSqlWhere w("email = :email", ":email", email);
    return mdb->select("users", "id", w).value("id").toULongLong();
}

quint64 Storage::senderId(quint64 sampleId)
{
    if (!sampleId || !isValid())
        return 0;
    BSqlWhere w("id = :id", ":id", sampleId);
    return mdb->select("samples", "sender_id", w).value("sender_id").toULongLong();
}

QString Storage::userLogin(quint64 userId)
{
    if (!userId || !isValid())
        return "";
    BSqlWhere w("id = :id", ":id", userId);
    return mdb->select("users", "login", w).value("login").toString();
}

TSampleInfo::Type Storage::sampleType(quint64 sampleId)
{
    if (!sampleId || !isValid())
        return TSampleInfo::Unverified;
    BSqlWhere w("id = :id", ":id", sampleId);
    return TSampleInfo::typeFromInt(mdb->select("samples", "type", w).value("type").toInt());
}

QString Storage::sampleFileName(quint64 sampleId)
{
    if (!sampleId || !isValid())
        return "";
    BSqlWhere w("id = :id", ":id", sampleId);
    return mdb->select("samples", "file_name", w).value("file_name").toString();
}

QDateTime Storage::sampleCreationDateTime(quint64 sampleId, Qt::TimeSpec spec)
{
    if (!sampleId || !isValid())
        return QDateTime().toTimeSpec(spec);
    BSqlWhere w("id = :id", ":id", sampleId);
    qint64 msecs = mdb->select("samples", "created_dt", w).value("created_dt").toULongLong();
    return QDateTime::fromMSecsSinceEpoch(msecs).toTimeSpec(spec);
}

QDateTime Storage::sampleModificationDateTime(quint64 sampleId, Qt::TimeSpec spec)
{
    if (!sampleId || !isValid())
        return QDateTime().toTimeSpec(spec);
    BSqlWhere w("id = :id", ":id", sampleId);
    qint64 msecs = mdb->select("samples", "modified_dt", w).value("modified_dt").toULongLong();
    return QDateTime::fromMSecsSinceEpoch(msecs).toTimeSpec(spec);
}

TAccessLevel Storage::userAccessLevel(quint64 userId)
{
    if (!userId || !isValid())
        return 0;
    BSqlWhere w("id = :id", ":id", userId);
    return mdb->select("users", "access_level", w).value("access_level").toInt();
}

quint64 Storage::inviteId(const QUuid &invite)
{
    if (invite.isNull() || !isValid())
        return 0;
    BSqlWhere w("uuid = :uuid AND expires_dt > :expires_dt", ":uuid", BeQt::pureUuidText(invite), ":expires_dt",
                QDateTime::currentDateTimeUtc().toMSecsSinceEpoch());
    return mdb->select("invites", "id", w).value("id").toULongLong();
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
    if (!isValid() || !mdb->transaction())
        return false;
    BSqlWhere w("expires_dt <= :current_dt", ":current_dt", QDateTime::currentDateTimeUtc().toMSecsSinceEpoch());
    bool b = mdb->deleteFrom("invites", w);
    return mdb->endTransaction(b) && b;
}

bool Storage::testRecoveryCodes()
{
    if (!isValid() || !mdb->transaction())
        return false;
    BSqlWhere w("expires_dt <= :current_dt", ":current_dt", QDateTime::currentDateTimeUtc().toMSecsSinceEpoch());
    bool b = mdb->deleteFrom("recovery_codes", w);
    return mdb->endTransaction(b) && b;
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
