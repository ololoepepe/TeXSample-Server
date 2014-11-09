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

#include "application.h"
#include "datasource.h"
#include "entity/sample.h"
#include "entity/user.h"
#include "repository/samplerepository.h"
#include "repository/userrepository.h"
#include "requestin.h"
#include "requestout.h"
#include "temporarylocation.h"
#include "transactionholder.h"
#include "translator.h"

#include <TeXSample/TeXSampleCore>

#include <BDirTools>
#include <BUuid>

#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QLocale>
#include <QString>
#include <QStringList>
#include <QTextCodec>

/*============================================================================
================================ SampleService ===============================
============================================================================*/

/*============================== Public constructors =======================*/

SampleService::SampleService(DataSource *source) :
    SampleRepo(new SampleRepository(source)), Source(source), UserRepo(new UserRepository(source))
{
    //
}

SampleService::~SampleService()
{
    delete SampleRepo;
    delete UserRepo;
}

/*============================== Public methods ============================*/

RequestOut<TAddSampleReplyData> SampleService::addSample(const RequestIn<TAddSampleRequestData> &in, quint64 userId)
{
    typedef RequestOut<TAddSampleReplyData> Out;
    Translator t(in.locale());
    QString error;
    const TAddSampleRequestData &requestData = in.data();
    if (!commonCheck(t, requestData, &error))
        return Out(error);
    if (!checkUserId(t, userId, &error))
        return Out(error);
    bool ok = false;
    Sample entity;
    entity.setSenderId(userId);
    entity.setAuthors(requestData.authors());
    entity.setDescription(requestData.description());
    entity.setSource(requestData.project());
    entity.setPreviewMainFile(compilePreview(requestData.project(), &ok));
    if (!ok)
        return Out(t.translate("SampleService", "Failed to compile project", "error"));
    entity.setTags(requestData.tags());
    entity.setTitle(requestData.title());
    TransactionHolder holder(Source);
    quint64 id = SampleRepo->add(entity, &ok);
    if (!ok || !id)
        return Out(t.translate("SampleService", "Failed to add sample (internal)", "error"));
    entity = SampleRepo->findOne(id, &ok);
    if (!ok || !entity.isValid())
        return Out(t.translate("SampleService", "Failed to get sample (internal)", "error"));
    TSampleInfo info = sampleToSampleInfo(entity, &ok);
    if (!ok)
        return Out(t.translate("SampleService", "Failed to create sample info (internal)", "error"));
    if (!commit(t, holder, &error))
        return Out(error);
    TAddSampleReplyData replyData;
    replyData.setSampleInfo(info);
    return Out(replyData, info.creationDateTime());
}

RequestOut<TCompileTexProjectReplyData> SampleService::compileTexProject(
        const RequestIn<TCompileTexProjectRequestData> &in)
{
    typedef RequestOut<TCompileTexProjectReplyData> Out;
    Translator t(in.locale());
    QString error;
    const TCompileTexProjectRequestData &requestData = in.data();
    if (!commonCheck(t, requestData, &error))
        return Out(error);
    QDateTime dt = QDateTime::currentDateTimeUtc();
    TemporaryLocation loc(Source);
    if (!loc.isValid())
        return Out(t.translate("SampleService", "Failed to create temporary location (internal)", "error"));
    QString path = loc.absolutePath();
    QByteArray codecName = requestData.codec() ? requestData.codec()->name() : QByteArray("UTF-8");
    QTextCodec *codec = QTextCodec::codecForName(codecName);
    if (!Application::copyTexsample(path, codecName))
        return Out(t.translate("SampleService", "Failed to copy texsample (internal)", "error"));
    if (!requestData.project().save(path, codec))
        return Out(t.translate("SampleService", "Failed to save project (internal)", "error"));
    QString fileName = requestData.project().rootFile().fileName();
    QString baseName = QFileInfo(fileName).baseName();
    QStringList args = QStringList() << "-interaction=nonstopmode";
    args << requestData.commands() << (loc.absoluteFilePath(fileName)) << requestData.options();
    QString command = requestData.compiler().toString().toLower();
    QString output;
    int code = BeQt::execProcess(path, command, args, 5 * BeQt::Second, 2 * BeQt::Minute, &output, codec);
    TCompileTexProjectReplyData replyData;
    replyData.setExitCode(code);
    replyData.setOutput(output);
    if (code >= 0 && requestData.makeindexEnabled()) {
        QStringList margs = QStringList() << loc.absoluteFilePath(baseName);
        QString moutput;
        int mcode = BeQt::execProcess(path, "makeindex", margs, 5 * BeQt::Second, BeQt::Minute, &moutput, codec);
        replyData.setMakeindexExitCode(mcode);
        replyData.setMakeindexOutput(moutput);
        if (!mcode) {
            code = BeQt::execProcess(path, command, args, 5 * BeQt::Second, 5 * BeQt::Minute, &output, codec);
            replyData.setExitCode(code);
            replyData.setOutput(output);
        }
    }
    if (code >= 0 && requestData.dvipsEnabled()) {
        QStringList dargs = QStringList() << loc.absoluteFilePath(baseName);
        QString doutput;
        int dcode = BeQt::execProcess(path, "dvips", dargs, 5 * BeQt::Second, BeQt::Minute, &doutput, codec);
        replyData.setDvipsExitCode(dcode);
        replyData.setDvipsOutput(doutput);
    }
    if (code < 0)
        return Out(replyData, dt);
    QStringList nameFilters = QStringList() << (baseName + "*");
    QStringList fileNames = BDirTools::entryList(path, nameFilters, QDir::Files);
    fileNames.removeAll(loc.absoluteFilePath(fileName));
    TBinaryFileList list;
    foreach (const QString &fn, fileNames) {
        TBinaryFile f(fn);
        if (!f.isValid())
            return Out(t.translate("SampleService", "Failed to load file (internal)", "error"));
        list << f;
    }
    replyData.setFiles(list);
    return Out(replyData, dt);
}

DataSource *SampleService::dataSource() const
{
    return Source;
}

RequestOut<TDeleteSampleReplyData> SampleService::deleteSample(const RequestIn<TDeleteSampleRequestData> &in)
{
    typedef RequestOut<TDeleteSampleReplyData> Out;
    Translator t(in.locale());
    QString error;
    if (!commonCheck(t, in.data(), &error))
        return Out(error);
    bool ok = false;
    const TDeleteSampleRequestData &requestData = in.data();
    TransactionHolder holder(Source);
    QDateTime dt = SampleRepo->deleteOne(requestData.id(), &ok);
    if (!ok)
        return Out(t.translate("SampleService", "Failed to delete sample (internal)", "error"));
    TDeleteSampleReplyData replyData;
    if (!commit(t, holder, &error))
        return Out(error);
    return Out(replyData, dt);
}

RequestOut<TEditSampleReplyData> SampleService::editSample(const RequestIn<TEditSampleRequestData> &in)
{
    typedef RequestOut<TEditSampleReplyData> Out;
    Translator t(in.locale());
    QString error;
    if (!commonCheck(t, in.data(), &error))
        return Out(error);
    bool ok = false;
    const TEditSampleRequestData &requestData = in.data();
    Sample entity = SampleRepo->findOne(requestData.id(), &ok);
    if (!ok)
        return Out(t.translate("SampleService", "Failed to get sample (internal)", "error"));
    if (!entity.isValid())
        return Out(t.translate("SampleService", "No such sample", "error"));
    entity.convertToCreatedByUser();
    entity.setAuthors(requestData.authors());
    entity.setDescription(requestData.description());
    entity.setSaveAdminRemark(false);
    entity.setSaveData(requestData.editProject());
    if (requestData.editProject()) {
        entity.setSource(requestData.project());
        entity.setPreviewMainFile(compilePreview(requestData.project(), &ok));
        if (!ok)
            return Out(t.translate("SampleService", "Failed to compile project", "error"));
    }
    entity.setTags(requestData.tags());
    entity.setTitle(requestData.title());
    TransactionHolder holder(Source);
    SampleRepo->edit(entity, &ok);
    if (!ok)
        return Out(t.translate("SampleService", "Failed to edit sample (internal)", "error"));
    entity = SampleRepo->findOne(entity.id(), &ok);
    if (!ok || !entity.isValid())
        return Out(t.translate("SampleService", "Failed to get sample (internal)", "error"));
    TEditSampleReplyData replyData;
    TSampleInfo info = sampleToSampleInfo(entity, &ok);
    if (!ok)
        return Out(t.translate("SampleService", "Failed to create sample info (internal)", "error"));
    if (!commit(t, holder, &error))
        return Out(error);
    replyData.setSampleInfo(info);
    return Out(replyData, entity.lastModificationDateTime());
}

RequestOut<TEditSampleAdminReplyData> SampleService::editSampleAdmin(const RequestIn<TEditSampleAdminRequestData> &in)
{
    typedef RequestOut<TEditSampleAdminReplyData> Out;
    Translator t(in.locale());
    QString error;
    if (!commonCheck(t, in.data(), &error))
        return Out(error);
    bool ok = false;
    const TEditSampleAdminRequestData &requestData = in.data();
    Sample entity = SampleRepo->findOne(requestData.id(), &ok);
    if (!ok)
        return Out(t.translate("SampleService", "Failed to get sample (internal)", "error"));
    if (!entity.isValid())
        return Out(t.translate("SampleService", "No such sample", "error"));
    entity.convertToCreatedByUser();
    entity.setAdminRemark(requestData.adminRemark());
    entity.setAuthors(requestData.authors());
    entity.setDescription(requestData.description());
    entity.setSaveAdminRemark(true);
    entity.setSaveData(requestData.editProject());
    if (requestData.editProject()) {
        entity.setSource(requestData.project());
        entity.setPreviewMainFile(compilePreview(requestData.project(), &ok));
        if (!ok)
            return Out(t.translate("SampleService", "Failed to compile project", "error"));
    }
    entity.setRating(requestData.rating());
    entity.setTags(requestData.tags());
    entity.setTitle(requestData.title());
    entity.setType(requestData.type());
    TransactionHolder holder(Source);
    SampleRepo->edit(entity, &ok);
    if (!ok)
        return Out(t.translate("SampleService", "Failed to edit sample (internal)", "error"));
    entity = SampleRepo->findOne(entity.id(), &ok);
    if (!ok || !entity.isValid())
        return Out(t.translate("SampleService", "Failed to get sample (internal)", "error"));
    TEditSampleAdminReplyData replyData;
    TSampleInfo info = sampleToSampleInfo(entity, &ok);
    if (!ok)
        return Out(t.translate("SampleService", "Failed to create sample info (internal)", "error"));
    if (!commit(t, holder, &error))
        return Out(error);
    replyData.setSampleInfo(info);
    return Out(replyData, entity.lastModificationDateTime());
}

RequestOut<TGetSampleInfoListReplyData> SampleService::getSampleInfoList(
        const RequestIn<TGetSampleInfoListRequestData> &in)
{
    typedef RequestOut<TGetSampleInfoListReplyData> Out;
    Translator t(in.locale());
    QString error;
    if (!commonCheck(t, &error))
        return Out(error);
    QDateTime dt = QDateTime::currentDateTime();
    bool ok = false;
    QList<Sample> entities = SampleRepo->findAllNewerThan(in.lastRequestDateTime(), &ok);
    if (!ok)
        return Out(t.translate("SampleService", "Failed to get sample list (internal)", "error"));
    TSampleInfoList newSamples;
    TIdList deletedSamples = SampleRepo->findAllDeletedNewerThan(in.lastRequestDateTime(), &ok);
    if (!ok)
        return Out(t.translate("SampleService", "Failed to get deleted sample list (internal)", "error"));
    foreach (const Sample &entity, entities) {
        newSamples << sampleToSampleInfo(entity, &ok);
        if (!ok)
            return Out(t.translate("SampleService", "Failed to create sample info (internal)", "error"));
    }
    TGetSampleInfoListReplyData replyData;
    replyData.setNewSamples(newSamples);
    replyData.setDeletedSamples(deletedSamples);
    return Out(replyData, dt);
}

RequestOut<TGetSamplePreviewReplyData> SampleService::getSamplePreview(
        const RequestIn<TGetSamplePreviewRequestData> &in)
{
    typedef RequestOut<TGetSamplePreviewReplyData> Out;
    Translator t(in.locale());
    QString error;
    if (!commonCheck(t, &error))
        return Out(error);
    QDateTime dt = QDateTime::currentDateTime();
    bool ok = false;
    if (in.cachingEnabled() && in.lastRequestDateTime().isValid()) {
        QDateTime lastModDT = SampleRepo->findLastModificationDateTime(in.data().id(), &ok);
        if (!ok) {
            return Out(t.translate("SampleService", "Failed to get sample last modification date time (internal)",
                                   "error"));
        }
        if (in.lastRequestDateTime() >= lastModDT)
            return Out(dt);
    }
    Sample entity = SampleRepo->findOne(in.data().id(), &ok);
    if (!ok)
        return Out(t.translate("SampleService", "Failed to get sample (internal)", "error"));
    TGetSamplePreviewReplyData replyData;
    replyData.setMainFile(entity.previewMainFile());
    return Out(replyData, dt);
}

RequestOut<TGetSampleSourceReplyData> SampleService::getSampleSource(const RequestIn<TGetSampleSourceRequestData> &in)
{
    typedef RequestOut<TGetSampleSourceReplyData> Out;
    Translator t(in.locale());
    QString error;
    if (!commonCheck(t, &error))
        return Out(error);
    QDateTime dt = QDateTime::currentDateTime();
    bool ok = false;
    if (in.cachingEnabled() && in.lastRequestDateTime().isValid()) {
        QDateTime lastModDT = SampleRepo->findLastModificationDateTime(in.data().id(), &ok);
        if (!ok) {
            return Out(t.translate("SampleService", "Failed to get sample last modification date time (internal)",
                                   "error"));
        }
        if (in.lastRequestDateTime() >= lastModDT)
            return Out(dt);
    }
    Sample entity = SampleRepo->findOne(in.data().id(), &ok);
    if (!ok)
        return Out(t.translate("SampleService", "Failed to get sample (internal)", "error"));
    TGetSampleSourceReplyData replyData;
    replyData.setProject(entity.source());
    return Out(replyData, dt);
}

bool SampleService::isValid() const
{
    return Source && Source->isValid() && SampleRepo->isValid();
}

/*============================== Private methods ===========================*/

bool SampleService::checkUserId(const Translator &t, quint64 userId, QString *error)
{
    if (!userId)
        return bRet(error, t.translate("SampleService", "Invalid user ID (internal)", "error"), false);
    return bRet(error, QString(), true);
}

bool SampleService::commit(const Translator &t, TransactionHolder &holder, QString *error)
{
    if (!holder.doCommit())
        return bRet(error, t.translate("SampleService", "Failed to commit (internal)", "error"), false);
    return bRet(error, QString(), true);
}

bool SampleService::commonCheck(const Translator &t, QString *error) const
{
    if (!isValid())
        return bRet(error, t.translate("SampleService", "Invalid SampleService instance (internal)", "error"), false);
    return bRet(error, QString(), true);
}

TBinaryFile SampleService::compilePreview(const TTexProject &source, bool *ok)
{
    if (!isValid() || !source.isValid())
        return bRet(ok, false, TBinaryFile());
    TemporaryLocation loc(Source);
    if (!loc.isValid())
        return bRet(ok, false, TBinaryFile());
    if (!Application::copyTexsample(loc.absolutePath(), "UTF-8"))
        return bRet(ok, false, TBinaryFile());
    TTexProject src = source;
    QString fileName = src.rootFile().fileName();
    src.rootFile().setFileName(BUuid::createUuid().toString(true) + ".tex");
    src.removeRestrictedCommands();
    if (!src.save(loc.absolutePath(), QTextCodec::codecForName("UTF-8")))
        return bRet(ok, false, TBinaryFile());
    QString command = "pdflatex";
    QString baseName = QFileInfo(fileName).baseName();
    QStringList args = QStringList() << "-interaction=nonstopmode";
    args << ("-jobname=" + baseName);
    args << ("\\input texsample.tex \\input " + src.rootFile().fileName() + " \\end{document}");
    if (BeQt::execProcess(loc.absolutePath(), command, args, 5 * BeQt::Second, 2 * BeQt::Minute))
        return bRet(ok, false, TBinaryFile());
    TBinaryFile file(loc.absoluteFilePath(baseName + ".pdf"));
    if (!file.isValid() || !file.size())
        return bRet(ok, false, TBinaryFile());
    return bRet(ok, true, file);
}

TSampleInfo SampleService::sampleToSampleInfo(const Sample &entity, bool *ok)
{
    TSampleInfo info;
    if (!isValid() || !entity.isValid() || !entity.isCreatedByRepo())
        return bRet(ok, false, info);
    info.setAdminRemark(entity.adminRemark());
    info.setAuthors(entity.authors());
    info.setCreationDateTime(entity.creationDateTime());
    info.setDescription(entity.description());
    info.setExtraSourceFiles(entity.sourceExtraFileInfos());
    info.setId(entity.id());
    info.setLastModificationDateTime(entity.lastModificationDateTime());
    info.setMainPreviewFile(entity.previewMainFileInfo());
    info.setMainSourceFile(entity.sourceMainFileInfo());
    info.setRating(entity.rating());
    info.setSenderId(entity.senderId());
    bool b = false;
    info.setSenderLogin(UserRepo->findLogin(entity.senderId(), &b));
    if (!b)
        return bRet(ok, false, TSampleInfo());
    info.setTags(entity.tags());
    info.setTitle(entity.title());
    info.setType(entity.type());
    return bRet(ok, true, info);
}
