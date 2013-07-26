#ifndef STORAGE_H
#define STORAGE_H

class TUserInfo;
class TOperationResult;
class TAccessLevel;
class TCompilationResult;
class TTexProject;
class TProjectFile;
class TMessage;
class TService;
class TLabInfoList;
class TLabProject;

class BSqlDatabase;

class QDateTime;

#include <TeXSample>
#include <TSampleInfo>
#include <TInviteInfo>
#include <TSampleInfoList>
#include <TInviteInfoList>
#include <TIdList>
#include <TServiceList>
#include <TLabInfo>

#include <BeQt>

#include <QString>
#include <QCoreApplication>
#include <QByteArray>
#include <QUuid>
#include <QVariantMap>
#include <QStringList>

/*============================================================================
================================ Storage =====================================
============================================================================*/

class Storage
{
    Q_DECLARE_TR_FUNCTIONS(Storage)
public:
    enum State
    {
        NormalState = 0,
        DeletedState
    };
    enum Service
    {
        TexsampleService
    };
public:
    static bool initStorage(QString *errs = 0);
    static bool copyTexsample(const QString &path, const QString &codecName = "UTF-8");
    static bool removeTexsample(const QString &path);
public:
    explicit Storage();
    ~Storage();
public:
    TOperationResult addUser(const TUserInfo &info, const QLocale &locale,
                             const QStringList &clabGroups = QStringList());
    TOperationResult registerUser(TUserInfo info, const QLocale &locale);
    TOperationResult editUser(const TUserInfo &info, bool editClab = false,
                              const QStringList &clabGroups = QStringList());
    TOperationResult updateUser(const TUserInfo info);
    TOperationResult getUserInfo(quint64 userId, TUserInfo &info, QDateTime &updateDT, bool &cacheOk);
    TOperationResult getShortUserInfo(quint64 userId, TUserInfo &info);
    TCompilationResult addSample(quint64 userId, TTexProject project, const TSampleInfo &info);
    TCompilationResult editSample(const TSampleInfo &info, TTexProject project);
    TOperationResult deleteSample(quint64 sampleId, const QString &reason);
    TOperationResult getSampleSource(quint64 sampleId, TTexProject &project, QDateTime &updateDT, bool &cacheOk);
    TOperationResult getSamplePreview(quint64 sampleId, TProjectFile &file, QDateTime &updateDT, bool &cacheOk);
    TOperationResult getSamplesList(TSampleInfoList &newSamples, TIdList &deletedSamples, QDateTime &updateDT);
    TOperationResult generateInvites(quint64 userId, const QDateTime &expirationDT, quint8 count,
                                     const TServiceList &services, const QStringList &clabGroups,
                                     TInviteInfoList &invites);
    TOperationResult getInvitesList(quint64 userId, TInviteInfoList &invites);
    TOperationResult getRecoveryCode(const QString &email, const QLocale &locale);
    TOperationResult recoverAccount(const QString &email, const QString &code, const QByteArray &password,
                                    const QLocale &locale);
    TOperationResult editClabGroups(const QStringList &newGroups, const QStringList &deletedGroups);
    TOperationResult getClabGroupsList(QStringList &groups);
    TOperationResult addLab(quint64 userId, const TLabInfo &info, const TLabProject &webProject,
                            const TLabProject &linuxProject, const TLabProject &macProject,
                            const TLabProject &winProject, const QString &url);
    TOperationResult editLab(const TLabInfo &info, const TLabProject &webProject, const TLabProject &linuxProject,
                             const TLabProject &macProject, const TLabProject &winProject, const QString &url);
    TOperationResult deleteLab(quint64 labId, const QString &reason);
    TOperationResult getLabsList(quint64 userId, BeQt::OSType osType, TLabInfoList &newLabs, TIdList &deletedLabs,
                                 QDateTime &updateDT);
    TOperationResult getLab(quint64 labId, BeQt::OSType osType, TLabProject &project, QString &url);
    bool isUserUnique(const QString &login, const QString &email);
    quint64 userId(const QString &login, const QByteArray &password = QByteArray());
    quint64 userIdByEmail(const QString &email);
    quint64 sampleSenderId(quint64 sampleId);
    quint64 labSenderId(quint64 labId);
    QString userLogin(quint64 userId);
    TAccessLevel userAccessLevel(quint64 userId);
    TServiceList userServices(quint64 userId);
    TServiceList newUserServices(quint64 inviteId);
    TServiceList newUserServices(const QString &inviteCode);
    bool userHasAccessTo(quint64 userId, const TService service);
    QStringList userClabGroups(quint64 userId);
    TSampleInfo::Type sampleType(quint64 sampleId);
    QString sampleFileName(quint64 sampleId);
    State sampleState(quint64 sampleId);
    State labState(quint64 labId);
    QStringList labGroups(quint64 labId);
    TLabInfo::Type labType(quint64 labId);
    QDateTime sampleCreationDateTime(quint64 sampleId, Qt::TimeSpec spec = Qt::UTC);
    QDateTime sampleUpdateDateTime(quint64 sampleId, Qt::TimeSpec spec = Qt::UTC);
    quint64 inviteId(const QString &inviteCode);
    bool isValid() const;
    bool testInvites();
    bool testRecoveryCodes();
private:
    static QString rootDir();
    static QString labSubdir(BeQt::OSType osType);
private:
    TOperationResult addUserInternal(const TUserInfo &info, const QLocale &locale,
                                     const QStringList &clabGroups = QStringList());
    TOperationResult editUserInternal(const TUserInfo &info, bool editClab = false,
                                      const QStringList &clabGroups = QStringList());
    bool saveUserAvatar(quint64 userId, const QByteArray &data) const;
    QByteArray loadUserAvatar(quint64 userId, bool *ok = 0) const;
private:
    static QString mtexsampleSty;
    static QString mtexsampleTex;
private:
    BSqlDatabase *mdb;
};

#endif // STORAGE_H
