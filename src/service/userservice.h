#ifndef USERSERVICE_H
#define USERSERVICE_H

class AccountRecoveryCodeRepository;
class DataSource;
class EmailChangeConfirmationCodeRepository;
class Group;
class GroupRepository;
class InviteCode;
class InviteCodeRepository;
class RegistrationConfirmationCodeRepository;
class User;
class UserRepository;

class TGroupInfo;
class TGroupInfoList;
class TIdList;
class TInviteInfo;
class TUserInfo;

class BUuid;

#include "application.h"
#include "requestin.h"
#include "requestout.h"
#include "translator.h"

#include <TeXSample/TeXSampleCore>

#include <BProperties>

#include <QCoreApplication>
#include <QLocale>
#include <QMap>
#include <QString>

/*============================================================================
================================ UserService =================================
============================================================================*/

class UserService
{
    Q_DECLARE_TR_FUNCTIONS(UserService)
private:
    AccountRecoveryCodeRepository * const AccountRecoveryCodeRepo;
    EmailChangeConfirmationCodeRepository * const EmailChangeConfirmationCodeRepo;
    GroupRepository * const GroupRepo;
    InviteCodeRepository * const InviteCodeRepo;
    RegistrationConfirmationCodeRepository * const RegistrationConfirmationCodeRepo;
    DataSource * const Source;
    UserRepository * const UserRepo;
public:
    explicit UserService(DataSource *source);
    ~UserService();
public:
    RequestOut<TAddGroupReplyData> addGroup(const RequestIn<TAddGroupRequestData> &in, quint64 userId);
    RequestOut<TAddUserReplyData> addUser(const RequestIn<TAddUserRequestData> &in);
    RequestOut<TAuthorizeReplyData> authorize(const RequestIn<TAuthorizeRequestData> &in);
    RequestOut<TChangeEmailReplyData> changeEmail(const RequestIn<TChangeEmailRequestData> &in, quint64 userId);
    RequestOut<TChangePasswordReplyData> changePassword(const RequestIn<TChangePasswordRequestData> &in,
                                                        quint64 userId);
    bool checkOutdatedEntries();
    RequestOut<TConfirmEmailChangeReplyData> confirmEmailChange(const RequestIn<TConfirmEmailChangeRequestData> &in);
    RequestOut<TConfirmRegistrationReplyData> confirmRegistration(
            const RequestIn<TConfirmRegistrationRequestData> &in);
    DataSource *dataSource() const;
    RequestOut<TGenerateInvitesReplyData> generateInvites(const RequestIn<TGenerateInvitesRequestData> &in,
                                                          quint64 userId);
    RequestOut<TGetGroupInfoListReplyData> getGroupInfoList(const RequestIn<TGetGroupInfoListRequestData> &in,
                                                            quint64 userId);
    RequestOut<TGetInviteInfoListReplyData> getInviteInfoList(const RequestIn<TGetInviteInfoListRequestData> &in,
                                                              quint64 userId);
    RequestOut<TGetSelfInfoReplyData> getSelfInfo(const RequestIn<TGetSelfInfoRequestData> &in, quint64 userId);
    RequestOut<TGetUserInfoReplyData> getUserInfo(const RequestIn<TGetUserInfoRequestData> &in);
    RequestOut<TGetUserInfoAdminReplyData> getUserInfoAdmin(const RequestIn<TGetUserInfoAdminRequestData> &in);
    RequestOut<TGetUserInfoListAdminReplyData> getUserInfoListAdmin(
            const RequestIn<TGetUserInfoListAdminRequestData> &in);
    bool initializeRoot(QString *error = 0);
    bool isRootInitialized();
    bool isValid() const;
    RequestOut<TRegisterReplyData> registerUser(const RequestIn<TRegisterRequestData> &in);
private:
    static bool sendEmail(const QString &receiver, const QString &templateName, const QLocale &locale,
                          const BProperties &replace = BProperties());
private:
    template <typename T> bool commonCheck(const Translator &translator, const T &data, QString *error) const
    {
        if (!commonCheck(translator, error))
            return false;
        if (!data.isValid())
            return bRet(error, translator.translate("UserService", "Invalid data", "error"), false);
        return bRet(error, QString(), true);
    }
private:
    bool addUser(const User &entity, User &newEntity, const QLocale &locale = Application::locale(),
                 QString *error = 0);
    bool checkUserId(const Translator &t, quint64 userId, QString *error);
    bool commonCheck(const Translator &translator, QString *error) const;
    bool confirmRegistration(const BUuid &code, const QLocale &locale = Application::locale(), QString *error = 0);
    TGroupInfoList getAllGroups();
    TGroupInfoList getGroups(const TIdList &ids);
    TGroupInfoList getGroups(quint64 userId);
    TGroupInfo groupToGroupInfo(const Group &entity);
    TInviteInfo inviteCodeToInviteInfo(const InviteCode &entity);
    TUserInfo userToUserInfo(const User &entity, bool includeEmail, bool includeAvatar);
private:
    Q_DISABLE_COPY(UserService)
};

#endif // USERSERVICE_H
