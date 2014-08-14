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
