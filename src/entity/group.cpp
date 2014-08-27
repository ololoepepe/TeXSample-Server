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
    createdByRepo = true;
}

/*============================== Public methods ============================*/

void Group::convertToCreatedByUser()
{
    if (!createdByRepo)
        return;
    createdByRepo = false;
    repo = 0;
    valid = false;
}

QDateTime Group::creationDateTime() const
{
    return mcreationDateTime;
}

quint64 Group::id() const
{
    return mid;
}

bool Group::isCreatedByRepo() const
{
    return createdByRepo;
}

bool Group::isValid() const
{
    if (createdByRepo)
        return valid;
    return !mname.isEmpty() && mownerId;
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

GroupRepository *Group::repository() const
{
    return repo;
}

void Group::setId(quint64 id)
{
    mid = id;
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
    createdByRepo = other.createdByRepo;
    repo = other.repo;
    valid = other.valid;
    return *this;
}

/*============================== Private methods ===========================*/

void Group::init()
{
    mcreationDateTime = QDateTime().toUTC();
    mid = 0;
    mlastModificationDateTime = QDateTime().toUTC();
    mownerId = 0;
    createdByRepo = false;
    repo = 0;
    valid = false;
}
