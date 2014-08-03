#include "group.h"

#include "repository/grouprepository.h"

#include <TeXSample>

#include <QDateTime>
#include <QString>

/*============================================================================
================================ Group =======================================
============================================================================*/

/*============================== Public constructors =======================*/

Group::Group()
{
    init();
}

Group::Group(const Group &other)
{
    init();
    *this = other;
}

Group::~Group()
{
    //
}

/*============================== Protected constructors ====================*/

Group::Group(GroupRepository *repo)
{
    init();
    this->repo = repo;
}

/*============================== Public methods ============================*/

QDateTime Group::creationDateTime() const
{
    return mcreationDateTime;
}

quint64 Group::id() const
{
    return mid;
}

bool Group::isValid() const
{
    return mcreationDateTime.isValid() && mlastModificationDateTime.isValid() && !mname.isEmpty() && mownerId
            && (mid || !mownerLogin.isEmpty());
}

QDateTime Group::lastModificationDateTime() const
{
    return mlastModificationDateTime;
}

QString Group::name() const
{
    return mname;
}

quint64 Group::ownerId() const
{
    return mownerId;
}

QString Group::ownerLogin() const
{
    return mownerLogin;
}

GroupRepository *Group::repository() const
{
    return repo;
}

void Group::setCreationDateTime(const QDateTime &dt)
{
    mcreationDateTime = dt.toUTC();
}

void Group::setId(quint64 id)
{
    mid = id;
}

void Group::setLastModificationDateTime(const QDateTime &dt)
{
    mlastModificationDateTime = dt.toUTC();
}

void Group::setName(const QString &name)
{
    mname = Texsample::testGroupName(name) ? name : QString();
}

void Group::setOwnerId(quint64 ownerId)
{
    mownerId = ownerId;
}

/*============================== Public operators ==========================*/

Group &Group::operator =(const Group &other)
{
    mcreationDateTime = other.mcreationDateTime;
    mid = other.mid;
    mlastModificationDateTime = other.mlastModificationDateTime;
    mname = other.mname;
    mownerId = other.mownerId;
    mownerLogin = other.mownerLogin;
    repo = other.repo;
    return *this;
}

/*============================== Private methods ===========================*/

void Group::init()
{
    mcreationDateTime = QDateTime().toUTC();
    mid = 0;
    mlastModificationDateTime = QDateTime().toUTC();
    mownerId = 0;
    repo = 0;
}
