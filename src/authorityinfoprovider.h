#ifndef AUTHORITYINFOPROVIDER_H
#define AUTHORITYINFOPROVIDER_H

class BProperties;

class QString;
class QTextCodec;

#include "authorityinfo.h"

#include <QList>
#include <QMap>

/*============================================================================
================================ AuthorityInfoProvider =======================
============================================================================*/

class AuthorityInfoProvider
{
public:
    enum OperationType
    {
        NoOperation = 0,
        AddGroup,
        AddLab,
        AddSample,
        AddUser,
        Authorize,
        ChangeEmail,
        ChangePassword,
        CheckEmailAvailability,
        CheckLoginAvailability,
        CompileTexProject,
        ConfirmEmailChange,
        ConfirmRegistration,
        DeleteGroup,
        DeleteInvites,
        DeleteLab,
        DeleteSample,
        DeleteUser,
        EditGroup,
        EditLab,
        EditSample,
        EditSampleAdmin,
        EditSelf,
        EditUser,
        GenerateInvites,
        GetGroupInfoList,
        GetInviteInfoList,
        GetLabData,
        GetLabExtraFile,
        GetLabInfoList,
        GetLatestAppVersion,
        GetSampleInfoList,
        GetSamplePreview,
        GetSampleSource,
        GetSelfInfo,
        GetServerState,
        GetUserConnectionInfoList,
        GetUserInfo,
        GetUserInfoAdmin,
        GetUserInfoListAdmin,
        RecoverAccount,
        Register,
        RequestRecoveryCode,
        SetLatestAppVersion,
        SetServerState,
        Subscribe
    };
private:
    static QMap<QString, OperationType> moperationTypeMap;
    static QMap<OperationType, QString> moperationVariableNameMap;
    static QMap<QString, OperationType> mtexsampleOperationTypeMap;
private:
    QMap< OperationType, QList<AuthorityInfo> > mauthListMap;
public:
    explicit AuthorityInfoProvider();
    ~AuthorityInfoProvider();
public:
    static QString operationVariableName(OperationType type);
    static OperationType operationType(const QString &variableName);
    static OperationType texsampleOperationType(const QString &texsampleId);
private:
    static inline void initializeOperationMaps();
    static void insertOperationPair(OperationType type, const QString &variableName, const QString &texsampleId);
public:
    QList<AuthorityInfo> authorityInfoList(OperationType type) const;
    void clear();
    bool loadFromFile(const QString &fileName, QTextCodec *codec = 0);
    bool loadFromFile(const QString &fileName, const QString &codecName);
    bool loadFromProperties(const BProperties &p);
    bool loadFromString(const QString &s);
private:
    QList<AuthorityInfo> parce(const QString &value, bool *ok = 0) const;
};

#endif // AUTHORITYINFOPROVIDER_H
