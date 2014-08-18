#ifndef USERSERVICE_H
#define USERSERVICE_H

class AccountRecoveryCodeRepository;
class DataSource;
class Group;
class GroupRepository;
class InviteCodeRepository;
class RegistrationConfirmationCodeRepository;
class User;
class UserRepository;

class TGroupInfo;
class TGroupInfoList;
class TIdList;
class TUserInfo;

class QString;

#include "requestin.h"
#include "requestout.h"

#include <TAddGroupReplyData>
#include <TAddGroupRequestData>
#include <TAuthorizeReplyData>
#include <TAuthorizeRequestData>
#include <TGetUserInfoAdminReplyData>
#include <TGetUserInfoAdminRequestData>
#include <TGetUserInfoListAdminReplyData>
#include <TGetUserInfoListAdminRequestData>
#include <TGetUserInfoReplyData>
#include <TGetUserInfoRequestData>

#include <QCoreApplication>

/*============================================================================
================================ UserService =================================
============================================================================*/

class UserService
{
    Q_DECLARE_TR_FUNCTIONS(UserService)
private:
    AccountRecoveryCodeRepository * const AccountRecoveryCodeRepo;
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
    RequestOut<TAuthorizeReplyData> authorize(const RequestIn<TAuthorizeRequestData> &in);
    bool checkOutdatedEntries();
    DataSource *dataSource() const;
    RequestOut<TGetUserInfoReplyData> getUserInfo(const RequestIn<TGetUserInfoRequestData> &in);
    RequestOut<TGetUserInfoAdminReplyData> getUserInfoAdmin(const RequestIn<TGetUserInfoAdminRequestData> &in);
    RequestOut<TGetUserInfoListAdminReplyData> getUserInfoListAdmin(const RequestIn<TGetUserInfoListAdminRequestData> &in);
    bool initializeRoot(QString *error = 0);
    bool isRootInitialized();
    bool isValid() const;
private:
    TGroupInfoList getGroups(const TIdList &ids);
    TGroupInfoList getGroups(quint64 userId);
    TGroupInfo groupToGroupInfo(const Group &entity);
    TUserInfo userToUserInfo(const User &entity, bool includeEmail, bool includeAvatar);
private:
    Q_DISABLE_COPY(UserService)
};

#endif // USERSERVICE_H
