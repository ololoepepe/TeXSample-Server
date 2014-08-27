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
