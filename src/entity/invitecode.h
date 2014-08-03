#ifndef INVITECODE_H
#define INVITECODE_H

class InviteCodeRepository;

#include <TAccessLevel>
#include <TGroupInfoList>
#include <TIdList>
#include <TServiceList>

#include <BUuid>

#include <QDateTime>
#include <QString>

/*============================================================================
================================ InviteCode ==================================
============================================================================*/

class InviteCode
{
private:
    quint64 mid;
    quint64 mownerId;
    QString mownerLogin;
    TAccessLevel maccessLevel;
    TServiceList mavailableServices;
    BUuid mcode;
    QDateTime mcreationDateTime;
    QDateTime mexpirationDateTime;
    TIdList mgroupIds;
    TGroupInfoList mgroups;
    bool createdByRepo;
    InviteCodeRepository *repo;
    bool valid;
public:
    explicit InviteCode();
    InviteCode(const InviteCode &other);
    ~InviteCode();
protected:
    explicit InviteCode(InviteCodeRepository *repo);
public:
    TAccessLevel accessLevel() const;
    TServiceList availableServices() const;
    BUuid code() const;
    QDateTime creationDateTime() const;
    QDateTime expirationDateTime() const;
    TIdList groupIds() const;
    TGroupInfoList groups() const;
    quint64 id() const;
    bool isCreatedByRepo() const;
    bool isValid() const;
    quint64 ownerId() const;
    QString ownerLogin() const;
    InviteCodeRepository *repository() const;
    void setAccessLevel(const TAccessLevel &accessLevel);
    void setAvailableServices(const TServiceList &services);
    void setCode(const BUuid &code);
    void setCreationDateTime(const QDateTime &dt);
    void setExpirationDateTime(const QDateTime &dt);
    void setGroupIds(const TIdList &ids);
    void setId(quint64 id);
    void setOwnerId(quint64 ownerId);
public:
    InviteCode &operator =(const InviteCode &other);
private:
    void init();
private:
    friend class InviteCodeRepository;
};

#endif // INVITECODE_H
