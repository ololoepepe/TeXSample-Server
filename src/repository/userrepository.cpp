#include "userrepository.h"

#include "datasource.h"
#include "entity/user.h"
#include "transactionholder.h"

#include <TAccessLevel>
#include <TIdList>
#include <TUserIdentifier>

#include <BSqlResult>
#include <BSqlWhere>

#include <QBuffer>
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

quint64 UserRepository::save(const User &entity, bool saveAvatar)
{
    if (!isValid() || !entity.isValid())
        return 0;
    TransactionHolder holder(Source);
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
    BSqlResult result = entity.id() ? Source->update("users", values, BSqlWhere("id = :id", ":id", entity.id())) :
                                      Source->insert("users", values);
    if (!result.success())
        return 0;
    quint64 id = result.lastInsertId().toULongLong();
    if (saveAvatar && !this->saveAvatar(entity, id))
        return 0;
    if (!holder.doCommit())
        return 0;
    return entity.id() ? entity.id() : id;
}

TIdList UserRepository::save(const QList<User> &entities, bool saveAvatar)
{
    TIdList list;
    foreach (const User &entity, entities) {
        quint64 id = save(entity, saveAvatar);
        if (!id)
            return list;
        list << id;
    }
    return list;
}

/*============================== Private methods ===========================*/

QImage UserRepository::fetchAvatar(quint64 userId, bool *ok)
{
    if (!isValid() || !userId)
        return bRet(ok, false, QImage());
    bool b = false;
    QByteArray avatar = Source->readFile("users/" + QString::number(userId) + "/avatar.dat", &b);
    if (!b)
        return bRet(ok, false, QImage());
    if (avatar.isEmpty())
        return bRet(ok, true, QImage());
    QBuffer buff(&avatar);
    if (!buff.open(QBuffer::ReadOnly))
        return bRet(ok, false, QImage());
    QImage img;
    if (!img.load(&buff, "png"))
        return bRet(ok, false, QImage());
    return bRet(ok, true, img);
}

bool UserRepository::saveAvatar(const User &entity, quint64 id)
{
    if (!entity.isValid() || (!entity.id() && !id))
        return false;
    QByteArray avatar;
    if (!entity.avatar().isNull()) {
        QBuffer buff(&avatar);
        buff.open(QBuffer::WriteOnly);
        if (!entity.avatar().save(&buff, "png"))
            return false;
        buff.close();
    }
    return entity.id() ? Source->updateFile("users/" + QString::number(entity.id()) + "/avatar.dat", avatar) :
                         Source->createFile("users/" + QString::number(id) + "/avatar.dat", avatar);
}
