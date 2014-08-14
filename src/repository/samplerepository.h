#ifndef SAMPLEREPOSITORY_H
#define SAMPLEREPOSITORY_H

class DataSource;

class TBinaryFile;
class TBinaryFileList;
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
    bool fetchPreview(quint64 sampleId, TBinaryFile &mainFile, TBinaryFileList &extraFiles);
    TTexProject fetchSource(quint64 sampleId, bool *ok = false);
    bool updateSource(quint64 sampleId, const TTexProject &data);
private:
    friend class Sample;
    Q_DISABLE_COPY(SampleRepository)
};

#endif // SAMPLEREPOSITORY_H
