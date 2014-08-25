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

#include "applicationversionservice.h"

#include "datasource.h"
#include "entity/applicationversion.h"
#include "repository/applicationversionrepository.h"
#include "transactionholder.h"
#include "translator.h"

#include <TClientInfo>
#include <TeXSample>
#include <TGetLatestAppVersionReplyData>
#include <TGetLatestAppVersionRequestData>
#include <TSetLatestAppVersionReplyData>
#include <TSetLatestAppVersionRequestData>

#include <BeQt>
#include <BVersion>

#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QString>
#include <QUrl>

/*============================================================================
================================ ApplicationVersionService ===================
============================================================================*/

/*============================== Public constructors =======================*/

ApplicationVersionService::ApplicationVersionService(DataSource *source) :
    ApplicationVersionRepo(new ApplicationVersionRepository(source)), Source(source)
{
    //
}

ApplicationVersionService::~ApplicationVersionService()
{
    //
}

/*============================== Public methods ============================*/

DataSource *ApplicationVersionService::dataSource() const
{
    return Source;
}

RequestOut<TGetLatestAppVersionReplyData> ApplicationVersionService::getLatestAppVersion(
        const RequestIn<TGetLatestAppVersionRequestData> &in)
{
    typedef RequestOut<TGetLatestAppVersionReplyData> Out;
    Translator t(in.locale());
    QString error;
    if (!commonCheck(t, &error))
        return Out(error);
    TClientInfo info = in.data().clientInfo();
    QDateTime dt = QDateTime::currentDateTimeUtc();
    ApplicationVersion entity = ApplicationVersionRepo->findOneByFields(info.applicationType(), info.osType(),
                                                                        info.processorArchitecture(),
                                                                        info.isPortable());
    if (!entity.isValid())
        return Out(t.translate("ApplicationVersionService", "No application version for this client found", "error"));
    TGetLatestAppVersionReplyData replyData;
    replyData.setDownloadUrl(entity.downloadUrl());
    replyData.setVersion(entity.version());
    return Out(replyData, dt);
}

bool ApplicationVersionService::isValid() const
{
    return Source && Source->isValid() && ApplicationVersionRepo->isValid();
}

bool ApplicationVersionService::setLatestAppVersion(Texsample::ClientType clienType, BeQt::OSType os,
                                                    BeQt::ProcessorArchitecture arch, bool portable,
                                                    const BVersion &version, const QUrl &downloadUrl)
{
    if (!isValid() || Texsample::UnknownClient == clienType || BeQt::UnknownOS == os
            || BeQt::UnknownArchitecture == arch || !version.isValid())
        return false;
    TransactionHolder holder(Source);
    ApplicationVersion entity = ApplicationVersionRepo->findOneByFields(clienType, os, arch, portable);
    if (!entity.isValid()) {
        entity = ApplicationVersion();
        entity.setClientType(clienType);
        entity.setOs(os);
        entity.setProcessorArchitecture(arch);
        entity.setPortable(portable);
        entity.setVersion(version);
        entity.setDownloadUrl(downloadUrl);
        return ApplicationVersionRepo->add(entity) && holder.doCommit();
    } else {
        entity.convertToCreatedByUser();
        entity.setVersion(version);
        entity.setDownloadUrl(downloadUrl);
        return ApplicationVersionRepo->edit(entity) && holder.doCommit();
    }
}

RequestOut<TSetLatestAppVersionReplyData> ApplicationVersionService::setLatestAppVersion(
        const RequestIn<TSetLatestAppVersionRequestData> &in)
{
    typedef RequestOut<TSetLatestAppVersionReplyData> Out;
    Translator t(in.locale());
    QString error;
    if (!commonCheck(t, &error))
        return Out(error);
    const TSetLatestAppVersionRequestData &requestData = in.data();
    Texsample::ClientType clientType = requestData.clientType();
    BeQt::OSType os = requestData.os();
    BeQt::ProcessorArchitecture arch = requestData.processorArchitecture();
    bool portable = requestData.portable();
    BVersion version = requestData.version();
    QUrl downloadUrl = requestData.downloadUrl();
    QDateTime dt = QDateTime::currentDateTimeUtc();
    TransactionHolder holder(Source);
    ApplicationVersion entity = ApplicationVersionRepo->findOneByFields(clientType, os, arch, portable);
    if (!entity.isValid()) {
        entity = ApplicationVersion();
        entity.setClientType(clientType);
        entity.setOs(os);
        entity.setProcessorArchitecture(arch);
        entity.setPortable(portable);
        entity.setVersion(version);
        entity.setDownloadUrl(downloadUrl);
        if (!ApplicationVersionRepo->add(entity)) {
            return Out(t.translate("ApplicationVersionService", "Failed to add application version (internal)",
                                   "error"));
        }
    } else {
        entity.convertToCreatedByUser();
        entity.setVersion(version);
        entity.setDownloadUrl(downloadUrl);
        if (!ApplicationVersionRepo->edit(entity)) {
            return Out(t.translate("ApplicationVersionService", "Failed to edit application version (internal)",
                                   "error"));
        }
    }
    if (!commit(t, holder, &error))
        return Out(error);
    TSetLatestAppVersionReplyData replyData;
    return Out(replyData, dt);
}

/*============================== Private methods ===========================*/

bool ApplicationVersionService::commit(const Translator &translator, TransactionHolder &holder, QString *error)
{
    if (!holder.doCommit()) {
        return bRet(error, translator.translate("ApplicationVersionService", "Failed to commit (internal)", "error"),
                    false);
    }
    return bRet(error, QString(), true);
}

bool ApplicationVersionService::commonCheck(const Translator &translator, QString *error) const
{
    if (!isValid()) {
        return bRet(error, translator.translate("ApplicationVersionService",
                                                "Invalid ApplicationVersionService instance (internal)", "error"),
                    false);
    }
    return bRet(error, QString(), true);
}
