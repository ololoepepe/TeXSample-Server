#ifndef SAMPLEREPOSITORY_H
#define SAMPLEREPOSITORY_H

class DataSource;

class TBinaryFile;
class TBinaryFileList;
class TTexProject;

#include "entity/sample.h"

#include <TIdList>

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
    long count();
    DataSource *dataSource() const;
    bool deleteAll();
    bool deleteOne(quint64 id);
    bool deleteSome(const TIdList &ids);
    bool exists(quint64 id);
    QList<Sample> findAll();
    QList<Sample> findAll(const TIdList &ids);
    Sample findOne(quint64 id);
    bool isValid() const;
    quint64 save(const Sample &entity);
    TIdList save(const QList<Sample> &entities);
private:
    bool fetchPreview(quint64 sampleId, TBinaryFile &mainFile, TBinaryFileList &extraFiles);
    TTexProject fetchSource(quint64 sampleId, bool *ok = false);
private:
    friend class Sample;
    Q_DISABLE_COPY(SampleRepository)
};

#endif // SAMPLEREPOSITORY_H
