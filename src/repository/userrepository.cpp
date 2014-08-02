#include "userrepository.h"

#include "datasource.h"
#include "entity/user.h"
#include "transactionholder.h"

#include <TAccessLevel>
#include <TIdList>
#include <TUserIdentifier>

#include <BSqlResult>
#include <BSqlWhere>

#include <QByteArray>
#include <QDateTime>
#include <QDebug>
#include <QImage>
#include <QList>
#include <QString>
#include <QVariant>
#include <QVariantMap>

/*============================================================================
================================ UserRepository ==============================
============================================================================*/

/*============================== Public constructors =======================*/

UserRepository::UserRepository(DataSource *source) :
    Source(source)
{
    //
}

UserRepository::~UserRepository()
{
    //
}

/*============================== Public methods ============================*/

long UserRepository::count()
{
    if (!isValid())
        return 0;
    TransactionHolder holder(Source);
    BSqlResult result = Source->select("users", "COUNT(id)");
    holder.setSuccess(result.success());
    return result.value("COUNT(id)").toLongLong();
}

long UserRepository::count(const TAccessLevel &accessLevel)
{
    if (!isValid())
        return 0;
    TransactionHolder holder(Source);
    BSqlResult result = Source->select("users", "COUNT(id)",
                                       BSqlWhere("access_level = :access_level", ":access_level", int(accessLevel)));
    holder.setSuccess(result.success());
    return result.value("COUNT(id)").toLongLong();
}

DataSource *UserRepository::dataSource() const
{
    return Source;
}

bool UserRepository::deleteAll()
{
    //
}

bool UserRepository::deleteOne(const TUserIdentifier &id)
{
    //
}

bool UserRepository::deleteSome(const TIdList &ids)
{
    //
}

bool UserRepository::exists(const TUserIdentifier &id)
{
    //
}

QList<User> UserRepository::findAll()
{
    //
}

QList<User> UserRepository::findAll(const TIdList &ids)
{
    //
}

User UserRepository::findOne(const TUserIdentifier &id)
{
    //
}

bool UserRepository::isValid() const
{
    return Source && Source->isValid();
}

QDateTime UserRepository::lastModificationDateTime(const TUserIdentifier &id)
{
    //
}

quint64 UserRepository::save(const User &entity)
{
    if (!isValid() || !entity.isValid())
        return 0;
    QVariantMap values;
    values.insert("access_level", int(entity.accessLevel()));
    values.insert("active", int(entity.active()));
    values.insert("email", entity.email());
    values.insert("last_modification_date_time", entity.lastModificationDateTime().toUTC().toMSecsSinceEpoch());
    values.insert("login", entity.login());
    values.insert("name", entity.name());
    values.insert("password", entity.password());
    values.insert("patronymic", entity.patronymic());
    values.insert("registration_date_time", entity.registrationDateTime().toUTC().toMSecsSinceEpoch());
    values.insert("surname", entity.surname());
    if (entity.id()) {
        return Source->update("users", values, BSqlWhere("id = :id", ":id", entity.id()));
    } else {
        BSqlResult result = Source->insert("users", values);
        return result.success() ? result.lastInsertId().toLongLong() : 0;
    }
}

TIdList UserRepository::save(const QList<User> &entities)
{
    //
}

/*============================== Private methods ===========================*/

QImage UserRepository::fetchAvatar(quint64 userId, bool *ok)
{
    //
}
