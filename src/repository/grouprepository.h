#ifndef GROUPREPOSITORY_H
#define GROUPREPOSITORY_H

class DataSource;

#include "entity/group.h"

#include <TIdList>

#include <QList>

/*============================================================================
================================ GroupRepository =============================
============================================================================*/

class GroupRepository
{
private:
    DataSource * const Source;
public:
    explicit GroupRepository(DataSource *source);
    ~GroupRepository();
public:
    long count();
    DataSource *dataSource() const;
    bool deleteAll();
    bool deleteOne(quint64 id);
    bool deleteSome(const TIdList &ids);
    bool exists(quint64 id);
    QList<Group> findAll();
    QList<Group> findAll(const TIdList &ids);
    Group findOne(quint64 id);
    bool isValid() const;
    quint64 save(const Group &entity);
    TIdList save(const QList<Group> &entities);
private:
    friend class Group;
    Q_DISABLE_COPY(GroupRepository)
};

#endif // GROUPREPOSITORY_H
