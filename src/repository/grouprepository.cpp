#include "grouprepository.h"

#include "datasource.h"
#include "entity/group.h"
#include "transactionholder.h"

#include <TIdList>

#include <BSqlResult>
#include <BSqlWhere>

#include <QDateTime>
#include <QList>
#include <QString>
#include <QVariant>
#include <QVariantList>

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

quint64 GroupRepository::add(const Group &entity)
{
    if (!isValid() || !entity.isValid() || entity.isCreatedByRepo())
        return false;
    QVariantMap values;
    values.insert("ownerId", entity.ownerId());
    values.insert("creation_date_time", entity.creationDateTime().toUTC().toMSecsSinceEpoch());
    values.insert("last_modification_date_time", entity.lastModificationDateTime().toUTC().toMSecsSinceEpoch());
    values.insert("name", entity.name());
    TransactionHolder holder(Source);
    QDateTime dt = QDateTime::currentDateTimeUtc();
    BSqlResult result = Source->insert("groups", values);
    if (!result.success())
        return 0;
    quint64 id = result.lastInsertId().toULongLong();
    if (!Source->update("users", "last_modification_date_time", dt.toMSecsSinceEpoch()) || !holder.doCommit())
        return 0;
    return id;
}

DataSource *GroupRepository::dataSource() const
{
    return Source;
}

bool GroupRepository::deleteOne(quint64 id)
{
    if (!isValid() || !id)
        return false;
    TransactionHolder holder(Source);
    return Source->deleteFrom("groups", BSqlWhere("id = :id", ":id", id)).success() && holder.doCommit();
}

bool GroupRepository::edit(const Group &entity)
{
    if (!isValid() || !entity.isValid() || entity.isCreatedByRepo())
        return false;
    QVariantMap values;
    values.insert("ownerId", entity.ownerId());
    values.insert("creation_date_time", entity.creationDateTime().toUTC().toMSecsSinceEpoch());
    values.insert("last_modification_date_time", entity.lastModificationDateTime().toUTC().toMSecsSinceEpoch());
    values.insert("name", entity.name());
    TransactionHolder holder(Source);
    QDateTime dt = QDateTime::currentDateTimeUtc();
    if (!Source->insert("groups", values, BSqlWhere("id = :id", ":id", entity.id())).success())
        return false;
    return Source->update("users", "last_modification_date_time", dt.toMSecsSinceEpoch()) && holder.doCommit();
}

QList<Group> GroupRepository::findAllByUserId(quint64 userId)
{
    QList<Group> list;
    if (!isValid() || !userId)
        return list;
    BSqlResult result = Source->exec("SELECT groups.id, groups.creation_date_time, groups.last_modification_date_time, "
        "groups.name, users.login FROM groups INNER JOIN users ON groups.owner_id = users.id "
        "WHERE groups.owner_id = :owner_id", QString(":owner_id"), userId); //NOTE: Avoiding ambiguity
    if (!result.success() || result.value().isEmpty())
        return list;
    foreach (const QVariantMap &m, result.values()) {
        Group entity(this);
        entity.mid = m.value("groups.id").toULongLong();
        entity.mownerId = m.value("groups.owner_id").toULongLong();
        entity.mownerLogin = m.value("users.login").toString();
        entity.mcreationDateTime.setMSecsSinceEpoch(m.value("groups.creation_date_time").toLongLong());
        entity.mlastModificationDateTime.setMSecsSinceEpoch(
                    m.value("groups.last_modification_date_time").toLongLong());
        entity.mname = m.value("groups.name").toString();
        entity.valid = true;
        list << entity;
    }
    return list;
}

Group GroupRepository::findOne(quint64 id)
{
    Group entity(this);
    if (!isValid() || !id)
        return entity;
    BSqlResult result = Source->exec("SELECT groups.owner_id, groups.creation_date_time, "
        "groups.last_modification_date_time, groups.name, users.login FROM groups INNER JOIN users ON "
        "groups.owner_id = users.id WHERE groups.id = :id", QString(":id"), id); //NOTE: Avoiding ambiguity
    if (!result.success() || result.value().isEmpty())
        return entity;
    entity.mid = id;
    entity.mownerId = result.value("groups.owner_id").toULongLong();
    entity.mownerLogin = result.value("users.login").toString();
    entity.mcreationDateTime.setMSecsSinceEpoch(result.value("groups.creation_date_time").toLongLong());
    entity.mlastModificationDateTime.setMSecsSinceEpoch(
                result.value("groups.last_modification_date_time").toLongLong());
    entity.mname = result.value("groups.name").toString();
    entity.valid = true;
    return entity;
}

bool GroupRepository::isValid() const
{
    return Source && Source->isValid();
}
