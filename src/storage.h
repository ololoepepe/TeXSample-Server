#ifndef STORAGE_H
#define STORAGE_H

class Database;

class TUserInfo;
class TOperationResult;
class TAccessLevel;
class TCompilationResult;
class TProject;
class TProjectFile;

class QDateTime;

#include <TeXSample>
#include <TSampleInfo>
#include <TInviteInfo>

#include <QString>
#include <QMutex>
#include <QCoreApplication>
#include <QByteArray>

/*============================================================================
================================ Storage =====================================
============================================================================*/

class Storage
{
    Q_DECLARE_TR_FUNCTIONS(Storage)
public:
    static TOperationResult invalidInstanceResult();
    static TOperationResult invalidParametersResult();
    static TOperationResult databaseErrorResult();
    static TOperationResult queryErrorResult();
    static TOperationResult fileSystemErrorResult();
    static void lockGlobal();
    static bool tryLockGlobal();
    static void unlockGlobal();
    static bool initStorage(const QString &rootDir, QString *errorString = 0);
    static bool copyTexsample(const QString &path, const QString &codecName = "UTF-8");
    static bool removeTexsample(const QString &path);
public:
    explicit Storage(const QString &rootDir);
    ~Storage();
public:
    void lock();
    bool tryLock();
    void unlock();
    TOperationResult addUser(const TUserInfo &info);
    TOperationResult editUser(const TUserInfo &info);
    TOperationResult getUserInfo(quint64 userId, TUserInfo &info, QDateTime &updateDT, bool &cacheOk);
    TOperationResult getShortUserInfo(quint64 userId, TUserInfo &info);
    TCompilationResult addSample(quint64 userId, TProject project, const TSampleInfo &info);
    TCompilationResult editSample(const TSampleInfo &info, TProject project);
    TOperationResult deleteSample(quint64 id, const QString &reason);
    TOperationResult getSampleSource(quint64 id, TProject &project, QDateTime &updateDT, bool &cacheOk);
    TOperationResult getSamplePreview(quint64 id, TProjectFile &file, QDateTime &updateDT, bool &cacheOk);
    TOperationResult getSamplesList(TSampleInfo::SamplesList &newSamples, Texsample::IdList &deletedSamples,
                                    QDateTime &updateDT);
    TOperationResult generateInvites(quint64 userId, const QDateTime &expiresDT, quint8 count,
                                     TInviteInfo::InvitesList &invites);
    TOperationResult getInvitesList(quint64 userId, TInviteInfo::InvitesList &invites);
    bool deleteInvite(quint64 id);
    quint64 userId(const QString &login, const QByteArray &password = QByteArray());
    quint64 authorId(quint64 sampleId);
    TSampleInfo::Type sampleType(quint64 id);
    QString sampleFileName(quint64 id);
    QDateTime sampleCreationDateTime(quint64 id, Qt::TimeSpec spec = Qt::UTC);
    QDateTime sampleModificationDateTime(quint64 id, Qt::TimeSpec spec = Qt::UTC);
    TAccessLevel userAccessLevel(quint64 userId);
    quint64 inviteId(const QString &inviteCode);
    bool isValid() const;
private:
    bool saveUserAvatar(quint64 userId, const QByteArray &data) const;
    QByteArray loadUserAvatar(quint64 userId, bool *ok = 0) const;
private:
    const QString RootDir;
private:
    static QMutex mglobalMutex;
    static QString mtexsampleSty;
    static QString mtexsampleTex;
private:
    Database *mdb;
    QMutex mmutex;
};

#endif // STORAGE_H
