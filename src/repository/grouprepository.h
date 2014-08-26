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

#ifndef GROUPREPOSITORY_H
#define GROUPREPOSITORY_H

class DataSource;

#include "entity/group.h"

#include <TIdList>

#include <QDateTime>
#include <QList>

/*============================================================================
================================ GroupRepository =============================
============================================================================*/

class GroupRepository
{
private:
    DataSource * const Source;
public:
    explicit GroupRepository(DataSource *source);
    ~GroupRepository();
public:
    quint64 add(const Group &entity, bool *ok = 0);
    DataSource *dataSource() const;
    void deleteOne(quint64 id, bool *ok = 0);
    void edit(const Group &entity, bool *ok = 0);
    QList<Group> findAll(bool *ok = 0);
    QList<Group> findAll(const TIdList &ids, bool *ok = 0);
    QList<Group> findAllByUserId(quint64 userId, const QDateTime &newerThan = QDateTime(), bool *ok = 0);
    Group findOne(quint64 id, bool *ok = 0);
    bool isValid() const;
private:
    friend class Group;
    Q_DISABLE_COPY(GroupRepository)
};

#endif // GROUPREPOSITORY_H
