#include "userservice.h"

#include "datasource.h"
#include "entity/group.h"
#include "entity/user.h"
#include "global.h"
#include "repository/accountrecoverycoderepository.h"
#include "repository/grouprepository.h"
#include "repository/invitecoderepository.h"
#include "repository/registrationconfirmationcoderepository.h"
#include "repository/userrepository.h"
#include "requestin.h"
#include "requestout.h"

#include <TAccessLevel>
#include <TAddGroupReplyData>
#include <TAddGroupRequestData>
#include <TeXSample>
#include <TGetUserInfoAdminReplyData>
#include <TGetUserInfoAdminRequestData>
#include <TGetUserInfoReplyData>
#include <TGetUserInfoRequestData>
#include <TGroupInfo>
#include <TGroupInfoList>
#include <TServiceList>
#include <TUserIdentifier>
#include <TUserInfo>

#include <BTerminal>

#include <QDateTime>
#include <QImage>
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
    if (!isValid())
        return Out(tr("Invalid UserService instance", "error"));
    if (!in.data().isValid())
        return Out(tr("Invalid data", "error"));
    if (!userId)
        return Out(tr("Invalid user ID (internal error)", "error"));
    Group entity;
    QDateTime dt = QDateTime::currentDateTime();
    entity.setCreationDateTime(dt);
    entity.setName(in.data().name());
    entity.setOwnerId(userId);
    quint64 id = GroupRepo->add(entity);
    if (!id)
        return Out(tr("Repository operation failed (internal error)", "error"));
    entity = GroupRepo->findOne(id);
    if (!entity.isValid())
        return Out(tr("Repository operation failed (internal error)", "error"));
    TGroupInfo info;
    info.setCreationDateTime(entity.creationDateTime());
    info.setId(entity.id());
    info.setLastModificationDateTime(entity.lastModificationDateTime());
    info.setName(entity.name());
    info.setOwnerId(entity.ownerId());
    info.setOwnerLogin(UserRepo->findLogin(entity.ownerId()));
    TAddGroupReplyData replyData;
    replyData.setGroupInfo(info);
    return Out(replyData, dt);
}

bool UserService::checkOutdatedEntries()
{
    return isValid() && AccountRecoveryCodeRepo->deleteExpired() && InviteCodeRepo->deleteExpired()
            && RegistrationConfirmationCodeRepo->deleteExpired();
}

DataSource *UserService::dataSource() const
{
    return Source;
}

RequestOut<TGetUserInfoReplyData> UserService::getUserInfo(const RequestIn<TGetUserInfoRequestData> &in)
{
    typedef RequestOut<TGetUserInfoReplyData> Out;
    if (!isValid())
        return Out(tr("Invalid UserService instance", "error"));
    if (!in.data().isValid())
        return Out(tr("Invalid data", "error"));
    TUserIdentifier id = in.data().identifier();
    if (in.cachingEnabled() && in.lastRequestDateTime().isValid()) {
        QDateTime dt = QDateTime::currentDateTime();
        if (in.lastRequestDateTime() >= UserRepo->findLastModificationDateTime(id))
            return Out(dt);
    }
    QDateTime dt = QDateTime::currentDateTime();
    User entity = UserRepo->findOne(id);
    if (!entity.isValid())
        return Out(tr("No such user", "error"));
    TUserInfo info;
    info.setAccessLevel(entity.accessLevel());
    info.setActive(entity.active());
    info.setAvailableGroups(getGroups(entity.id()));
    info.setAvailableServices(entity.availableServices());
    if (in.data().includeAvatar())
        info.setAvatar(entity.avatar());
    info.setGroups(getGroups(entity.groups()));
    info.setId(entity.id());
    info.setLastModificationDateTime(entity.lastModificationDateTime());
    info.setLogin(entity.login());
    info.setName(entity.name());
    info.setPatronymic(entity.patronymic());
    info.setRegistrationDateTime(entity.registrationDateTime());
    info.setSurname(entity.surname());
    TGetUserInfoReplyData replyData;
    replyData.setUserInfo(info);
    return Out(replyData, dt);
}

RequestOut<TGetUserInfoAdminReplyData> UserService::getUserInfoAdmin(const RequestIn<TGetUserInfoAdminRequestData> &in)
{
    typedef RequestOut<TGetUserInfoAdminReplyData> Out;
    if (!isValid())
        return Out(tr("Invalid UserService instance", "error"));
    if (!in.data().isValid())
        return Out(tr("Invalid data", "error"));
    TUserIdentifier id = in.data().identifier();
    if (in.cachingEnabled() && in.lastRequestDateTime().isValid()) {
        QDateTime dt = QDateTime::currentDateTime();
        if (in.lastRequestDateTime() >= UserRepo->findLastModificationDateTime(id))
            return Out(dt);
    }
    QDateTime dt = QDateTime::currentDateTime();
    User entity = UserRepo->findOne(id);
    if (!entity.isValid())
        return Out(tr("No such user", "error"));
    TUserInfo info;
    info.setAccessLevel(entity.accessLevel());
    info.setActive(entity.active());
    info.setAvailableGroups(getGroups(entity.id()));
    info.setAvailableServices(entity.availableServices());
    if (in.data().includeAvatar())
        info.setAvatar(entity.avatar());
    info.setEmail(entity.email());
    info.setGroups(getGroups(entity.groups()));
    info.setId(entity.id());
    info.setLastModificationDateTime(entity.lastModificationDateTime());
    info.setLogin(entity.login());
    info.setName(entity.name());
    info.setPatronymic(entity.patronymic());
    info.setRegistrationDateTime(entity.registrationDateTime());
    info.setSurname(entity.surname());
    TGetUserInfoAdminReplyData replyData;
    replyData.setUserInfo(info);
    return Out(replyData, dt);
}

bool UserService::initializeRoot(QString *error)
{
    bool users = UserRepo->countByAccessLevel(TAccessLevel::SuperuserLevel);
    if (Global::readOnly() && !users)
        return bRet(error, tr("Can't create users in read-only mode", "error"), false);
    if (!Global::readOnly() && !checkOutdatedEntries())
        return bRet(error, tr("Failed to delete outdated entries", "error"), false);
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
    QString patronymic = bReadLine(tr("Enter superuser patronymic [default: \"\"]:", "prompt") + " ");
    if (!patronymic.isEmpty() && !Texsample::testName(patronymic, error))
        return false;
    QString surname = bReadLine(tr("Enter superuser surname [default: \"\"]:", "prompt") + " ");
    if (!surname.isEmpty() && !Texsample::testName(surname, error))
        return false;
    User entity;
    entity.setLogin(login);
    entity.setEmail(email);
    entity.setPassword(Texsample::encryptPassword(password));
    entity.setName(name);
    entity.setPatronymic(patronymic);
    entity.setSurname(surname);
    entity.setAccessLevel(TAccessLevel::SuperuserLevel);
    entity.setActive(true);
    bWriteLine(tr("Creating superuser account...", "message"));
    if (!UserRepo->add(entity))
        return bRet(error, tr("Failed to create superuser", "error"), false);
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

/*============================== Private methods ===========================*/

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
    info.setId(entity.id());
    info.setName(entity.name());
    info.setOwnerId(entity.ownerId());
    info.setOwnerLogin(UserRepo->findLogin(entity.ownerId()));
    return info;
}
