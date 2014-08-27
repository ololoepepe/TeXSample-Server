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

#ifndef APPLICATIONVERSION_H
#define APPLICATIONVERSION_H

class ApplicationVersionRepository;

#include <TeXSample>

#include <BeQt>
#include <BVersion>

#include <QUrl>

/*============================================================================
================================ ApplicationVersion ==========================
============================================================================*/

class ApplicationVersion
{
private:
    Texsample::ClientType mclienType;
    BeQt::OSType mos;
    bool mportable;
    BeQt::ProcessorArchitecture mprocessorArchitecture;
    QUrl mdownloadUrl;
    BVersion mversion;
    bool createdByRepo;
    ApplicationVersionRepository *repo;
    bool valid;
public:
    explicit ApplicationVersion();
    ApplicationVersion(const ApplicationVersion &other);
    ~ApplicationVersion();
protected:
    explicit ApplicationVersion(ApplicationVersionRepository *repo);
public:
    Texsample::ClientType clienType() const;
    void convertToCreatedByUser();
    QUrl downloadUrl() const;
    bool isCreatedByRepo() const;
    bool isValid() const;
    BeQt::OSType os() const;
    bool portable() const;
    BeQt::ProcessorArchitecture processorArchitecture() const;
    ApplicationVersionRepository *repository() const;
    void setClientType(Texsample::ClientType clienType);
    void setDownloadUrl(const QUrl &url);
    void setOs(BeQt::OSType os);
    void setPortable(bool portable);
    void setProcessorArchitecture(BeQt::ProcessorArchitecture arch);
    void setVersion(const BVersion &version);
    BVersion version() const;
public:
    ApplicationVersion &operator =(const ApplicationVersion &other);
private:
    void init();
private:
    friend class ApplicationVersionRepository;
};

#endif // APPLICATIONVERSION_H
