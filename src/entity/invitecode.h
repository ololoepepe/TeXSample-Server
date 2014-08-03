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
    TAccessLevel maccessLevel;
    TServiceList mavailableServices;
    BUuid mcode;
    QDateTime mcreationDateTime;
    QDateTime mexpirationDateTime;
    TIdList mgroupIds;
    TGroupInfoList mgroups;
    quint64 mid;
    quint64 mownerId;
    QString mownerLogin;
    InviteCodeRepository *repo;
private:
    friend class InviteCodeRepository;
};

#endif // INVITECODE_H
