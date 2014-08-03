#ifndef USERREPOSITORY_H
#define USERREPOSITORY_H

class DataSource;

class TAccessLevel;
class TUserIdentifier;

class QDateTime;
class QImage;
class QString;

#include "entity/user.h"

#include <TIdList>

#include <QList>

/*============================================================================
================================ UserRepository ==============================
============================================================================*/

class UserRepository
{
public:
    explicit UserRepository(DataSource *source);
    ~UserRepository();
public:
    long count();
    long count(const TAccessLevel &accessLevel);
    DataSource *dataSource() const;
    bool deleteAll();
    bool deleteOne(const TUserIdentifier &id);
    bool deleteSome(const TIdList &ids);
    bool exists(const TUserIdentifier &id);
    QList<User> findAll();
    QList<User> findAll(const TIdList &ids);
    User findOne(const TUserIdentifier &id);
    bool isValid() const;
    QDateTime lastModificationDateTime(const TUserIdentifier &id);
    quint64 save(const User &entity, bool saveAvatar = true);
    TIdList save(const QList<User> &entities, bool saveAvatar = true);
private:
    QImage fetchAvatar(quint64 userId, bool *ok = 0);
    bool saveAvatar(const User &entity, quint64 id = 0);
private:
    DataSource * const Source;
private:
    friend class User;
    Q_DISABLE_COPY(UserRepository)
};

#endif // USERREPOSITORY_H
