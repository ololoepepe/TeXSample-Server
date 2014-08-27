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

#ifndef REPOSITORYTOOLS_H
#define REPOSITORYTOOLS_H

class DataSource;

class TAuthorInfoList;
class TIdList;
class TServiceList;

class QString;
class QStringList;

#include <QtGlobal>

/*============================================================================
================================ RepositoryTools =============================
============================================================================*/

namespace RepositoryTools
{

bool deleteHelper(DataSource *source, const QString &table, const QString &idField, quint64 id);
bool deleteHelper(DataSource *source, const QStringList &tables, const QString &idField, quint64 id);
TAuthorInfoList getAuthorInfoList(DataSource *source, const QString &table, const QString &idField, quint64 id,
                                  bool *ok = 0);
TIdList getGroupIdList(DataSource *source, const QString &table, const QString &idField, quint64 id, bool *ok = 0);
TServiceList getServices(DataSource *source, const QString &table, const QString &idField, quint64 id, bool *ok = 0);
QStringList getTags(DataSource *source, const QString &table, const QString &idField, quint64 id, bool *ok = 0);
bool setAuthorInfoList(DataSource *source, const QString &table, const QString &idField, quint64 id,
                       const TAuthorInfoList &list);
bool setGroupIdList(DataSource *source, const QString &table, const QString &idField, quint64 id, const TIdList &list);
bool setServices(DataSource *source, const QString &table, const QString &idField, quint64 id,
                 const TServiceList &list);
bool setTags(DataSource *source, const QString &table, const QString &idField, quint64 id, const QStringList &list);

}

#endif // REPOSITORYTOOLS_H
