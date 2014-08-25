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

#ifndef SAMPLE_H
#define SAMPLE_H

class SampleRepository;

#include <TAuthorInfoList>
#include <TBinaryFile>
#include <TSampleType>
#include <TTexProject>

#include <QDateTime>
#include <QString>
#include <QStringList>

/*============================================================================
================================ Sample ======================================
============================================================================*/

class Sample
{
private:
    quint64 mid;
    quint64 msenderId;
    bool mdeleted;
    QString madminRemark;
    TAuthorInfoList mauthors;
    QDateTime mcreationDateTime;
    QString mdescription;
    QDateTime mlastModificationDateTime;
    TBinaryFile mpreviewMainFile;
    quint8 mrating;
    bool msaveAdminRemark;
    bool msaveData;
    TTexProject msource;
    QStringList mtags;
    QString mtitle;
    TSampleType mtype;
    bool createdByRepo;
    QString mainFileName;
    bool previewFetched;
    SampleRepository *repo;
    bool sourceFetched;
    bool valid;
public:
    explicit Sample();
    Sample(const Sample &other);
    ~Sample();
protected:
    explicit Sample(SampleRepository *repo);
public:
    QString adminRemark() const;
    TAuthorInfoList authors() const;
    void convertToCreatedByUser();
    QDateTime creationDateTime() const;
    bool deleted() const;
    QString description() const;
    quint64 id() const;
    bool isCreatedByRepo() const;
    bool isValid() const;
    QDateTime lastModificationDateTime() const;
    const TBinaryFile &previewMainFile() const;
    quint8 rating() const;
    SampleRepository *repository() const;
    bool saveAdminRemark() const;
    bool saveData() const;
    quint64 senderId() const;
    void setAdminRemark(const QString &remark);
    void setAuthors(const TAuthorInfoList &authors);
    void setDeleted(bool deleted);
    void setDescription(const QString &description);
    void setId(quint64 id);
    void setProject(const TTexProject &project);
    void setRating(quint8 rating);
    void setSaveAdminRemark(bool save);
    void setSaveData(bool save);
    void setSenderId(quint64 senderId);
    void setTags(const QStringList &tags);
    void setTitle(const QString &title);
    void setType(const TSampleType &type);
    const TTexProject &source() const;
    QStringList tags() const;
    QString title() const;
    TSampleType type() const;
public:
    Sample &operator =(const Sample &other);
private:
    void init();
private:
    friend class SampleRepository;
};

#endif // SAMPLE_H
