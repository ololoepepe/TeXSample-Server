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

bool Sample::deleted() const
{
    return mdeleted;
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
    return msenderId && msource.isValid() && !mtitle.isEmpty() && ((mid && !msaveData) || msource.isValid());
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
    bool ok = false;
    *const_cast<TBinaryFile *>(&mpreviewMainFile) = const_cast<SampleRepository *>(repo)->fetchPreview(mid, &ok);
    if (!ok)
        return Default;
    *const_cast<bool *>(&previewFetched) = true;
    return mpreviewMainFile;
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

void Sample::setDeleted(bool deleted)
{
    mdeleted = deleted;
}

void Sample::setDescription(const QString &description)
{
    mdescription = Texsample::testSampleDescription(description) ? description : QString();
}

void Sample::setId(quint64 id)
{
    mid = id;
}

void Sample::setProject(const TTexProject &project)
{
    msource = project;
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
    bool ok = false;
    *const_cast<TTexProject *>(&msource) = const_cast<SampleRepository *>(repo)->fetchSource(mid, &ok);
    if (!ok)
        return Default;
    *const_cast<bool *>(&sourceFetched) = true;
    return msource;
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
    mdeleted = other.mdeleted;
    madminRemark = other.madminRemark;
    mauthors = other.mauthors;
    mcreationDateTime = other.mcreationDateTime;
    mdescription = other.mdescription;
    mlastModificationDateTime = other.mlastModificationDateTime;
    mpreviewMainFile = other.mpreviewMainFile;
    mrating = other.mrating;
    msaveAdminRemark = other.msaveAdminRemark;
    msaveData = other.msaveData;
    msource = other.msource;
    mtags = other.mtags;
    mtitle = other.mtitle;
    mtype = other.mtype;
    mainFileName = other.mainFileName;
    previewFetched = other.previewFetched;
    repo = other.repo;
    sourceFetched = other.sourceFetched;
    return *this;
}

/*============================== Private methods ===========================*/

void Sample::init()
{
    mid = 0;
    msenderId = 0;
    mdeleted = false;
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
