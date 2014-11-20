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
class TFileInfo;
class TFileInfoList;
class TTexProject;

class QByteArray;

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
    quint64 add(const Sample &entity, bool *ok = 0);
    DataSource *dataSource() const;
    QDateTime deleteOne(quint64 id, bool *ok = 0);
    void edit(const Sample &entity, bool *ok = 0);
    QList<Sample> findAllNewerThan(const QDateTime &newerThan = QDateTime(), bool *ok = 0);
    TIdList findAllDeletedNewerThan(const QDateTime &newerThan = QDateTime(), bool *ok = 0);
    QDateTime findLastModificationDateTime(quint64 id, bool *ok = 0);
    Sample findOne(quint64 id, bool *ok = 0);
    bool isValid() const;
private:
    static TFileInfo deserializedPreviewMainFileInfo(const QByteArray &data);
    static TFileInfoList deserializedSouceExtraFileInfos(const QByteArray &data);
    static TFileInfo deserializedSouceMainFileInfo(const QByteArray &data);
    static QByteArray serializedPreviewMainFileInfo(const TBinaryFile &preview);
    static QByteArray serializedSouceExtraFileInfos(const TTexProject &source);
    static QByteArray serializedSouceMainFileInfo(const TTexProject &source);
private:
    bool createPreview(quint64 sampleId, const TBinaryFile &previewMainFile);
    bool createSource(quint64 sampleId, const TTexProject &source);
    TBinaryFile fetchPreview(quint64 sampleId, bool *ok = 0);
    TTexProject fetchSource(quint64 sampleId, bool *ok = 0);
    bool updatePreview(quint64 sampleId, const TBinaryFile &previewMainFile);
    bool updateSource(quint64 sampleId, const TTexProject &source);
private:
    friend class Sample;
    Q_DISABLE_COPY(SampleRepository)
};

#endif // SAMPLEREPOSITORY_H
