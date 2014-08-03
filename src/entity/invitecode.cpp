#include "invitecode.h"

//#include "repository/invitecoderepository.h"

#include <TAccessLevel>
#include <TeXSample>
#include <TGroupInfoList>
#include <TIdList>
#include <TServiceList>

#include <BUuid>

#include <QDateTime>
#include <QString>

/*============================================================================
================================ InviteCode ==================================
============================================================================*/

/*============================== Public constructors =======================*/

InviteCode::InviteCode()
{
    init();
}

InviteCode::InviteCode(const InviteCode &other)
{
    init();
    *this = other;
}

InviteCode::~InviteCode()
{
    //
}

/*============================== Protected constructors ====================*/

InviteCode::InviteCode(InviteCodeRepository *repo)
{
    init();
    this->repo = repo;
}

/*============================== Public methods ============================*/

TAccessLevel InviteCode::accessLevel() const
{
    return maccessLevel;
}

TServiceList InviteCode::availableServices() const
{
    return mavailableServices;
}

BUuid InviteCode::code() const
{
    return mcode;
}

QDateTime InviteCode::creationDateTime() const
{
    return mcreationDateTime;
}

QDateTime InviteCode::expirationDateTime() const
{
    return mexpirationDateTime;
}

TIdList InviteCode::groupIds() const
{
    return mgroupIds;
}

TGroupInfoList InviteCode::groups() const
{
    return mgroups;
}

quint64 InviteCode::id() const
{
    return mid;
}

bool InviteCode::isValid() const
{
    return maccessLevel.isValid() && !mcode.isNull() && mcreationDateTime.isValid() && mexpirationDateTime.isValid()
            && mownerId && (mid || !mownerLogin.isEmpty());
}

quint64 InviteCode::ownerId() const
{
    return mownerId;
}

QString InviteCode::ownerLogin() const
{
    return mownerLogin;
}

InviteCodeRepository *InviteCode::repository() const
{
    return repo;
}

void InviteCode::setAccessLevel(const TAccessLevel &accessLevel)
{
    maccessLevel = accessLevel;
}

void InviteCode::setAvailableServices(const TServiceList &services)
{
    mavailableServices = services;
}

void InviteCode::setCode(const BUuid &code)
{
    mcode = code;
}

void InviteCode::setCreationDateTime(const QDateTime &dt)
{
    mcreationDateTime = dt.toUTC();
}

void InviteCode::setExpirationDateTime(const QDateTime &dt)
{
    mexpirationDateTime = dt.toUTC();
}

void InviteCode::setGroupIds(const TIdList &ids)
{
    mgroupIds = ids;
    mgroupIds.removeAll(0);
    bRemoveDuplicates(mgroupIds);
}

void InviteCode::setId(quint64 id)
{
    mid = id;
}

void InviteCode::setOwnerId(quint64 ownerId)
{
    mownerId = ownerId;
}

/*============================== Public operators ==========================*/

InviteCode &InviteCode::operator =(const InviteCode &other)
{
    maccessLevel = other.maccessLevel;
    mavailableServices = other.mavailableServices;
    mcode = other.mcode;
    mcreationDateTime = other.mcreationDateTime;
    mexpirationDateTime = other.mexpirationDateTime;
    mgroupIds = other.mgroupIds;
    mgroups = other.mgroups;
    mid = other.mid;
    mownerId = other.mownerId;
    mownerLogin = other.mownerLogin;
    repo = other.repo;
    return *this;
}

/*============================== Private methods ===========================*/

void InviteCode::init()
{
    mcreationDateTime = QDateTime().toUTC();
    mexpirationDateTime = QDateTime().toUTC();
    mid = 0;
    mownerId = 0;
    repo = 0;
}
