#include "lab.h"

#include "repository/labrepository.h"

#include <TAuthorInfoList>
#include <TBinaryFile>
#include <TBinaryFileList>
#include <TeXSample>
#include <TIdList>
#include <TLabData>
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
    mextraFiles.clear();
    mgroups.clear();
    fetchedData.clear();
    fetchedExtraFiles.clear();
}

QDateTime Lab::creationDateTime() const
{
    return mcreationDateTime;
}

const TLabData &Lab::labData(BeQt::OSType os) const
{
    //
}

const TLabDataList &Lab::labDataList() const
{
    //
}

bool Lab::deleted() const
{
    return mdeleted;
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
    //return mextraFiles;
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
    return msenderId && !mdataList.isEmpty() && !mtitle.isEmpty() && ((mid && !msaveData) || !mdataList.isEmpty());
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

void Lab::setCreationDateTime(const QDateTime &dt)
{
    mcreationDateTime = dt.toUTC();
}

void Lab::setDataList(const TLabDataList &list)
{
    mdataList = list;
}

void Lab::setDeleted(bool deleted)
{
    mdeleted = deleted;
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
    mdeleted = other.mdeleted;
    mauthors = other.mauthors;
    mcreationDateTime = other.mcreationDateTime;
    mdataList = other.mdataList;
    mdeletedExtraFiles = other.mdeletedExtraFiles;
    mdescription = other.mdescription;
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
    mdeleted = false;
    mcreationDateTime = QDateTime().toUTC();
    mlastModificationDateTime = QDateTime().toUTC();
    msaveData = false;
    createdByRepo = false;
    repo = 0;
    valid = false;
}

bool Lab::fetchAllData()
{
    //
}

bool Lab::fetchData(BeQt::OSType os)
{
    //
}

bool Lab::fetchExtraFile(const QString &fileName)
{
    //
}