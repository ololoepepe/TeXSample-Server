#ifndef STORAGE_H
#define STORAGE_H

class Translator;

class TUserInfo;
class TOperationResult;
class TAccessLevel;
class TCompilationResult;
class TProject;
class TProjectFile;
class TMessage;

class BSqlDatabase;

class QDateTime;

#include <TeXSample>
#include <TSampleInfo>
#include <TInviteInfo>
#include <TSampleInfoList>
#include <TInviteInfoList>
#include <TIdList>

#include <QString>
#include <QCoreApplication>
#include <QByteArray>
#include <QUuid>

/*============================================================================
================================ Storage =====================================
============================================================================*/

class Storage
{
    Q_DECLARE_TR_FUNCTIONS(Storage)
public:
    static bool initStorage(TMessage *msg = 0);
    static bool copyTexsample(const QString &path, const QString &codecName = "UTF-8");
    static bool removeTexsample(const QString &path);
public:
    explicit Storage(Translator *t = 0);
    ~Storage();
public:
    void setTranslator(Translator *t);
    TOperationResult addUser(const TUserInfo &info, Translator *t = 0, const QUuid &invite = QUuid());
    TOperationResult editUser(const TUserInfo &info);
    TOperationResult getUserInfo(quint64 userId, TUserInfo &info, QDateTime &updateDT, bool &cacheOk);
    TOperationResult getShortUserInfo(quint64 userId, TUserInfo &info);
    TCompilationResult addSample(quint64 userId, TProject project, const TSampleInfo &info);
    TCompilationResult editSample(const TSampleInfo &info, TProject project);
    TOperationResult deleteSample(quint64 sampleId, const QString &reason);
    TOperationResult getSampleSource(quint64 sampleId, TProject &project, QDateTime &updateDT, bool &cacheOk);
    TOperationResult getSamplePreview(quint64 sampleId, TProjectFile &file, QDateTime &updateDT, bool &cacheOk);
    TOperationResult getSamplesList(TSampleInfoList &newSamples, TIdList &deletedSamples, QDateTime &updateDT);
    TOperationResult generateInvites(quint64 userId, const QDateTime &expiresDT, quint8 count,
                                     TInviteInfoList &invites);
    TOperationResult getInvitesList(quint64 userId, TInviteInfoList &invites);
    TOperationResult getRecoveryCode(const QString &email, const Translator &t);
    TOperationResult recoverAccount(const QString &email, const QUuid &code, const QByteArray &password,
                                    const Translator &t);
    bool isUserUnique(const QString &login, const QString &email);
    quint64 userId(const QString &login, const QByteArray &password = QByteArray());
    quint64 userIdByEmail(const QString &email);
    quint64 senderId(quint64 sampleId);
    QString userLogin(quint64 userId);
    TSampleInfo::Type sampleType(quint64 sampleId);
    QString sampleFileName(quint64 sampleId);
    QDateTime sampleCreationDateTime(quint64 sampleId, Qt::TimeSpec spec = Qt::UTC);
    QDateTime sampleModificationDateTime(quint64 sampleId, Qt::TimeSpec spec = Qt::UTC);
    TAccessLevel userAccessLevel(quint64 userId);
    quint64 inviteId(const QUuid &invite);
    quint64 inviteId(const QString &inviteCode);
    bool isValid() const;
    bool testInvites();
    bool testRecoveryCodes();
private:
    static QString rootDir();
private:
    bool saveUserAvatar(quint64 userId, const QByteArray &data) const;
    QByteArray loadUserAvatar(quint64 userId, bool *ok = 0) const;
private:
    static QString mtexsampleSty;
    static QString mtexsampleTex;
private:
    BSqlDatabase *mdb;
    Translator *mtranslator;
};

#endif // STORAGE_H
