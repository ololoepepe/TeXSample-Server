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

#ifndef SAMPLESERVICE_H
#define SAMPLESERVICE_H

class DataSource;
class Sample;
class SampleRepository;
class TransactionHolder;
class UserRepository;

#include "requestin.h"
#include "requestout.h"
#include "translator.h"

#include <TeXSample/TeXSampleCore>

#include <QCoreApplication>
#include <QString>

/*============================================================================
================================ SampleService ===============================
============================================================================*/

class SampleService
{
    Q_DECLARE_TR_FUNCTIONS(SampleService)
private:
    SampleRepository * const SampleRepo;
    DataSource * const Source;
    UserRepository * const UserRepo;
public:
    explicit SampleService(DataSource *source);
    ~SampleService();
public:
    RequestOut<TAddSampleReplyData> addSample(const RequestIn<TAddSampleRequestData> &in, quint64 userId);
    RequestOut<TCompileTexProjectReplyData> compileTexProject(const RequestIn<TCompileTexProjectRequestData> &in);
    DataSource *dataSource() const;
    RequestOut<TDeleteSampleReplyData> deleteSample(const RequestIn<TDeleteSampleRequestData> &in);
    RequestOut<TEditSampleReplyData> editSample(const RequestIn<TEditSampleRequestData> &in);
    RequestOut<TEditSampleAdminReplyData> editSampleAdmin(const RequestIn<TEditSampleAdminRequestData> &in);
    RequestOut<TGetSampleInfoListReplyData> getSampleInfoList(const RequestIn<TGetSampleInfoListRequestData> &in);
    RequestOut<TGetSamplePreviewReplyData> getSamplePreview(const RequestIn<TGetSamplePreviewRequestData> &in);
    RequestOut<TGetSampleSourceReplyData> getSampleSource(const RequestIn<TGetSampleSourceRequestData> &in);
    bool isValid() const;
private:
    template <typename T> bool commonCheck(const Translator &t, const T &data, QString *error) const
    {
        if (!commonCheck(t, error))
            return false;
        if (!data.isValid())
            return bRet(error, t.translate("SampleService", "Invalid data", "error"), false);
        return bRet(error, QString(), true);
    }
private:
    bool checkUserId(const Translator &t, quint64 userId, QString *error);
    bool commit(const Translator &translator, TransactionHolder &holder, QString *error);
    bool commonCheck(const Translator &translator, QString *error) const;
    TBinaryFile compilePreview(const TTexProject &source, bool *ok = 0);
    TSampleInfo sampleToSampleInfo(const Sample &entity, bool *ok = 0);
private:
    Q_DISABLE_COPY(SampleService)
};

#endif // SAMPLESERVICE_H
