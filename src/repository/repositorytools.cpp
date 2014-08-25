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

#include "repositorytools.h"

#include "datasource.h"

#include <TAuthorInfo>
#include <TAuthorInfoList>
#include <TIdList>
#include <TService>
#include <TServiceList>

#include <BSqlResult>
#include <BSqlWhere>

#include <QString>
#include <QStringList>
#include <QVariant>
#include <QVariantMap>

/*============================================================================
================================ RepositoryTools =============================
============================================================================*/

namespace RepositoryTools
{

bool deleteHelper(DataSource *source, const QString &table, const QString &idField, quint64 id)
{
    if (!source || !source->isValid() || table.isEmpty() || idField.isEmpty() || !id)
        return false;
    return source->deleteFrom(table, BSqlWhere(idField + " = :" + idField, ":" + idField, id));
}

bool deleteHelper(DataSource *source, const QStringList &tables, const QString &idField, quint64 id)
{
    foreach (const QString &table, tables) {
        if (!deleteHelper(source, table, idField, id))
            return false;
    }
    return true;
}

TAuthorInfoList getAuthorInfoList(DataSource *source, const QString &table, const QString &idField, quint64 id,
                                  bool *ok)
{
    if (!source || !source->isValid() || table.isEmpty() || idField.isEmpty() || !id)
        return bRet(ok, false, TAuthorInfoList());
    BSqlResult result = source->select(table, QStringList() << "name" << "organization" << "patronymic" << "post"
                                       << "role" << "surname",
                                       BSqlWhere(idField + " = :" + idField, ":" + idField, id));
    if (!result.success())
        return bRet(ok, false, TAuthorInfoList());
    TAuthorInfoList list;
    foreach (const QVariantMap &m, result.values()) {
        TAuthorInfo info;
        info.setName(m.value("name").toString());
        info.setOrganization(m.value("organization").toString());
        info.setPatronymic(m.value("patronymic").toString());
        info.setPost(m.value("post").toString());
        info.setRole(m.value("role").toString());
        info.setSurname(m.value("surname").toString());
        list << info;
    }
    return bRet(ok, false, list);
}

TIdList getGroupIdList(DataSource *source, const QString &table, const QString &idField, quint64 id, bool *ok)
{
    if (!source || !source->isValid() || table.isEmpty() || idField.isEmpty() || !id)
        return bRet(ok, false, TIdList());
    BSqlResult result = source->select(table, "group_id", BSqlWhere(idField + " = :" + idField, ":" + idField, id));
    if (!result.success())
        return  bRet(ok, false, TIdList());
    TIdList list;
    foreach (const QVariantMap &m, result.values())
        list << m.value("group_id").toULongLong();
    return bRet(ok, true, list);
}

TServiceList getServices(DataSource *source, const QString &table, const QString &idField, quint64 id, bool *ok)
{
    if (!source || !source->isValid() || table.isEmpty() || idField.isEmpty() || !id)
        return bRet(ok, false, TServiceList());
    BSqlResult result = source->select(table, "service_type",
                                       BSqlWhere(idField + " = :" + idField, ":" + idField, id));
    if (!result.success())
        return  bRet(ok, false, TServiceList());
    TServiceList list;
    foreach (const QVariantMap &m, result.values())
        list << m.value("service_type").toInt();
    return bRet(ok, true, list);
}

QStringList getTags(DataSource *source, const QString &table, const QString &idField, quint64 id, bool *ok)
{
    if (!source || !source->isValid() || table.isEmpty() || idField.isEmpty() || !id)
        return bRet(ok, false, QStringList());
    BSqlResult result = source->select(table, "tag", BSqlWhere(idField + " = :" + idField, ":" + idField, id));
    if (!result.success())
        return  bRet(ok, false, QStringList());
    QStringList list;
    foreach (const QVariantMap &m, result.values())
        list << m.value("tag").toString();
    return bRet(ok, true, list);
}

bool setAuthorInfoList(DataSource *source, const QString &table, const QString &idField, quint64 id,
                       const TAuthorInfoList &list)
{
    if (!source || !source->isValid() || table.isEmpty() || idField.isEmpty() || !id)
        return false;
    foreach (const TAuthorInfo &info, list) {
        QVariantMap values;
        values.insert(idField, id);
        values.insert("name", info.name());
        values.insert("organization", info.organization());
        values.insert("patronymic", info.patronymic());
        values.insert("post", info.post());
        values.insert("role", info.role());
        values.insert("surname", info.surname());
        if (source->insert(table, values))
            return false;
    }
    return true;
}

bool setGroupIdList(DataSource *source, const QString &table, const QString &idField, quint64 id, const TIdList &list)
{
    if (!source || !source->isValid() || table.isEmpty() || idField.isEmpty() || !id)
        return false;
    foreach (quint64 groupId, list) {
        if (!source->insert(table, idField, id, "group_id", groupId))
            return false;
    }
    return true;
}

bool setServices(DataSource *source, const QString &table, const QString &idField, quint64 id,
                 const TServiceList &list)
{
    if (!source || !source->isValid() || table.isEmpty() || idField.isEmpty() || !id)
        return false;
    foreach (const TService &service, list) {
        if (!source->insert(table, idField, id, "service_type", int(service)))
            return false;
    }
    return true;
}

bool setTags(DataSource *source, const QString &table, const QString &idField, quint64 id, const QStringList &list)
{
    if (!source || !source->isValid() || table.isEmpty() || idField.isEmpty() || !id)
        return false;
    foreach (const QString &tag, list) {
        if (!source->insert(table, idField, id, "tag", tag))
            return false;
    }
    return true;
}

}
