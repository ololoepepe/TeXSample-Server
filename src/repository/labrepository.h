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

#ifndef LABREPOSITORY_H
#define LABREPOSITORY_H

class DataSource;

class TBinaryFileList;
class TLabDataList;

#include "entity/lab.h"

#include <TIdList>

#include <QDateTime>
#include <QList>

/*============================================================================
================================ LabRepository ===============================
============================================================================*/

class LabRepository
{
private:
    DataSource * const Source;
public:
    explicit LabRepository(DataSource *source);
    ~LabRepository();
public:
    quint64 add(const Lab &entity, bool *ok = 0);
    DataSource *dataSource() const;
    void edit(const Lab &entity, bool *ok = 0);
    QList<Lab> findAllNewerThan(const TIdList &groups = TIdList(), bool *ok = 0);
    QList<Lab> findAllNewerThan(const QDateTime &newerThan, const TIdList &groups = TIdList(), bool *ok = 0);
    Lab findOne(quint64 id, bool *ok = 0);
    bool isValid() const;
private:
    //fetch
    bool createData(quint64 labId, const TLabDataList &data);
    bool createExtraFiles(quint64 labId, const TBinaryFileList &files);
    bool deleteExtraFiles(quint64 labId, const QStringList &fileNames);
    bool updateData(quint64 labId, const TLabDataList &data);
private:
    friend class Lab;
    Q_DISABLE_COPY(LabRepository)
};

#endif // LABREPOSITORY_H
