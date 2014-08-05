#include "storage.h"
#include "global.h"
#include "application.h"

//#include <TOperationResult>
#include <TUserInfo>
#include <TAccessLevel>
#include <TSampleInfo>
#include <TeXSample>
#include <TInviteInfo>
//#include <TCompilationResult>
#include <TTexProject>
#include <TService>
#include <TServiceList>
#include <TLabInfo>
#include <TLabInfoList>
//#include <TLabProject>
//#include <TProjectFile>
//#include <TProjectFileList>

#include <BSmtpSender>
#include <BEmail>
#include <BGenericSocket>
#include <BDirTools>
#include <BeQt>
#include <BSqlDatabase>
#include <BSqlQuery>
#include <BSqlWhere>
#include <BSqlResult>
#include <BTextTools>

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
#include <QLocale>
#include <QCryptographicHash>

#include <QDebug>

/*============================================================================
================================ Storage =====================================
============================================================================*/

/*============================== Static public methods =====================*/

/*

bool Storage::initStorage(QString *errs)
{
    static bool initialized = false;
    if (initialized)
    {
        //bWriteLine(tr("Storage already initialized"));
        return true;
    }
    //bWriteLine(tr("Initializing storage..."));
    QString sty = BDirTools::findResource("texsample-framework/texsample.sty", BDirTools::GlobalOnly);
    sty = BDirTools::readTextFile(sty, "UTF-8");
    if (sty.isEmpty())
        return bRet(errs, tr("Failed to load texsample.sty"), false);
    QString tex = BDirTools::findResource("texsample-framework/texsample.tex", BDirTools::GlobalOnly);
    tex = BDirTools::readTextFile(tex, "UTF-8");
    if (tex.isEmpty())
        return bRet(errs, tr("Failed to load texsample.tex"), false);
    BSqlDatabase db("QSQLITE", QUuid::createUuid().toString());
    db.setDatabaseName(rootDir() + "/texsample.sqlite");
    db.setOpenOnDemand(true);
    db.setOnOpenQuery("PRAGMA foreign_keys = ON");
    QString fn = BDirTools::findResource("db/texsample.schema", BDirTools::GlobalOnly);
    QStringList list = BSqlDatabase::schemaFromFile(fn, "UTF-8");
    if (list.isEmpty())
        return bRet(errs, tr("Failed to load database schema"), false);
    if (Global::readOnly())
    {
        foreach (const QString &qs, list)
        {
            QString table;
            if (qs.startsWith("CREATE TABLE IF NOT EXISTS"))
                table = qs.section(QRegExp("\\s+"), 5, 5);
            else if (qs.startsWith("CREATE TABLE"))
                table = qs.section(QRegExp("\\s+"), 2, 2);
            if (table.isEmpty())
                continue;
            if (!db.tableExists(table))
                return bRet(errs, tr("Can't create tables in read-only mode"), false);
        }
    }
    BSqlWhere w("access_level = :access_level", ":access_level", TAccessLevel::SuperuserLevel);
    bool users = !db.select("users", "id", w).values().isEmpty();
    if (Global::readOnly() && !users)
        return bRet(errs, tr("Can't create users in read-only mode"), false);
    if (!Global::readOnly() && !db.initializeFromSchema(list))
        return bRet(errs, tr("Database initialization failed"), false);
    Storage s;
    if (!s.isValid())
        return bRet(errs, tr("Invalid storage instance"), false);
    if (!Global::readOnly() && !s.testInvites())
        return bRet(errs, tr("Failed to test invite codes"), false);
    if (!Global::readOnly() && !s.testRecoveryCodes())
        return bRet(errs, tr("Failed to test recovery codes"), false);
    if (!users)
    {
        QString login = bReadLine(tr("Enter superuser login:") + " ");
        if (login.isEmpty())
            return bRet(errs, tr("Operation aborted"), false);
        QString mail = bReadLine(tr("Enter superuser e-mail:") + " ");
        if (mail.isEmpty())
            return bRet(errs, tr("Operation aborted"), false);
        QString pwd = bReadLineSecure(tr("Enter superuser password:") + " ");
        if (pwd.isEmpty())
            return bRet(errs, tr("Operation aborted"), false);
        TUserInfo info(TUserInfo::AddContext);
        info.setLogin(login);
        info.setEmail(mail);
        info.setPassword(pwd);
        info.setAccessLevel(TAccessLevel::SuperuserLevel);
        info.setServices(TServiceList::allServices());
        bWriteLine(tr("Creating superuser account..."));
        TOperationResult r = s.addUser(info, BCoreApplication::locale());
        if (!r)
            return bRet(errs, r.messageString(), false);
    }
    mtexsampleSty = sty;
    mtexsampleTex = tex;
    initialized = true;
    bWriteLine(tr("Done!"));
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
}*/

/*============================== Public constructors =======================*/

/*Storage::Storage()
{
    mdb = new BSqlDatabase("QSQLITE", QUuid::createUuid().toString());
    mdb->setDatabaseName(BDirTools::findResource("texsample.sqlite", BDirTools::UserOnly));
    mdb->setOnOpenQuery("PRAGMA foreign_keys = ON");
    mdb->open();
}

Storage::~Storage()
{
    delete mdb;
}*/

/*============================== Public methods ============================*/

/*TOperationResult Storage::addUser(const TUserInfo &info, const QLocale &locale, const QStringList &clabGroups)
{
    if (Global::readOnly())
        return TOperationResult(TMessage::ReadOnlyError);
    if (!info.isValid(TUserInfo::AddContext))
        return TOperationResult(TMessage::InvalidDataError);
    if (!isValid())
        return TOperationResult(TMessage::InternalStorageError);
    return addUserInternal(info, locale, clabGroups);
}

TOperationResult Storage::registerUser(TUserInfo info, const QLocale &locale)
{
    if (Global::readOnly())
        return TOperationResult(TMessage::ReadOnlyError);
    if (!info.isValid(TUserInfo::RegisterContext))
        return TOperationResult(TMessage::InvalidUserInfoError);
    if (!isValid())
        return TOperationResult(TMessage::InternalStorageError);
    qint64 iid = inviteId(info.inviteCode());
    if (!iid)
        return TOperationResult(TMessage::NoSuchCodeError);
    info.setContext(TUserInfo::AddContext);
    info.setAccessLevel(TAccessLevel::UserLevel);
    info.setServices(newUserServices(info.inviteCode()));
    TOperationResult r = addUserInternal(info, locale, newUserClabGroups(info.inviteCode()));
    if (!r)
        return r;
    mdb->deleteFrom("invite_codes", BSqlWhere("id = :id", ":id", iid));
    return r;

}

TOperationResult Storage::editUser(const TUserInfo &info, bool editClab, const QStringList &clabGroups)
{
    if (Global::readOnly())
        return TOperationResult(TMessage::ReadOnlyError);
    if (!info.isValid(TUserInfo::EditContext))
        return TOperationResult(TMessage::InvalidDataError);
    if (!isValid())
        return TOperationResult(TMessage::InternalStorageError);
    return editUserInternal(info, editClab, clabGroups);
}

TOperationResult Storage::updateUser(const TUserInfo info)
{
    if (Global::readOnly())
        return TOperationResult(TMessage::ReadOnlyError);
    if (!info.isValid(TUserInfo::UpdateContext))
        return TOperationResult(TMessage::InvalidDataError);
    if (!isValid())
        return TOperationResult(TMessage::InternalStorageError);
    return editUserInternal(info);
}

TOperationResult Storage::getUserInfo(quint64 userId, TUserInfo &info, QDateTime &updateDT, bool &cacheOk)
{
    if (!userId)
        return TOperationResult(TMessage::InvalidUserIdError);
    if (!isValid())
        return TOperationResult(TMessage::InternalStorageError);
    QStringList sl = QStringList() << "login" << "access_level" << "real_name" << "creation_dt" << "update_dt";
    BSqlResult r = mdb->select("users", sl, BSqlWhere("id = :id", ":id", userId));
    if (!r)
        return TOperationResult(TMessage::InternalQueryError);
    cacheOk = r.value("update_dt").toLongLong() <= updateDT.toUTC().toMSecsSinceEpoch();
    updateDT = QDateTime::currentDateTimeUtc();
    if (cacheOk)
        return TOperationResult(true);
    bool ok = false;
    QByteArray avatar = loadUserAvatar(userId, &ok);
    if (!ok)
        return TOperationResult(TMessage::InternalFileSystemError);
    info.setId(userId);
    info.setLogin(r.value("login").toString());
    info.setAccessLevel(r.value("access_level").toInt());
    info.setServices(userServices(userId));
    info.setRealName(r.value("real_name").toString());
    info.setAvatar(avatar);
    info.setCreationDateTime(QDateTime::fromMSecsSinceEpoch(r.value("creation_dt").toULongLong()));
    info.setUpdateDateTime(QDateTime::fromMSecsSinceEpoch(r.value("update_dt").toULongLong()));
    return TOperationResult(true);
}

TOperationResult Storage::getShortUserInfo(quint64 userId, TUserInfo &info)
{
    if (!userId)
        return TOperationResult(TMessage::InvalidUserIdError);
    if (!isValid())
        return TOperationResult(TMessage::InternalStorageError);
    BSqlResult r = mdb->select("users", QStringList() << "login" << "real_name", BSqlWhere("id = :id", ":id", userId));
    if (!r)
        return TOperationResult(TMessage::InternalQueryError);
    info.setContext(TUserInfo::BriefInfoContext);
    info.setId(userId);
    info.setLogin(r.value("login").toString());
    info.setRealName(r.value("real_name").toString());
    return TOperationResult(true);
}

TCompilationResult Storage::addSample(quint64 userId, TTexProject project, const TSampleInfo &info)
{
    if (Global::readOnly())
        return TOperationResult(TMessage::ReadOnlyError);
    if (!userId || !project.isValid() || !info.isValid(TSampleInfo::AddContext))
        return TOperationResult(TMessage::InvalidDataError);
    if (!isValid())
        return TOperationResult(TMessage::InternalStorageError);
    if (!mdb->transaction())
        return TOperationResult(TMessage::InternalDatabaseError);
    qint64 msecs = QDateTime::currentDateTimeUtc().toMSecsSinceEpoch();
    QVariantMap m;
    m.insert("sender_id", userId);
    m.insert("title", info.title());
    m.insert("file_name", info.fileName());
    m.insert("authors", BeQt::serialize(info.authors()));
    m.insert("tags", BeQt::serialize(info.tags()));
    m.insert("comment", info.comment());
    m.insert("creation_dt", msecs);
    m.insert("update_dt", msecs);
    BSqlResult qr = mdb->insert("samples", m);
    if (!qr)
    {
        mdb->rollback();
        return TOperationResult(TMessage::InternalQueryError);
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
    if (!BDirTools::moveDir(p.path, spath))
    {
        mdb->rollback();
        BDirTools::rmdir(p.path);
        return TOperationResult(TMessage::InternalFileSystemError);
    }
    if (!mdb->commit())
    {
        BDirTools::rmdir(p.path);
        return TOperationResult(TMessage::InternalDatabaseError);
    }
    return cr;
}

TCompilationResult Storage::editSample(const TSampleInfo &info, TTexProject project)
{
    if (Global::readOnly())
        return TOperationResult(TMessage::ReadOnlyError);
    if (!info.isValid(TSampleInfo::EditContext) && !info.isValid(TSampleInfo::UpdateContext))
        return TOperationResult(TMessage::InvalidDataError);
    if (!isValid())
        return TOperationResult(TMessage::InternalStorageError);
    QString pfn = sampleFileName(info.id());
    if (pfn.isEmpty())
        return TOperationResult(TMessage::NoSuchSampleError);
    if (!mdb->transaction())
        return TOperationResult(TMessage::InternalDatabaseError);
    QVariantMap m;
    m.insert("title", info.title());
    m.insert("authors", BeQt::serialize(info.authors()));
    m.insert("tags", BeQt::serialize(info.tags()));
    m.insert("comment", info.comment());
    m.insert("update_dt", QDateTime::currentDateTimeUtc().toMSecsSinceEpoch());
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
        return TOperationResult(TMessage::InternalQueryError);
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
                BDirTools::rmdir(spath);
                return TOperationResult(TMessage::InternalFileSystemError);
            }
        }
    }
    TCompilationResult cr(true);
    QString bupath = QDir::tempPath() + "/texsample-server/backup/" + BeQt::pureUuidText(QUuid::createUuid());
    if (project.isValid())
    {
        project.rootFile()->setFileName(info.fileName());
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
        if (!BDirTools::copyDir(spath, bupath, true))
        {
            mdb->rollback();
            BDirTools::rmdir(bupath);
            BDirTools::rmdir(p.path);
            return TOperationResult(TMessage::InternalFileSystemError);
        }
        if (!BDirTools::rmdir(spath) || !BDirTools::moveDir(p.path, spath))
        {
            mdb->rollback();
            BDirTools::rmdir(p.path);
            BDirTools::copyDir(bupath, spath, true);
            BDirTools::rmdir(bupath);
            return TOperationResult(TMessage::InternalFileSystemError);
        }
    }
    if (!mdb->commit())
    {
        BDirTools::rmdir(spath);
        BDirTools::copyDir(bupath, spath, true);
        BDirTools::rmdir(bupath);
        return TOperationResult(TMessage::InternalDatabaseError);
    }
    BDirTools::rmdir(bupath);
    return cr;
}

TOperationResult Storage::deleteSample(quint64 sampleId, const QString &reason)
{
    if (Global::readOnly())
        return TOperationResult(TMessage::ReadOnlyError);
    if (!sampleId)
        return TOperationResult(TMessage::InvalidSampleIdError);
    if (!isValid())
        return TOperationResult(TMessage::InternalStorageError);
    if (sampleState(sampleId) == DeletedState)
        return TOperationResult(TMessage::SampleAlreadyDeletedError);
    if (!mdb->transaction())
        return TOperationResult(TMessage::InternalDatabaseError);
    QVariantMap bv;
    bv.insert("state", 1);
    bv.insert("deletion_reason", reason);
    bv.insert("deletion_dt", QDateTime::currentDateTimeUtc().toMSecsSinceEpoch());
    if (!mdb->update("samples", bv, BSqlWhere("id = :id", ":id", sampleId)))
    {
        mdb->rollback();
        return TOperationResult(TMessage::InternalQueryError);
    }
    //Data is not deleted and files are not deleted, so the sample may be undeleted later
    if (!mdb->commit())
        return TOperationResult(TMessage::InternalDatabaseError);
    return TOperationResult(true);
}

TOperationResult Storage::getSampleSource(quint64 sampleId, TTexProject &project, QDateTime &updateDT, bool &cacheOk)
{
    if (!sampleId)
        return TOperationResult(TMessage::InvalidSampleIdError);
    if (!isValid())
        return TOperationResult(TMessage::InternalStorageError);
    if (updateDT.toUTC() >= sampleUpdateDateTime(sampleId))
    {
        cacheOk = true;
        return TOperationResult(true);
    }
    QString fn = sampleFileName(sampleId);
    if (fn.isEmpty())
        return TOperationResult(TMessage::NoSuchSampleError);
    updateDT = QDateTime::currentDateTimeUtc();
    bool b = project.load(rootDir() + "/samples/" + QString::number(sampleId) + "/" + fn, "UTF-8");
    return TOperationResult(b, b ? TMessage::NoMessage : TMessage::InternalFileSystemError);
}

TOperationResult Storage::getSamplePreview(quint64 sampleId, TProjectFile &file, QDateTime &updateDT, bool &cacheOk)
{
    if (!sampleId)
        return TOperationResult(TMessage::InvalidSampleIdError);
    if (!isValid())
        return TOperationResult(TMessage::InternalStorageError);
    if (updateDT.toUTC() >= sampleUpdateDateTime(sampleId))
    {
        cacheOk = true;
        return TOperationResult(true);
    }
    QString fn = sampleFileName(sampleId);
    if (fn.isEmpty())
        return TOperationResult(TMessage::NoSuchSampleError);
    updateDT = QDateTime::currentDateTimeUtc();
    fn = rootDir() + "/samples/" + QString::number(sampleId) + "/" + QFileInfo(fn).baseName() + ".pdf";
    bool b = file.loadAsBinary(fn, "");
    return TOperationResult(b, b ? TMessage::NoMessage : TMessage::InternalFileSystemError);
}

TOperationResult Storage::getSamplesList(TSampleInfoList &newSamples, TIdList &deletedSamples, QDateTime &updateDT)
{
    if (!isValid())
        return TOperationResult(TMessage::InternalStorageError);
    qint64 updateMsecs = updateDT.toUTC().toMSecsSinceEpoch();
    QStringList sl1 = QStringList() << "id" << "sender_id" << "authors" << "title" << "file_name" << "type" << "tags"
                                    << "comment" << "rating" << "admin_remark" << "creation_dt" << "update_dt";
    BSqlWhere w1("state = :state AND update_dt >= :update_dt", ":state", 0, ":update_dt", updateMsecs);
    BSqlResult r1 = mdb->select("samples", sl1, w1);
    if (!r1)
        return TOperationResult(TMessage::InternalQueryError);
    QVariantMap wbv2;
    wbv2.insert(":state", 1);
    wbv2.insert(":update_dt", updateMsecs);
    wbv2.insert(":update_dt_hack", updateMsecs);
    BSqlWhere w2("state = :state AND deletion_dt >= :update_dt AND creation_dt < :update_dt_hack", wbv2);
    BSqlResult r2 = mdb->select("samples", "id", w2);
    if (!r2)
        return TOperationResult(TMessage::InternalQueryError);
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
        info.setAuthors(BeQt::deserialize(m.value("authors").toByteArray()).toStringList());
        info.setTitle(m.value("title").toString());
        info.setFileName(m.value("file_name").toString());
        info.setType(m.value("type").toInt());
        info.setProjectSize(TTexProject::size(rootDir() + "/samples/" + info.idString() + "/" + info.fileName(),
                                           "UTF-8"));
        info.setTags(BeQt::deserialize(m.value("tags").toByteArray()).toStringList());
        info.setRating(m.value("rating").toUInt());
        info.setComment(m.value("comment").toString());
        info.setAdminRemark(m.value("admin_remark").toString());
        info.setCreationDateTime(QDateTime::fromMSecsSinceEpoch(m.value("creation_dt").toLongLong()));
        info.setUpdateDateTime(QDateTime::fromMSecsSinceEpoch(m.value("update_dt").toLongLong()));
        newSamples << info;
    }
    foreach (const QVariant &v, r2.values())
        deletedSamples << v.toMap().value("id").toULongLong();
    return TOperationResult(true);
}

TOperationResult Storage::generateInvites(quint64 userId, const QDateTime &expirationDT, quint8 count,
                                          const TServiceList &services, const QStringList &clabGroups,
                                          TInviteInfoList &invites)
{
    if (Global::readOnly())
        return TOperationResult(TMessage::ReadOnlyError);
    if (!userId || !expirationDT.isValid() || !count || count > 10) //TODO: Make max invite count configurable
        return TOperationResult(TMessage::InvalidDataError);
    if (!isValid())
        return TOperationResult(TMessage::InternalStorageError);
    if (!mdb->transaction())
        return TOperationResult(TMessage::InternalDatabaseError);
    invites.clear();
    QVariantMap m;
    m.insert("creator_id", userId);
    m.insert("expiration_dt", expirationDT.toUTC().toMSecsSinceEpoch());
    TInviteInfo info;
    info.setCreatorId(userId);
    info.setExpirationDateTime(expirationDT.toUTC());
    quint8 i = 0;
    while (i < count)
    {
        QUuid uuid = QUuid::createUuid();
        QDateTime createdDT = QDateTime::currentDateTimeUtc();
        m.insert("code", BeQt::pureUuidText(uuid));
        m.insert("creation_dt", createdDT.toMSecsSinceEpoch());
        BSqlResult r = mdb->insert("invite_codes", m);
        if (!r)
        {
            mdb->rollback();
            return TOperationResult(TMessage::InternalQueryError);
        }
        quint64 id = r.lastInsertId().toULongLong();
        foreach (const TService &s, services)
        {
            if (!mdb->insert("new_user_services", "invite_id", id, "service_id", (int) s))
            {
                mdb->rollback();
                return TOperationResult(TMessage::InternalQueryError);
            }
        }
        foreach (const QString &gr, clabGroups)
        {
            if (!mdb->insert("new_user_clab_groups", "invite_id", id, "group_name", gr))
            {
                mdb->rollback();
                return TOperationResult(TMessage::InternalQueryError);
            }
        }
        info.setId(id);
        info.setCode(uuid);
        info.setServices(services);
        info.setCreationDateTime(createdDT);
        invites << info;
        ++i;
    }
    if (!mdb->commit())
        return TOperationResult(TMessage::InternalDatabaseError);
    return TOperationResult(true);
}

TOperationResult Storage::getInvitesList(quint64 userId, TInviteInfoList &invites)
{
    if (!userId)
        return TOperationResult(TMessage::InvalidUserIdError);
    if (!isValid())
        return TOperationResult(TMessage::InternalStorageError);
    QStringList fields = QStringList() << "id" << "code" << "creator_id" << "expiration_dt" << "creation_dt";
    BSqlResult r = mdb->select("invite_codes", fields, BSqlWhere("creator_id = :creator_id", ":creator_id", userId));
    if (!r)
        return TOperationResult(TMessage::InternalQueryError);
    foreach (const QVariantMap &m, r.values())
    {
        TInviteInfo info;
        info.setId(m.value("id").toULongLong());
        info.setCode(m.value("code").toString());
        info.setCreatorId(userId);
        info.setServices(newUserServices(info.id()));
        info.setExpirationDateTime(QDateTime::fromMSecsSinceEpoch(m.value("expiration_dt").toULongLong()));
        info.setCreationDateTime(QDateTime::fromMSecsSinceEpoch(m.value("creation_dt").toULongLong()));
        invites << info;
    }
    return TOperationResult(true);
}

TOperationResult Storage::getRecoveryCode(const QString &email, const QLocale &locale)
{
    if (Global::readOnly())
        return TOperationResult(TMessage::ReadOnlyError);
    if (email.isEmpty())
        return TOperationResult(TMessage::InvalidEmailError);
    if (!isValid())
        return TOperationResult(TMessage::InternalStorageError);
    quint64 userId = userIdByEmail(email);
    if (!userId)
        return TOperationResult(TMessage::NoSuchUserError);
    if (!mdb->transaction())
        return TOperationResult(TMessage::InternalDatabaseError);
    QString code = BeQt::pureUuidText(QUuid::createUuid());
    QDateTime crDt = QDateTime::currentDateTimeUtc();
    QDateTime expDt = crDt.addDays(1);
    QVariantMap m;
    m.insert("code", code);
    m.insert("requester_id", userId);
    m.insert("expiration_dt", expDt.toMSecsSinceEpoch());
    m.insert("creation_dt", crDt.toMSecsSinceEpoch());
    BSqlResult r = mdb->insert("recovery_codes", m);
    if (!r)
    {
        mdb->rollback();
        return TOperationResult(TMessage::InternalQueryError);
    }
    Global::StringMap replace;
    replace.insert("%code%", code);
    replace.insert("%username%", userLogin(userId));
    TOperationResult sr = Global::sendEmail(email, "get_recovery_code", locale, replace);
    if (!sr)
    {
        mdb->rollback();
        return sr;
    }
    if (!mdb->commit())
        return TOperationResult(TMessage::InternalDatabaseError);
    return TOperationResult(true);
}

TOperationResult Storage::recoverAccount(const QString &email, const QString &code, const QByteArray &password,
                                         const QLocale &locale)
{
    if (Global::readOnly())
        return TOperationResult(TMessage::ReadOnlyError);
    if (email.isEmpty() || BeQt::uuidFromText(code).isNull() || password.isEmpty())
        return TOperationResult(TMessage::InvalidDataError);
    if (!isValid())
        return TOperationResult(TMessage::InternalStorageError);
    quint64 userId = userIdByEmail(email);
    if (!userId)
        return TOperationResult(TMessage::NoSuchUserError);
    BSqlWhere w("requester_id = :requester_id AND code = :code", ":requester_id", userId, ":code",
                BeQt::pureUuidText(code));
    BSqlResult r = mdb->select("recovery_codes", "id", w);
    quint64 codeId = r.value("id").toULongLong();
    if (!r || !codeId)
        return TOperationResult(TMessage::NoSuchCodeError);
    if (!mdb->transaction())
        return TOperationResult(TMessage::InternalDatabaseError);
    if (!mdb->update("users", "password", password, "", QVariant(), BSqlWhere("id = :id", ":id", userId)))
    {
        mdb->rollback();
        return TOperationResult(TMessage::InternalQueryError);
    }
    if (!mdb->deleteFrom("recovery_codes", BSqlWhere("id = :id", ":id", codeId)))
    {
        mdb->rollback();
        return TOperationResult(TMessage::InternalQueryError);
    }
    TOperationResult sr = Global::sendEmail(email, "recover_account", locale);
    if (!sr)
    {
        mdb->rollback();
        return sr;
    }
    if (!mdb->commit())
        return TOperationResult(TMessage::InternalDatabaseError);
    return TOperationResult(true);
}

TOperationResult Storage::editClabGroups(const QStringList &newGroups, const QStringList &deletedGroups)
{
    if (!isValid())
        return TOperationResult(TMessage::InternalStorageError);
    if (!mdb->transaction())
        return TOperationResult(TMessage::InternalDatabaseError);
    foreach (const QString &gr, deletedGroups)
    {
        if (!mdb->deleteFrom("clab_groups", BSqlWhere("name = :name", ":name", gr)))
        {
            mdb->rollback();
            return TOperationResult(TMessage::InternalQueryError);
        }
    }
    foreach (const QString &gr, newGroups)
    {
        if (!mdb->insert("clab_groups", "name", gr))
        {
            mdb->rollback();
            return TOperationResult(TMessage::InternalQueryError);
        }
    }
    if (!mdb->commit())
        return TOperationResult(TMessage::InternalDatabaseError);
    return TOperationResult(true);
}

TOperationResult Storage::getClabGroupsList(QStringList &groups)
{
    if (!isValid())
        return TOperationResult(TMessage::InternalStorageError);
    BSqlResult r = mdb->select("clab_groups", "name");
    if (!r)
        return TOperationResult(TMessage::InternalQueryError);
    groups.clear();
    foreach (const QVariantMap &m, r.values())
        groups << m.value("name").toString();
    return TOperationResult(true);
}

TOperationResult Storage::addLab(quint64 userId, const TLabInfo &info, const TLabProject &webProject,
                                 const TLabProject &linuxProject, const TLabProject &macProject,
                                 const TLabProject &winProject, const QString &url, const TProjectFileList &extraFiles)
{
    if (Global::readOnly())
        return TOperationResult(TMessage::ReadOnlyError);
    if (!userId || !info.isValid(TLabInfo::AddContext))
        return TOperationResult(TMessage::InvalidDataError);
    if (!webProject.isValid() && !linuxProject.isValid() && !macProject.isValid() && !winProject.isValid()
            && url.isEmpty())
        return TOperationResult(TMessage::InvalidDataError);
    if (!isValid())
        return TOperationResult(TMessage::InternalStorageError);
    if (!mdb->transaction())
        return TOperationResult(TMessage::InternalDatabaseError);
    qint64 msecs = QDateTime::currentDateTimeUtc().toMSecsSinceEpoch();
    QVariantMap m;
    m.insert("sender_id", userId);
    m.insert("title", info.title());
    TLabInfo::Type type = TLabInfo::NoType;
    if (webProject.isValid())
    {
        m.insert("file_name_web", webProject.mainFileName());
        type = TLabInfo::WebType;
    }
    else if (url.isEmpty())
    {
        m.insert("file_name_linux", linuxProject.mainFileName());
        m.insert("file_name_mac", macProject.mainFileName());
        m.insert("file_name_win", winProject.mainFileName());
        type = TLabInfo::DesktopType;
    }
    else
    {
        m.insert("url", url);
        type = TLabInfo::UrlType;
    }
    m.insert("authors", BeQt::serialize(info.authors()));
    m.insert("type", (int) type);
    m.insert("tags", BeQt::serialize(info.tags()));
    m.insert("comment", info.comment());
    m.insert("creation_dt", msecs);
    m.insert("update_dt", msecs);
    BSqlResult qr = mdb->insert("labs", m);
    if (!qr)
    {
        mdb->rollback();
        return TOperationResult(TMessage::InternalQueryError);
    }
    foreach (const QString &gr, info.groups())
    {
        if (!mdb->insert("lab_clab_groups", "lab_id", qr.lastInsertId(), "group_name", gr))
        {
            mdb->rollback();
            return TOperationResult(TMessage::InternalQueryError);
        }
    }
    QString path = rootDir() + "/labs/" + QString::number(qr.lastInsertId().toULongLong());
    if (!BDirTools::mkpath(path))
    {
        mdb->rollback();
        BDirTools::rmdir(path);
        return TOperationResult(TMessage::InternalFileSystemError);
    }
    if (webProject.isValid())
    {
        if (!webProject.save(path))
        {
            mdb->rollback();
            BDirTools::rmdir(path);
            return TOperationResult(TMessage::InternalFileSystemError);
        }
    }
    else if (url.isEmpty())
    {
        if (linuxProject.isValid())
        {
            if (!linuxProject.save(path + "/" + labSubdir(BeQt::LinuxOS)))
            {
                mdb->rollback();
                BDirTools::rmdir(path);
                return TOperationResult(TMessage::InternalFileSystemError);
            }
        }
        if (macProject.isValid())
        {
            if (!macProject.save(path + "/" + labSubdir(BeQt::MacOS)))
            {
                mdb->rollback();
                BDirTools::rmdir(path);
                return TOperationResult(TMessage::InternalFileSystemError);
            }
        }
        if (winProject.isValid())
        {
            if (!winProject.save(path + "/" + labSubdir(BeQt::WindowsOS)))
            {
                mdb->rollback();
                BDirTools::rmdir(path);
                return TOperationResult(TMessage::InternalFileSystemError);
            }
        }
    }
    QString epath = path + "_extra";
    if (!BDirTools::mkpath(epath))
    {
        mdb->rollback();
        BDirTools::rmdir(path);
        BDirTools::rmdir(epath);
        return TOperationResult(TMessage::InternalFileSystemError);
    }
    foreach (const TProjectFile &f, extraFiles)
    {
        if (!f.save(epath))
        {
            BDirTools::rmdir(path);
            BDirTools::rmdir(epath);
            return TOperationResult(TMessage::InternalDatabaseError);
        }
    }
    if (!mdb->commit())
    {
        BDirTools::rmdir(path);
        BDirTools::rmdir(epath);
        return TOperationResult(TMessage::InternalDatabaseError);
    }
    return TOperationResult(true);
}

TOperationResult Storage::editLab(const TLabInfo &info, const TLabProject &webProject, const TLabProject &linuxProject,
                                  const TLabProject &macProject, const TLabProject &winProject, const QString &url,
                                  const QStringList &deletedExtraFiles, const TProjectFileList &newExtraFiles)
{
    if (Global::readOnly())
        return TOperationResult(TMessage::ReadOnlyError);
    if (!info.isValid(TLabInfo::EditContext))
        return TOperationResult(TMessage::InvalidDataError);
    if (!isValid())
        return TOperationResult(TMessage::InternalStorageError);
    if (!mdb->transaction())
        return TOperationResult(TMessage::InternalDatabaseError);
    qint64 msecs = QDateTime::currentDateTimeUtc().toMSecsSinceEpoch();
    QVariantMap m;
    m.insert("title", info.title());
    TLabInfo::Type type = labType(info.id());
    if (webProject.isValid())
    {
        m.insert("file_name_web", webProject.mainFileName());
        m.insert("file_name_linux", "");
        m.insert("file_name_mac", "");
        m.insert("file_name_win", "");
        m.insert("url", "");
        type = TLabInfo::WebType;
    }
    else if (linuxProject.isValid() || macProject.isValid() || winProject.isValid())
    {
        m.insert("file_name_web", "");
        m.insert("file_name_linux", linuxProject.mainFileName());
        m.insert("file_name_mac", macProject.mainFileName());
        m.insert("file_name_win", winProject.mainFileName());
        m.insert("url", "");
        type = TLabInfo::DesktopType;
    }
    else if (!url.isEmpty())
    {
        m.insert("file_name_web", "");
        m.insert("file_name_linux", "");
        m.insert("file_name_mac", "");
        m.insert("file_name_win", "");
        m.insert("url", url);
        type = TLabInfo::UrlType;
    }
    m.insert("authors", BeQt::serialize(info.authors()));
    m.insert("type", (int) type);
    m.insert("tags", BeQt::serialize(info.tags()));
    m.insert("comment", info.comment());
    m.insert("update_dt", msecs);
    BSqlResult qr = mdb->update("labs", m, BSqlWhere("id = :id", ":id", info.id()));
    if (!qr)
    {
        mdb->rollback();
        return TOperationResult(TMessage::InternalQueryError);
    }
    if (!mdb->deleteFrom("lab_clab_groups", BSqlWhere("lab_id = :lab_id", ":lab_id", info.id())))
    {
        mdb->rollback();
        return TOperationResult(TMessage::InternalQueryError);
    }
    foreach (const QString &gr, info.groups())
    {
        if (!mdb->insert("lab_clab_groups", "lab_id", info.id(), "group_name", gr))
        {
            mdb->rollback();
            return TOperationResult(TMessage::InternalQueryError);
        }
    }
    QString path = rootDir() + "/labs/" + QString::number(info.id());
    QString bupath = QDir::tempPath() + "/texsample-server/backup/" + BeQt::pureUuidText(QUuid::createUuid());
    bool empty = BDirTools::entryList(path, QDir::NoDotAndDotDot).isEmpty();
    if (!empty && !BDirTools::copyDir(path, bupath, true))
    {
        mdb->rollback();
        BDirTools::rmdir(bupath);
        return TOperationResult(TMessage::InternalFileSystemError);
    }
    if (!empty && !BDirTools::rmdir(path))
    {
        mdb->rollback();
        BDirTools::copyDir(bupath, path, true);
        BDirTools::rmdir(bupath);
        return TOperationResult(TMessage::InternalFileSystemError);
    }
    QString epath = path + "_extra";
    QString ebupath = bupath + "_extra";
    bool eempty = BDirTools::entryList(epath, QDir::NoDotAndDotDot).isEmpty();
    if (!eempty && !BDirTools::copyDir(epath, ebupath, true))
    {
        mdb->rollback();
        BDirTools::rmdir(bupath);
        BDirTools::rmdir(ebupath);
        return TOperationResult(TMessage::InternalFileSystemError);
    }
    if (!eempty && !BDirTools::rmdir(epath))
    {
        mdb->rollback();
        BDirTools::copyDir(bupath, path, true);
        BDirTools::rmdir(bupath);
        BDirTools::copyDir(ebupath, epath, true);
        BDirTools::rmdir(ebupath);
        return TOperationResult(TMessage::InternalFileSystemError);
    }
    if (webProject.isValid())
    {
        if (!webProject.save(path))
        {
            mdb->rollback();
            BDirTools::copyDir(bupath, path, true);
            BDirTools::rmdir(bupath);
            return TOperationResult(TMessage::InternalFileSystemError);
        }
    }
    else if (url.isEmpty())
    {
        if (linuxProject.isValid())
        {
            if (!linuxProject.save(path + "/" + labSubdir(BeQt::LinuxOS)))
            {
                mdb->rollback();
                BDirTools::copyDir(bupath, path, true);
                BDirTools::rmdir(bupath);
                return TOperationResult(TMessage::InternalFileSystemError);
            }
        }
        if (macProject.isValid())
        {
            if (!macProject.save(path + "/" + labSubdir(BeQt::MacOS)))
            {
                mdb->rollback();
                BDirTools::copyDir(bupath, path, true);
                BDirTools::rmdir(bupath);
                return TOperationResult(TMessage::InternalFileSystemError);
            }
        }
        if (winProject.isValid())
        {
            if (!winProject.save(path + "/" + labSubdir(BeQt::WindowsOS)))
            {
                mdb->rollback();
                BDirTools::copyDir(bupath, path, true);
                BDirTools::rmdir(bupath);
                return TOperationResult(TMessage::InternalFileSystemError);
            }
        }
    }
    foreach (const QString &fn, deletedExtraFiles)
    {
        if (!QFile::remove(epath + "/" + fn))
        {
            mdb->rollback();
            BDirTools::copyDir(bupath, path, true);
            BDirTools::rmdir(bupath);
            BDirTools::copyDir(ebupath, epath, true);
            BDirTools::rmdir(ebupath);
            return TOperationResult(TMessage::InternalDatabaseError);
        }
    }
    foreach (const TProjectFile &f, newExtraFiles)
    {
        if (!f.save(epath))
        {
            mdb->rollback();
            BDirTools::copyDir(bupath, path, true);
            BDirTools::rmdir(bupath);
            BDirTools::copyDir(ebupath, epath, true);
            BDirTools::rmdir(ebupath);
            return TOperationResult(TMessage::InternalDatabaseError);
        }
    }
    if (!mdb->commit())
    {
        BDirTools::copyDir(bupath, path, true);
        BDirTools::rmdir(bupath);
        BDirTools::copyDir(ebupath, epath, true);
        BDirTools::rmdir(ebupath);
        return TOperationResult(TMessage::InternalDatabaseError);
    }
    BDirTools::rmdir(bupath);
    BDirTools::rmdir(ebupath);
    return TOperationResult(true);
}

TOperationResult Storage::deleteLab(quint64 labId, const QString &reason)
{
    if (Global::readOnly())
        return TOperationResult(TMessage::ReadOnlyError);
    if (!labId)
        return TOperationResult(TMessage::InvalidLabIdError);
    if (!isValid())
        return TOperationResult(TMessage::InternalStorageError);
    if (labState(labId) == DeletedState)
        return TOperationResult(TMessage::LabAlreadyDeletedError);
    if (!mdb->transaction())
        return TOperationResult(TMessage::InternalDatabaseError);
    QVariantMap bv;
    bv.insert("state", 1);
    bv.insert("deletion_reason", reason);
    bv.insert("deletion_dt", QDateTime::currentDateTimeUtc().toMSecsSinceEpoch());
    if (!mdb->update("labs", bv, BSqlWhere("id = :id", ":id", labId)))
    {
        mdb->rollback();
        return TOperationResult(TMessage::InternalQueryError);
    }
    //Data is not deleted and files are not deleted, so the lab may be undeleted later
    if (!mdb->commit())
        return TOperationResult(TMessage::InternalDatabaseError);
    return TOperationResult(true);
}

TOperationResult Storage::getLabsList(quint64 userId, BeQt::OSType osType, TLabInfoList &newLabs, TIdList &deletedLabs,
                                      QDateTime &updateDT)
{
    if (!userId)
        return TOperationResult(TMessage::InvalidLabIdError);
    if (BeQt::UnknownOS == osType)
        return TOperationResult(TMessage::InvalidOSTypeError);
    if (!isValid())
        return TOperationResult(TMessage::InternalStorageError);
    qint64 updateMsecs = updateDT.toUTC().toMSecsSinceEpoch();
    QStringList sl1 = QStringList() << "id" << "sender_id" << "title" << "url" << "authors" << "type" << "tags"
                                    << "comment" << "creation_dt" << "update_dt";
    BSqlWhere w1("state = :state AND update_dt >= :update_dt", ":state", 0, ":update_dt", updateMsecs);
    BSqlResult r1 = mdb->select("labs", sl1, w1);
    if (!r1)
        return TOperationResult(TMessage::InternalQueryError);
    QVariantMap wbv2;
    wbv2.insert(":state", 1);
    wbv2.insert(":update_dt", updateMsecs);
    wbv2.insert(":update_dt_hack", updateMsecs);
    BSqlWhere w2("state = :state AND deletion_dt >= :update_dt AND creation_dt < :update_dt_hack", wbv2);
    BSqlResult r2 = mdb->select("labs", "id", w2);
    if (!r2)
        return TOperationResult(TMessage::InternalQueryError);
    updateDT = QDateTime::currentDateTimeUtc();
    newLabs.clear();
    deletedLabs.clear();
    QStringList groups = userClabGroups(userId);
    foreach (const QVariantMap &m, r1.values())
    {
        quint64 labId = m.value("id").toULongLong();
        QStringList lg = labGroups(labId);
        if (!lg.isEmpty() && !BTextTools::intersects(lg, groups))
            continue;
        TLabInfo info;
        info.setId(labId);
        TUserInfo uinfo;
        TOperationResult ur = getShortUserInfo(m.value("sender_id").toULongLong(), uinfo);
        if (!ur)
            return ur;
        info.setSender(uinfo);
        info.setAuthors(BeQt::deserialize(m.value("authors").toByteArray()).toStringList());
        info.setTitle(m.value("title").toString());
        info.setType(m.value("type").toInt());
        info.setGroups(lg);
        QString url = m.value("url").toString();
        QString path = rootDir() + "/labs/" + info.idString();
        if (url.isEmpty())
        {
            switch (info.type())
            {
            case TLabInfo::DesktopType:
                path += "/" + labSubdir(osType);
                break;
            case TLabInfo::WebType:
                break;
            default:
                return TOperationResult(TMessage::InternalStorageError);
            }
            if (!QFileInfo(path).isDir())
            {
                if (userAccessLevel(userId) < TAccessLevel::ModeratorLevel)
                    continue;
                info.setProjectSize(TLabProject::size(QFileInfo(path).path()));
            }
            else
            {
                info.setProjectSize(TLabProject::size(path));
            }
        }
        else
        {
            info.setProjectSize(url.length() * 2);
        }
        info.setTags(BeQt::deserialize(m.value("tags").toByteArray()).toStringList());
        info.setComment(m.value("comment").toString());
        info.setCreationDateTime(QDateTime::fromMSecsSinceEpoch(m.value("creation_dt").toLongLong()));
        info.setUpdateDateTime(QDateTime::fromMSecsSinceEpoch(m.value("update_dt").toLongLong()));
        info.setExtraAttachedFileNames(QDir(path + "_extra").entryList(QDir::Files));
        newLabs << info;
    }
    foreach (const QVariant &v, r2.values())
    {
        quint64 labId = v.toMap().value("id").toULongLong();
        QStringList lg = labGroups(labId);
        if (!lg.isEmpty() && !BTextTools::intersects(lg, groups))
            continue;
        deletedLabs << labId;
    }
    return TOperationResult(true);
}

TOperationResult Storage::getLab(quint64 labId, BeQt::OSType osType, TLabProject &project, TLabInfo::Type &t,
                                 QString &url)
{
    if (!labId)
        return TOperationResult(TMessage::InvalidLabIdError);
    if (BeQt::UnknownOS == osType)
        return TOperationResult(TMessage::InvalidOSTypeError);
    if (!isValid())
        return TOperationResult(TMessage::InternalStorageError);
    t = labType(labId);
    if (TLabInfo::NoType == t)
        return TOperationResult(TMessage::InternalStorageError);
    project.clear();
    url.clear();
    switch (t)
    {
    case TLabInfo::DesktopType:
    {
        QString subdir = labSubdir(osType);
        if (subdir.isEmpty())
            return TOperationResult(TMessage::InternalStorageError);
        BSqlWhere w("id = :id", ":id", labId);
        QString fn = mdb->select("labs", "file_name_" + subdir, w).value("file_name_" + subdir).toString();
        if (fn.isEmpty())
            return TOperationResult(TMessage::NoLabForPlatformError);
        if (!project.load(rootDir() + "/labs/" + QString::number(labId) + "/" + subdir, fn))
            return TOperationResult(TMessage::InternalFileSystemError);
        break;
    }
    case TLabInfo::WebType:
    {
        BSqlWhere w("id = :id", ":id", labId);
        QString fn = mdb->select("labs", "file_name_web", w).value("file_name_web").toString();
        if (fn.isEmpty())
            return TOperationResult(TMessage::InternalStorageError);
        if (!project.load(rootDir() + "/labs/" + QString::number(labId), fn))
            return TOperationResult(TMessage::InternalFileSystemError);
        break;
    }
    case TLabInfo::UrlType:
    {
        url = mdb->select("labs", "url", BSqlWhere("id = :id", ":id", labId)).value("url").toString();
        if (url.isEmpty())
            return TOperationResult(TMessage::InternalStorageError);
        break;
    }
    default:
        return TOperationResult(TMessage::InternalStorageError);
    }
    return TOperationResult(true);
}

TOperationResult Storage::getLabExtraAttachedFile(quint64 labId, const QString &fn, TProjectFile &file)
{
    if (!labId)
        return TOperationResult(TMessage::InvalidLabIdError);
    if (fn.isEmpty())
        return TOperationResult(TMessage::InvalidFileNameError);
    if (!isValid())
        return TOperationResult(TMessage::InternalStorageError);
    file.clear();
    if (!file.loadAsBinary(rootDir() + "/labs/" + QString::number(labId) + "_extra/" + fn))
        return TOperationResult(TMessage::InternalFileSystemError);
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

quint64 Storage::sampleSenderId(quint64 sampleId)
{
    if (!sampleId || !isValid())
        return 0;
    BSqlWhere w("id = :id", ":id", sampleId);
    return mdb->select("samples", "sender_id", w).value("sender_id").toULongLong();
}

quint64 Storage::labSenderId(quint64 labId)
{
    if (!labId || !isValid())
        return 0;
    BSqlWhere w("id = :id", ":id", labId);
    return mdb->select("labs", "sender_id", w).value("sender_id").toULongLong();
}

QString Storage::userLogin(quint64 userId)
{
    if (!userId || !isValid())
        return "";
    BSqlWhere w("id = :id", ":id", userId);
    return mdb->select("users", "login", w).value("login").toString();
}

TAccessLevel Storage::userAccessLevel(quint64 userId)
{
    if (!userId || !isValid())
        return 0;
    BSqlWhere w("id = :id", ":id", userId);
    return mdb->select("users", "access_level", w).value("access_level").toInt();
}

TServiceList Storage::userServices(quint64 userId)
{
    if (!userId || !isValid())
        return TServiceList();
    if (userAccessLevel(userId) >= TAccessLevel::SuperuserLevel)
        return TServiceList::allServices();
    BSqlResult r = mdb->select("user_services", "service_id", BSqlWhere("user_id = :user_id", ":user_id", userId));
    if (!r)
        return TServiceList();
    QList<int> list;
    foreach (const QVariantMap &m, r.values())
        list << m.value("service_id").toInt();
    return TServiceList::serviceListFromIntList(list);
}

TServiceList Storage::newUserServices(quint64 inviteId)
{
    if (!inviteId || !isValid())
        return TServiceList();
    BSqlResult r = mdb->select("new_user_services", "service_id", BSqlWhere("invite_id = :id", ":id", inviteId));
    if (!r)
        return TServiceList();
    QList<int> list;
    foreach (const QVariantMap &m, r.values())
        list << m.value("service_id").toInt();
    return TServiceList::serviceListFromIntList(list);
}

TServiceList Storage::newUserServices(const QString &inviteCode)
{
    if (BeQt::uuidFromText(inviteCode).isNull() || !isValid())
        return TServiceList();
    BSqlWhere w("code = :code", ":code", inviteCode);
    return newUserServices(mdb->select("invite_codes", "id", w).value("id").toULongLong());
}

bool Storage::userHasAccessTo(quint64 userId, const TService service)
{
    if (!userId || !isValid())
        return false;
    if (userAccessLevel(userId) >= TAccessLevel::SuperuserLevel)
        return true;
    BSqlWhere w("user_id = :user_id", ":user_id", userId);
    return mdb->select("user_services", "service_id", w).value("service_id").toInt() == service;
}

QStringList Storage::newUserClabGroups(quint64 inviteId)
{
    if (!inviteId || !isValid())
        return QStringList();
    BSqlResult r = mdb->select("new_user_clab_groups", "group_name", BSqlWhere("invite_id = :id", ":id", inviteId));
    if (!r)
        return QStringList();
    QStringList list;
    foreach (const QVariantMap &m, r.values())
        list << m.value("group_name").toString();
    return list;
}

QStringList Storage::newUserClabGroups(const QString &inviteCode)
{
    if (BeQt::uuidFromText(inviteCode).isNull() || !isValid())
        return QStringList();
    BSqlWhere w("code = :code", ":code", inviteCode);
    return newUserClabGroups(mdb->select("invite_codes", "id", w).value("id").toULongLong());
}

QStringList Storage::userClabGroups(quint64 userId)
{
    if (!userId || !isValid())
        return QStringList();
    QStringList list;
    if (userAccessLevel(userId) >= TAccessLevel::AdminLevel)
    {
        foreach (const QVariantMap &m, mdb->select("clab_groups", "name").values())
            list << m.value("name").toString();
    }
    else
    {
        BSqlWhere w("user_id = :user_id", ":user_id", userId);
        foreach (const QVariantMap &m, mdb->select("user_clab_groups", "group_name", w).values())
            list << m.value("group_name").toString();
    }
    return list;
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

Storage::State Storage::sampleState(quint64 sampleId)
{
    if (!sampleId || !isValid())
        return NormalState;
    BSqlWhere w("id = :id", ":id", sampleId);
    return static_cast<State>(mdb->select("samples", "state", w).value("state", NormalState).toInt());
}

Storage::State Storage::labState(quint64 labId)
{
    if (!labId || !isValid())
        return NormalState;
    BSqlWhere w("id = :id", ":id", labId);
    return static_cast<State>(mdb->select("labs", "state", w).value("state", NormalState).toInt());
}

QStringList Storage::labGroups(quint64 labId)
{
    if (!labId || !isValid())
        return QStringList();
    QStringList list;
    BSqlWhere w("lab_id = :lab_id", ":lab_id", labId);
    foreach (const QVariantMap &m, mdb->select("lab_clab_groups", "group_name", w).values())
        list << m.value("group_name").toString();
    return list;
}

TLabInfo::Type Storage::labType(quint64 labId)
{
    if (!labId || !isValid())
        return TLabInfo::NoType;
    BSqlWhere w("id = :id", ":id", labId);
    return static_cast<TLabInfo::Type>(mdb->select("labs", "type", w).value("type").toInt());
}

QDateTime Storage::sampleCreationDateTime(quint64 sampleId, Qt::TimeSpec spec)
{
    if (!sampleId || !isValid())
        return QDateTime().toTimeSpec(spec);
    BSqlWhere w("id = :id", ":id", sampleId);
    qint64 msecs = mdb->select("samples", "creation_dt", w).value("creation_dt").toULongLong();
    return QDateTime::fromMSecsSinceEpoch(msecs).toTimeSpec(spec);
}

QDateTime Storage::sampleUpdateDateTime(quint64 sampleId, Qt::TimeSpec spec)
{
    if (!sampleId || !isValid())
        return QDateTime().toTimeSpec(spec);
    BSqlWhere w("id = :id", ":id", sampleId);
    qint64 msecs = mdb->select("samples", "update_dt", w).value("update_dt").toULongLong();
    return QDateTime::fromMSecsSinceEpoch(msecs).toTimeSpec(spec);
}

quint64 Storage::inviteId(const QString &inviteCode)
{
    if (BeQt::uuidFromText(inviteCode).isNull() || !isValid())
        return 0;
    BSqlWhere w("code = :code AND expiration_dt > :expiration_dt", ":code", BeQt::pureUuidText(inviteCode),
                ":expiration_dt", QDateTime::currentDateTimeUtc().toMSecsSinceEpoch());
    return mdb->select("invite_codes", "id", w).value("id").toULongLong();
}

bool Storage::isValid() const
{
    return QFileInfo(rootDir()).isDir() && mdb && mdb->isOpen();
}

bool Storage::testInvites()
{
    if (!isValid() || !mdb->transaction())
        return false;
    BSqlWhere w("expiration_dt <= :current_dt", ":current_dt", QDateTime::currentDateTimeUtc().toMSecsSinceEpoch());
    bool b = mdb->deleteFrom("invite_codes", w);
    return mdb->endTransaction(b) && b;
}

bool Storage::testRecoveryCodes()
{
    if (!isValid() || !mdb->transaction())
        return false;
    BSqlWhere w("expiration_dt <= :current_dt", ":current_dt", QDateTime::currentDateTimeUtc().toMSecsSinceEpoch());
    bool b = mdb->deleteFrom("recovery_codes", w);
    return mdb->endTransaction(b) && b;
}*/

/*============================== Static private methods ====================*/

/*QString Storage::rootDir()
{
    return BCoreApplication::location(BCoreApplication::DataPath, BCoreApplication::UserResource);
}

QString Storage::labSubdir(BeQt::OSType osType)
{
    switch (osType)
    {
    case BeQt::LinuxOS:
        return "linux";
    case BeQt::MacOS:
        return "mac";
    case BeQt::WindowsOS:
        return "win";
    default:
        return "";
    }
}*/

/*============================== Private methods ===========================*/

/*TOperationResult Storage::addUserInternal(const TUserInfo &info, const QLocale &locale, const QStringList &clabGroups)
{
    if (Global::readOnly())
        return TOperationResult(TMessage::ReadOnlyError);
    if (!info.isValid(TUserInfo::AddContext))
        return TOperationResult(TMessage::InternalParametersError);
    if (!isValid())
        return TOperationResult(TMessage::InternalStorageError);
    if (!isUserUnique(info.login(), info.email()))
        return TOperationResult(TMessage::LoginOrEmailOccupiedError);
    if (!mdb->transaction())
        return TOperationResult(TMessage::InternalDatabaseError);
    qint64 msecs = QDateTime::currentDateTimeUtc().toMSecsSinceEpoch();
    QVariantMap m;
    m.insert("email", info.email());
    m.insert("login", info.login());
    if (!info.password().isEmpty())
        m.insert("password", info.password());
    m.insert("access_level", (int) info.accessLevel());
    m.insert("real_name", info.realName());
    m.insert("creation_dt", msecs);
    m.insert("update_dt", msecs);
    BSqlResult qr = mdb->insert("users", m);
    QUuid uuid = BeQt::uuidFromText(info.inviteCode());
    BSqlWhere w("code = :code", ":code", BeQt::pureUuidText(uuid));
    if (!qr || (!uuid.isNull() && !mdb->deleteFrom("invite_codes", w)))
    {
        mdb->rollback();
        return TOperationResult(TMessage::InternalQueryError);
    }
    quint64 userId = qr.lastInsertId().toULongLong();
    foreach (const TService &s, info.services())
    {
        if (!mdb->insert("user_services", "user_id", userId, "service_id", (int) s))
        {
            mdb->rollback();
            return TOperationResult(TMessage::InternalQueryError);
        }
    }
    foreach (const QString &gr, clabGroups)
    {
        if (!mdb->insert("user_clab_groups", "user_id", userId, "group_name", gr))
        {
            mdb->rollback();
            return TOperationResult(TMessage::InternalQueryError);
        }
    }
    if(!saveUserAvatar(userId, info.avatar()))
        return TOperationResult(TMessage::InternalFileSystemError);
    if (!mdb->commit())
    {
        BDirTools::rmdir(rootDir() + "/users/" + QString::number(userId));
        return TOperationResult(TMessage::InternalDatabaseError);
    }
    Global::StringMap replace;
    replace.insert("%username%", info.login());
    Global::sendEmail(info.email(), "register", locale, replace);
    return TOperationResult(true);
}

TOperationResult Storage::editUserInternal(const TUserInfo &info, bool editClab, const QStringList &clabGroups)
{
    if (Global::readOnly())
        return TOperationResult(TMessage::ReadOnlyError);
    if (!info.isValid(TUserInfo::EditContext) && !info.isValid(TUserInfo::UpdateContext))
        return TOperationResult(TMessage::InternalParametersError);
    quint64 id = info.id();
    if (!id)
        id = userId(info.login());
    if (!id)
        return TOperationResult(TMessage::InternalParametersError);
    if (!isValid())
        return TOperationResult(TMessage::InternalStorageError);
    if (!mdb->transaction())
        return TOperationResult(TMessage::InternalDatabaseError);
    QVariantMap m;
    m.insert("real_name", info.realName());
    if (info.context() == TUserInfo::EditContext)
        m.insert("access_level", (int) info.accessLevel());
    else
        m.insert("password", info.password());
    m.insert("update_dt", QDateTime::currentDateTimeUtc().toMSecsSinceEpoch());
    if (!mdb->update("users", m, BSqlWhere("id = :id", ":id", id)))
    {
        mdb->rollback();
        return TOperationResult(TMessage::InternalQueryError);
    }
    if (info.context() == TUserInfo::EditContext)
    {
        if (!mdb->deleteFrom("user_services", BSqlWhere("user_id = :user_id", ":user_id", id)))
        {
            mdb->rollback();
            return TOperationResult(TMessage::InternalQueryError);
        }
        foreach (const TService &s, info.services())
        {
            if (!mdb->insert("user_services", "user_id", id, "service_id", (int) s))
            {
                mdb->rollback();
                return TOperationResult(TMessage::InternalQueryError);
            }
        }
        if (editClab)
        {
            if (!mdb->deleteFrom("user_clab_groups", BSqlWhere("user_id = :user_id", ":user_id", id)))
            {
                mdb->rollback();
                return TOperationResult(TMessage::InternalQueryError);
            }
            foreach (const QString &gr, clabGroups)
            {
                if (!mdb->insert("user_clab_groups", "user_id", id, "group_name", gr))
                {
                    mdb->rollback();
                    return TOperationResult(TMessage::InternalQueryError);
                }
            }
        }
    }
    QByteArray avatarOld = loadUserAvatar(id); //Backing up previous avatar
    if(!saveUserAvatar(id, info.avatar()))
    {
        saveUserAvatar(id, avatarOld);
        return TOperationResult(TMessage::InternalFileSystemError);
    }
    if (!mdb->commit())
    {
        saveUserAvatar(id, avatarOld);
        return TOperationResult(TMessage::InternalDatabaseError);
    }
    return TOperationResult(true);
}

bool Storage::saveUserAvatar(quint64 userId, const QByteArray &data) const
{
    if (!isValid() || !userId)
        return false;
    QString path = rootDir() + "/users/" + QString::number(userId);
    if (!BDirTools::mkpath(path))
        return false;
    if (!data.isEmpty())
    {
        return BDirTools::writeFile(path + "/avatar.dat", data);
    }
    else
    {
        QFileInfo fi(path + "/avatar.dat");
        return !fi.isFile() || QFile(fi.filePath()).remove();
    }
}

QByteArray Storage::loadUserAvatar(quint64 userId, bool *ok) const
{
    if (!isValid() || !userId)
        return bRet(ok, false, QByteArray());
    QString path = rootDir() + "/users/" + QString::number(userId);
    if (!QFileInfo(path).isDir())
        return bRet(ok, false, QByteArray());
    if (!QFileInfo(path + "/avatar.dat").exists())
        return bRet(ok, true, QByteArray());
    return BDirTools::readFile(path + "/avatar.dat", -1, ok);
}*/

/*============================== Static private members ====================*/

//QString Storage::mtexsampleSty;
//QString Storage::mtexsampleTex;
