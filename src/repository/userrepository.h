#ifndef USERREPOSITORY_H
#define USERREPOSITORY_H

class DataSource;

class TAccessLevel;
class TUserIdentifier;

class QByteArray;
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
private:
    DataSource * const Source;
public:
    explicit UserRepository(DataSource *source);
    ~UserRepository();
public:
    quint64 add(const User &entity);
    long countByAccessLevel(const TAccessLevel &accessLevel);
    DataSource *dataSource() const;
    bool deleteOne(quint64 userId);
    bool edit(const User &entity);
    bool exists(const TUserIdentifier &id);
    TAccessLevel findAccessLevel(quint64 id);
    QList<User> findAllNewerThan(const QDateTime &newerThan);
    QDateTime findLastModificationDateTime(const TUserIdentifier &id);
    QString findLogin(quint64 id);
    User findOne(const TUserIdentifier &id);
    User findOne(const QString &identifier, const QByteArray &password);
    bool isValid() const;
private:
    bool createAvatar(quint64 userId, const QImage &avatar);
    bool deleteAvatar(quint64 userId);
    QImage fetchAvatar(quint64 userId, bool *ok = 0);
    bool updateAvatar(quint64 userId, const QImage &avatar);
private:
    friend class User;
    Q_DISABLE_COPY(UserRepository)
};

#endif // USERREPOSITORY_H
