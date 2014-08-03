#ifndef SAMPLE_H
#define SAMPLE_H

class SampleRepository;

#include <TAuthorInfoList>
#include <TBinaryFile>
#include <TBinaryFileList>
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
    QString msenderLogin;
    bool mdeleted;
    QString madminRemark;
    TAuthorInfoList mauthors;
    QDateTime mcreationDateTime;
    QString mdescription;
    QDateTime mlastModificationDateTime;
    TBinaryFileList mpreviewExtraFiles;
    TBinaryFile mpreviewMainFile;
    quint8 mrating;
    bool msaveAdminRemark;
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
    explicit Sample(bool saveAdminRemark = true);
    Sample(const Sample &other);
    ~Sample();
protected:
    explicit Sample(SampleRepository *repo);
public:
    QString adminRemark() const;
    TAuthorInfoList authors() const;
    QDateTime creationDateTime() const;
    bool deleted() const;
    QString description() const;
    quint64 id() const;
    bool isCreatedByRepo() const;
    bool isValid() const;
    QDateTime lastModificationDateTime() const;
    const TBinaryFileList &previewExtraFiles() const;
    const TBinaryFile &previewMainFile() const;
    quint8 rating() const;
    SampleRepository *repository() const;
    quint64 senderId() const;
    QString senderLogin() const;
    void setAdminRemark(const QString &remark);
    void setAuthors(const TAuthorInfoList &authors);
    void setCreationDateTime(const QDateTime &dt);
    void setDeleted(bool deleted);
    void setDescription(const QString &description);
    void setId(quint64 id);
    void setLastModificationDateTime(const QDateTime &dt);
    void setProject(const TTexProject &project);
    void setRating(quint8 rating);
    void setSenderId(quint64 senderId);
    void setTitle(const QString &title);
    void setType(const TSampleType &type);
    const TTexProject &source() const;
    QString title() const;
    TSampleType type() const;
public:
    Sample &operator =(const Sample &other);
private:
    void init();
    bool fetchPreview();
private:
    friend class SampleRepository;
};

#endif // SAMPLE_H
