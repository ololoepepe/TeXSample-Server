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

#ifndef LAB_H
#define LAB_H

class LabRepository;

class TBinaryFile;
class TLabData;

#include <TAuthorInfoList>
#include <TBinaryFileList>
#include <TIdList>
#include <TLabDataList>
#include <TLabType>

#include <BeQt>

#include <QDateTime>
#include <QMap>
#include <QString>
#include <QStringList>

/*============================================================================
================================ Lab =========================================
============================================================================*/

class Lab
{
private:
    quint64 mid;
    quint64 msenderId;
    TAuthorInfoList mauthors;
    QDateTime mcreationDateTime;
    TLabDataList mdataList;
    QStringList mdeletedExtraFiles;
    QString mdescription;
    TBinaryFileList mextraFiles;
    TIdList mgroups;
    QDateTime mlastModificationDateTime;
    bool msaveData;
    QStringList mtags;
    QString mtitle;
    TLabType mtype;
    bool createdByRepo;
    QMap<BeQt::OSType, bool> fetchedData;
    QStringList fetchedExtraFiles;
    LabRepository *repo;
    bool valid;
public:
    explicit Lab();
    Lab(const Lab &other);
    ~Lab();
protected:
    explicit Lab(LabRepository *repo);
public:
    TAuthorInfoList authors() const;
    void convertToCreatedByUser();
    QDateTime creationDateTime() const;
    const TLabData &labData(BeQt::OSType os) const;
    const TLabDataList &labDataList() const;
    QStringList deletedExtraFiles() const;
    QString description() const;
    const TBinaryFile &extraFile(const QString &fileName) const;
    const TBinaryFileList &extraFiles() const;
    TIdList groups() const;
    quint64 id() const;
    bool isCreatedByRepo() const;
    bool isValid() const;
    QDateTime lastModificationDateTime() const;
    LabRepository *repository() const;
    bool saveData() const;
    quint64 senderId() const;
    void setAuthors(const TAuthorInfoList &authors);
    void setDataList(const TLabDataList &list);
    void setDeletedExtraFiles(const QStringList &fileNames);
    void setDescription(const QString &description);
    void setExtraFiles(const TBinaryFileList &extraFiles);
    void setGroups(const TIdList &ids);
    void setId(quint64 id);
    void setSaveData(bool save);
    void setSenderId(quint64 senderId);
    void setTags(const QStringList &tags);
    void setTitle(const QString &title);
    void setType(const TLabType &type);
    QStringList tags() const;
    QString title() const;
    TLabType type() const;
public:
    Lab &operator =(const Lab &other);
private:
    void init();
    bool fetchAllData();
    bool fetchData(BeQt::OSType os);
    bool fetchExtraFile(const QString &fileName = QString());
private:
    friend class LabRepository;
};

#endif // LAB_H
