#include "storage.h"
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

/*============================== Public methods ============================*/

/*TCompilationResult Storage::addSample(quint64 userId, TTexProject project, const TSampleInfo &info)
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
}*/
