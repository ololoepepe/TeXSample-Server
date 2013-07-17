#include "storage.h"
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
#include <TMessage>

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
#include <QLocale>

#include <QDebug>

/*============================================================================
================================ Storage =====================================
============================================================================*/

/*============================== Static public methods =====================*/

bool Storage::initStorage(TMessage *msg)
{
    static QMutex mutex;
    static bool isInit = false;
    QMutexLocker locker(&mutex);
    if (isInit)
        return true; //TODO: message
    QString sty = BDirTools::findResource("texsample-framework/texsample.sty", BDirTools::GlobalOnly);
    sty = BDirTools::readTextFile(sty, "UTF-8");
    if (sty.isEmpty())
        return bRet(msg, TMessage(), false); //TODO: message
    QString tex = BDirTools::findResource("texsample-framework/texsample.tex", BDirTools::GlobalOnly);
    tex = BDirTools::readTextFile(tex, "UTF-8");
    if (tex.isEmpty())
        return bRet(msg, TMessage(), false); //TODO: message
    BSqlDatabase db("QSQLITE", QUuid::createUuid().toString());
    db.setDatabaseName(rootDir() + "/texsample.sqlite");
    db.setOnOpenQuery("PRAGMA foreign_keys = ON");
    QString schema = BDirTools::findResource("db/texsample.schema", BDirTools::GlobalOnly);
    QStringList list = BDirTools::readTextFile(schema, "UTF-8").split(";\n");
    foreach (int i, bRangeD(0, list.size() - 1))
    {
        list[i].replace('\n', ' ');
        list[i].replace(QRegExp("\\s+"), " ");
    }
    list.removeAll("");
    list.removeDuplicates();
    if (list.isEmpty())
        return bRet(msg, TMessage(), false); //TODO: message
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
                return bRet(msg, TMessage(), false); //TODO
        }
    }
    bool users = !db.select("users", "id", BSqlWhere("login = :login", ":login", QString("root"))).values().isEmpty();
    if (Global::readOnly() && !users)
        return bRet(msg, TMessage(), false); //TODO: message
    foreach (const QString &qs, list)
    {
        if (!db.transaction())
            return bRet(msg, TMessage(), false); //TODO
        if (!db.exec(qs))
        {
            db.rollback();
            return bRet(msg, TMessage(), false); //TODO
        }
        if (!db.commit())
            return bRet(msg, TMessage(), false); //TODO
    }
    Storage s;
    if (!s.isValid())
        return bRet(msg, TMessage(), false); //TODO
    if (!Global::readOnly() && (!s.testInvites() || !s.testRecoveryCodes()))
        return bRet(msg, TMessage(), false); //TODO
    if (!users)
    {
        QString mail = BTerminalIOHandler::readLine("\n" + tr("Enter root e-mail:") + " ");
        if (mail.isEmpty())
            return bRet(msg, TMessage(), false); //TODO
        BTerminalIOHandler::setStdinEchoEnabled(false);
        QString pwd = BTerminalIOHandler::readLine(tr("Enter root password:") + " ");
        BTerminalIOHandler::setStdinEchoEnabled(true);
        BTerminalIOHandler::writeLine();
        if (pwd.isEmpty())
            return bRet(msg, TMessage(), false); //TODO
        TUserInfo info(TUserInfo::AddContext);
        info.setLogin("root");
        info.setEmail(mail);
        info.setPassword(pwd);
        info.setAccessLevel(TAccessLevel::RootLevel);
        TOperationResult r = s.addUser(info, BCoreApplication::locale());
        if (!r)
            return bRet(msg, TMessage(), false); //TODO
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
    mdb = new BSqlDatabase("QSQLITE", QUuid::createUuid().toString());
    mdb->setDatabaseName(BDirTools::findResource("texsample.sqlite", BDirTools::UserOnly));
    mdb->setOnOpenQuery("PRAGMA foreign_keys = ON");
    mdb->open();
}

Storage::~Storage()
{
    delete mdb;
}

/*============================== Public methods ============================*/

TOperationResult Storage::addUser(const TUserInfo &info, const QLocale &locale, const QString &inviteCode)
{
    if (Global::readOnly())
        return TOperationResult(0); //TODO
    if (!info.isValid(TUserInfo::AddContext))
        return TOperationResult(0); //TODO
    if (!isValid())
        return TOperationResult(0); //TODO
    if (!isUserUnique(info.login(), info.email()))
        return TOperationResult(0); //TODO
    if (!mdb->transaction())
        return TOperationResult(0); //TODO
    qint64 msecs = QDateTime::currentDateTimeUtc().toMSecsSinceEpoch();
    QVariantMap m;
    m.insert("email", info.email());
    m.insert("login", info.login());
    m.insert("password", info.password());
    m.insert("access_level", (int) info.accessLevel());
    m.insert("real_name", info.realName());
    m.insert("createdion_dt", msecs);
    m.insert("update_dt", msecs);
    BSqlResult qr = mdb->insert("users", m);
    QUuid uuid = BeQt::uuidFromText(inviteCode);
    if (!qr || (!uuid.isNull() && !mdb->deleteFrom("invites", BSqlWhere("code = :code", ":code",
                                                                        BeQt::pureUuidText(uuid)))))
    {
        mdb->rollback();
        return TOperationResult(0); //TODO
    }
    quint64 userId = qr.lastInsertId().toULongLong();
    if(!saveUserAvatar(userId, info.avatar()))
        return TOperationResult(0); //TODO
    if (!mdb->commit())
    {
        BDirTools::rmdir(rootDir() + "/users/" + QString::number(userId));
        return TOperationResult(0); //TODO
    }
    Global::StringMap replace;
    replace.insert("%username%", info.login());
    Global::sendEmail(info.email(), "register", locale, replace);
    return TOperationResult(true);
}

TOperationResult Storage::editUser(const TUserInfo &info)
{
    if (Global::readOnly())
        return TOperationResult(0); //TODO
    if (!info.isValid(TUserInfo::EditContext))
        return TOperationResult(0); //TODO
    if (!isValid())
        return TOperationResult(0); //TODO
    if (!mdb->transaction())
        return TOperationResult(0); //TODO
    QVariantMap m;
    m.insert("real_name", info.realName());
    m.insert("access_level", (int) info.accessLevel());
    m.insert("mupdate_dt", QDateTime::currentDateTimeUtc().toMSecsSinceEpoch());
    if (!info.password().isEmpty())
        m.insert("password", info.password());
    if (!mdb->update("users", m, BSqlWhere("id = :id", ":id", info.id())))
    {
        mdb->rollback();
        return TOperationResult(0); //TODO
    }
    //TODO: backup avatar
    if(!saveUserAvatar(info.id(), info.avatar()))
        return TOperationResult(0); //TODO
    if (!mdb->commit())
        return TOperationResult(0); //TODO
    return TOperationResult(true);
}

TOperationResult Storage::getUserInfo(quint64 userId, TUserInfo &info, QDateTime &updateDT, bool &cacheOk)
{
    if (!userId)
        return TOperationResult(0); //TODO
    if (!isValid())
        return TOperationResult(0); //TODO
    QStringList sl = QStringList() << "login" << "access_level" << "real_name" << "creation_dt" << "update_dt";
    BSqlResult r = mdb->select("users", sl, BSqlWhere("id = :id", ":id", userId));
    if (!r)
        return TOperationResult(0); //TODO
    cacheOk = r.value("update_dt").toLongLong() <= updateDT.toUTC().toMSecsSinceEpoch();
    updateDT = QDateTime::currentDateTimeUtc();
    if (cacheOk)
        return TOperationResult(true);
    bool ok = false;
    QByteArray avatar = loadUserAvatar(userId, &ok);
    if (!ok)
        return TOperationResult(0); //TODO
    info.setId(userId);
    info.setLogin(r.value("login").toString());
    info.setAccessLevel(r.value("access_level").toInt());
    info.setRealName(r.value("real_name").toString());
    info.setAvatar(avatar);
    info.setCreationDateTime(QDateTime::fromMSecsSinceEpoch(r.value("creation_dt").toULongLong()));
    info.setUpdateDateTime(QDateTime::fromMSecsSinceEpoch(r.value("update_dt").toULongLong()));
    return TOperationResult(true);
}

TOperationResult Storage::getShortUserInfo(quint64 userId, TUserInfo &info)
{
    if (!userId)
        return TOperationResult(0); //TODO
    if (!isValid())
        return TOperationResult(0); //TODO
    BSqlResult r = mdb->select("users", QStringList() << "login" << "real_name", BSqlWhere("id = :id", ":id", userId));
    if (!r)
        return TOperationResult(0); //TODO
    info.setContext(TUserInfo::ShortInfoContext);
    info.setId(userId);
    info.setLogin(r.value("login").toString());
    info.setRealName(r.value("real_name").toString());
    return TOperationResult(true);
}

TCompilationResult Storage::addSample(quint64 userId, TProject project, const TSampleInfo &info)
{
    if (Global::readOnly())
        return TOperationResult(0); //TODO
    if (!userId || !project.isValid() || !info.isValid(TSampleInfo::AddContext))
        return TOperationResult(0); //TODO
    if (!isValid())
        return TOperationResult(0); //TODO
    if (!mdb->transaction())
        return TOperationResult(0); //TODO
    qint64 msecs = QDateTime::currentDateTimeUtc().toMSecsSinceEpoch();
    QVariantMap m;
    m.insert("sender_id", userId);
    m.insert("title", info.title());
    m.insert("file_name", info.fileName());
    m.insert("authors", info.authorsString());
    m.insert("tags", info.tagsString());
    m.insert("comment", info.comment());
    m.insert("creation_dt", msecs);
    m.insert("update_dt", msecs);
    BSqlResult qr = mdb->insert("samples", m);
    if (!qr)
    {
        mdb->rollback();
        return TOperationResult(0); //TODO
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
        return TOperationResult(0); //TODO
    }
    //TODO: rmdir if fails
    if (!mdb->commit())
        return TOperationResult(0); //TODO
    return cr;
}

TCompilationResult Storage::editSample(const TSampleInfo &info, TProject project)
{
    if (Global::readOnly())
        return TOperationResult(0); //TODO
    if (!info.isValid(TSampleInfo::EditContext) && !info.isValid(TSampleInfo::UpdateContext))
        return TOperationResult(0); //TODO
    if (!isValid())
        return TOperationResult(0); //TODO
    QString pfn = sampleFileName(info.id());
    if (pfn.isEmpty() || !mdb->transaction())
        return TOperationResult(0); //TODO
    QVariantMap m;
    m.insert("title", info.title());
    m.insert("authors", info.authorsString());
    m.insert("tags", info.tagsString());
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
        return TOperationResult(0); //TODO
    }
    QString spath = rootDir() + "/samples/" + QString::number(info.id());
    //TODO: rmdir if fails
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
                return TOperationResult(0); //TODO
            }
        }
    }
    //TODO: rmdir if fails
    TCompilationResult cr(true);
    if (project.isValid())
    {
        project.rootFile()->setFileName(info.fileName());
        project.removeRestrictedCommands();
        if (!BDirTools::rmdir(spath))
        {
            mdb->rollback();
            return TOperationResult(0); //TODO
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
            return TOperationResult(0); //TODO
        }
    }
    //TODO: rmdir if fails
    if (!mdb->commit())
        return TOperationResult(0); //TODO
    return cr;
}

TOperationResult Storage::deleteSample(quint64 sampleId, const QString &reason)
{
    if (Global::readOnly())
        return TOperationResult(0); //TODO
    if (!sampleId)
        return TOperationResult(0); //TODO
    if (!isValid())
        return TOperationResult(0); //TODO
    if (sampleState(sampleId))
        return TOperationResult(0); //TODO
    if (!mdb->transaction())
        return TOperationResult(0); //TODO
    if (!mdb->update("samples", "state", 1, "deletion_reason", reason, BSqlWhere("id = :id", ":id", sampleId)))
    {
        mdb->rollback();
        return TOperationResult(0); //TODO
    }
    //TODO: backup files
    if (!BDirTools::rmdir(rootDir() + "/samples/" + QString::number(sampleId)))
    {
        mdb->rollback();
        return TOperationResult(0); //TODO
    }
    if (!mdb->commit())
        return TOperationResult(0); //TODO
    return TOperationResult(true);
}

TOperationResult Storage::getSampleSource(quint64 sampleId, TProject &project, QDateTime &updateDT, bool &cacheOk)
{
    if (!sampleId)
        return TOperationResult(0); //TODO
    if (!isValid())
        return TOperationResult(0); //TODO
    if (updateDT.toUTC() >= sampleUpdateDateTime(sampleId))
    {
        cacheOk = true;
        return TOperationResult(true);
    }
    QString fn = sampleFileName(sampleId);
    if (fn.isEmpty())
        return TOperationResult(0); //TODO
    updateDT = QDateTime::currentDateTimeUtc();
    //TODO: set message if error
    return TOperationResult(project.load(rootDir() + "/samples/" + QString::number(sampleId) + "/" + fn, "UTF-8"));
}

TOperationResult Storage::getSamplePreview(quint64 sampleId, TProjectFile &file, QDateTime &updateDT, bool &cacheOk)
{
    if (!sampleId)
        return TOperationResult(0); //TODO
    if (!isValid())
        return TOperationResult(0); //TODO
    if (updateDT.toUTC() >= sampleUpdateDateTime(sampleId))
    {
        cacheOk = true;
        return TOperationResult(true);
    }
    QString fn = sampleFileName(sampleId);
    if (fn.isEmpty())
        return TOperationResult(0); //TODO
    updateDT = QDateTime::currentDateTimeUtc();
    fn = rootDir() + "/samples/" + QString::number(sampleId) + "/" + QFileInfo(fn).baseName() + ".pdf";
    //TODO: set message if error
    return TOperationResult(file.loadAsBinary(fn, ""));
}

TOperationResult Storage::getSamplesList(TSampleInfoList &newSamples, TIdList &deletedSamples, QDateTime &updateDT)
{
    if (!isValid())
        return TOperationResult(0); //TODO
    qint64 updateMsecs = updateDT.toUTC().toMSecsSinceEpoch();
    QStringList sl1 = QStringList() << "id" << "sender_id" << "authors" << "title" << "file_name" << "type" << "tags"
                                    << "rating" << "admin_remark" << "creation_dt" << "update_dt";
    BSqlWhere w1("state = :state AND update_dt >= :update_dt", ":state", 0, ":update_dt", updateMsecs);
    BSqlResult r1 = mdb->select("samples", sl1, w1);
    if (!r1)
        return TOperationResult(0); //TODO
    QVariantMap wbv2;
    wbv2.insert(":state", 1);
    wbv2.insert(":update_dt", updateMsecs);
    wbv2.insert(":update_dt_hack", updateMsecs);
    BSqlWhere w2("state = :state AND deletion_dt >= :update_dt AND creation_dt < :update_dt_hack", wbv2);
    BSqlResult r2 = mdb->select("samples", "id", w2);
    if (!r2)
        return TOperationResult(0); //TODO
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
        info.setProjectSize(TProject::size(rootDir() + "/samples/" + info.idString() + "/" + info.fileName(),
                                           "UTF-8"));
        info.setTags(m.value("tags").toString());
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

TOperationResult Storage::generateInvites(quint64 userId, const QDateTime &expiresDT, quint8 count,
                                          TInviteInfoList &invites)
{
    if (Global::readOnly())
        return TOperationResult(0); //TODO
    if (!userId || !expiresDT.isValid() || !count || count > Texsample::MaximumInvitesCount)
        return TOperationResult(0); //TODO
    if (!isValid())
        return TOperationResult(0); //TODO
    if (!mdb->transaction())
        return TOperationResult(0); //TODO
    invites.clear();
    QVariantMap m;
    m.insert("creator_id", userId);
    m.insert("expiration_dt", expiresDT.toUTC().toMSecsSinceEpoch());
    TInviteInfo info;
    info.setCreatorId(userId);
    info.setExpirationDateTime(expiresDT.toUTC());
    quint8 i = 0;
    while (i < count)
    {
        QUuid uuid = QUuid::createUuid();
        QDateTime createdDT = QDateTime::currentDateTimeUtc();
        m.insert("code", BeQt::pureUuidText(uuid));
        m.insert("creation_dt", createdDT.toMSecsSinceEpoch());
        BSqlResult r = mdb->insert("invites", m);
        if (!r)
        {
            mdb->rollback();
            return TOperationResult(0); //TODO
        }
        info.setId(r.lastInsertId().toULongLong());
        info.setCode(uuid);
        info.setCreationDateTime(createdDT);
        invites << info;
        ++i;
        bApp->scheduleInvitesAutoTest(info);
    }
    if (!mdb->commit())
        return TOperationResult(0); //TODO
    return TOperationResult(true);
}

TOperationResult Storage::getInvitesList(quint64 userId, TInviteInfoList &invites)
{
    if (!userId)
        return TOperationResult(0); //TODO
    if (!isValid())
        return TOperationResult(0); //TODO
    QStringList fields = QStringList() << "id" << "code" << "creator_id" << "expiration_dt" << "creation_dt";
    BSqlResult r = mdb->select("invites", fields, BSqlWhere("creator_id = :creator_id", ":creator_id", userId));
    if (!r)
        return TOperationResult(0); //TODO
    foreach (const QVariantMap &m, r.values())
    {
        TInviteInfo info;
        info.setId(m.value("id").toULongLong());
        info.setCode(m.value("code").toString());
        info.setCreatorId(userId);
        info.setExpirationDateTime(QDateTime::fromMSecsSinceEpoch(m.value("expiration_dt").toULongLong()));
        info.setCreationDateTime(QDateTime::fromMSecsSinceEpoch(m.value("creation_dt").toULongLong()));
        invites << info;
    }
    return TOperationResult(true);
}

TOperationResult Storage::getRecoveryCode(const QString &email, const QLocale &locale)
{
    if (Global::readOnly())
        return TOperationResult(0); //TODO
    if (email.isEmpty())
        return TOperationResult(0); //TODO
    if (!isValid())
        return TOperationResult(0); //TODO
    quint64 userId = userIdByEmail(email);
    if (!userId || !mdb->transaction())
        return TOperationResult(0); //TODO
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
        return TOperationResult(0); //TODO
    }
    if (!mdb->commit())
        return TOperationResult(0); //TODO
    bApp->scheduleRecoveryCodesAutoTest(expDt);
    Global::StringMap replace;
    replace.insert("%code%", code);
    replace.insert("%username%", userLogin(userId));
    Global::sendEmail(email, "get_recovery_code", locale, replace);
    return TOperationResult(true);
}

TOperationResult Storage::recoverAccount(const QString &email, const QString &code, const QByteArray &password,
                                         const QLocale &locale)
{
    if (Global::readOnly())
        return TOperationResult(0); //TODO
    if (email.isEmpty() || BeQt::uuidFromText(code).isNull() || password.isEmpty())
        return TOperationResult(0); //TODO
    if (!isValid())
        return TOperationResult(0); //TODO
    quint64 userId = userIdByEmail(email);
    if (!userId)
        return TOperationResult(0); //TODO
    BSqlWhere w("requester_id = :requester_id AND code = :code", ":requester_id", userId, ":code",
                BeQt::pureUuidText(code));
    BSqlResult r = mdb->select("recovery_codes", "id", w);
    quint64 codeId = r.value("id").toULongLong();
    if (!r || !codeId)
        return TOperationResult(0); //TODO
    if (!mdb->transaction())
        return TOperationResult(0); //TODO
    if (!mdb->update("users", "password", password, "", QVariant(), BSqlWhere("id = :id", ":id", userId)))
    {
        mdb->rollback();
        return TOperationResult(0); //TODO
    }
    if (!mdb->deleteFrom("recovery_codes", BSqlWhere("id = :id", ":id", codeId)))
    {
        mdb->rollback();
        return TOperationResult(0); //TODO
    }
    if (!mdb->commit())
        return TOperationResult(0); //TODO
    Global::sendEmail(email, "recover_account", locale);
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

TAccessLevel Storage::userAccessLevel(quint64 userId)
{
    if (!userId || !isValid())
        return 0;
    BSqlWhere w("id = :id", ":id", userId);
    return mdb->select("users", "access_level", w).value("access_level").toInt();
}

quint64 Storage::inviteId(const QString &inviteCode)
{
    if (BeQt::uuidFromText(inviteCode).isNull() || !isValid())
        return 0;
    BSqlWhere w("code = :code AND expiration_dt > :expiration_dt", ":code", BeQt::pureUuidText(inviteCode),
                ":expiration_dt", QDateTime::currentDateTimeUtc().toMSecsSinceEpoch());
    return mdb->select("invites", "id", w).value("id").toULongLong();
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
    bool b = mdb->deleteFrom("invites", w);
    return mdb->endTransaction(b) && b;
}

bool Storage::testRecoveryCodes()
{
    if (!isValid() || !mdb->transaction())
        return false;
    BSqlWhere w("expiration_dt <= :current_dt", ":current_dt", QDateTime::currentDateTimeUtc().toMSecsSinceEpoch());
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
    QString path = rootDir() + "/users/" + QString::number(userId);
    if (!QFileInfo(path).isDir())
        return false;
    return data.isEmpty() || BDirTools::writeFile(path + "/avatar.dat", data);
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
    return  BDirTools::readFile(path + "/avatar.dat", -1, ok);
}

/*============================== Static private members ====================*/

QString Storage::mtexsampleSty;
QString Storage::mtexsampleTex;
