/****************************************************************************
**
** Copyright (C) 2012-2014 Andrey Bogdanov
**
** This file is part of TeXSample Server.
**
** TeXSample Server is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** TeXSample Server is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with TeXSample Server.  If not, see <http://www.gnu.org/licenses/>.
**
****************************************************************************/

#include "user.h"

#include "repository/userrepository.h"

#include <TAccessLevel>
#include <TeXSample>
#include <TGroupInfoList>
#include <TIdList>
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
    createdByRepo = true;
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

TServiceList User::availableServices() const
{
    return mavailableServices;
}

QImage User::avatar() const
{
    if (!createdByRepo || avatarFetched)
        return mavatar;
    if (!repo || !repo->isValid())
        return QImage();
    User *self = getSelf();
    bool ok = false;
    self->mavatar = self->repo->fetchAvatar(mid, &ok);
    if (!ok)
        return QImage();
    self->avatarFetched = true;
    return mavatar;
}

void User::convertToCreatedByUser()
{
    if (!createdByRepo)
        return;
    createdByRepo = false;
    repo = 0;
    valid = false;
    mgroups.clear();
    avatarFetched = false;
}

QString User::email() const
{
    return memail;
}

TIdList User::groups() const
{
    return mgroups;
}

quint64 User::id() const
{
    return mid;
}

bool User::isCreatedByRepo() const
{
    return createdByRepo;
}

bool User::isValid() const
{
    if (createdByRepo)
        return valid;
    return maccessLevel.isValid() && !memail.isEmpty() && !mlogin.isEmpty() && !mpassword.isEmpty();
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

bool User::saveAvatar() const
{
    return msaveAvatar;
}

void User::setAccessLevel(const TAccessLevel &lvl)
{
    maccessLevel = lvl;
}

void User::setActive(bool active)
{
    mactive = active;
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

void User::setGroups(const TIdList &ids)
{
    mgroups = ids;
    mgroups.removeAll(0);
    bRemoveDuplicates(mgroups);
}

void User::setId(quint64 id)
{
    mid = id;
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

void User::setSaveAvatar(bool save)
{
    msaveAvatar = save;
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
    msaveAvatar = other.msaveAvatar;
    msurname = other.msurname;
    createdByRepo = other.createdByRepo;
    repo = other.repo;
    valid = other.valid;
    return *this;
}

/*============================== Private methods ===========================*/

User *User::getSelf() const
{
    return const_cast<User *>(this);
}

void User::init()
{
    avatarFetched = false;
    mactive = false;
    mid = 0;
    mlastModificationDateTime = QDateTime().toUTC();
    mregistrationDateTime = QDateTime().toUTC();
    msaveAvatar = false;
    createdByRepo = false;
    repo = 0;
    valid = false;
}
