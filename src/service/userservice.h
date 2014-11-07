/****************************************************************************
**
** Copyright (C) 2012-2014 Andrey Bogdanov
**
** This file is part of TeXSample Server.
**
** TeXSample Server is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** TeXSample Server is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with TeXSample Server.  If not, see <http://www.gnu.org/licenses/>.
**
****************************************************************************/

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
class TransactionHolder;
class User;
class UserRepository;

class TGroupInfo;
class TGroupInfoList;
class TIdList;
class TInviteInfo;
class TUserInfo;

class BUuid;

class QImage;

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
    RequestOut<TCheckEmailAvailabilityReplyData> checkEmailAvailability(
            const RequestIn<TCheckEmailAvailabilityRequestData> &in);
    RequestOut<TCheckLoginAvailabilityReplyData> checkLoginAvailability(
            const RequestIn<TCheckLoginAvailabilityRequestData> &in);
    bool checkOutdatedEntries(QString *error = 0);
    RequestOut<TConfirmEmailChangeReplyData> confirmEmailChange(const RequestIn<TConfirmEmailChangeRequestData> &in);
    RequestOut<TConfirmRegistrationReplyData> confirmRegistration(
            const RequestIn<TConfirmRegistrationRequestData> &in);
    DataSource *dataSource() const;
    RequestOut<TDeleteGroupReplyData> deleteGroup(const RequestIn<TDeleteGroupRequestData> &in, quint64 userId);
    RequestOut<TDeleteInvitesReplyData> deleteInvites(const RequestIn<TDeleteInvitesRequestData> &in, quint64 userId);
    RequestOut<TDeleteUserReplyData> deleteUser(const RequestIn<TDeleteUserRequestData> &in);
    RequestOut<TEditGroupReplyData> editGroup(const RequestIn<TEditGroupRequestData> &in, quint64 userId);
    RequestOut<TEditSelfReplyData> editSelf(const RequestIn<TEditSelfRequestData> &in, quint64 userId);
    RequestOut<TEditUserReplyData> editUser(const RequestIn<TEditUserRequestData> &in, quint64 userId);
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
    bool isRootInitialized(bool *ok = 0, QString *error = 0);
    bool isValid() const;
    RequestOut<TRecoverAccountReplyData> recoverAccount(const RequestIn<TRecoverAccountRequestData> &in);
    RequestOut<TRegisterReplyData> registerUser(const RequestIn<TRegisterRequestData> &in);
    RequestOut<TRequestRecoveryCodeReplyData> requestRecoveryCode(
            const RequestIn<TRequestRecoveryCodeRequestData> &in);
    TUserInfo userInfo(quint64 userId, bool includeEmail, bool *ok = 0);
private:
    static bool sendEmail(const QString &receiver, const QString &templateName, const QLocale &locale,
                          const BProperties &replace = BProperties());
private:
    template <typename T> bool commonCheck(const Translator &t, const T &data, QString *error) const
    {
        if (!commonCheck(t, error))
            return false;
        if (!data.isValid())
            return bRet(error, t.translate("UserService", "Invalid data", "error"), false);
        return bRet(error, QString(), true);
    }
private:
    bool addUser(const User &entity, User &newEntity, const QLocale &locale = Application::locale(),
                 QString *error = 0);
    bool checkUserAvatar(const Translator &t, const QImage &avatar, QString *error);
    bool checkUserId(const Translator &t, quint64 userId, QString *error);
    bool commit(const QLocale &locale, TransactionHolder &holder, QString *error);
    bool commit(const Translator &t, TransactionHolder &holder, QString *error);
    bool commonCheck(const Translator &t, QString *error) const;
    bool confirmRegistration(const BUuid &code, QDateTime &dt, const QLocale &locale = Application::locale(),
                             QString *error = 0);
    TGroupInfoList getAllGroups(bool *ok = 0);
    TGroupInfoList getGroups(const TIdList &ids, bool *ok = 0);
    TGroupInfoList getGroups(quint64 userId, bool *ok = 0);
    TGroupInfo groupToGroupInfo(const Group &entity, bool *ok = 0);
    TInviteInfo inviteCodeToInviteInfo(const InviteCode &entity, bool *ok = 0);
    TUserInfo userToUserInfo(const User &entity, bool includeEmail, bool *ok = 0);
private:
    Q_DISABLE_COPY(UserService)
};

#endif // USERSERVICE_H
