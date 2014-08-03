#ifndef LABREPOSITORY_H
#define LABREPOSITORY_H

class DataSource;

#include "entity/lab.h"

#include <TIdList>

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
    long count();
    DataSource *dataSource() const;
    bool deleteAll();
    bool deleteOne(quint64 id);
    bool deleteSome(const TIdList &ids);
    bool exists(quint64 id);
    QList<Lab> findAll();
    QList<Lab> findAll(const TIdList &ids);
    Lab findOne(quint64 id);
    bool isValid() const;
    quint64 save(const Lab &entity);
    TIdList save(const QList<Lab> &entities);
private:
    //fetch
private:
    friend class Lab;
    Q_DISABLE_COPY(LabRepository)
};

#endif // LABREPOSITORY_H
