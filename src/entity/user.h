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

#ifndef USER_H
#define USER_H

class UserRepository;

#include <TAccessLevel>
#include <TIdList>
#include <TServiceList>

#include <QByteArray>
#include <QDateTime>
#include <QImage>
#include <QString>

/*============================================================================
================================ User ========================================
============================================================================*/

class User
{
private:
    quint64 mid;
    TAccessLevel maccessLevel;
    bool mactive;
    TIdList mavailableGroups;
    TServiceList mavailableServices;
    QImage mavatar;
    QString memail;
    TIdList mgroups;
    QDateTime mlastModificationDateTime;
    QString mlogin;
    QString mname;
    QByteArray mpassword;
    QString mpatronymic;
    QDateTime mregistrationDateTime;
    QString msurname;
    bool avatarFetched;
    bool createdByRepo;
    UserRepository *repo;
    bool valid;
public:
    explicit User();
    User(const User &other);
    ~User();
protected:
    explicit User(UserRepository *repo);
public:
    TAccessLevel accessLevel() const;
    bool active() const;
    TServiceList availableServices() const;
    QImage avatar() const;
    void convertToCreatedByUser();
    QString email() const;
    TIdList groups() const;
    quint64 id() const;
    bool isCreatedByRepo() const;
    bool isValid() const;
    QDateTime lastModificationDateTime() const;
    QString login() const;
    QString name() const;
    QByteArray password() const;
    QString patronymic() const;
    QDateTime registrationDateTime() const;
    UserRepository *repository() const;
    void setAccessLevel(const TAccessLevel &accessLevel);
    void setActive(bool active);
    void setAvailableServices(const TServiceList &services);
    void setAvatar(const QImage &avatar);
    void setEmail(const QString &email);
    void setGroups(const TIdList &ids);
    void setId(quint64 id);
    void setLogin(const QString &login);
    void setName(const QString &name);
    void setPassword(const QByteArray &password);
    void setPatronymic(const QString &partonymic);
    void setSurname(const QString &surname);
    QString surname() const;
public:
    User &operator =(const User &other);
private:
    User *getSelf() const;
    void init();
private:
    friend class UserRepository;
};

#endif // USER_H
