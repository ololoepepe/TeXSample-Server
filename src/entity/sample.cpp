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

#include "sample.h"

#include "repository/samplerepository.h"

#include <TAuthorInfoList>
#include <TBinaryFile>
#include <TeXSample>
#include <TSampleType>
#include <TTexProject>

#include <QDateTime>
#include <QString>
#include <QStringList>

/*============================================================================
================================ Sample ======================================
============================================================================*/

/*============================== Public constructors =======================*/

Sample::Sample()
{
    init();
}

Sample::Sample(const Sample &other)
{
    init();
    *this = other;
}

Sample::~Sample()
{
    //
}

/*============================== Protected constructors ====================*/

Sample::Sample(SampleRepository *repo)
{
    init();
    this->repo = repo;
    createdByRepo = true;
}

/*============================== Public methods ============================*/

QString Sample::adminRemark() const
{
    return madminRemark;
}

TAuthorInfoList Sample::authors() const
{
    return mauthors;
}

void Sample::convertToCreatedByUser()
{
    if (!createdByRepo)
        return;
    createdByRepo = false;
    repo = 0;
    valid = false;
    mpreviewMainFile.clear();
    msource.clear();
    previewFetched = false;
    sourceFetched = false;
}

QDateTime Sample::creationDateTime() const
{
    return mcreationDateTime;
}

QString Sample::description() const
{
    return mdescription;
}

quint64 Sample::id() const
{
    return mid;
}

bool Sample::isCreatedByRepo() const
{
    return createdByRepo;
}

bool Sample::isValid() const
{
    if (createdByRepo)
        return valid;
    return msenderId && !mtitle.isEmpty()
            && ((mid && !msaveData) || (msource.isValid() && mpreviewMainFile.isValid()));
}

QDateTime Sample::lastModificationDateTime() const
{
    return mlastModificationDateTime;
}

const TBinaryFile &Sample::previewMainFile() const
{
    static const TBinaryFile Default;
    if (!createdByRepo || previewFetched)
        return mpreviewMainFile;
    if (!repo || !repo->isValid())
        return Default;
    Sample *self = getSelf();
    bool ok = false;
    self->mpreviewMainFile = self->repo->fetchPreview(mid, &ok);
    if (!ok)
        return Default;
    self->previewFetched = true;
    return mpreviewMainFile;
}

TFileInfo Sample::previewMainFileInfo() const
{
    return mpreviewMainFileInfo;
}

quint8 Sample::rating() const
{
    return mrating;
}

SampleRepository *Sample::repository() const
{
    return repo;
}

bool Sample::saveAdminRemark() const
{
    return msaveAdminRemark;
}

bool Sample::saveData() const
{
    return msaveData;
}

quint64 Sample::senderId() const
{
    return msenderId;
}

void Sample::setAdminRemark(const QString &remark)
{
    madminRemark = Texsample::testAdminRemark(remark) ? remark : QString();
}

void Sample::setAuthors(const TAuthorInfoList &authors)
{
    mauthors = authors;
}

void Sample::setDescription(const QString &description)
{
    mdescription = Texsample::testSampleDescription(description) ? description : QString();
}

void Sample::setId(quint64 id)
{
    mid = id;
}

void Sample::setPreviewMainFile(const TBinaryFile &file)
{
    mpreviewMainFile = file;
}

void Sample::setRating(quint8 rating)
{
    mrating = (rating < 100) ? rating : 100;
}

void Sample::setSaveAdminRemark(bool save)
{
    msaveAdminRemark = save;
}

void Sample::setSaveData(bool save)
{
    msaveData = save;
}

void Sample::setSenderId(quint64 senderId)
{
    msenderId = senderId;
}

void Sample::setSource(const TTexProject &project)
{
    msource = project;
}

void Sample::setTags(const QStringList &tags)
{
    mtags = tags;
}

void Sample::setTitle(const QString &title)
{
    mtitle = Texsample::testSampleTitle(title) ? title : QString();
}

void Sample::setType(const TSampleType &type)
{
    mtype = type;
}

const TTexProject &Sample::source() const
{
    static const TTexProject Default;
    if (!createdByRepo || sourceFetched)
        return msource;
    if (!repo || !repo->isValid())
        return Default;
    Sample *self = getSelf();
    bool ok = false;
    self->msource = self->repo->fetchSource(mid, &ok);
    if (!ok)
        return Default;
    self->sourceFetched = true;
    return msource;
}

TFileInfoList Sample::sourceExtraFileInfos() const
{
    return msourceExtraFileInfos;
}

TFileInfo Sample::sourceMainFileInfo() const
{
    return msourceMainFileInfo;
}

QStringList Sample::tags() const
{
    return mtags;
}

QString Sample::title() const
{
    return mtitle;
}

TSampleType Sample::type() const
{
    return mtype;
}

/*============================== Public operators ==========================*/

Sample &Sample::operator =(const Sample &other)
{
    mid = other.mid;
    msenderId = other.msenderId;
    madminRemark = other.madminRemark;
    mauthors = other.mauthors;
    mcreationDateTime = other.mcreationDateTime;
    mdescription = other.mdescription;
    mlastModificationDateTime = other.mlastModificationDateTime;
    mpreviewMainFile = other.mpreviewMainFile;
    mpreviewMainFileInfo = other.mpreviewMainFileInfo;
    mrating = other.mrating;
    msaveAdminRemark = other.msaveAdminRemark;
    msaveData = other.msaveData;
    msource = other.msource;
    msourceExtraFileInfos = other.msourceExtraFileInfos;
    msourceMainFileInfo = other.msourceMainFileInfo;
    mtags = other.mtags;
    mtitle = other.mtitle;
    mtype = other.mtype;
    createdByRepo = other.createdByRepo;
    mainFileName = other.mainFileName;
    previewFetched = other.previewFetched;
    repo = other.repo;
    sourceFetched = other.sourceFetched;
    valid = other.valid;
    return *this;
}

/*============================== Private methods ===========================*/

Sample *Sample::getSelf() const
{
    return const_cast<Sample *>(this);
}

void Sample::init()
{
    mid = 0;
    msenderId = 0;
    mcreationDateTime = QDateTime().toUTC();
    mlastModificationDateTime = QDateTime().toUTC();
    mrating = 0;
    msaveAdminRemark = false;
    msaveData = false;
    createdByRepo = false;
    previewFetched = false;
    repo = 0;
    sourceFetched = false;
    valid = false;
}
