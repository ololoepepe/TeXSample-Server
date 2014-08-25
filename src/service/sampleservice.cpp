/****************************************************************************
**
** Copyright (C) 2012-2014 Andrey Bogdanov
**
** This file is part of TeXSample Server.
**
** TeXSample Server is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** TeXSample Server is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with TeXSample Server.  If not, see <http://www.gnu.org/licenses/>.
**
****************************************************************************/

#include "sampleservice.h"

#include "datasource.h"
#include "entity/sample.h"
#include "repository/samplerepository.h"

/*============================================================================
================================ SampleService ===============================
============================================================================*/

/*============================== Public constructors =======================*/

SampleService::SampleService(DataSource *source) :
    SampleRepo(new SampleRepository(source)), Source(source)
{
    //
}

SampleService::~SampleService()
{
    //
}

/*============================== Public methods ============================*/

DataSource *SampleService::dataSource() const
{
    return Source;
}

bool SampleService::isValid() const
{
    return Source && Source->isValid() && SampleRepo->isValid();
}

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

/*TCompilationResult compileProject(const CompileParameters &p)
{
    static const QStringList Suffixes = QStringList() << "*.aux" << "*.dvi" << "*.idx" << "*.ilg" << "*.ind"
                                                      << "*.log" << "*.out" << "*.pdf" << "*.toc";
    if (!p.project.isValid())
        return TCompilationResult(TMessage::InternalParametersError);
    if (p.path.isEmpty() || !BDirTools::mkpath(p.path))
        return TCompilationResult(TMessage::InternalFileSystemError);
    QString codecName = p.compiledProject ? p.param.codecName() : QString("UTF-8");
    if (!p.project.save(p.path, codecName) || !Storage::copyTexsample(p.path, codecName))
    {
        BDirTools::rmdir(p.path);
        return TCompilationResult(TMessage::InternalFileSystemError);
    }
    QString fn = p.project.rootFileName();
    QString tmpfn = BeQt::pureUuidText(QUuid::createUuid()) + ".tex";
    QString baseName = QFileInfo(fn).baseName();
    TCompilationResult r;
    QString command = !p.compiledProject ? "pdflatex" : p.param.compilerCommand();
    QStringList args = QStringList() << "-interaction=nonstopmode";
    if (!p.compiledProject)
    {
        if (!QFile::rename(p.path + "/" + fn, p.path + "/" + tmpfn))
            return TCompilationResult(TMessage::InternalFileSystemError);
        args << ("-jobname=" + baseName) << ("\\input texsample.tex \\input " + tmpfn + " \\end{document}");
    }
    else
    {
        args << p.param.commands() << (p.path + "/" + fn) << p.param.options();
    }
    QString log;
    int code = BeQt::execProcess(p.path, command, args, 5 * BeQt::Second, 2 * BeQt::Minute, &log);
    if (!p.compiledProject && !QFile::rename(p.path + "/" + tmpfn, p.path + "/" + fn))
        return TCompilationResult(TMessage::InternalFileSystemError);
    if (!Storage::removeTexsample(p.path))
        return TCompilationResult(TMessage::InternalFileSystemError);
    r.setSuccess(!code);
    r.setExitCode(code);
    r.setLog(log);
    if (p.compiledProject && r && p.param.makeindexEnabled())
    {
        QString mlog;
        int mcode = BeQt::execProcess(p.path, "makeindex", QStringList() << (p.path + "/" + baseName),
                                      5 * BeQt::Second, BeQt::Minute, &mlog);
        if (p.makeindexResult)
        {
            p.makeindexResult->setSuccess(!mcode);
            p.makeindexResult->setExitCode(mcode);
            p.makeindexResult->setLog(mlog);
        }
        if (!mcode)
        {
            code = BeQt::execProcess(p.path, p.param.compilerCommand(), args, 5 * BeQt::Second, 5 * BeQt::Minute, &log);
            r.setSuccess(!code);
            r.setExitCode(code);
            r.setLog(log);
        }
    }
    if (p.compiledProject && r && p.param.dvipsEnabled())
    {
        QString dlog;
        int dcode = BeQt::execProcess(p.path, "dvips", QStringList() << (p.path + "/" + baseName),
                                      5 * BeQt::Second, BeQt::Minute, &dlog);
        if (p.dvipsResult)
        {
            p.dvipsResult->setSuccess(!dcode);
            p.dvipsResult->setExitCode(dcode);
            p.dvipsResult->setLog(dlog);
        }
    }
    if (r && p.compiledProject)
        p.compiledProject->load(p.path, Suffixes);
    if (p.compiledProject || !r)
        BDirTools::rmdir(p.path);
    return r;
}*/

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
}*/
