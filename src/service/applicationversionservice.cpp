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

#include <TeXSample>

#include <BeQt>
#include <BVersion>

#include <QCoreApplication>
#include <QDebug>
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

bool ApplicationVersionService::isValid() const
{
    return Source && Source->isValid() && ApplicationVersionRepo->isValid();
}

bool ApplicationVersionService::setApplicationVersion(Texsample::ClientType clienType, BeQt::OSType os,
                                                      BeQt::ProcessorArchitecture arch, bool portable,
                                                      const BVersion &version, const QUrl &downloadUrl)
{
    if (!isValid() || Texsample::UnknownClient == clienType || BeQt::UnknownOS == os
            || BeQt::UnknownArchitecture == arch || !version.isValid())
        return false;
    ApplicationVersion entity = ApplicationVersionRepo->findOneByFields(clienType, os, arch, portable);
    if (!entity.isValid()) {
        entity = ApplicationVersion();
        entity.setClientType(clienType);
        entity.setOs(os);
        entity.setProcessorArchitecture(arch);
        entity.setPortable(portable);
        entity.setVersion(version);
        entity.setDownloadUrl(downloadUrl);
        return ApplicationVersionRepo->add(entity);
    } else {
        entity.convertToCreatedByUser();
        entity.setVersion(version);
        entity.setDownloadUrl(downloadUrl);
        return ApplicationVersionRepo->edit(entity);
    }
}
