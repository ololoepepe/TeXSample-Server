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

#include "application.h"
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
    if (!commonCheck(t, in.data(), &error))
        return Out(error);
    TClientInfo info = in.data().clientInfo();
    QDateTime dt = QDateTime::currentDateTimeUtc();
    bool ok = false;
    ApplicationVersion entity = ApplicationVersionRepo->findOneByFields(info.applicationType(), info.osType(),
                                                                        info.processorArchitecture(),
                                                                        info.isPortable(), &ok);
    if (!ok)
        return Out(t.translate("ApplicationVersionService", "Failed to get application version (internal)", "error"));
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

bool ApplicationVersionService::setLatestAppVersion(const Translator &t, Texsample::ClientType clientType,
                                                    BeQt::OSType os, BeQt::ProcessorArchitecture arch, bool portable,
                                                    const BVersion &version, const QUrl &downloadUrl, QString *error)
{
    if (!commonCheck(t, error))
        return false;
    if (Texsample::UnknownClient == clientType || BeQt::UnknownOS == os || BeQt::UnknownArchitecture == arch
            || !version.isValid()) {
        return bRet(error, t.translate("ApplicationVersionService", "Invalid data", "error"), false);
    }
    TransactionHolder holder(Source);
    bool ok = false;
    ApplicationVersion entity = ApplicationVersionRepo->findOneByFields(clientType, os, arch, portable, &ok);
    if (!ok) {
        return bRet(error, t.translate("ApplicationVersionService", "Failed to get application version (internal)",
                                       "error"), false);
    }
    if (!entity.isValid()) {
        entity = ApplicationVersion();
        entity.setClientType(clientType);
        entity.setOs(os);
        entity.setProcessorArchitecture(arch);
        entity.setPortable(portable);
        entity.setVersion(version);
        entity.setDownloadUrl(downloadUrl);
        ApplicationVersionRepo->add(entity, &ok);
        if (!ok) {
            return bRet(error, t.translate("ApplicationVersionService", "Failed to add application version (internal)",
                                           "error"), false);
        }
    } else {
        entity.convertToCreatedByUser();
        entity.setVersion(version);
        entity.setDownloadUrl(downloadUrl);
        ApplicationVersionRepo->edit(entity, &ok);
        if (!ok) {
            return bRet(error, t.translate("ApplicationVersionService",
                                           "Failed to edit application version (internal)", "error"), false);
        }
    }
    if (!commit(t, holder, error))
        return false;
    return bRet(error, QString(), true);
}

bool ApplicationVersionService::setLatestAppVersion(Texsample::ClientType clientType, BeQt::OSType os,
                                                    BeQt::ProcessorArchitecture arch, bool portable,
                                                    const BVersion &version, const QUrl &downloadUrl, QString *error)
{
    Translator t(Application::locale());
    return setLatestAppVersion(t, clientType, os, arch, portable, version, downloadUrl, error);
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
    if (setLatestAppVersion(t, clientType, os, arch, portable, version, downloadUrl, &error))
        return Out(error);
    TSetLatestAppVersionReplyData replyData;
    return Out(replyData, dt);
}

/*============================== Private methods ===========================*/

bool ApplicationVersionService::commit(const Translator &t, TransactionHolder &holder, QString *error)
{
    if (!holder.doCommit())
        return bRet(error, t.translate("ApplicationVersionService", "Failed to commit (internal)", "error"), false);
    return bRet(error, QString(), true);
}

bool ApplicationVersionService::commonCheck(const Translator &t, QString *error) const
{
    if (!isValid()) {
        return bRet(error, t.translate("ApplicationVersionService",
                                       "Invalid ApplicationVersionService instance (internal)", "error"), false);
    }
    return bRet(error, QString(), true);
}
