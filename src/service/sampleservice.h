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
class SampleRepository;

#include <QCoreApplication>

/*============================================================================
================================ SampleService ===============================
============================================================================*/

class SampleService
{
    Q_DECLARE_TR_FUNCTIONS(SampleService)
private:
    SampleRepository * const SampleRepo;
    DataSource * const Source;
public:
    explicit SampleService(DataSource *source);
    ~SampleService();
public:
    DataSource *dataSource() const;
    bool isValid() const;
private:
    Q_DISABLE_COPY(SampleService)
};

#endif // SAMPLESERVICE_H
