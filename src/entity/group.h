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
private:
    quint64 mid;
    quint64 mownerId;
    QDateTime mcreationDateTime;
    QDateTime mlastModificationDateTime;
    QString mname;
    bool createdByRepo;
    GroupRepository *repo;
    bool valid;
public:
    explicit Group();
    Group(const Group &other);
    ~Group();
protected:
    explicit Group(GroupRepository *repo);
public:
    void convertToCreatedByUser();
    QDateTime creationDateTime() const;
    quint64 id() const;
    bool isCreatedByRepo() const;
    bool isValid() const;
    QDateTime lastModificationDateTime() const;
    QString name() const;
    quint64 ownerId() const;
    GroupRepository *repository() const;
    void setCreationDateTime(const QDateTime &dt);
    void setId(quint64 id);
    void setName(const QString &name);
    void setOwnerId(quint64 ownerId);
public:
    Group &operator =(const Group &other);
private:
    void init();
private:
    friend class GroupRepository;
};

#endif // GROUP_H
