#include "grouprepository.h"

#include "datasource.h"
#include "entity/group.h"
#include "transactionholder.h"

#include <TIdList>

#include <BSqlResult>
#include <BSqlWhere>

#include <QList>
#include <QString>

/*============================================================================
================================ GroupRepository =============================
============================================================================*/

/*============================== Public constructors =======================*/

GroupRepository::GroupRepository(DataSource *source) :
    Source(source)
{
    //
}

GroupRepository::~GroupRepository()
{
    //
}

/*============================== Public methods ============================*/

long GroupRepository::count()
{
    //
}

DataSource *GroupRepository::dataSource() const
{
    return Source;
}

bool GroupRepository::deleteAll()
{
    //
}

bool GroupRepository::deleteOne(quint64 id)
{
    if (!isValid() || !id)
        return false;
    TransactionHolder holder(Source);
    holder.setSuccess(Source->deleteFrom("groups", BSqlWhere("id = :id", ":id", id)).success());
    return holder.success();
}

bool GroupRepository::deleteSome(const TIdList &ids)
{
    //
}

bool GroupRepository::exists(quint64 id)
{
    //
}

QList<Group> GroupRepository::findAll()
{
    //
}

QList<Group> GroupRepository::findAll(const TIdList &ids)
{
    //
}

Group GroupRepository::findOne(quint64 id)
{
    //
}

bool GroupRepository::isValid() const
{
    return Source && Source->isValid();
}

quint64 GroupRepository::save(const Group &entity)
{
    //
}

TIdList GroupRepository::save(const QList<Group> &entities)
{
    //
}
