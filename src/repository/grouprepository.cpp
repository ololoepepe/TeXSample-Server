#include "grouprepository.h"

#include "datasource.h"
#include "entity/group.h"

#include <TIdList>

#include <BSqlResult>
#include <BSqlWhere>

#include <QDateTime>
#include <QList>
#include <QString>
#include <QStringList>
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
    QDateTime dt = QDateTime::currentDateTimeUtc();
    QVariantMap values;
    values.insert("owner_id", entity.ownerId());
    values.insert("creation_date_time", dt.toMSecsSinceEpoch());
    values.insert("last_modification_date_time", dt.toMSecsSinceEpoch());
    values.insert("name", entity.name());
    BSqlResult result = Source->insert("groups", values);
    if (!result.success())
        return 0;
    return result.lastInsertId().toULongLong();
}

DataSource *GroupRepository::dataSource() const
{
    return Source;
}

bool GroupRepository::deleteOne(quint64 id)
{
    if (!isValid() || !id)
        return false;
    BSqlWhere where("id = :id", ":id", id);
    return Source->deleteFrom("groups", where).success();
}

bool GroupRepository::edit(const Group &entity)
{
    if (!isValid() || !entity.isValid() || !entity.id() || entity.isCreatedByRepo())
        return false;
    QVariantMap values;
    values.insert("owner_id", entity.ownerId());
    values.insert("last_modification_date_time", entity.lastModificationDateTime().toUTC().toMSecsSinceEpoch());
    values.insert("name", entity.name());
    return Source->update("groups", values, BSqlWhere("id = :id", ":id", entity.id())).success();
}

QList<Group> GroupRepository::findAll()
{
    QList<Group> list;
    if (!isValid())
        return list;
    BSqlResult result = Source->select("groups", QStringList() << "id" << "owner_id" << "creation_date_time"
                                       << "last_modification_date_time" << "name");
    if (!result.success() || result.values().isEmpty())
        return list;
    foreach (const QVariantMap &m, result.values()) {
        Group entity(this);
        entity.mid = m.value("id").toULongLong();
        entity.mownerId = m.value("owner_id").toULongLong();
        entity.mcreationDateTime.setMSecsSinceEpoch(m.value("creation_date_time").toLongLong());
        entity.mlastModificationDateTime.setMSecsSinceEpoch(m.value("last_modification_date_time").toLongLong());
        entity.mname = m.value("name").toString();
        entity.valid = true;
        list << entity;
    }
    return list;
}

QList<Group> GroupRepository::findAll(const TIdList &ids)
{
    QList<Group> list;
    if (!isValid() || ids.isEmpty())
        return list;
    QString ws = "id IN (";
    QVariantMap values;
    foreach (int i, bRangeD(0, ids.size() - 1)) {
        ws += ":id" + QString::number(i);
        if (i < ids.size() - 1)
            ws += ", ";
        values.insert(":id" + QString::number(i), ids.at(i));
    }
    ws += ")";
    BSqlWhere where(ws, values);
    BSqlResult result = Source->select("groups", QStringList() << "id" << "owner_id" << "creation_date_time"
                                       << "last_modification_date_time" << "name", where);
    if (!result.success() || result.values().isEmpty())
        return list;
    foreach (const QVariantMap &m, result.values()) {
        Group entity(this);
        entity.mid = m.value("id").toULongLong();
        entity.mownerId = m.value("owner_id").toULongLong();
        entity.mcreationDateTime.setMSecsSinceEpoch(m.value("creation_date_time").toLongLong());
        entity.mlastModificationDateTime.setMSecsSinceEpoch(m.value("last_modification_date_time").toLongLong());
        entity.mname = m.value("name").toString();
        entity.valid = true;
        list << entity;
    }
    return list;
}

QList<Group> GroupRepository::findAllByUserId(quint64 userId, const QDateTime &newerThan)
{
    QList<Group> list;
    if (!isValid() || !userId)
        return list;
    QString ws = "owner_id = :owner_id";
    QVariantMap wvalues;
    wvalues.insert(":owner_id", userId);
    if (newerThan.isValid()) {
        ws += " AND last_modification_date_time > :last_modification_date_time";
        wvalues.insert("last_modification_date_time", newerThan.toUTC().toMSecsSinceEpoch());
    }
    BSqlResult result = Source->select("groups", QStringList() << "id" << "creation_date_time"
                                       << "last_modification_date_time" << "name", BSqlWhere(ws, wvalues));
    if (!result.success() || result.values().isEmpty())
        return list;
    foreach (const QVariantMap &m, result.values()) {
        Group entity(this);
        entity.mid = m.value("id").toULongLong();
        entity.mownerId = userId;
        entity.mcreationDateTime.setMSecsSinceEpoch(m.value("creation_date_time").toLongLong());
        entity.mlastModificationDateTime.setMSecsSinceEpoch(m.value("last_modification_date_time").toLongLong());
        entity.mname = m.value("name").toString();
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
    BSqlResult result = Source->select("groups", QStringList() << "creation_date_time" << "last_modification_date_time"
                                       << "name", BSqlWhere("id = :id", ":id", id));
    if (!result.success() || result.value().isEmpty())
        return entity;
    entity.mid = id;
    entity.mownerId = result.value("owner_id").toULongLong();
    entity.mcreationDateTime.setMSecsSinceEpoch(result.value("creation_date_time").toLongLong());
    entity.mlastModificationDateTime.setMSecsSinceEpoch(result.value("last_modification_date_time").toLongLong());
    entity.mname = result.value("name").toString();
    entity.valid = true;
    return entity;
}

bool GroupRepository::isValid() const
{
    return Source && Source->isValid();
}
