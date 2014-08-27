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

#ifndef APPLICATIONVERSIONREPOSITORY_H
#define APPLICATIONVERSIONREPOSITORY_H

class DataSource;

#include "entity/applicationversion.h"

#include <TeXSample>

#include <BeQt>

/*============================================================================
================================ ApplicationVersionRepository ================
============================================================================*/

class ApplicationVersionRepository
{
private:
    DataSource * const Source;
public:
    explicit ApplicationVersionRepository(DataSource *source);
    ~ApplicationVersionRepository();
public:
    void add(const ApplicationVersion &entity, bool *ok = 0);
    DataSource *dataSource() const;
    void edit(const ApplicationVersion &entity, bool *ok = 0);
    ApplicationVersion findOneByFields(Texsample::ClientType clienType, BeQt::OSType os,
                                       BeQt::ProcessorArchitecture arch, bool portable, bool *ok = 0);
    bool isValid() const;
private:
    friend class ApplicationVersion;
    Q_DISABLE_COPY(ApplicationVersionRepository)
};

#endif // APPLICATIONVERSIONREPOSITORY_H
