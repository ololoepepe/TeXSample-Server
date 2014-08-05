#include "userservice.h"

#include "datasource.h"
#include "entity/group.h"
#include "entity/user.h"
#include "global.h"
#include "repository/grouprepository.h"
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
    Source(source), GroupRepo(new GroupRepository(source)), UserRepo(new UserRepository(source))
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
    entity.setLastModificationDateTime(dt);
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
    info.setOwnerLogin(entity.ownerLogin());
    TAddGroupReplyData replyData;
    replyData.setGroupInfo(info);
    return Out(replyData, dt);
}

bool UserService::checkOutdatedEntries(QString *error)
{
    //
    return true;
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
        if (in.lastRequestDateTime() >= UserRepo->lastModificationDateTime(id))
            return Out(dt);
    }
    QDateTime dt = QDateTime::currentDateTime();
    User entity = UserRepo->findOne(id);
    if (!entity.isValid())
        return Out(tr("No such user", "error"));
    TUserInfo info;
    info.setAccessLevel(entity.accessLevel());
    info.setActive(entity.active());
    info.setAvailableGroups(entity.availableGroups());
    info.setAvailableServices(entity.availableServices());
    if (in.data().includeAvatar())
        info.setAvatar(entity.avatar());
    info.setGroups(entity.groups());
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
        if (in.lastRequestDateTime() >= UserRepo->lastModificationDateTime(id))
            return Out(dt);
    }
    QDateTime dt = QDateTime::currentDateTime();
    User entity = UserRepo->findOne(id);
    if (!entity.isValid())
        return Out(tr("No such user", "error"));
    TUserInfo info;
    info.setAccessLevel(entity.accessLevel());
    info.setActive(entity.active());
    info.setAvailableGroups(entity.availableGroups());
    info.setAvailableServices(entity.availableServices());
    if (in.data().includeAvatar())
        info.setAvatar(entity.avatar());
    info.setEmail(entity.email());
    info.setGroups(entity.groups());
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
    bool users = UserRepo->count(TAccessLevel::SuperuserLevel);
    if (Global::readOnly() && !users)
        return bRet(error, tr("Can't create users in read-only mode", "error"), false);
    if (!Global::readOnly() && !checkOutdatedEntries(error))
        return false;
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
    QString name = bReadLineSecure(tr("Enter superuser name [default: \"\"]:", "prompt") + " ");
    if (!name.isEmpty() && !Texsample::testName(name, error))
        return false;
    QString patronymic = bReadLineSecure(tr("Enter superuser patronymic [default: \"\"]:", "prompt") + " ");
    if (!patronymic.isEmpty() && !Texsample::testName(patronymic, error))
        return false;
    QString surname = bReadLineSecure(tr("Enter superuser surname [default: \"\"]:", "prompt") + " ");
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
    QDateTime dt = QDateTime::currentDateTime();
    entity.setRegistrationDateTime(dt);
    entity.setLastModificationDateTime(dt);
    bWriteLine(tr("Creating superuser account...", "message"));
    if (!UserRepo->save(entity))
        return bRet(error, tr("Failed to create superuser", "error"), false);
    return bRet(error, QString(), true);
}

bool UserService::isRootInitialized()
{
    return isValid() && UserRepo->count(TAccessLevel::SuperuserLevel);
}

bool UserService::isValid() const
{
    return Source && Source->isValid() && GroupRepo->isValid() && UserRepo->isValid();
}
