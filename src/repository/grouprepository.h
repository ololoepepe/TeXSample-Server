#ifndef GROUPREPOSITORY_H
#define GROUPREPOSITORY_H

class DataSource;

#include "entity/group.h"

#include <TIdList>

#include <QDateTime>
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
    quint64 add(const Group &entity);
    DataSource *dataSource() const;
    bool deleteOne(quint64 id);
    bool edit(const Group &entity);
    QList<Group> findAll();
    QList<Group> findAll(const TIdList &ids);
    QList<Group> findAllByUserId(quint64 userId, const QDateTime &newerThan = QDateTime());
    Group findOne(quint64 id);
    bool isValid() const;
private:
    friend class Group;
    Q_DISABLE_COPY(GroupRepository)
};

#endif // GROUPREPOSITORY_H
