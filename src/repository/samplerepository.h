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

#ifndef SAMPLEREPOSITORY_H
#define SAMPLEREPOSITORY_H

class DataSource;

class TBinaryFile;
class TTexProject;

#include "entity/sample.h"

#include <TIdList>

#include <QDateTime>
#include <QList>

/*============================================================================
================================ SampleRepository ============================
============================================================================*/

class SampleRepository
{
private:
    DataSource * const Source;
public:
    explicit SampleRepository(DataSource *source);
    ~SampleRepository();
public:
    quint64 add(const Sample &entity);
    DataSource *dataSource() const;
    bool edit(const Sample &entity);
    QList<Sample> findAllNewerThan(const QDateTime &newerThan = QDateTime());
    Sample findOne(quint64 id);
    bool isValid() const;
private:
    bool createSource(quint64 sampleId, const TTexProject &data);
    TBinaryFile fetchPreview(quint64 sampleId, bool *ok = false);
    TTexProject fetchSource(quint64 sampleId, bool *ok = false);
    bool updateSource(quint64 sampleId, const TTexProject &data);
private:
    friend class Sample;
    Q_DISABLE_COPY(SampleRepository)
};

#endif // SAMPLEREPOSITORY_H
