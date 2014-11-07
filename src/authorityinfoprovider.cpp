#include "authorityinfoprovider.h"

#include "authorityinfo.h"

#include <TOperation>

#include <BDirTools>
#include <BeQt>
#include <BProperties>

#include <QDebug>
#include <QList>
#include <QMap>
#include <QRegExp>
#include <QString>
#include <QStringList>

/*============================================================================
================================ AuthorityInfoProvider =======================
============================================================================*/

/*============================== Static private variables ==================*/

QMap<QString, AuthorityInfoProvider::OperationType> AuthorityInfoProvider::moperationTypeMap;
QMap<AuthorityInfoProvider::OperationType, QString> AuthorityInfoProvider::moperationVariableNameMap;
QMap<QString, AuthorityInfoProvider::OperationType> AuthorityInfoProvider::mtexsampleOperationTypeMap;

/*============================== Public constructors =======================*/

AuthorityInfoProvider::AuthorityInfoProvider()
{
    //
}

AuthorityInfoProvider::~AuthorityInfoProvider()
{
    //
}

/*============================== Static public methods =====================*/

QString AuthorityInfoProvider::operationVariableName(OperationType type)
{
    initializeOperationMaps();
    return moperationVariableNameMap.value(type);
}

AuthorityInfoProvider::OperationType AuthorityInfoProvider::operationType(const QString &variableName)
{
    initializeOperationMaps();
    return moperationTypeMap.value(variableName);
}

AuthorityInfoProvider::OperationType AuthorityInfoProvider::texsampleOperationType(const QString &texsampleId)
{
    initializeOperationMaps();
    return mtexsampleOperationTypeMap.value(texsampleId);
}

/*============================== Public methods ============================*/

QList<AuthorityInfo> AuthorityInfoProvider::authorityInfoList(OperationType type) const
{
    static const QList<AuthorityInfo> Default;
    if (mauthListMap.contains(type))
        return mauthListMap.value(type);
    else
        return Default;
}

void AuthorityInfoProvider::clear()
{
    mauthListMap.clear();
}

bool AuthorityInfoProvider::loadFromFile(const QString &fileName, QTextCodec *codec)
{
    clear();
    bool ok = false;
    QString s = BDirTools::readTextFile(fileName, codec, &ok);
    if (!ok)
        return false;
    return loadFromString(s);
}

bool AuthorityInfoProvider::loadFromFile(const QString &fileName, const QString &codecName)
{
    clear();
    bool ok = false;
    QString s = BDirTools::readTextFile(fileName, codecName, &ok);
    if (!ok)
        return false;
    return loadFromString(s);
}

bool AuthorityInfoProvider::loadFromProperties(const BProperties &p)
{
    clear();
    foreach (const QString key, p.keys()) {
        QList<AuthorityInfo> list;
        OperationType type = operationType(key);
        if (NoOperation == type)
            continue;
        QString value = p.value(key);
        bool ok = false;
        list = parce(value, &ok);
        if (!ok) {
            clear();
            qDebug() << "pss" << value;
            return false;
        }
        mauthListMap.insert(type, list);
    }
    return true;
}

bool AuthorityInfoProvider::loadFromString(const QString &s)
{
    clear();
    bool ok = false;
    BProperties p = BeQt::propertiesFromString(s, &ok);
    if (!ok)
        return false;
    loadFromProperties(p);
    return true;
}

/*============================== Static prviate methods ====================*/

void AuthorityInfoProvider::initializeOperationMaps()
{
    do_once(initialize) {
        insertOperationPair(AddGroup, "addGroup", TOperation::AddGroup);
        insertOperationPair(AddLab, "addLab", TOperation::AddLab);
        insertOperationPair(AddSample, "addSample", TOperation::AddSample);
        insertOperationPair(AddUser, "addUser", TOperation::AddUser);
        insertOperationPair(Authorize, "authorize", TOperation::Authorize);
        insertOperationPair(ChangeEmail, "changeEmail", TOperation::ChangeEmail);
        insertOperationPair(ChangePassword, "changePassword", TOperation::ChangePassword);
        insertOperationPair(CheckEmailAvailability, "checkEmailAvailability", TOperation::CheckEmailAvailability);
        insertOperationPair(CheckLoginAvailability, "checkLoginAvailability", TOperation::CheckLoginAvailability);
        insertOperationPair(CompileTexProject, "compileTexProject", TOperation::CompileTexProject);
        insertOperationPair(ConfirmEmailChange, "confirmEmailChange", TOperation::ConfirmEmailChange);
        insertOperationPair(ConfirmRegistration, "confirmRegistration", TOperation::ConfirmRegistration);
        insertOperationPair(DeleteGroup, "deleteGroup", TOperation::DeleteGroup);
        insertOperationPair(DeleteInvites, "deleteInvites", TOperation::DeleteInvites);
        insertOperationPair(DeleteLab, "deleteLab", TOperation::DeleteLab);
        insertOperationPair(DeleteSample, "deleteSample", TOperation::DeleteSample);
        insertOperationPair(DeleteUser, "deleteUser", TOperation::DeleteUser);
        insertOperationPair(EditGroup, "editGroup", TOperation::EditGroup);
        insertOperationPair(EditLab, "editLab", TOperation::EditLab);
        insertOperationPair(EditSample, "editSample", TOperation::EditSample);
        insertOperationPair(EditSampleAdmin, "editSampleAdmin", TOperation::EditSampleAdmin);
        insertOperationPair(EditSelf, "editSelf", TOperation::EditSelf);
        insertOperationPair(EditUser, "editUser", TOperation::EditUser);
        insertOperationPair(GenerateInvites, "generateInvites", TOperation::GenerateInvites);
        insertOperationPair(GetGroupInfoList, "getGroupInfoList", TOperation::GetGroupInfoList);
        insertOperationPair(GetInviteInfoList, "getInviteInfoList", TOperation::GetInviteInfoList);
        insertOperationPair(GetLabData, "getLabData", TOperation::GetLabData);
        insertOperationPair(GetLabExtraFile, "getLabExtraFile", TOperation::GetLabExtraFile);
        insertOperationPair(GetLabInfoList, "getLabInfoList", TOperation::GetLabInfoList);
        insertOperationPair(GetLatestAppVersion, "getLatestAppVersion", TOperation::GetLatestAppVersion);
        insertOperationPair(GetSampleInfoList, "getSampleInfoList", TOperation::GetSampleInfoList);
        insertOperationPair(GetSamplePreview, "getSamplePreview", TOperation::GetSamplePreview);
        insertOperationPair(GetSampleSource, "getSampleSource", TOperation::GetSampleSource);
        insertOperationPair(GetSelfInfo, "getSelfInfo", TOperation::GetSelfInfo);
        insertOperationPair(GetServerState, "getServerState", TOperation::GetServerState);
        insertOperationPair(GetUserConnectionInfoList, "getUserConnectionInfoList", TOperation::GetUserConnectionInfoList);
        insertOperationPair(GetUserInfo, "getUserInfo", TOperation::GetUserInfo);
        insertOperationPair(GetUserInfoAdmin, "getUserInfoAdmin", TOperation::GetUserInfoAdmin);
        insertOperationPair(GetUserInfoListAdmin, "getUserInfoListAdmin", TOperation::GetUserInfoListAdmin);
        insertOperationPair(RecoverAccount, "recoverAccount", TOperation::RecoverAccount);
        insertOperationPair(Register, "register", TOperation::Register);
        insertOperationPair(RequestRecoveryCode, "requestRecoveryCode", TOperation::RequestRecoveryCode);
        insertOperationPair(SetLatestAppVersion, "setLatestAppVersion", TOperation::SetLatestAppVersion);
        insertOperationPair(SetServerState, "setServerState", TOperation::SetServerState);
        insertOperationPair(Subscribe, "subscribe", TOperation::Subscribe);
    }
}

void AuthorityInfoProvider::insertOperationPair(OperationType type, const QString &variableName,
                                                const QString &texsampleId)
{
    moperationTypeMap.insert(variableName, type);
    moperationVariableNameMap.insert(type, variableName);
    mtexsampleOperationTypeMap.insert(texsampleId, type);
}

/*============================== Private methods ===========================*/

QList<AuthorityInfo> AuthorityInfoProvider::parce(const QString &value, bool *ok) const
{
    QList<AuthorityInfo> list;
    QStringList any = value.split(QRegExp("\\s*\\|\\s*"));
    any.removeDuplicates();
    if (any.contains(""))
        return bRet(ok, false, list);
    foreach (const QString &conditions, any) {
        QStringList all = conditions.split(QRegExp("\\s*\\,\\s*"));
        all.removeDuplicates();
        if (all.contains("") || all.isEmpty())
            return bRet(ok, false, list);
        AuthorityInfo info;
        foreach (const QString &condition, all) {
            if ("no" == condition) {
                info = AuthorityInfo();
                break;
            } else if ("any" == condition) {
                info = AuthorityInfo();
                info.basicState = AuthorityInfo::Any;
                break;
            } else if ("authorized" == condition) {
                info.basicState = AuthorityInfo::Authorized;
            } else if ("active" == condition) {
                info.basicState = AuthorityInfo::Active;
            } else if ("cloudlab" == condition) {
                info.basicState = AuthorityInfo::Active;
                info.services |= AuthorityInfo::CloudlabService;
            } else if ("texsample" == condition) {
                info.basicState = AuthorityInfo::Active;
                info.services |= AuthorityInfo::TexsampleService;
            } else if ("user" == condition) {
                info.basicState = AuthorityInfo::Active;
                if (info.accessLevel < AuthorityInfo::UserLevel)
                    info.accessLevel = AuthorityInfo::UserLevel;
            } else if ("moder" == condition) {
                info.basicState = AuthorityInfo::Active;
                if (info.accessLevel < AuthorityInfo::ModerLevel)
                    info.accessLevel = AuthorityInfo::ModerLevel;
            } else if ("admin" == condition) {
                info.basicState = AuthorityInfo::Active;
                if (info.accessLevel < AuthorityInfo::AdminLevel)
                    info.accessLevel = AuthorityInfo::AdminLevel;
            } else if ("super" == condition) {
                info.basicState = AuthorityInfo::Active;
                info.accessLevel = AuthorityInfo::SuperLevel;
            } else if ("self" == condition) {
                info.basicState = AuthorityInfo::Active;
                info.self = true;
            } else if ("groups" == condition) {
                info.basicState = AuthorityInfo::Active;
                info.groupsMatch = true;
            } else if ("selfGroups" == condition) {
                info.basicState = AuthorityInfo::Active;
                info.selfGroupsMatch = true;
            } else if ("services" == condition) {
                info.basicState = AuthorityInfo::Active;
                info.servicesMach = true;
            } else if ("unverified" == condition) {
                info.basicState = AuthorityInfo::Active;
                info.sampleTypes |= AuthorityInfo::Unverified;
            } else if ("accepted" == condition) {
                info.basicState = AuthorityInfo::Active;
                info.sampleTypes |= AuthorityInfo::Approved;
            } else if ("rejected" == condition) {
                info.basicState = AuthorityInfo::Active;
                info.sampleTypes |= AuthorityInfo::Rejected;
            } else if ("-unverified" == condition) {
                info.basicState = AuthorityInfo::Active;
                info.sampleTypes &= ~AuthorityInfo::Unverified;
            } else if ("-accepted" == condition) {
                info.basicState = AuthorityInfo::Active;
                info.sampleTypes &= ~AuthorityInfo::Approved;
            } else if ("-rejected" == condition) {
                info.basicState = AuthorityInfo::Active;
                info.sampleTypes &= ~AuthorityInfo::Rejected;
            } else if (QRegExp("level\\-(lt|leq)(\\-user|moder|admin|super)?").exactMatch(condition)) {
                info.basicState = AuthorityInfo::Active;
                int inflen = 2;
                if (condition.mid(6, 2) == "lt") {
                    info.targetAccessLevelMode = AuthorityInfo::LessMode;
                    inflen = 2;
                } else {
                    info.targetAccessLevelMode = AuthorityInfo::LessOrEqualMode;
                    inflen = 3;
                }
                if (condition.length() > 6 + inflen) {
                    QString slvl = condition.mid(6 + inflen);
                    if ("user" == slvl && info.accessLevel < AuthorityInfo::UserLevel)
                        info.targetAccessLevel = AuthorityInfo::UserLevel;
                    else if ("moder" == slvl && info.accessLevel < AuthorityInfo::ModerLevel)
                        info.targetAccessLevel = AuthorityInfo::ModerLevel;
                    else if ("admin" == slvl && info.accessLevel < AuthorityInfo::AdminLevel)
                        info.targetAccessLevel = AuthorityInfo::AdminLevel;
                    else
                        info.targetAccessLevel = AuthorityInfo::SuperLevel;
                }
            } else {
                return bRet(ok, false, QList<AuthorityInfo>());
            }
        }
        list << info;
    }
    return bRet(ok, !list.isEmpty(), list);
}
