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

#include "lab.h"

#include "repository/labrepository.h"

#include <TAuthorInfoList>
#include <TBinaryFile>
#include <TBinaryFileList>
#include <TeXSample>
#include <TFileInfoList>
#include <TIdList>
#include <TLabData>
#include <TLabDataInfo>
#include <TLabDataInfoList>
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

/*============================== Public constructors =======================*/

Lab::Lab()
{
    init();
}

Lab::Lab(const Lab &other)
{
    init();
    *this = other;
}

Lab::~Lab()
{
    //
}

/*============================== Protected constructors ====================*/

Lab::Lab(LabRepository *repo)
{
    init();
    this->repo = repo;
    createdByRepo = true;
}

/*============================== Public methods ============================*/

TAuthorInfoList Lab::authors() const
{
    return mauthors;
}

void Lab::convertToCreatedByUser()
{
    if (!createdByRepo)
        return;
    createdByRepo = false;
    repo = 0;
    valid = false;
    mdataInfoList.clear();
    mextraFileInfos.clear();
    mextraFiles.clear();
    mgroups.clear();
    fetchedData.clear();
    fetchedExtraFiles.clear();
}

QDateTime Lab::creationDateTime() const
{
    return mcreationDateTime;
}

QStringList Lab::deletedExtraFiles() const
{
    return mdeletedExtraFiles;
}

QString Lab::description() const
{
    return mdescription;
}

const TBinaryFile &Lab::extraFile(const QString &fileName) const
{
    static const TBinaryFile Default;
    if (!createdByRepo || fetchedExtraFiles.contains(fileName)) {
        foreach (int i, bRangeD(0, mextraFiles.size() - 1)) {
            if (mextraFiles.at(i).fileName() == fileName)
                return mextraFiles.at(i);
        }
        return Default;
    }
    if (!repo || !repo->isValid())
        return Default;
    bool b = false;
    foreach (const TFileInfo &fi, mextraFileInfos) {
        if (fi.fileName() == fileName) {
            b = true;
            break;
        }
    }
    if (!b)
        return Default;
    Lab *self = getSelf();
    bool ok = false;
    TBinaryFile f = self->repo->fetchExtraFile(mid, fileName, &ok);
    if (!ok)
        return Default;
    self->mextraFiles << f;
    self->fetchedExtraFiles << fileName;
    return mextraFiles.last();
}

TFileInfoList Lab::extraFileInfos() const
{
    return mextraFileInfos;
}

const TBinaryFileList &Lab::extraFiles() const
{
    return mextraFiles;
}

TIdList Lab::groups() const
{
    return mgroups;
}

quint64 Lab::id() const
{
    return mid;
}

bool Lab::isCreatedByRepo() const
{
    return createdByRepo;
}

bool Lab::isValid() const
{
    if (createdByRepo)
        return valid;
    return msenderId && !mtitle.isEmpty() && ((mid && !msaveData) || !mdataList.isEmpty());
}

const TLabData &Lab::labData(BeQt::OSType os) const
{
    static const TLabData Default;
    if (!createdByRepo || fetchedData.contains(os)) {
        foreach (int i, bRangeD(0, mdataList.size() - 1)) {
            if (mdataList.at(i).os() == os || TLabType::DesktopApplication != mdataList.at(i).type())
                return mdataList.at(i);
        }
        return Default;
    }
    if (!repo || !repo->isValid())
        return Default;
    bool b = false;
    foreach (const TLabDataInfo &ldi, mdataInfoList) {
        if (ldi.os() == os || TLabType::DesktopApplication != ldi.type()) {
            b = true;
            break;
        }
    }
    if (!b)
        return Default;
    Lab *self = getSelf();
    bool ok = false;
    TLabData data = self->repo->fetchData(mid, mtype, os, &ok);
    if (!ok)
        return Default;
    self->mdataList << data;
    self->fetchedData.insert(os, true);
    if (int(data.type()) != TLabType::DesktopApplication) {
        foreach (BeQt::OSType t, BeQt::allOSTypes())
            self->fetchedData.insert(t, true);
    }
    return mdataList.last();
}

TLabDataInfoList Lab::labDataInfos() const
{
    return mdataInfoList;
}

const TLabDataList &Lab::labDataList() const
{
    return mdataList;
}

QDateTime Lab::lastModificationDateTime() const
{
    return mlastModificationDateTime;
}

LabRepository *Lab::repository() const
{
    return repo;
}

bool Lab::saveData() const
{
    return msaveData;
}

quint64 Lab::senderId() const
{
    return msenderId;
}

void Lab::setAuthors(const TAuthorInfoList &authors)
{
    mauthors = authors;
}

void Lab::setDataList(const TLabDataList &list)
{
    mdataList = list;
}

void Lab::setDeletedExtraFiles(const QStringList &fileNames)
{
    mdeletedExtraFiles = fileNames;
}

void Lab::setDescription(const QString &description)
{
    mdescription = Texsample::testLabDescription(description) ? description : QString();
}

void Lab::setExtraFiles(const TBinaryFileList &extraFiles)
{
    mextraFiles = extraFiles;
}

void Lab::setGroups(const TIdList &ids)
{
    mgroups = ids;
    mgroups.removeAll(0);
    bRemoveDuplicates(mgroups);
}

void Lab::setId(quint64 id)
{
    mid = id;
}

void Lab::setSaveData(bool save)
{
    msaveData = save;
}

void Lab::setSenderId(quint64 senderId)
{
    msenderId = senderId;
}

void Lab::setTags(const QStringList &tags)
{
    mtags = tags;
}

void Lab::setTitle(const QString &title)
{
    mtitle = Texsample::testLabTitle(title) ? title : QString();
}

void Lab::setType(const TLabType &type)
{
    mtype = type;
}

QStringList Lab::tags() const
{
    return mtags;
}

QString Lab::title() const
{
    return mtitle;
}

TLabType Lab::type() const
{
    return mtype;
}

/*============================== Public operators ==========================*/

Lab &Lab::operator =(const Lab &other)
{
    mid = other.mid;
    msenderId = other.msenderId;
    mauthors = other.mauthors;
    mcreationDateTime = other.mcreationDateTime;
    mdataInfoList = other.mdataInfoList;
    mdataList = other.mdataList;
    mdeletedExtraFiles = other.mdeletedExtraFiles;
    mdescription = other.mdescription;
    mextraFileInfos = other.mextraFileInfos;
    mextraFiles = other.mextraFiles;
    mgroups = other.mgroups;
    mlastModificationDateTime = other.mlastModificationDateTime;
    msaveData = other.msaveData;
    mtags = other.mtags;
    mtitle = other.mtitle;
    mtype = other.mtype;
    createdByRepo = other.createdByRepo;
    fetchedData = other.fetchedData;
    fetchedExtraFiles = other.fetchedExtraFiles;
    repo = other.repo;
    valid = other.valid;
    return *this;
}

/*============================== Private methods ===========================*/

void Lab::init()
{
    mid = 0;
    msenderId = 0;
    mcreationDateTime = QDateTime().toUTC();
    mlastModificationDateTime = QDateTime().toUTC();
    msaveData = false;
    createdByRepo = false;
    repo = 0;
    valid = false;
}

Lab *Lab::getSelf() const
{
    return const_cast<Lab *>(this);
}
