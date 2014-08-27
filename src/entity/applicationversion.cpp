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

#include "applicationversion.h"

#include "repository/applicationversionrepository.h"

#include <TeXSample>

#include <BeQt>
#include <BVersion>

#include <QUrl>

/*============================================================================
================================ ApplicationVersion ==========================
============================================================================*/

/*============================== Public constructors =======================*/

ApplicationVersion::ApplicationVersion()
{
    init();
}

ApplicationVersion::ApplicationVersion(const ApplicationVersion &other)
{
    init();
    *this = other;
}

ApplicationVersion::~ApplicationVersion()
{
    //
}

/*============================== Protected constructors ====================*/

ApplicationVersion::ApplicationVersion(ApplicationVersionRepository *repo)
{
    init();
    this->repo = repo;
    createdByRepo = true;
}

/*============================== Public methods ============================*/

Texsample::ClientType ApplicationVersion::clienType() const
{
    return mclienType;
}

void ApplicationVersion::convertToCreatedByUser()
{
    if (!createdByRepo)
        return;
    createdByRepo = false;
    repo = 0;
    valid = false;
}

QUrl ApplicationVersion::downloadUrl() const
{
    return mdownloadUrl;
}

bool ApplicationVersion::isCreatedByRepo() const
{
    return createdByRepo;
}

bool ApplicationVersion::isValid() const
{
    if (createdByRepo)
        return valid;
    return Texsample::UnknownClient != mclienType && BeQt::UnknownOS != mos
            && BeQt::UnknownArchitecture != mprocessorArchitecture && mversion.isValid();
}

BeQt::OSType ApplicationVersion::os() const
{
    return mos;
}

bool ApplicationVersion::ApplicationVersion::portable() const
{
    return mportable;
}

BeQt::ProcessorArchitecture ApplicationVersion::processorArchitecture() const
{
    return mprocessorArchitecture;
}

ApplicationVersionRepository *ApplicationVersion::repository() const
{
    return repo;
}

void ApplicationVersion::setClientType(Texsample::ClientType clienType)
{
    mclienType = clienType;
}

void ApplicationVersion::setDownloadUrl(const QUrl &url)
{
    mdownloadUrl = url;
}

void ApplicationVersion::setOs(BeQt::OSType os)
{
    mos = os;
}

void ApplicationVersion::setPortable(bool portable)
{
    mportable = portable;
}

void ApplicationVersion::setProcessorArchitecture(BeQt::ProcessorArchitecture arch)
{
    mprocessorArchitecture = arch;
}

void ApplicationVersion::setVersion(const BVersion &version)
{
    mversion = version;
}

BVersion ApplicationVersion::version() const
{
    return mversion;
}

/*============================== Public operators ==========================*/

ApplicationVersion &ApplicationVersion::operator =(const ApplicationVersion &other)
{
    mclienType = other.mclienType;
    mdownloadUrl = other.mdownloadUrl;
    mos = other.mos;
    mportable = other.mportable;
    mprocessorArchitecture = other.mprocessorArchitecture;
    mversion = other.mversion;
    createdByRepo = other.createdByRepo;
    repo = other.repo;
    valid = other.valid;
    return *this;
}

/*============================== Private methods ===========================*/

void ApplicationVersion::init()
{
    mclienType = Texsample::UnknownClient;
    mos = BeQt::UnknownOS;
    mportable = false;
    mprocessorArchitecture = BeQt::UnknownArchitecture;
    createdByRepo = false;
    repo = 0;
    valid = false;
}
