#include "invitecoderepository.h"

#include "datasource.h"
#include "entity/invitecode.h"
#include "repositorytools.h"

#include <TAccessLevel>
#include <TIdList>
#include <TService>
#include <TServiceList>

#include <BSqlResult>
#include <BSqlWhere>
#include <BUuid>

#include <QDateTime>
#include <QDebug>
#include <QList>
#include <QString>
#include <QStringList>

/*============================================================================
================================ InviteCodeRepository ========================
============================================================================*/

/*============================== Public constructors =======================*/

InviteCodeRepository::InviteCodeRepository(DataSource *source) :
    Source(source)
{
    //
}

InviteCodeRepository::~InviteCodeRepository()
{
    //
}

/*============================== Public methods ============================*/

quint64 InviteCodeRepository::add(const InviteCode &entity)
{
    if (!isValid() || !entity.isValid() || entity.isCreatedByRepo())
        return false;
    QDateTime dt = QDateTime::currentDateTimeUtc();
    QVariantMap values;
    values.insert("owner_id", entity.ownerId());
    values.insert("access_level", int(entity.accessLevel()));
    values.insert("code", entity.code().toString(true));
    values.insert("creation_date_time", dt.toMSecsSinceEpoch());
    values.insert("expiration_date_time", entity.expirationDateTime().toUTC().toMSecsSinceEpoch());
    BSqlResult result = Source->insert("invite_codes", values);
    if (!result.success())
        return 0;
    quint64 id = result.lastInsertId().toULongLong();
    if (!RepositoryTools::setGroupIdList(Source, "invite_code_groups", "invite_code_id", id, entity.groups()))
        return 0;
    if (!RepositoryTools::setServices(Source, "invite_code_services", "invite_code_id", id,
                                      entity.availableServices())) {
        return 0;
    }
    return id;
}

DataSource *InviteCodeRepository::dataSource() const
{
    return Source;
}

bool InviteCodeRepository::deleteExpired()
{
    return isValid() && Source->deleteFrom("invite_codes",
                                           BSqlWhere("expiration_date_time <= :date_time", ":date_time",
                                                     QDateTime::currentDateTimeUtc().toMSecsSinceEpoch()));
}

bool InviteCodeRepository::deleteOne(quint64 id)
{
    if (!id)
        return false;
    TIdList list;
    list << id;
    return deleteSome(list);
}

bool InviteCodeRepository::deleteSome(const TIdList &ids)
{
    if (!isValid())
        return false;
    if (ids.isEmpty())
        return true;
    QString ws = "id IN (";
    QVariantMap values;
    foreach (int i, bRangeD(0, ids.size() - 1)) {
        ws += ":id" + QString::number(i);
        if (i < ids.size() - 1)
            ws += ", ";
        values.insert(":id" + QString::number(i), ids.at(i));
    }
    ws += ")";
    return Source->deleteFrom("invite_codes", BSqlWhere(ws, values)).success();
}

QList<InviteCode> InviteCodeRepository::findAllByOwnerId(quint64 ownerId)
{
    QList<InviteCode> list;
    if (!isValid() || !ownerId)
        return list;
    BSqlResult result = Source->select("invite_codes", QStringList() << "id" << "access_level" << "code"
                                       << "creation_date_time" << "expiration_date_time",
                                       BSqlWhere("owner_id = :owner_id", ":owner_id", ownerId));
    if (!result.success() || result.values().isEmpty())
        return list;
    foreach (const QVariantMap &m, result.values()) {
        quint64 id = m.value("id").toULongLong();
        InviteCode entity(this);
        entity.mid = id;
        entity.mownerId = ownerId;
        entity.maccessLevel = m.value("access_level").toInt();
        entity.mcode = BUuid(m.value("code").toString());
        entity.mcreationDateTime.setMSecsSinceEpoch(m.value("creation_date_time").toLongLong());
        entity.mexpirationDateTime.setMSecsSinceEpoch(m.value("expiration_date_time").toLongLong());
        bool ok = false;
        entity.mgroups = RepositoryTools::getGroupIdList(Source, "invite_code_groups", "invite_code_id", id, &ok);
        if (!ok)
            return QList<InviteCode>();
        entity.mavailableServices = RepositoryTools::getServices(Source, "invite_code_services", "invite_code_id", id,
                                                                 &ok);
        if (!ok)
            return QList<InviteCode>();
        entity.valid = true;
        list << entity;
    }
    return list;
}

InviteCode InviteCodeRepository::findOne(quint64 id)
{
    InviteCode entity(this);
    if (!isValid() || !id)
        return entity;
    BSqlResult result = Source->select("invite_codes", QStringList() << "owner_id" << "access_level" << "code"
                                       << "creation_date_time" << "expiration_date_time",
                                       BSqlWhere("id = :id", ":id", id));
    if (!result.success() || result.values().isEmpty())
        return entity;
    entity.mid = id;
    entity.mownerId = result.value("owner_id").toULongLong();
    entity.maccessLevel = result.value("access_level").toInt();
    entity.mcreationDateTime.setMSecsSinceEpoch(result.value("creation_date_time").toLongLong());
    entity.mexpirationDateTime.setMSecsSinceEpoch(result.value("expiration_date_time").toLongLong());
    entity.mcode = BUuid(result.value("code").toString());
    bool ok = false;
    entity.mgroups = RepositoryTools::getGroupIdList(Source, "invite_code_groups", "invite_code_id", entity.id(), &ok);
    if (!ok)
        return entity;
    entity.mavailableServices = RepositoryTools::getServices(Source, "invite_code_services", "invite_code_id",
                                                             entity.id(), &ok);
    if (!ok)
        return entity;
    entity.valid = true;
    return entity;
}

InviteCode InviteCodeRepository::findOneByCode(const BUuid &code)
{
    InviteCode entity(this);
    if (!isValid() || code.isNull())
        return entity;
    BSqlResult result = Source->select("invite_codes", QStringList() << "id" << "owner_id" << "access_level"
                                       << "creation_date_time" << "expiration_date_time",
                                       BSqlWhere("code = :code", ":code", code.toString(true)));
    if (!result.success() || result.values().isEmpty())
        return entity;
    entity.mid = result.value("id").toULongLong();
    entity.mownerId = result.value("owner_id").toULongLong();
    entity.maccessLevel = result.value("access_level").toInt();
    entity.mcreationDateTime.setMSecsSinceEpoch(result.value("creation_date_time").toLongLong());
    entity.mexpirationDateTime.setMSecsSinceEpoch(result.value("expiration_date_time").toLongLong());
    entity.mcode = code;
    bool ok = false;
    entity.mgroups = RepositoryTools::getGroupIdList(Source, "invite_code_groups", "invite_code_id", entity.id(), &ok);
    if (!ok)
        return entity;
    entity.mavailableServices = RepositoryTools::getServices(Source, "invite_code_services", "invite_code_id",
                                                             entity.id(), &ok);
    if (!ok)
        return entity;
    entity.valid = true;
    return entity;
}

bool InviteCodeRepository::isValid() const
{
    return Source && Source->isValid();
}
