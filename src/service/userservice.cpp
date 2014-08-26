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

#include "userservice.h"

#include "application.h"
#include "datasource.h"
#include "entity/emailchangeconfirmationcode.h"
#include "entity/group.h"
#include "entity/invitecode.h"
#include "entity/registrationconfirmationcode.h"
#include "entity/user.h"
#include "repository/accountrecoverycoderepository.h"
#include "repository/emailchangeconfirmationcoderepository.h"
#include "repository/grouprepository.h"
#include "repository/invitecoderepository.h"
#include "repository/registrationconfirmationcoderepository.h"
#include "repository/userrepository.h"
#include "requestin.h"
#include "requestout.h"
#include "settings.h"
#include "transactionholder.h"
#include "translator.h"

#include <TeXSample/TeXSampleCore>

#include <BDirTools>
#include <BEmail>
#include <BGenericSocket>
#include <BProperties>
#include <BSmtpSender>
#include <BTerminal>
#include <BUuid>

#include <QDateTime>
#include <QDebug>
#include <QFileInfo>
#include <QImage>
#include <QLocale>
#include <QString>

/*============================================================================
================================ UserService =================================
============================================================================*/

/*============================== Public constructors =======================*/

UserService::UserService(DataSource *source) :
    AccountRecoveryCodeRepo(new AccountRecoveryCodeRepository(source)),
    EmailChangeConfirmationCodeRepo(new EmailChangeConfirmationCodeRepository(source)),
    GroupRepo(new GroupRepository(source)), InviteCodeRepo(new InviteCodeRepository(source)),
    RegistrationConfirmationCodeRepo(new RegistrationConfirmationCodeRepository(source)), Source(source),
    UserRepo(new UserRepository(source))
{
    //
}

UserService::~UserService()
{
    //
}

/*============================== Public methods ============================*/

RequestOut<TAddGroupReplyData> UserService::addGroup(const RequestIn<TAddGroupRequestData> &in, quint64 userId)
{
    typedef RequestOut<TAddGroupReplyData> Out;
    Translator t(in.locale());
    QString error;
    if (!commonCheck(t, in.data(), &error))
        return Out(error);
    if (!userId)
        return Out(t.translate("UserService", "Invalid user ID (internal)", "error"));
    Group entity;
    entity.setName(in.data().name());
    entity.setOwnerId(userId);
    TransactionHolder holder(Source);
    quint64 id = GroupRepo->add(entity);
    if (!id)
        return Out(t.translate("UserService", "Failed to add group (internal)", "error"));
    entity = GroupRepo->findOne(id);
    if (!entity.isValid())
        return Out(t.translate("UserService", "Failed to get group (internal)", "error"));
    if (!commit(t, holder, &error))
        return Out(error);
    TGroupInfo info = groupToGroupInfo(entity);
    TAddGroupReplyData replyData;
    replyData.setGroupInfo(info);
    return Out(replyData, info.creationDateTime());
}

RequestOut<TAddUserReplyData> UserService::addUser(const RequestIn<TAddUserRequestData> &in)
{
    typedef RequestOut<TAddUserReplyData> Out;
    Translator t(in.locale());
    QString error;
    if (!commonCheck(t, in.data(), &error))
        return Out(error);
    const TAddUserRequestData &data = in.data();
    User entity;
    entity.setAccessLevel(data.accessLevel());
    entity.setActive(false);
    entity.setAvailableServices(data.availableServices());
    entity.setAvatar(data.avatar());
    entity.setEmail(data.email());
    entity.setGroups(data.groups());
    entity.setLogin(data.login());
    entity.setName(data.name());
    entity.setPassword(data.password());
    entity.setPatronymic(data.patronymic());
    entity.setSurname(data.surname());
    TransactionHolder holder(Source);
    if (!addUser(entity, entity, in.locale(), &error))
        return Out(error);
    if (!commit(t, holder, &error))
        return Out(error);
    TUserInfo info = userToUserInfo(entity, true);
    TAddUserReplyData replyData;
    replyData.setUserInfo(info);
    return Out(replyData, info.registrationDateTime());
}

RequestOut<TAuthorizeReplyData> UserService::authorize(const RequestIn<TAuthorizeRequestData> &in)
{
    typedef RequestOut<TAuthorizeReplyData> Out;
    Translator t(in.locale());
    QString error;
    if (!commonCheck(t, in.data(), &error))
        return Out(error);
    QDateTime dt = QDateTime::currentDateTime();
    User entity = UserRepo->findOne(in.data().identifier(), in.data().password());
    if (!entity.isValid())
        return Out(t.translate("UserService", "Invalid login, e-mail, or password", "error"));
    if (!entity.active())
        return Out(t.translate("UserService", "Account is inactive", "error"));
    TUserInfo info = userToUserInfo(entity, true);
    TAuthorizeReplyData replyData;
    replyData.setUserInfo(info);
    return Out(replyData, dt);
}

RequestOut<TChangeEmailReplyData> UserService::changeEmail(const RequestIn<TChangeEmailRequestData> &in,
                                                           quint64 userId)
{
    typedef RequestOut<TChangeEmailReplyData> Out;
    Translator t(in.locale());
    QString error;
    if (!commonCheck(t, in.data(), &error))
        return Out(error);
    if (!checkUserId(t, userId, &error))
        return Out(error);
    QDateTime dt = QDateTime::currentDateTime();
    User entity = UserRepo->findOne(userId);
    if (!entity.isValid())
        return Out(t.translate("UserService", "No such user", "error"));
    TChangeEmailRequestData requestData = in.data();
    if (requestData.password() != entity.password())
        return Out(t.translate("UserService", "Invalid password", "error"));
    TransactionHolder holder(Source);
    EmailChangeConfirmationCode ecccEntity;
    ecccEntity.setUserId(userId);
    BUuid code = BUuid::createUuid();
    ecccEntity.setCode(code);
    ecccEntity.setEmail(requestData.email());
    ecccEntity.setExpirationDateTime(QDateTime::currentDateTimeUtc().addDays(1));
    if (!EmailChangeConfirmationCodeRepo->add(ecccEntity))
        return Out(t.translate("UserService", "Failed to add e-mail change confirmation code (internal)", "error"));
    BProperties replace;
    replace.insert("%username%", entity.login());
    replace.insert("%email%", requestData.email());
    if (!sendEmail(entity.email(), "email_change_notification", t.locale(), replace))
        return Out(t.translate("UserService", "Failed to send e-mail message", "error"));
    replace.remove("%email%");
    replace.insert("%code%", code.toString(true));
    if (!sendEmail(requestData.email(), "email_change_confirmation", t.locale(), replace))
        return Out(t.translate("UserService", "Failed to send e-mail message", "error"));
    if (!commit(t, holder, &error))
        return Out(error);
    TChangeEmailReplyData replyData;
    return Out(replyData, dt);
}

RequestOut<TChangePasswordReplyData> UserService::changePassword(const RequestIn<TChangePasswordRequestData> &in,
                                                                 quint64 userId)
{
    typedef RequestOut<TChangePasswordReplyData> Out;
    Translator t(in.locale());
    QString error;
    if (!commonCheck(t, in.data(), &error))
        return Out(error);
    if (!checkUserId(t, userId, &error))
        return Out(error);
    QDateTime dt = QDateTime::currentDateTime();
    User entity = UserRepo->findOne(userId);
    if (!entity.isValid())
        return Out(t.translate("UserService", "No such user", "error"));
    TChangePasswordRequestData requestData = in.data();
    if (requestData.oldPassword() != entity.password())
        return Out(t.translate("UserService", "Invalid password", "error"));
    TransactionHolder holder(Source);
    entity.convertToCreatedByUser();
    entity.setPassword(requestData.newPassword());
    if (!UserRepo->edit(entity))
        return Out(t.translate("UserService", "Failed to edit user (internal)", "error"));
    if (!commit(t, holder, &error))
        return Out(error);
    TChangePasswordReplyData replyData;
    return Out(replyData, dt);
}

RequestOut<TCheckEmailAvailabilityReplyData> UserService::checkEmailAvailability(
        const RequestIn<TCheckEmailAvailabilityRequestData> &in)
{
    typedef RequestOut<TCheckEmailAvailabilityReplyData> Out;
    Translator t(in.locale());
    QString error;
    if (!commonCheck(t, in.data(), &error))
        return Out(error);
    QDateTime dt = QDateTime::currentDateTimeUtc();
    QString email = in.data().email();
    TCheckEmailAvailabilityReplyData replyData;
    replyData.setAvailable(!UserRepo->emailOccupied(email) && !EmailChangeConfirmationCodeRepo->emailOccupied(email));
    return Out(replyData, dt);
}

RequestOut<TCheckLoginAvailabilityReplyData> UserService::checkLoginAvailability(
        const RequestIn<TCheckLoginAvailabilityRequestData> &in)
{
    typedef RequestOut<TCheckLoginAvailabilityReplyData> Out;
    Translator t(in.locale());
    QString error;
    if (!commonCheck(t, in.data(), &error))
        return Out(error);
    QDateTime dt = QDateTime::currentDateTimeUtc();
    TCheckLoginAvailabilityReplyData replyData;
    replyData.setAvailable(!UserRepo->loginOccupied(in.data().login()));
    return Out(replyData, dt);
}

bool UserService::checkOutdatedEntries()
{
    if (!isValid())
        return false;
    TransactionHolder holder(Source);
    bool ok = false;
    AccountRecoveryCodeRepo->deleteExpired(&ok);
    if (!ok || !InviteCodeRepo->deleteExpired())
        return false;
    foreach (const RegistrationConfirmationCode &entity, RegistrationConfirmationCodeRepo->findExpired()) {
        if (!UserRepo->deleteOne(entity.userId()))
            return false;
    }
    if (!RegistrationConfirmationCodeRepo->deleteExpired() || !EmailChangeConfirmationCodeRepo->deleteExpired())
        return false;
    return holder.doCommit();
}

RequestOut<TConfirmEmailChangeReplyData> UserService::confirmEmailChange(
        const RequestIn<TConfirmEmailChangeRequestData> &in)
{
    typedef RequestOut<TConfirmEmailChangeReplyData> Out;
    Translator t(in.locale());
    QString error;
    if (!commonCheck(t, in.data(), &error))
        return Out(error);
    QDateTime dt = QDateTime::currentDateTime();
    TConfirmEmailChangeReplyData replyData;
    replyData.setSuccess(false);
    BUuid code = in.data().confirmationCode();
    EmailChangeConfirmationCode ecccEntity = EmailChangeConfirmationCodeRepo->findOneByCode(code);
    if (!ecccEntity.isValid())
        return Out(t.translate("UserService", "No such code", "error"));
    User entity = UserRepo->findOne(ecccEntity.userId());
    if (!entity.isValid())
        return Out(t.translate("UserService", "Failed to get user (internal)", "error"));
    entity.convertToCreatedByUser();
    entity.setEmail(ecccEntity.email());
    TransactionHolder holder(Source);
    if (!UserRepo->edit(entity))
        return Out(t.translate("UserService", "Failed to edit user (internal)", "error"));
    if (!EmailChangeConfirmationCodeRepo->deleteOneByUserId(ecccEntity.userId()))
        return Out(t.translate("UserService", "Failed to delete email change confirmation code (internal)", "error"));
    if (!commit(t, holder, &error))
        return Out(error);
    replyData.setSuccess(true);
    return Out(replyData, dt);
}

RequestOut<TConfirmRegistrationReplyData>  UserService::confirmRegistration(
        const RequestIn<TConfirmRegistrationRequestData> &in)
{
    typedef RequestOut<TConfirmRegistrationReplyData> Out;
    Translator t(in.locale());
    QString error;
    if (!commonCheck(t, in.data(), &error))
        return Out(error);
    QDateTime dt = QDateTime::currentDateTime();
    TConfirmRegistrationReplyData replyData;
    replyData.setSuccess(false);
    TransactionHolder holder(Source);
    if (!confirmRegistration(in.data().confirmationCode(), in.locale(), &error))
        return Out(error);
    if (!commit(t, holder, &error))
        return Out(error);
    replyData.setSuccess(true);
    return Out(replyData, dt);
}

DataSource *UserService::dataSource() const
{
    return Source;
}

RequestOut<TDeleteGroupReplyData> UserService::deleteGroup(const RequestIn<TDeleteGroupRequestData> &in,
                                                           quint64 userId)
{
    typedef RequestOut<TDeleteGroupReplyData> Out;
    Translator t(in.locale());
    QString error;
    if (!commonCheck(t, in.data(), &error))
        return Out(error);
    if (!checkUserId(t, userId, &error))
        return Out(error);
    if (UserRepo->findAccessLevel(userId).level() < TAccessLevel::SuperuserLevel) {
        Group entity = GroupRepo->findOne(in.data().id());
        if (!entity.isValid())
            return Out(t.translate("UserService", "Failed to get group (internal)", "error"));
        if (entity.ownerId() != userId)
            return Out(t.translate("UserService", "Unable to delete group owned by another user", "error"));
    }
    QDateTime dt = QDateTime::currentDateTime();
    TransactionHolder holder(Source);
    if (!GroupRepo->deleteOne(in.data().id()))
        return Out(t.translate("UserService", "Failed to delete group (internal)", "error"));
    if (!commit(t, holder, &error))
        return Out(error);
    TDeleteGroupReplyData replyData;
    return Out(replyData, dt);
}

RequestOut<TDeleteInvitesReplyData> UserService::deleteInvites(const RequestIn<TDeleteInvitesRequestData> &in,
                                                               quint64 userId)
{
    typedef RequestOut<TDeleteInvitesReplyData> Out;
    Translator t(in.locale());
    QString error;
    if (!commonCheck(t, in.data(), &error))
        return Out(error);
    if (!checkUserId(t, userId, &error))
        return Out(error);
    TIdList ids;
    bool superuser = (UserRepo->findAccessLevel(userId).level() < TAccessLevel::SuperuserLevel);
    foreach (quint64 id, in.data().ids()) {
        InviteCode entity = InviteCodeRepo->findOne(id);
        if (!entity.isValid())
            continue;
        if (!superuser && entity.ownerId() != userId)
            continue;
        ids << id;
    }
    QDateTime dt = QDateTime::currentDateTime();
    TransactionHolder holder(Source);
    if (!InviteCodeRepo->deleteSome(ids))
        return Out(t.translate("UserService", "Failed to delete invite(s) (internal)", "error"));
    if (!commit(t, holder, &error))
        return Out(error);
    TDeleteInvitesReplyData replyData;
    replyData.setIdentifiers(ids);
    return Out(replyData, dt);
}

RequestOut<TEditGroupReplyData> UserService::editGroup(const RequestIn<TEditGroupRequestData> &in, quint64 userId)
{
    typedef RequestOut<TEditGroupReplyData> Out;
    Translator t(in.locale());
    QString error;
    if (!commonCheck(t, in.data(), &error))
        return Out(error);
    if (!checkUserId(t, userId, &error))
        return Out(error);
    Group entity = GroupRepo->findOne(in.data().id());
    if (!entity.isValid())
        return Out(t.translate("UserService", "No such group", "error"));
    if (UserRepo->findAccessLevel(userId).level() < TAccessLevel::SuperuserLevel) {
        if (entity.ownerId() != userId)
            return Out(t.translate("UserService", "Unable to edit group owned by another user", "error"));
    }
    QDateTime dt = QDateTime::currentDateTime();
    TEditGroupRequestData requestData = in.data();
    entity.convertToCreatedByUser();
    entity.setName(requestData.name());
    TransactionHolder holder(Source);
    if (!GroupRepo->edit(entity))
        return Out(t.translate("UserService", "Failed to edit group (internal)", "error"));
    entity = GroupRepo->findOne(in.data().id());
    if (!entity.isValid())
        return Out(t.translate("UserService", "Failed to get group (internal)", "error"));
    if (!commit(t, holder, &error))
        return Out(error);
    TEditGroupReplyData replyData;
    replyData.setGroupInfo(groupToGroupInfo(entity));
    return Out(replyData, dt);
}

RequestOut<TEditSelfReplyData> UserService::editSelf(const RequestIn<TEditSelfRequestData> &in, quint64 userId)
{
    typedef RequestOut<TEditSelfReplyData> Out;
    Translator t(in.locale());
    QString error;
    if (!commonCheck(t, in.data(), &error))
        return Out(error);
    if (!checkUserId(t, userId, &error))
        return Out(error);
    User entity = UserRepo->findOne(userId);
    if (!entity.isValid())
        return Out(t.translate("UserService", "No such user", "error"));
    TEditSelfRequestData requestData = in.data();
    entity.convertToCreatedByUser();
    entity.setAvatar(requestData.avatar());
    entity.setName(requestData.name());
    entity.setPatronymic(requestData.patronymic());
    entity.setSurname(requestData.surname());
    TransactionHolder holder(Source);
    if (!UserRepo->edit(entity))
        return Out(t.translate("UserService", "Failed to edit user (internal)", "error"));
    entity = UserRepo->findOne(userId);
    if (!entity.isValid())
        return Out(t.translate("UserService", "Failed to get user (internal)", "error"));
    if (!commit(t, holder, &error))
        return Out(error);
    TEditSelfReplyData replyData;
    replyData.setUserInfo(userToUserInfo(entity, true));
    return Out(replyData, entity.lastModificationDateTime());

}

RequestOut<TEditUserReplyData> UserService::editUser(const RequestIn<TEditUserRequestData> &in, quint64 userId)
{
    typedef RequestOut<TEditUserReplyData> Out;
    Translator t(in.locale());
    QString error;
    if (!commonCheck(t, in.data(), &error))
        return Out(error);
    if (!checkUserId(t, userId, &error))
        return Out(error);
    TEditUserRequestData requestData = in.data();
    User entity = UserRepo->findOne(requestData.identifier());
    if (!entity.isValid())
        return Out(t.translate("UserService", "No such user", "error"));
    if (UserRepo->findAccessLevel(userId).level() < TAccessLevel::SuperuserLevel) {
        if (entity.id() == userId)
            return Out(t.translate("UserService", "Unable to edit self account", "error"));
        if (entity.accessLevel().level() >= TAccessLevel::AdminLevel)
            return Out(t.translate("UserService", "Not enough rights to edit user", "error"));
    }
    entity.convertToCreatedByUser();
    entity.setAccessLevel(requestData.accessLevel());
    entity.setActive(requestData.active());
    entity.setAvailableServices(requestData.availableServices());
    entity.setAvatar(requestData.avatar());
    if (requestData.editEmail())
        entity.setEmail(requestData.email());
    if (requestData.editPassword())
        entity.setPassword(requestData.password());
    entity.setGroups(requestData.groups());
    entity.setName(requestData.name());
    entity.setPatronymic(requestData.patronymic());
    entity.setSurname(requestData.surname());
    TransactionHolder holder(Source);
    if (!UserRepo->edit(entity))
        return Out(t.translate("UserService", "Failed to edit user (internal)", "error"));
    entity = UserRepo->findOne(userId);
    if (!entity.isValid())
        return Out(t.translate("UserService", "Failed to get user (internal)", "error"));
    if (!commit(t, holder, &error))
        return Out(error);
    TEditUserReplyData replyData;
    replyData.setUserInfo(userToUserInfo(entity, true));
    return Out(replyData, entity.lastModificationDateTime());
}

RequestOut<TGenerateInvitesReplyData> UserService::generateInvites(const RequestIn<TGenerateInvitesRequestData> &in,
                                                                   quint64 userId)
{
    typedef RequestOut<TGenerateInvitesReplyData> Out;
    Translator t(in.locale());
    QString error;
    TGenerateInvitesRequestData requestData = in.data();
    if (!commonCheck(t, requestData, &error))
        return Out(error);
    if (!userId)
        return Out(t.translate("UserService", "Invalid user ID (internal)", "error"));
    QDateTime dt = QDateTime::currentDateTime();
    TransactionHolder holder(Source);
    InviteCode entity;
    entity.setAccessLevel(requestData.accessLevel());
    entity.setAvailableServices(requestData.services());
    entity.setExpirationDateTime(requestData.expirationDateTime());
    entity.setGroups(requestData.groups());
    entity.setOwnerId(userId);
    QList<InviteCode> entityList;
    for (quint16 i = 0; i < requestData.count(); ++i) {
        entity.setCode(BUuid::createUuid());
        quint64 id = InviteCodeRepo->add(entity);
        if (!id)
            return Out(t.translate("UserService", "Failed to add invite code (internal)", "error"));
        InviteCode newEntity = InviteCodeRepo->findOne(id);
        if (!newEntity.isValid())
            return Out(t.translate("UserService", "Failed to get invite code (internal)", "error"));
        entityList << newEntity;
    }
    if (!commit(t, holder, &error))
        return Out(error);
    TGenerateInvitesReplyData replyData;
    TInviteInfoList infoList;
    foreach (const InviteCode &e, entityList)
        infoList << inviteCodeToInviteInfo(e);
    replyData.setGeneratedInvites(infoList);
    return Out(replyData, dt);
}

RequestOut<TGetGroupInfoListReplyData> UserService::getGroupInfoList(const RequestIn<TGetGroupInfoListRequestData> &in,
                                                                     quint64 userId)
{
    typedef RequestOut<TGetGroupInfoListReplyData> Out;
    Translator t(in.locale());
    QString error;
    if (!commonCheck(t, &error))
        return Out(error);
    if (!userId)
        return Out(t.translate("UserService", "Invalid user ID (internal)", "error"));
    QDateTime dt = QDateTime::currentDateTime();
    bool superuser = (UserRepo->findAccessLevel(userId).level() >= TAccessLevel::SuperuserLevel);
    QList<Group> entityList = superuser ? GroupRepo->findAll() : GroupRepo->findAllByUserId(userId);
    TGetGroupInfoListReplyData replyData;
    TGroupInfoList infoList;
    foreach (const Group &e, entityList)
        infoList << groupToGroupInfo(e);
    replyData.setNewGroups(infoList);
    return Out(replyData, dt);
}

RequestOut<TGetInviteInfoListReplyData> UserService::getInviteInfoList(
        const RequestIn<TGetInviteInfoListRequestData> &in, quint64 userId)
{
    typedef RequestOut<TGetInviteInfoListReplyData> Out;
    Translator t(in.locale());
    QString error;
    if (!commonCheck(t, &error))
        return Out(error);
    if (!userId)
        return Out(t.translate("UserService", "Invalid user ID (internal)", "error"));
    QDateTime dt = QDateTime::currentDateTime();
    bool superuser = (UserRepo->findAccessLevel(userId).level() >= TAccessLevel::SuperuserLevel);
    QList<InviteCode> entityList = superuser ? InviteCodeRepo->findAll() : InviteCodeRepo->findAllByOwnerId(userId);
    TGetInviteInfoListReplyData replyData;
    TInviteInfoList infoList;
    foreach (const InviteCode &e, entityList)
        infoList << inviteCodeToInviteInfo(e);
    replyData.setNewInvites(infoList);
    return Out(replyData, dt);
}

RequestOut<TGetSelfInfoReplyData> UserService::getSelfInfo(const RequestIn<TGetSelfInfoRequestData> &in,
                                                           quint64 userId)
{
    typedef RequestOut<TGetSelfInfoReplyData> Out;
    Translator t(in.locale());
    QString error;
    if (!commonCheck(t, &error))
        return Out(error);
    if (!userId)
        return Out(t.translate("UserService", "Invalid user ID (internal)", "error"));
    if (in.cachingEnabled() && in.lastRequestDateTime().isValid()) {
        QDateTime dt = QDateTime::currentDateTime();
        if (in.lastRequestDateTime() >= UserRepo->findLastModificationDateTime(userId))
            return Out(dt);
    }
    QDateTime dt = QDateTime::currentDateTime();
    User entity = UserRepo->findOne(userId);
    if (!entity.isValid())
        return Out(t.translate("UserService", "No such user", "error"));
    TGetSelfInfoReplyData replyData;
    replyData.setUserInfo(userToUserInfo(entity, true));
    return Out(replyData, dt);
}

RequestOut<TGetUserInfoReplyData> UserService::getUserInfo(const RequestIn<TGetUserInfoRequestData> &in)
{
    typedef RequestOut<TGetUserInfoReplyData> Out;
    Translator t(in.locale());
    QString error;
    if (!commonCheck(t, in.data(), &error))
        return Out(error);
    TUserIdentifier id = in.data().identifier();
    if (in.cachingEnabled() && in.lastRequestDateTime().isValid()) {
        QDateTime dt = QDateTime::currentDateTime();
        if (in.lastRequestDateTime() >= UserRepo->findLastModificationDateTime(id))
            return Out(dt);
    }
    QDateTime dt = QDateTime::currentDateTime();
    User entity = UserRepo->findOne(id);
    if (!entity.isValid())
        return Out(t.translate("UserService", "No such user", "error"));
    TGetUserInfoReplyData replyData;
    replyData.setUserInfo(userToUserInfo(entity, false));
    return Out(replyData, dt);
}

RequestOut<TGetUserInfoAdminReplyData> UserService::getUserInfoAdmin(const RequestIn<TGetUserInfoAdminRequestData> &in)
{
    typedef RequestOut<TGetUserInfoAdminReplyData> Out;
    Translator t(in.locale());
    QString error;
    if (!commonCheck(t, in.data(), &error))
        return Out(error);
    TUserIdentifier id = in.data().identifier();
    if (in.cachingEnabled() && in.lastRequestDateTime().isValid()) {
        QDateTime dt = QDateTime::currentDateTime();
        if (in.lastRequestDateTime() >= UserRepo->findLastModificationDateTime(id))
            return Out(dt);
    }
    QDateTime dt = QDateTime::currentDateTime();
    User entity = UserRepo->findOne(id);
    if (!entity.isValid())
        return Out(t.translate("UserService", "No such user", "error"));
    TGetUserInfoAdminReplyData replyData;
    replyData.setUserInfo(userToUserInfo(entity, true));
    return Out(replyData, dt);
}

RequestOut<TGetUserInfoListAdminReplyData> UserService::getUserInfoListAdmin(
        const RequestIn<TGetUserInfoListAdminRequestData> &in)
{
    typedef RequestOut<TGetUserInfoListAdminReplyData> Out;
    Translator t(in.locale());
    QString error;
    if (!commonCheck(t, &error))
        return Out(error);
    QDateTime dt = QDateTime::currentDateTime();
    QList<User> entities = UserRepo->findAllNewerThan(in.lastRequestDateTime());
    TUserInfoList newUsers;
    //TIdList deletedUsers;
    foreach (const User &entity, entities)
        newUsers << userToUserInfo(entity, true);
    TGetUserInfoListAdminReplyData replyData;
    //replyData.setDeletedUsers(deletedUsers);
    replyData.setNewUsers(newUsers);
    return Out(replyData, dt);
}

bool UserService::initializeRoot(QString *error)
{
    if (!isValid())
        return bRet(error, tr("Invalid UserService instance (internal)", "error"), false);
    bool users = UserRepo->countByAccessLevel(TAccessLevel::SuperuserLevel);
    if (Settings::Server::readonly() && !users)
        return bRet(error, tr("Can't create users in read-only mode", "error"), false);
    if (users)
        return bRet(error, QString(), true);
    QString login = bReadLine(tr("Enter superuser login [default: \"root\"]:", "prompt") + " ");
    if (login.isEmpty())
        login = "root";
    if (!Texsample::testLogin(login, error))
        return false;
    QString email = bReadLine(tr("Enter superuser e-mail:", "prompt") + " ");
    if (!Texsample::testEmail(email, error))
        return false;
    QString password = bReadLineSecure(tr("Enter superuser password:", "prompt") + " ");
    if (!Texsample::testPassword(password, error))
        return false;
    if (password != bReadLineSecure(tr("Confirm password:", "prompt") + " "))
        return bRet(error, tr("Passwords does not match", "error"), false);
    QString name = bReadLine(tr("Enter superuser name [default: \"\"]:", "prompt") + " ");
    if (!name.isEmpty() && !Texsample::testName(name, error))
        return false;
    QString patronymic = bReadLine(tr("Enter superuser middlename [default: \"\"]:", "prompt") + " ");
    if (!patronymic.isEmpty() && !Texsample::testName(patronymic, error))
        return false;
    QString surname = bReadLine(tr("Enter superuser surname [default: \"\"]:", "prompt") + " ");
    if (!surname.isEmpty() && !Texsample::testName(surname, error))
        return false;
    TransactionHolder holder(Source);
    User entity;
    entity.setAccessLevel(TAccessLevel::SuperuserLevel);
    entity.setActive(false);
    entity.setEmail(email);
    entity.setLogin(login);
    entity.setName(name);
    entity.setPassword(Texsample::encryptPassword(password));
    entity.setPatronymic(patronymic);
    entity.setSurname(surname);
    bWriteLine(tr("Creating superuser account...", "message"));
    if (!addUser(entity, entity, Application::locale(), error))
        return false;
    bWriteLine(tr("Superuser account was created. Please, check your e-mail for the confirmation code.", "message"));
    QString confirmation = bReadLine(tr("Enter confirmation code (xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx):", "message")
                                     + " ");
    if (confirmation.isEmpty())
        return bRet(error, tr("Confirmation code is empty", "error"), false);
    BUuid code = BUuid(confirmation);
    if (!confirmRegistration(code, Application::locale(), error))
        return false;
    if (!commit(Application::locale(), holder, error))
        return false;
    return bRet(error, QString(), true);
}

bool UserService::isRootInitialized()
{
    return isValid() && UserRepo->countByAccessLevel(TAccessLevel::SuperuserLevel);
}

bool UserService::isValid() const
{
    return Source && Source->isValid() && AccountRecoveryCodeRepo->isValid() && GroupRepo->isValid()
            && InviteCodeRepo->isValid() && RegistrationConfirmationCodeRepo->isValid() && UserRepo->isValid();
}

RequestOut<TRecoverAccountReplyData> UserService::recoverAccount(const RequestIn<TRecoverAccountRequestData> &in)
{
    typedef RequestOut<TRecoverAccountReplyData> Out;
    const TRecoverAccountRequestData &requestData = in.data();
    Translator t(in.locale());
    QString error;
    if (!commonCheck(t, &error))
        return Out(error);
    bool ok = false;
    AccountRecoveryCode arcEntity = AccountRecoveryCodeRepo->findOneByCode(requestData.recoveryCode(), &ok);
    if (!ok)
        return Out(t.translate("UserService", "Failed to get account recovery code (internal)", "error"));
    if (!arcEntity.isValid())
        return Out(t.translate("UserService", "No such code", "error"));
    User userEntity = UserRepo->findOne(arcEntity.userId());
    if (!userEntity.isValid())
        return Out(t.translate("UserService", "Failed to get user (internal)", "error"));
    QDateTime dt = QDateTime::currentDateTimeUtc();
    TransactionHolder holder(Source);
    userEntity.convertToCreatedByUser();
    userEntity.setPassword(requestData.password());
    if (!UserRepo->edit(userEntity))
        return Out(t.translate("UserService", "Failed to edit user (internal)", "error"));
    AccountRecoveryCodeRepo->deleteOneByUserId(userEntity.id(), &ok);
    if (!ok)
        return Out(t.translate("UserService", "Failed to remove account recovery code (internal)", "error"));
    if (!sendEmail(userEntity.email(), "recover_account", in.locale()))
        return Out(t.translate("UserService", "Failed to send e-mail message", "error"));
    if (!commit(t, holder, &error))
        return Out(error);
    TRecoverAccountReplyData replyData;
    replyData.setSuccess(true);
    return Out(replyData, dt);
}

RequestOut<TRegisterReplyData> UserService::registerUser(const RequestIn<TRegisterRequestData> &in)
{
    typedef RequestOut<TRegisterReplyData> Out;
    const TRegisterRequestData &data = in.data();
    InviteCode inviteEntity = InviteCodeRepo->findOneByCode(data.inviteCode());
    Translator t(in.locale());
    QString error;
    if (!commonCheck(t, &error))
        return Out(error);
    if (!inviteEntity.isValid())
        return Out(t.translate("UserService", "Invalid invite code", "error"));
    TransactionHolder holder(Source);
    if (!InviteCodeRepo->deleteOne(inviteEntity.id()))
        return Out(t.translate("UserService", "Failed to remove invite code (internal)", "error"));
    User entity;
    entity.setAccessLevel(TAccessLevel::UserLevel);
    entity.setActive(false);
    entity.setAvailableServices(inviteEntity.availableServices());
    entity.setAvatar(data.avatar());
    entity.setEmail(data.email());
    entity.setGroups(inviteEntity.groups());
    entity.setLogin(data.login());
    entity.setName(data.name());
    entity.setPassword(data.encryptedPassword());
    entity.setPatronymic(data.patronymic());
    entity.setSurname(data.surname());
    if (!addUser(entity, entity, in.locale(), &error))
        return Out(error);
    if (!commit(t, holder, &error))
        return Out(error);
    TUserInfo info = userToUserInfo(entity, true);
    TRegisterReplyData replyData;
    replyData.setUserInfo(info);
    return Out(replyData, info.registrationDateTime());
}

RequestOut<TRequestRecoveryCodeReplyData> UserService::requestRecoveryCode(
        const RequestIn<TRequestRecoveryCodeRequestData> &in)
{
    typedef RequestOut<TRequestRecoveryCodeReplyData> Out;
    const TRequestRecoveryCodeRequestData &requestData = in.data();
    Translator t(in.locale());
    QString error;
    if (!commonCheck(t, &error))
        return Out(error);
    bool ok = false;
    User userEntity = UserRepo->findOneByEmail(requestData.email());
    if (!userEntity.isValid())
        return Out(t.translate("UserService", "No user with this e-mail", "error"));
    QDateTime dt = QDateTime::currentDateTimeUtc();
    TransactionHolder holder(Source);
    AccountRecoveryCode entity;
    BUuid code = BUuid::createUuid();
    entity.setCode(code);
    entity.setExpirationDateTime(QDateTime::currentDateTimeUtc().addDays(1));
    entity.setUserId(userEntity.id());
    AccountRecoveryCodeRepo->add(entity, &ok);
    if (!ok)
        return Out(t.translate("UserService", "Failed to add account recovery code (internal)", "error"));
    BProperties replace;
    replace.insert("%username%", userEntity.login());
    replace.insert("%code%", code.toString(true));
    if (!sendEmail(requestData.email(), "request_recovery_code", in.locale(), replace))
        return Out(t.translate("UserService", "Failed to send e-mail message", "error"));
    if (!commit(t, holder, &error))
        return Out(error);
    TRequestRecoveryCodeReplyData replyData;
    return Out(replyData, dt);
}

/*============================== Static private methods ====================*/

bool UserService::sendEmail(const QString &receiver, const QString &templateName, const QLocale &locale,
                            const BProperties &replace)
{
    if (receiver.isEmpty() || templateName.isEmpty())
        return false;
    QString templatePath = BDirTools::findResource("templates/" + templateName, BDirTools::GlobalOnly);
    if (!QFileInfo(templatePath).isDir())
        return false;
    QString subject = BDirTools::localeBasedFileName(templatePath + "/subject.txt", locale);
    QString body = BDirTools::localeBasedFileName(templatePath + "/body.txt", locale);
    bool ok = false;
    subject = BDirTools::readTextFile(subject, "UTF-8", &ok);
    if (!ok)
        return false;
    body = BDirTools::readTextFile(body, "UTF-8", &ok);
    if (!ok)
        return false;
    foreach (const QString &k, replace.keys()) {
        QString v = replace.value(k);
        subject.replace(k, v);
        body.replace(k, v);
    }
    BSmtpSender sender;
    sender.setServer(Settings::Email::serverAddress(), Settings::Email::serverPort());
    sender.setLocalHostName(Settings::Email::localHostName());
    sender.setSocketType(Settings::Email::sslRequired() ? BGenericSocket::SslSocket : BGenericSocket::TcpSocket);
    sender.setUser(Settings::Email::login(), Settings::Email::password());
    BEmail email;
    email.setSender("TeXSample Team");
    email.setReceiver(receiver);
    email.setSubject(subject);
    email.setBody(body);
    sender.setEmail(email);
    sender.send();
    return sender.waitForFinished();
}

/*============================== Private methods ===========================*/

bool UserService::addUser(const User &entity, User &newEntity, const QLocale &locale, QString *error)
{
    Translator t(locale);
    if (!entity.isValid())
        return bRet(error, t.translate("UserService", "Invalid User entity instance (internal)", "error"), false);
    if (UserRepo->loginOccupied(entity.login()))
        return bRet(error, t.translate("UserService", "Login is occupied", "error"), false);
    if (UserRepo->emailOccupied(entity.email()))
        return bRet(error, t.translate("UserService", "E-mail is occupied", "error"), false);
    quint64 id = UserRepo->add(entity);
    if (!id)
        return bRet(error, t.translate("UserService", "Failed to add add user (internal)", "error"), false);
    newEntity = UserRepo->findOne(id);
    if (!newEntity.isValid())
        return bRet(error, t.translate("UserService", "Failed to get user (internal)", "error"), false);
    RegistrationConfirmationCode codeEntity;
    BUuid code = BUuid::createUuid();
    codeEntity.setCode(code);
    codeEntity.setExpirationDateTime(QDateTime::currentDateTimeUtc().addDays(1));
    codeEntity.setUserId(id);
    if (!RegistrationConfirmationCodeRepo->add(codeEntity))
        return bRet(error, t.translate("UserService", "Failed to add registration confirmation code (internal)",
                                       "error"), false);
    BProperties replace;
    replace.insert("%username%", entity.login());
    replace.insert("%code%", code.toString(true));
    if (!sendEmail(entity.email(), "registration_confirmation", locale, replace))
        return bRet(error, t.translate("UserService", "Failed to send e-mail message", "error"), false);
    return bRet(error, QString(), true);
}

bool UserService::checkUserId(const Translator &t, quint64 userId, QString *error)
{
    if (!userId)
        return bRet(error, t.translate("UserService", "Invalid user ID (internal)", "error"), false);
    return bRet(error, QString(), true);
}

bool UserService::commit(const QLocale &locale, TransactionHolder &holder, QString *error)
{
    Translator t(locale);
    return commit(t, holder, error);
}

bool UserService::commit(const Translator &translator, TransactionHolder &holder, QString *error)
{
    if (!holder.doCommit())
        return bRet(error, translator.translate("UserService", "Failed to commit (internal)", "error"), false);
    return bRet(error, QString(), true);
}

bool UserService::commonCheck(const Translator &translator, QString *error) const
{
    if (!isValid()) {
        return bRet(error, translator.translate("UserService", "Invalid UserService instance (internal)", "error"),
                    false);
    }
    return bRet(error, QString(), true);
}

bool UserService::confirmRegistration(const BUuid &code, const QLocale &locale, QString *error)
{
    Translator t(locale);
    if (code.isNull())
        return bRet(error, t.translate("UserService", "Invalid registration confirmation code", "error"), false);
    RegistrationConfirmationCode codeEntity = RegistrationConfirmationCodeRepo->findOneByCode(code);
    if (!codeEntity.isValid())
        return bRet(error, t.translate("UserService", "No such code", "error"), false);
    User userEntity = UserRepo->findOne(codeEntity.userId());
    if (!userEntity.isValid())
        return bRet(error, t.translate("UserService", "Failed to get user (internal)", "error"), false);
    userEntity.convertToCreatedByUser();
    userEntity.setActive(true);
    if (!UserRepo->edit(userEntity))
        return bRet(error, t.translate("UserService", "Failed to edit user (internal)", "error"), false);
    if (!RegistrationConfirmationCodeRepo->deleteOneByUserId(codeEntity.userId())) {
        return bRet(error, t.translate("UserService", "Failed to remove registration confirmation code (internal)",
                                       "error"), false);
    }
    return bRet(error, QString(), true);
}

TGroupInfoList UserService::getAllGroups()
{
    TGroupInfoList groups;
    if (!isValid())
        return groups;
    foreach (const Group &entity, GroupRepo->findAll())
        groups << groupToGroupInfo(entity);
    return groups;
}

TGroupInfoList UserService::getGroups(const TIdList &ids)
{
    TGroupInfoList groups;
    if (!isValid())
        return groups;
    foreach (const Group &entity, GroupRepo->findAll(ids))
        groups << groupToGroupInfo(entity);
    return groups;
}

TGroupInfoList UserService::getGroups(quint64 userId)
{
    TGroupInfoList groups;
    if (!isValid())
        return groups;
    foreach (const Group &entity, GroupRepo->findAllByUserId(userId))
        groups << groupToGroupInfo(entity);
    return groups;
}

TGroupInfo UserService::groupToGroupInfo(const Group &entity)
{
    TGroupInfo info;
    if (!isValid() || !entity.isValid() || !entity.isCreatedByRepo())
        return info;
    info.setCreationDateTime(entity.creationDateTime());
    info.setLastModificationDateTime(entity.lastModificationDateTime());
    info.setId(entity.id());
    info.setName(entity.name());
    info.setOwnerId(entity.ownerId());
    info.setOwnerLogin(UserRepo->findLogin(entity.ownerId()));
    return info;
}

TInviteInfo UserService::inviteCodeToInviteInfo(const InviteCode &entity)
{
    TInviteInfo info;
    if (!isValid() || !entity.isValid() || !entity.isCreatedByRepo())
        return info;
    info.setAccessLevel(entity.accessLevel());
    info.setCode(entity.code());
    info.setCreationDateTime(entity.creationDateTime());
    info.setExpirationDateTime(entity.expirationDateTime());
    info.setGroups(getGroups(entity.groups()));
    info.setId(entity.id());
    info.setOwnerId(entity.ownerId());
    info.setOwnerLogin(UserRepo->findLogin(entity.ownerId()));
    info.setServices(entity.availableServices());
    return info;
}

TUserInfo UserService::userToUserInfo(const User &entity, bool includeEmail)
{
    TUserInfo info;
    if (!isValid() || !entity.isValid() || !entity.isCreatedByRepo())
        return info;
    info.setAccessLevel(entity.accessLevel());
    bool superuser = entity.accessLevel().level() >= TAccessLevel::SuperuserLevel;
    info.setActive(entity.active());
    info.setAvailableGroups(superuser ? getAllGroups() : getGroups(entity.id()));
    info.setAvailableServices(superuser ? TServiceList::allServices() : entity.availableServices());
    info.setAvatar(entity.avatar());
    if (includeEmail)
        info.setEmail(entity.email());
    info.setGroups(getGroups(entity.groups()));
    info.setId(entity.id());
    info.setLastModificationDateTime(entity.lastModificationDateTime());
    info.setLogin(entity.login());
    info.setName(entity.name());
    info.setPatronymic(entity.patronymic());
    info.setRegistrationDateTime(entity.registrationDateTime());
    info.setSurname(entity.surname());
    return info;
}
