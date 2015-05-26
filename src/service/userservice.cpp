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

#include <QBuffer>
#include <QByteArray>
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
    delete AccountRecoveryCodeRepo;
    delete EmailChangeConfirmationCodeRepo;
    delete GroupRepo;
    delete InviteCodeRepo;
    delete RegistrationConfirmationCodeRepo;
    delete UserRepo;
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
    bool ok = false;
    Group entity;
    entity.setName(in.data().name());
    entity.setOwnerId(userId);
    TransactionHolder holder(Source);
    quint64 id = GroupRepo->add(entity, &ok);
    if (!ok || !id)
        return Out(t.translate("UserService", "Failed to add group (internal)", "error"));
    entity = GroupRepo->findOne(id, &ok);
    if (!ok || !entity.isValid())
        return Out(t.translate("UserService", "Failed to get group (internal)", "error"));
    TGroupInfo info = groupToGroupInfo(entity, &ok);
    if (!ok)
        return Out(t.translate("UserService", "Failed to create group info (internal)", "error"));
    if (!commit(t, holder, &error))
        return Out(error);
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
    entity.setSaveAvatar(true);
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
    bool ok = false;
    TUserInfo info = userToUserInfo(entity, true, &ok);
    if (!ok)
        return Out(t.translate("UserService", "Failed to create user info (internal)", "error"));
    if (!commit(t, holder, &error))
        return Out(error);
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
    bool ok = false;
    User entity = UserRepo->findOne(in.data().identifier(), in.data().password(), &ok);
    if (!ok)
        return Out(t.translate("UserService", "Failed to get user (internal)", "error"));
    if (!entity.isValid())
        return Out(t.translate("UserService", "Invalid login, e-mail, or password", "error"));
    if (!entity.active())
        return Out(t.translate("UserService", "Account is inactive", "error"));
    TUserInfo info = userToUserInfo(entity, true, &ok);
    if (!ok)
        return Out(t.translate("UserService", "Failed to create user info (internal)", "error"));
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
    bool ok = false;
    User entity = UserRepo->findOne(userId, &ok);
    if (!ok)
        return Out(t.translate("UserService", "Failed to get user (internal)", "error"));
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
    EmailChangeConfirmationCodeRepo->add(ecccEntity, &ok);
    if (!ok)
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
    bool ok = false;
    User entity = UserRepo->findOne(userId, &ok);
    if (!ok)
        return Out(t.translate("UserService", "Failed to get user (internal)", "error"));
    if (!entity.isValid())
        return Out(t.translate("UserService", "No such user", "error"));
    TChangePasswordRequestData requestData = in.data();
    if (requestData.oldPassword() != entity.password())
        return Out(t.translate("UserService", "Invalid password", "error"));
    TransactionHolder holder(Source);
    entity.convertToCreatedByUser();
    entity.setPassword(requestData.newPassword());
    QDateTime dt = UserRepo->edit(entity, &ok);
    if (!ok)
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
    bool ok = false;
    QString email = in.data().email();
    TCheckEmailAvailabilityReplyData replyData;
    bool emailOccupied = UserRepo->emailOccupied(email, &ok);
    if (!ok)
        return Out(t.translate("UserService", "Failed to check e-mail freeness (internal)", "error"));
    emailOccupied = emailOccupied && EmailChangeConfirmationCodeRepo->emailOccupied(email, &ok);
    if (!ok)
        return Out(t.translate("UserService", "Failed to check e-mail freeness (internal)", "error"));
    replyData.setAvailable(!emailOccupied);
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
    bool ok = false;
    TCheckLoginAvailabilityReplyData replyData;
    bool loginOccupied = UserRepo->loginOccupied(in.data().login(), &ok);
    if (!ok)
        return Out(t.translate("UserService", "Failed to check login freeness (internal)", "error"));
    replyData.setAvailable(!loginOccupied);
    return Out(replyData, dt);
}

bool UserService::checkOutdatedEntries(QString *error)
{
    Translator t(Application::locale());
    if (!commonCheck(t, error))
        return false;
    TransactionHolder holder(Source);
    bool ok = false;
    AccountRecoveryCodeRepo->deleteExpired(&ok);
    if (!ok) {
        return bRet(error, t.translate("UserService", "Failed to delete expired account recovery codes (internal)",
                                       "error"), false);
    }
    InviteCodeRepo->deleteExpired(&ok);
    if (!ok) {
        return bRet(error, t.translate("UserService", "Failed to delete expired invite codes (internal)", "error"),
                    false);
    }
    QList<RegistrationConfirmationCode> rccEntityList = RegistrationConfirmationCodeRepo->findExpired(&ok);
    if (!ok) {
        return bRet(error, t.translate("UserService", "Failed to get registration confirmation code list (internal)",
                                       "error"), false);
    }
    foreach (const RegistrationConfirmationCode &entity, rccEntityList) {
        UserRepo->deleteOne(entity.userId(), &ok);
        if (!ok)
            return bRet(error, t.translate("UserService", "Failed to delete user (internal)", "error"), false);
    }
    RegistrationConfirmationCodeRepo->deleteExpired(&ok);
    if (!ok) {
        return bRet(error, t.translate("UserService",
                                       "Failed to delete expired registration confirmation codes (internal)", "error"),
                    false);
    }
    EmailChangeConfirmationCodeRepo->deleteExpired(&ok);
    if (!ok) {
        return bRet(error, t.translate("UserService",
                                       "Failed to delete expired email change confirmation code list (internal)",
                                       "error"), false);
    }
    if (!commit(t, holder, error))
        return false;
    return bRet(error, QString(), true);
}

RequestOut<TConfirmEmailChangeReplyData> UserService::confirmEmailChange(
        const RequestIn<TConfirmEmailChangeRequestData> &in)
{
    typedef RequestOut<TConfirmEmailChangeReplyData> Out;
    Translator t(in.locale());
    QString error;
    if (!commonCheck(t, in.data(), &error))
        return Out(error);
    bool ok = false;
    TConfirmEmailChangeReplyData replyData;
    replyData.setSuccess(false);
    BUuid code = in.data().confirmationCode();
    EmailChangeConfirmationCode ecccEntity = EmailChangeConfirmationCodeRepo->findOneByCode(code, &ok);
    if (!ok)
        return Out(t.translate("UserService", "Failed to get email change confirmation code (internal)", "error"));
    if (!ecccEntity.isValid())
        return Out(t.translate("UserService", "No such code", "error"));
    User entity = UserRepo->findOne(ecccEntity.userId(), &ok);
    if (!ok || !entity.isValid())
        return Out(t.translate("UserService", "Failed to get user (internal)", "error"));
    entity.convertToCreatedByUser();
    entity.setEmail(ecccEntity.email());
    TransactionHolder holder(Source);
    QDateTime dt = UserRepo->edit(entity, &ok);
    if (!ok)
        return Out(t.translate("UserService", "Failed to edit user (internal)", "error"));
    EmailChangeConfirmationCodeRepo->deleteOneByUserId(ecccEntity.userId(), &ok);
    if (!ok)
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
    TConfirmRegistrationReplyData replyData;
    replyData.setSuccess(false);
    TransactionHolder holder(Source);
    QDateTime dt;
    if (!confirmRegistration(in.data().confirmationCode(), dt, in.locale(), &error))
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

RequestOut<TDeleteGroupReplyData> UserService::deleteGroup(const RequestIn<TDeleteGroupRequestData> &in)
{
    typedef RequestOut<TDeleteGroupReplyData> Out;
    Translator t(in.locale());
    QString error;
    if (!commonCheck(t, in.data(), &error))
        return Out(error);
    bool ok = false;
    QDateTime dt = QDateTime::currentDateTime();
    TransactionHolder holder(Source);
    GroupRepo->deleteOne(in.data().id(), &ok);
    if (!ok)
        return Out(t.translate("UserService", "Failed to delete group (internal)", "error"));
    if (!commit(t, holder, &error))
        return Out(error);
    TDeleteGroupReplyData replyData;
    return Out(replyData, dt);
}

RequestOut<TDeleteInvitesReplyData> UserService::deleteInvites(const RequestIn<TDeleteInvitesRequestData> &in)
{
    typedef RequestOut<TDeleteInvitesReplyData> Out;
    Translator t(in.locale());
    QString error;
    if (!commonCheck(t, in.data(), &error))
        return Out(error);
    bool ok = false;
    QDateTime dt = QDateTime::currentDateTime();
    TransactionHolder holder(Source);
    InviteCodeRepo->deleteSome(in.data().ids(), &ok);
    if (!ok)
        return Out(t.translate("UserService", "Failed to delete invite(s) (internal)", "error"));
    if (!commit(t, holder, &error))
        return Out(error);
    TDeleteInvitesReplyData replyData;
    replyData.setIdentifiers(in.data().ids());
    return Out(replyData, dt);
}

RequestOut<TDeleteUserReplyData> UserService::deleteUser(const RequestIn<TDeleteUserRequestData> &in)
{
    typedef RequestOut<TDeleteUserReplyData> Out;
    Translator t(in.locale());
    QString error;
    if (!commonCheck(t, in.data(), &error))
        return Out(error);
    bool ok = false;
    TransactionHolder holder(Source);
    QDateTime dt = UserRepo->deleteOne(in.data().identifier(), &ok);
    if (!ok)
        return Out(t.translate("UserService", "Failed to delete user (internal)", "error"));
    if (!commit(t, holder, &error))
        return Out(error);
    TDeleteUserReplyData replyData;
    return Out(replyData, dt);
}

RequestOut<TEditGroupReplyData> UserService::editGroup(const RequestIn<TEditGroupRequestData> &in)
{
    typedef RequestOut<TEditGroupReplyData> Out;
    Translator t(in.locale());
    QString error;
    if (!commonCheck(t, in.data(), &error))
        return Out(error);
    bool ok = false;
    Group entity = GroupRepo->findOne(in.data().id(), &ok);
    if (!ok)
        return Out(t.translate("UserService", "Failed to get group (internal)", "error"));
    TEditGroupRequestData requestData = in.data();
    entity.convertToCreatedByUser();
    entity.setName(requestData.name());
    TransactionHolder holder(Source);
    GroupRepo->edit(entity, &ok);
    if (!ok)
        return Out(t.translate("UserService", "Failed to edit group (internal)", "error"));
    entity = GroupRepo->findOne(in.data().id(), &ok);
    if (!ok || !entity.isValid())
        return Out(t.translate("UserService", "Failed to get group (internal)", "error"));
    TEditGroupReplyData replyData;
    replyData.setGroupInfo(groupToGroupInfo(entity, &ok));
    if (!ok)
        return Out(t.translate("UserService", "Failed to create group info (internal)", "error"));
    if (!commit(t, holder, &error))
        return Out(error);
    return Out(replyData, entity.lastModificationDateTime());
}

RequestOut<TEditSelfReplyData> UserService::editSelf(const RequestIn<TEditSelfRequestData> &in, quint64 userId)
{
    typedef RequestOut<TEditSelfReplyData> Out;
    Translator t(in.locale());
    QString error;
    const TEditSelfRequestData &requestData = in.data();
    if (!commonCheck(t, requestData, &error))
        return Out(error);
    if (!checkUserId(t, userId, &error))
        return Out(error);
    if (!checkUserAvatar(t, requestData.avatar(), &error))
        return Out(error);
    bool ok = false;
    User entity = UserRepo->findOne(userId, &ok);
    if (!ok)
        return Out(t.translate("UserService", "Failed to get user (internal)", "error"));
    if (!entity.isValid())
        return Out(t.translate("UserService", "No such user", "error"));
    entity.convertToCreatedByUser();
    entity.setSaveAvatar(true);
    entity.setAvatar(requestData.avatar());
    entity.setName(requestData.name());
    entity.setPatronymic(requestData.patronymic());
    entity.setSurname(requestData.surname());
    TransactionHolder holder(Source);
    UserRepo->edit(entity, &ok);
    if (!ok)
        return Out(t.translate("UserService", "Failed to edit user (internal)", "error"));
    entity = UserRepo->findOne(userId, &ok);
    if (!ok || !entity.isValid())
        return Out(t.translate("UserService", "Failed to get user (internal)", "error"));
    TEditSelfReplyData replyData;
    TUserInfo info = userToUserInfo(entity, true, &ok);
    if (!ok)
        return Out(t.translate("UserService", "Failed to create user info (internal)", "error"));
    if (!commit(t, holder, &error))
        return Out(error);
    replyData.setUserInfo(info);
    return Out(replyData, entity.lastModificationDateTime());

}

RequestOut<TEditUserReplyData> UserService::editUser(const RequestIn<TEditUserRequestData> &in)
{
    typedef RequestOut<TEditUserReplyData> Out;
    Translator t(in.locale());
    QString error;
    const TEditUserRequestData &requestData = in.data();
    if (!commonCheck(t, requestData, &error))
        return Out(error);
    if (!checkUserAvatar(t, requestData.avatar(), &error))
        return Out(error);
    bool ok = false;
    User entity = UserRepo->findOne(requestData.identifier(), &ok);
    if (!ok)
        return Out(t.translate("UserService", "Failed to get user (internal)", "error"));
    if (!entity.isValid())
        return Out(t.translate("UserService", "No such user", "error"));
    entity.convertToCreatedByUser();
    entity.setAccessLevel(requestData.accessLevel());
    entity.setActive(requestData.active());
    entity.setAvailableServices(requestData.availableServices());
    entity.setSaveAvatar(true);
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
    UserRepo->edit(entity, &ok);
    if (!ok)
        return Out(t.translate("UserService", "Failed to edit user (internal)", "error"));
    entity = UserRepo->findOne(requestData.identifier(), &ok);
    if (!ok || !entity.isValid())
        return Out(t.translate("UserService", "Failed to get user (internal)", "error"));
    TEditUserReplyData replyData;
    TUserInfo info = userToUserInfo(entity, true, &ok);
    if (!ok)
        return Out(t.translate("UserService", "Failed to create user info (internal)", "error"));
    if (!commit(t, holder, &error))
        return Out(error);
    replyData.setUserInfo(info);
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
    if (!checkUserId(t, userId, &error))
        return Out(error);
    QDateTime dt = QDateTime::currentDateTime();
    bool ok = false;
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
        quint64 id = InviteCodeRepo->add(entity, &ok);
        if (!ok || !id)
            return Out(t.translate("UserService", "Failed to add invite code (internal)", "error"));
        InviteCode newEntity = InviteCodeRepo->findOne(id, &ok);
        if (!ok || !newEntity.isValid())
            return Out(t.translate("UserService", "Failed to get invite code (internal)", "error"));
        entityList << newEntity;
    }
    TGenerateInvitesReplyData replyData;
    TInviteInfoList infoList;
    foreach (const InviteCode &e, entityList) {
        infoList << inviteCodeToInviteInfo(e, &ok);
        if (!ok)
            return Out(t.translate("UserService", "Failed to create invite code info (internal)", "error"));
    }
    if (!commit(t, holder, &error))
        return Out(error);
    replyData.setGeneratedInvites(infoList);
    return Out(replyData, dt);
}

RequestOut<TGetGroupInfoListReplyData> UserService::getGroupInfoList(const RequestIn<TGetGroupInfoListRequestData> &in)
{
    typedef RequestOut<TGetGroupInfoListReplyData> Out;
    Translator t(in.locale());
    QString error;
    if (!commonCheck(t, &error))
        return Out(error);
    QDateTime dt = QDateTime::currentDateTime();
    bool ok = false;
    TGroupInfoList newGroups;
    TIdList deletedGroups = GroupRepo->findAllDeleted(in.lastRequestDateTime(), &ok);
    if (!ok)
        return Out(t.translate("UserService", "Failed to get deleted group list (internal)", "error"));
    QList<Group> entityList = GroupRepo->findAll(in.lastRequestDateTime(), &ok);
    if (!ok)
        return Out(t.translate("UserService", "Failed to get group list (internal)", "error"));
    foreach (const Group &e, entityList) {
        newGroups << groupToGroupInfo(e, &ok);
        if (!ok)
            return Out(t.translate("UserService", "Failed to create group info (internal)", "error"));
    }
    TGetGroupInfoListReplyData replyData;
    replyData.setNewGroups(newGroups);
    replyData.setDeletedGroups(deletedGroups);
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
    if (!checkUserId(t, userId, &error))
        return Out(error);
    QDateTime dt = QDateTime::currentDateTime();
    bool ok = false;
    bool super = UserRepo->findAccessLevel(userId, &ok).level() >= TAccessLevel::SuperuserLevel;
    if (!ok)
        return Out(t.translate("LabService", "Failed to get user access level (internal)", "error"));
    QDateTime last = in.lastRequestDateTime();
    TInviteInfoList newInvites;
    TIdList deletedInvites = super ? InviteCodeRepo->findAllDeleted(last, &ok) :
                                     InviteCodeRepo->findAllDeletedByOwnerId(userId, last, &ok);
    if (!ok)
        return Out(t.translate("UserService", "Failed to get deleted invite code list (internal)", "error"));
    QList<InviteCode> entityList = super ? InviteCodeRepo->findAll(last, &ok) :
                                           InviteCodeRepo->findAllByOwnerId(userId, last, &ok);
    if (!ok)
        return Out(t.translate("UserService", "Failed to get invite code list (internal)", "error"));
    foreach (const InviteCode &e, entityList) {
        newInvites << inviteCodeToInviteInfo(e, &ok);
        if (!ok)
            return Out(t.translate("UserService", "Failed to create invite code info (internal)", "error"));
    }
    TGetInviteInfoListReplyData replyData;
    replyData.setNewInvites(newInvites);
    replyData.setDeletedInvites(deletedInvites);
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
    if (!checkUserId(t, userId, &error))
        return Out(error);
    QDateTime dt = QDateTime::currentDateTime();
    bool ok = false;
    if (in.cachingEnabled() && in.lastRequestDateTime().isValid()) {
        QDateTime lastModDT = UserRepo->findLastModificationDateTime(userId, &ok);
        if (!ok) {
            return Out(t.translate("UserService", "Failed to get user last modification date time (internal)",
                                   "error"));
        }
        if (in.lastRequestDateTime() >= lastModDT)
            return Out(dt);
    }
    User entity = UserRepo->findOne(userId, &ok);
    if (!ok)
        return Out(t.translate("UserService", "Failed to get user (internal)", "error"));
    if (!entity.isValid())
        return Out(t.translate("UserService", "No such user", "error"));
    TGetSelfInfoReplyData replyData;
    TUserInfo info = userToUserInfo(entity, true, &ok);
    if (!ok)
        return Out(t.translate("UserService", "Failed to create user info (internal)", "error"));
    replyData.setUserInfo(info);
    return Out(replyData, dt);
}

RequestOut<TGetUserInfoReplyData> UserService::getUserInfo(const RequestIn<TGetUserInfoRequestData> &in)
{
    typedef RequestOut<TGetUserInfoReplyData> Out;
    Translator t(in.locale());
    QString error;
    if (!commonCheck(t, in.data(), &error))
        return Out(error);
    QDateTime dt = QDateTime::currentDateTime();
    bool ok = false;
    TUserIdentifier id = in.data().identifier();
    if (in.cachingEnabled() && in.lastRequestDateTime().isValid()) {
        QDateTime lastModDT = UserRepo->findLastModificationDateTime(id, &ok);
        if (!ok) {
            return Out(t.translate("UserService", "Failed to get user last modification date time (internal)",
                                   "error"));
        }
        if (in.lastRequestDateTime() >= lastModDT)
            return Out(dt);
    }
    User entity = UserRepo->findOne(id, &ok);
    if (!ok)
        return Out(t.translate("UserService", "Failed to get user (internal)", "error"));
    if (!entity.isValid())
        return Out(t.translate("UserService", "No such user", "error"));
    TGetUserInfoReplyData replyData;
    TUserInfo info = userToUserInfo(entity, false, &ok);
    if (!ok)
        return Out(t.translate("UserService", "Failed to create user info (internal)", "error"));
    replyData.setUserInfo(info);
    return Out(replyData, dt);
}

RequestOut<TGetUserInfoAdminReplyData> UserService::getUserInfoAdmin(const RequestIn<TGetUserInfoAdminRequestData> &in)
{
    typedef RequestOut<TGetUserInfoAdminReplyData> Out;
    Translator t(in.locale());
    QString error;
    if (!commonCheck(t, in.data(), &error))
        return Out(error);
    QDateTime dt = QDateTime::currentDateTime();
    bool ok = false;
    TUserIdentifier id = in.data().identifier();
    if (in.cachingEnabled() && in.lastRequestDateTime().isValid()) {
        QDateTime lastModDT = UserRepo->findLastModificationDateTime(id, &ok);
        if (!ok) {
            return Out(t.translate("UserService", "Failed to get user last modification date time (internal)",
                                   "error"));
        }
        if (in.lastRequestDateTime() >= lastModDT)
            return Out(dt);
    }
    User entity = UserRepo->findOne(id, &ok);
    if (!ok)
        return Out(t.translate("UserService", "Failed to get user (internal)", "error"));
    if (!entity.isValid())
        return Out(t.translate("UserService", "No such user", "error"));
    TGetUserInfoAdminReplyData replyData;
    TUserInfo info = userToUserInfo(entity, true, &ok);
    if (!ok)
        return Out(t.translate("UserService", "Failed to create user info (internal)", "error"));
    replyData.setUserInfo(info);
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
    bool ok = false;
    QList<User> entities = UserRepo->findAllNewerThan(in.lastRequestDateTime(), &ok);
    if (!ok)
        return Out(t.translate("UserService", "Failed to get user list (internal)", "error"));
    TUserInfoList newUsers;
    TIdList deletedUsers = UserRepo->findAllDeletedNewerThan(in.lastRequestDateTime(), &ok);
    if (!ok)
        return Out(t.translate("UserService", "Failed to get deleted user list (internal)", "error"));
    foreach (const User &entity, entities) {
        newUsers << userToUserInfo(entity, true, &ok);
        if (!ok)
            return Out(t.translate("UserService", "Failed to create user info (internal)", "error"));
    }
    TGetUserInfoListAdminReplyData replyData;
    replyData.setDeletedUsers(deletedUsers);
    replyData.setNewUsers(newUsers);
    return Out(replyData, dt);
}

bool UserService::initializeRoot(QString *error)
{
    Translator t(Application::locale());
    if (!commonCheck(t, error))
        return false;
    bool b = false;
    bool initialized = isRootInitialized(&b, error);
    if (!b)
        return false;
    if (Settings::Server::readonly() && !initialized)
        return bRet(error, tr("Can't create users in read-only mode", "error"), false);
    if (initialized)
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
    QDateTime dt;
    if (!confirmRegistration(code, dt, Application::locale(), error))
        return false;
    if (!commit(Application::locale(), holder, error))
        return false;
    return bRet(error, QString(), true);
}

bool UserService::isRootInitialized(bool *ok, QString *error)
{
    Translator t(Application::locale());
    if (!commonCheck(t, error))
        return bRet(ok, false, false);
    bool b = false;
    long count = UserRepo->countByAccessLevel(TAccessLevel::SuperuserLevel, &b);
    if (!b) {
        return bRet(ok, false, error, t.translate("UserService", "Failed to get user count (internal)", "error"),
                    false);
    }
    return bRet(ok, true, error, QString(), (count > 0));
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
    User userEntity = UserRepo->findOne(arcEntity.userId(), &ok);
    if (!ok || !userEntity.isValid())
        return Out(t.translate("UserService", "Failed to get user (internal)", "error"));
    TransactionHolder holder(Source);
    userEntity.convertToCreatedByUser();
    userEntity.setPassword(requestData.password());
    QDateTime dt = UserRepo->edit(userEntity, &ok);
    if (!ok)
        return Out(t.translate("UserService", "Failed to edit user (internal)", "error"));
    AccountRecoveryCodeRepo->deleteOneByUserId(userEntity.id(), &ok);
    if (!ok)
        return Out(t.translate("UserService", "Failed to delete account recovery code (internal)", "error"));
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
    Translator t(in.locale());
    bool ok = false;
    InviteCode inviteEntity = InviteCodeRepo->findOneByCode(data.inviteCode(), &ok);
    if (!ok)
        return Out(t.translate("UserService", "Failed to get invite code (internal)", "error"));
    QString error;
    if (!commonCheck(t, &error))
        return Out(error);
    if (!inviteEntity.isValid())
        return Out(t.translate("UserService", "No such code", "error"));
    TransactionHolder holder(Source);
    InviteCodeRepo->deleteOne(inviteEntity.id(), &ok);
    if (!ok)
        return Out(t.translate("UserService", "Failed to delete invite code (internal)", "error"));
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
    TUserInfo info = userToUserInfo(entity, true, &ok);
    if (!ok)
        return Out(t.translate("UserService", "Failed to create user info (internal)", "error"));
    if (!commit(t, holder, &error))
        return Out(error);
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
    User userEntity = UserRepo->findOneByEmail(requestData.email(), &ok);
    if (!ok)
        return Out(t.translate("UserService", "Failed to get user (internal)", "error"));
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

TUserInfo UserService::userInfo(quint64 userId, bool includeEmail, bool *ok)
{
    if (!isValid() || !userId)
        return bRet(ok, false, TUserInfo());
    bool b = false;
    User entity = UserRepo->findOne(userId, &b);
    if (!b)
        return bRet(ok, false, TUserInfo());
    if (!entity.isValid())
        return bRet(ok, false, TUserInfo());
    return userToUserInfo(entity, includeEmail, ok);
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
    return sender.waitForFinished() && sender.lastTransferSuccess();
}

/*============================== Private methods ===========================*/

bool UserService::addUser(const User &entity, User &newEntity, const QLocale &locale, QString *error)
{
    Translator t(locale);
    bool ok = false;
    if (!entity.isValid())
        return bRet(error, t.translate("UserService", "Invalid User entity instance (internal)", "error"), false);
    bool loginOccupied = UserRepo->loginOccupied(entity.login(), &ok);
    if (!ok)
        return bRet(error, t.translate("UserService", "Failed to check login freeness (internal)", "error"), false);
    if (loginOccupied)
        return bRet(error, t.translate("UserService", "Login is occupied", "error"), false);
    bool emailOccupied = UserRepo->emailOccupied(entity.email(), &ok);
    if (!ok)
        return bRet(error, t.translate("UserService", "Failed to check e-mail freeness (internal)", "error"), false);
    if (emailOccupied)
        return bRet(error, t.translate("UserService", "E-mail is occupied", "error"), false);
    if (!entity.avatar().isNull() && !checkUserAvatar(t, entity.avatar(), error))
        return false;
    quint64 id = UserRepo->add(entity, &ok);
    if (!ok || !id)
        return bRet(error, t.translate("UserService", "Failed to add user (internal)", "error"), false);
    newEntity = UserRepo->findOne(id, &ok);
    if (!ok || !newEntity.isValid())
        return bRet(error, t.translate("UserService", "Failed to get user (internal)", "error"), false);
    RegistrationConfirmationCode codeEntity;
    BUuid code = BUuid::createUuid();
    codeEntity.setCode(code);
    codeEntity.setExpirationDateTime(QDateTime::currentDateTimeUtc().addDays(1));
    codeEntity.setUserId(id);
    RegistrationConfirmationCodeRepo->add(codeEntity, &ok);
    if (!ok) {
        return bRet(error, t.translate("UserService", "Failed to add registration confirmation code (internal)",
                                       "error"), false);
    }
    BProperties replace;
    replace.insert("%username%", entity.login());
    replace.insert("%code%", code.toString(true));
    if (!sendEmail(entity.email(), "registration_confirmation", locale, replace))
        return bRet(error, t.translate("UserService", "Failed to send e-mail message", "error"), false);
    return bRet(error, QString(), true);
}

bool UserService::checkUserAvatar(const Translator &t, const QImage &avatar, QString *error)
{
    if (avatar.isNull())
        return bRet(error, QString(), true);
    if (avatar.height() > Texsample::MaximumAvatarExtent)
        return bRet(error, t.translate("UserService", "Avatar\'s height is too big", "error"), false);
    if (avatar.width() > Texsample::MaximumAvatarExtent)
        return bRet(error, t.translate("UserService", "Avatar\'s width is too big", "error"), false);
    QByteArray ba;
    QBuffer buff(&ba);
    buff.open(QBuffer::WriteOnly);
    if (!avatar.save(&buff, "png"))
        return bRet(error, t.translate("UserService", "Unable to test avatar", "error"), false);
    if (ba.size() > Texsample::MaximumAvatarSize)
        return bRet(error, t.translate("UserService", "Avatar is too big", "error"), false);
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

bool UserService::commit(const Translator &t, TransactionHolder &holder, QString *error)
{
    if (!holder.doCommit())
        return bRet(error, t.translate("UserService", "Failed to commit (internal)", "error"), false);
    return bRet(error, QString(), true);
}

bool UserService::commonCheck(const Translator &t, QString *error) const
{
    if (!isValid())
        return bRet(error, t.translate("UserService", "Invalid UserService instance (internal)", "error"), false);
    return bRet(error, QString(), true);
}

bool UserService::confirmRegistration(const BUuid &code, QDateTime &dt, const QLocale &locale, QString *error)
{
    Translator t(locale);
    if (code.isNull())
        return bRet(error, t.translate("UserService", "Invalid registration confirmation code", "error"), false);
    bool ok = false;
    RegistrationConfirmationCode codeEntity = RegistrationConfirmationCodeRepo->findOneByCode(code, &ok);
    if (!ok) {
        return bRet(error, t.translate("UserService", "Failed to get registration confirmation code (internal)",
                                       "error"), false);
    }
    if (!codeEntity.isValid())
        return bRet(error, t.translate("UserService", "No such code", "error"), false);
    User userEntity = UserRepo->findOne(codeEntity.userId(), &ok);
    if (!ok || !userEntity.isValid())
        return bRet(error, t.translate("UserService", "Failed to get user (internal)", "error"), false);
    userEntity.convertToCreatedByUser();
    userEntity.setActive(true);
    dt = UserRepo->edit(userEntity, &ok);
    if (!ok)
        return bRet(error, t.translate("UserService", "Failed to edit user (internal)", "error"), false);
    RegistrationConfirmationCodeRepo->deleteOneByUserId(codeEntity.userId(), &ok);
    if (!ok) {
        return bRet(error, t.translate("UserService", "Failed to delete registration confirmation code (internal)",
                                       "error"), false);
    }
    return bRet(error, QString(), true);
}

TGroupInfoList UserService::getAllGroups(bool *ok)
{
    TGroupInfoList groups;
    if (!isValid())
        return bRet(ok, false, groups);
    bool b = false;
    QList<Group> entityList = GroupRepo->findAll(QDateTime(), &b);
    if (!b)
        return bRet(ok, false, groups);
    foreach (const Group &entity, entityList) {
        groups << groupToGroupInfo(entity, &b);
        if (!b)
            return bRet(ok, false, TGroupInfoList());
    }
    return bRet(ok, true, groups);
}

TGroupInfoList UserService::getGroups(const TIdList &ids, bool *ok)
{
    TGroupInfoList groups;
    if (!isValid())
        return bRet(ok, false, groups);
    bool b = false;
    QList<Group> entityList = GroupRepo->findAll(ids, &b);
    if (!b)
        return bRet(ok, false, groups);
    foreach (const Group &entity, entityList) {
        groups << groupToGroupInfo(entity, &b);
        if (!b)
            return bRet(ok, false, TGroupInfoList());
    }
    return bRet(ok, true, groups);
}

TGroupInfoList UserService::getGroups(quint64 userId, bool *ok)
{
    TGroupInfoList groups;
    if (!isValid())
        return bRet(ok, false, groups);
    bool b = false;
    QList<Group> entityList = GroupRepo->findAllByUserId(userId, QDateTime(), &b);
    if (!b)
        return bRet(ok, false, groups);
    foreach (const Group &entity, entityList) {
        groups << groupToGroupInfo(entity, &b);
        if (!b)
            return bRet(ok, false, TGroupInfoList());
    }
    return bRet(ok, true, groups);
}

TGroupInfo UserService::groupToGroupInfo(const Group &entity, bool *ok)
{
    TGroupInfo info;
    if (!isValid() || !entity.isValid() || !entity.isCreatedByRepo())
        return bRet(ok, false, info);
    info.setCreationDateTime(entity.creationDateTime());
    info.setLastModificationDateTime(entity.lastModificationDateTime());
    info.setId(entity.id());
    info.setName(entity.name());
    info.setOwnerId(entity.ownerId());
    bool b = false;
    info.setOwnerLogin(UserRepo->findLogin(entity.ownerId(), &b));
    if (!b)
        return bRet(ok, false, TGroupInfo());
    return bRet(ok, true, info);
}

TInviteInfo UserService::inviteCodeToInviteInfo(const InviteCode &entity, bool *ok)
{
    TInviteInfo info;
    if (!isValid() || !entity.isValid() || !entity.isCreatedByRepo())
        return bRet(ok, false, info);
    info.setAccessLevel(entity.accessLevel());
    info.setCode(entity.code());
    info.setCreationDateTime(entity.creationDateTime());
    info.setExpirationDateTime(entity.expirationDateTime());
    bool b = false;
    info.setGroups(getGroups(entity.groups(), &b));
    if (!b)
        return bRet(ok, false, TInviteInfo());
    info.setId(entity.id());
    info.setOwnerId(entity.ownerId());
    info.setOwnerLogin(UserRepo->findLogin(entity.ownerId(), &b));
    if (!b)
        return bRet(ok, false, TInviteInfo());
    info.setServices(entity.availableServices());
    return bRet(ok, true, info);
}

TUserInfo UserService::userToUserInfo(const User &entity, bool includeEmail, bool *ok)
{
    TUserInfo info;
    if (!isValid() || !entity.isValid() || !entity.isCreatedByRepo())
        return bRet(ok, false, info);
    info.setAccessLevel(entity.accessLevel());
    bool superuser = entity.accessLevel().level() >= TAccessLevel::SuperuserLevel;
    info.setActive(entity.active());
    bool b = false;
    info.setAvailableGroups(superuser ? getAllGroups(&b) : getGroups(entity.id(), &b));
    if (!b)
        return bRet(ok, false, TUserInfo());
    info.setAvailableServices(superuser ? TServiceList::allServices() : entity.availableServices());
    info.setAvatar(entity.avatar());
    if (includeEmail)
        info.setEmail(entity.email());
    info.setGroups(getGroups(entity.groups(), &b));
    if (!b)
        return bRet(ok, false, TUserInfo());
    info.setId(entity.id());
    info.setLastModificationDateTime(entity.lastModificationDateTime());
    info.setLogin(entity.login());
    info.setName(entity.name());
    info.setPatronymic(entity.patronymic());
    info.setRegistrationDateTime(entity.registrationDateTime());
    info.setSurname(entity.surname());
    return bRet(ok, true, info);
}
