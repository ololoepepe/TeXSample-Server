#ifndef LAB_H
#define LAB_H

class LabRepository;

class TBinaryFile;
class TLabData;

#include <TAuthorInfoList>
#include <TBinaryFileList>
#include <TGroupInfoList>
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
    QString msenderLogin;
    bool mdeleted;
    TAuthorInfoList mauthors;
    QDateTime mcreationDateTime;
    TLabDataList mdataList;
    QStringList mdeletedExtraFiles;
    QString mdescription;
    TBinaryFileList mextraFiles;
    TIdList mgroupIds;
    TGroupInfoList mgroups;
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
    bool deleted() const;
    const TLabData &labData(BeQt::OSType os) const;
    const TLabDataList &labDataList() const;
    QStringList deletedExtraFiles() const;
    QString description() const;
    const TBinaryFile &extraFile(const QString &fileName) const;
    const TBinaryFileList &extraFiles() const;
    TIdList groupIds() const;
    TGroupInfoList groups() const;
    quint64 id() const;
    bool isCreatedByRepo() const;
    bool isValid() const;
    QDateTime lastModificationDateTime() const;
    LabRepository *repository() const;
    bool saveData() const;
    quint64 senderId() const;
    QString senderLogin() const;
    void setAuthors(const TAuthorInfoList &authors);
    void setCreationDateTime(const QDateTime &dt);
    void setDataList(const TLabDataList &list);
    void setDeleted(bool deleted);
    void setDeletedExtraFiles(const QStringList &fileNames);
    void setDescription(const QString &description);
    void setExtraFiles(const TBinaryFileList &extraFiles);
    void setGroupIds(const TIdList &ids);
    void setId(quint64 id);
    void setLastModificationDateTime(const QDateTime &dt);
    void setSaveData(bool save);
    void setSenderId(quint64 senderId);
    void setTitle(const QString &title);
    void setType(const TLabType &type);
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
