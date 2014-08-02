#ifndef GROUP_H
#define GROUP_H

class GroupRepository;

#include <QDateTime>
#include <QString>

/*============================================================================
================================ Group =======================================
============================================================================*/

class Group
{
public:
    explicit Group();
    Group(const Group &other);
    ~Group();
protected:
    explicit Group(GroupRepository *repo);
public:
    QDateTime creationDateTime() const;
    quint64 id() const;
    bool isValid() const;
    QDateTime lastModificationDateTime() const;
    QString name() const;
    quint64 ownerId() const;
    QString ownerLogin() const;
    GroupRepository *repository() const;
    void setCreationDateTime(const QDateTime &dt);
    void setId(quint64 id);
    void setLastModificationDateTime(const QDateTime &dt);
    void setName(const QString &name);
    void setOwnerId(quint64 ownerId);
    void setOwnerLogin(const QString &login);
public:
    Group &operator =(const Group &other);
private:
    void init();
private:
    QDateTime mcreationDateTime;
    quint64 mid;
    QDateTime mlastModificationDateTime;
    QString mname;
    quint64 mownerId;
    QString mownerLogin;
    GroupRepository *repo;
private:
    friend class GroupRepository;
};

#endif // GROUP_H
