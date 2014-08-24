#include "userservice.h"

#include "application.h"
#include "datasource.h"
#include "entity/group.h"
#include "entity/invitecode.h"
#include "entity/registrationconfirmationcode.h"
#include "entity/user.h"
#include "repository/accountrecoverycoderepository.h"
#include "repository/grouprepository.h"
#include "repository/invitecoderepository.h"
#include "repository/registrationconfirmationcoderepository.h"
#include "repository/userrepository.h"
#include "requestin.h"
#include "requestout.h"
#include "settings.h"
#include "transactionholder.h"
#include "translator.h"

#include <TAccessLevel>
#include <TAddGroupReplyData>
#include <TAddGroupRequestData>
#include <TAddUserReplyData>
#include <TAddUserRequestData>
#include <TAuthorizeReplyData>
#include <TAuthorizeRequestData>
#include <TClientInfo>
#include <TConfirmRegistrationReplyData>
#include <TConfirmRegistrationRequestData>
#include <TeXSample>
#include <TGenerateInvitesReplyData>
#include <TGenerateInvitesRequestData>
#include <TGetInviteInfoListReplyData>
#include <TGetInviteInfoListRequestData>
#include <TGetSelfInfoReplyData>
#include <TGetSelfInfoRequestData>
#include <TGetUserInfoAdminReplyData>
#include <TGetUserInfoAdminRequestData>
#include <TGetUserInfoListAdminReplyData>
#include <TGetUserInfoListAdminRequestData>
#include <TGetUserInfoReplyData>
#include <TGetUserInfoRequestData>
#include <TGroupInfo>
#include <TGroupInfoList>
#include <TInviteInfo>
#include <TInviteInfoList>
#include <TRegisterReplyData>
#include <TRegisterRequestData>
#include <TRequest>
#include <TServiceList>
#include <TUserIdentifier>
#include <TUserInfo>
#include <TUserInfoList>

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
    AccountRecoveryCodeRepo(new AccountRecoveryCodeRepository(source)), GroupRepo(new GroupRepository(source)),
    InviteCodeRepo(new InviteCodeRepository(source)),
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
    if (!holder.doCommit())
        return Out(t.translate("UserService", "Failed to commit (internal)", "error"));
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
    if (!holder.doCommit())
        return Out(t.translate("UserService", "Failed to commit (internal)", "error"));
    TUserInfo info = userToUserInfo(entity, true, false);
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
    TUserInfo info = userToUserInfo(entity, true, false);
    TAuthorizeReplyData replyData;
    replyData.setUserInfo(info);
    return Out(replyData, dt);
}

bool UserService::checkOutdatedEntries()
{
    if (!isValid())
        return false;
    TransactionHolder holder(Source);
    if (!AccountRecoveryCodeRepo->deleteExpired() || !InviteCodeRepo->deleteExpired())
        return false;
    foreach (const RegistrationConfirmationCode &entity, RegistrationConfirmationCodeRepo->findExpired()) {
        if (!UserRepo->deleteOne(entity.userId()))
            return false;
    }
    return RegistrationConfirmationCodeRepo->deleteExpired() && holder.doCommit();
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
    if (!holder.doCommit())
        return Out(t.translate("UserService", "Failed to commit (internal)", "error"));
    replyData.setSuccess(true);
    return Out(replyData, dt);
}

DataSource *UserService::dataSource() const
{
    return Source;
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
    if (!holder.doCommit())
        return Out(t.translate("UserService", "Failed to commit (internal)", "error"));
    TGenerateInvitesReplyData replyData;
    TInviteInfoList infoList;
    foreach (const InviteCode &e, entityList)
        infoList << inviteCodeToInviteInfo(e);
    replyData.setGeneratedInvites(infoList);
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
    QList<InviteCode> entityList = InviteCodeRepo->findAllByOwnerId(userId);
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
    replyData.setUserInfo(userToUserInfo(entity, true, true));
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
    replyData.setUserInfo(userToUserInfo(entity, false, in.data().includeAvatar()));
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
    replyData.setUserInfo(userToUserInfo(entity, true, in.data().includeAvatar()));
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
        newUsers << userToUserInfo(entity, true, false);
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
    if (!holder.doCommit())
        return bRet(error, tr("Failed to commit (internal)", "error"), false);
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
    if (!holder.doCommit())
        return Out(t.translate("UserService", "Failed to commit (internal)", "error"));
    TUserInfo info = userToUserInfo(entity, true, false);
    TRegisterReplyData replyData;
    replyData.setUserInfo(info);
    return Out(replyData, info.registrationDateTime());
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

bool UserService::commonCheck(const Translator &translator, QString *error) const
{
    if (!isValid())
        return bRet(error, translator.translate("UserService", "Invalid UserService instance (internal)", "error"),
                    false);
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
    if (!RegistrationConfirmationCodeRepo->deleteOneByUserId(codeEntity.userId()))
        return bRet(error, t.translate("UserService", "Failed to remove registration confirmation code (internal)",
                                       "error"), false);
    return bRet(error, QString(), true);
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

TUserInfo UserService::userToUserInfo(const User &entity, bool includeEmail, bool includeAvatar)
{
    TUserInfo info;
    if (!isValid() || !entity.isValid() || !entity.isCreatedByRepo())
        return info;
    info.setAccessLevel(entity.accessLevel());
    info.setActive(entity.active());
    info.setAvailableGroups(getGroups(entity.id()));
    info.setAvailableServices(entity.availableServices());
    if (includeAvatar)
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
