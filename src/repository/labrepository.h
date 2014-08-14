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
    quint64 add(const Lab &entity);
    DataSource *dataSource() const;
    bool edit(const Lab &entity);
    QList<Lab> findAllNewerThan(const TIdList &groups = TIdList());
    QList<Lab> findAllNewerThan(const QDateTime &newerThan, const TIdList &groups = TIdList());
    Lab findOne(quint64 id);
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
