#include "user.h"

#include "repository/userrepository.h"

#include <TAccessLevel>
#include <TeXSample>
#include <TGroupInfoList>
#include <TServiceList>

#include <QByteArray>
#include <QDateTime>
#include <QImage>
#include <QString>

/*============================================================================
================================ User ========================================
============================================================================*/

/*============================== Public constructors =======================*/

User::User()
{
    init();
}

User::User(const User &other)
{
    init();
    *this = other;
}

User::~User()
{
    //
}

/*============================== Protected constructors ====================*/

User::User(UserRepository *repo)
{
    init();
    this->repo = repo;
}

/*============================== Public methods ============================*/

TAccessLevel User::accessLevel() const
{
    return maccessLevel;
}

bool User::active() const
{
    return mactive;
}

TGroupInfoList User::availableGroups() const
{
    return mavailableGroups;
}

TServiceList User::availableServices() const
{
    return mavailableServices;
}

QImage User::avatar() const
{
    if (avatarFetched)
        return mavatar;
    if (!repo || !repo->isValid())
        return QImage();
    bool ok = false;
    *const_cast<QImage *>(&mavatar) = const_cast<UserRepository *>(repo)->fetchAvatar(mid, &ok);
    if (!ok)
        return QImage();
    *const_cast<bool *>(&avatarFetched) = true;
    return mavatar;
}

QString User::email() const
{
    return memail;
}

TGroupInfoList User::groups() const
{
    return mgroups;
}

quint64 User::id() const
{
    return mid;
}

bool User::isValid() const
{
    return maccessLevel.isValid() && !memail.isEmpty() && !mlogin.isEmpty() && mlastModificationDateTime.isValid()
            && !mpassword.isEmpty() && mregistrationDateTime.isValid();
}

QDateTime User::lastModificationDateTime() const
{
    return mlastModificationDateTime;
}

QString User::login() const
{
    return mlogin;
}

QString User::name() const
{
    return mname;
}

QByteArray User::password() const
{
    return mpassword;
}

QString User::patronymic() const
{
    return mpatronymic;
}

QDateTime User::registrationDateTime() const
{
    return mregistrationDateTime;
}

UserRepository *User::repository() const
{
    return repo;
}

void User::setAccessLevel(const TAccessLevel &lvl)
{
    maccessLevel = lvl;
}

void User::setActive(bool active)
{
    mactive = active;
}

void User::setAvailableGroups(const TGroupInfoList &groups)
{
    mavailableGroups = groups;
    bRemoveDuplicates(mavailableGroups);
}

void User::setAvailableServices(const TServiceList &services)
{
    mavailableServices = services;
    bRemoveDuplicates(mavailableServices);
}

void User::setAvatar(const QImage &avatar)
{
    mavatar = Texsample::testAvatar(avatar) ? avatar : QImage();
}

void User::setEmail(const QString &email)
{
    memail = Texsample::testEmail(email) ? email : QString();
}

void User::setGroups(const TGroupInfoList &groups)
{
    mgroups = groups;
    bRemoveDuplicates(mgroups);
}

void User::setId(quint64 id)
{
    mid = id;
}

void User::setLastModificationDateTime(const QDateTime &dt)
{
    mlastModificationDateTime = dt.toUTC();
}

void User::setLogin(const QString &login)
{
    mlogin = Texsample::testLogin(login) ? login : QString();
}

void User::setName(const QString &name)
{
    mname = Texsample::testName(name) ? name : QString();
}

void User::setPassword(const QByteArray &password)
{
    mpassword = Texsample::testPassword(password) ? password : QByteArray();
}

void User::setPatronymic(const QString &patronymic)
{
    mpatronymic = Texsample::testName(patronymic) ? patronymic : QString();
}

void User::setRegistrationDateTime(const QDateTime &dt)
{
    mregistrationDateTime = dt.toUTC();
}

void User::setSurname(const QString &surname)
{
    msurname = Texsample::testName(surname) ? surname : QString();
}

QString User::surname() const
{
    return msurname;
}

/*============================== Public operators ==========================*/

User &User::operator =(const User &other)
{
    avatarFetched = other.avatarFetched;
    maccessLevel = other.maccessLevel;
    mactive = other.mactive;
    mavailableGroups = other.mavailableGroups;
    mavailableServices = other.mavailableServices;
    mavatar = other.mavatar;
    memail = other.memail;
    mgroups = other.mgroups;
    mid = other.mid;
    mlastModificationDateTime = other.mlastModificationDateTime;
    mlogin = other.mlogin;
    mname = other.mname;
    mpassword = other.mpassword;
    mpatronymic = other.mpatronymic;
    mregistrationDateTime = other.mregistrationDateTime;
    msurname = other.msurname;
    repo = other.repo;
    return *this;
}

/*============================== Private methods ===========================*/

void User::init()
{
    avatarFetched = false;
    mactive = false;
    mid = 0;
    mlastModificationDateTime = QDateTime().toUTC();
    mregistrationDateTime = QDateTime().toUTC();
    repo = 0;
}
