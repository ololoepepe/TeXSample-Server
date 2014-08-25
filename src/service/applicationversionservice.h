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

#ifndef APPLICATIONVERSIONSERVICE_H
#define APPLICATIONVERSIONSERVICE_H

class ApplicationVersionRepository;
class DataSource;

#include <TeXSample>

#include <BeQt>
#include <BVersion>

#include <QCoreApplication>
#include <QUrl>

/*============================================================================
================================ ApplicationVersionService ===================
============================================================================*/

class ApplicationVersionService
{
    Q_DECLARE_TR_FUNCTIONS(ApplicationVersionService)
private:
    ApplicationVersionRepository * const ApplicationVersionRepo;
    DataSource * const Source;
public:
    explicit ApplicationVersionService(DataSource *source);
    ~ApplicationVersionService();
public:
    DataSource *dataSource() const;
    bool isValid() const;
    bool setApplicationVersion(Texsample::ClientType clienType, BeQt::OSType os, BeQt::ProcessorArchitecture arch,
                               bool portable, const BVersion &version, const QUrl &downloadUrl = QUrl());
private:
    Q_DISABLE_COPY(ApplicationVersionService)
};

#endif // APPLICATIONVERSIONSERVICE_H
