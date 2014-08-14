#ifndef USERSERVICE_H
#define USERSERVICE_H

class DataSource;
class GroupRepository;
class UserRepository;

class TGroupInfoList;
class TIdList;

class QString;

#include "requestin.h"
#include "requestout.h"

#include <TAddGroupReplyData>
#include <TAddGroupRequestData>
#include <TGetUserInfoAdminReplyData>
#include <TGetUserInfoAdminRequestData>
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
    DataSource * const Source;
    GroupRepository * const GroupRepo;
    UserRepository * const UserRepo;
public:
    explicit UserService(DataSource *source);
    ~UserService();
public:
    RequestOut<TAddGroupReplyData> addGroup(const RequestIn<TAddGroupRequestData> &in, quint64 userId);
    bool checkOutdatedEntries(QString *error = 0);
    DataSource *dataSource() const;
    RequestOut<TGetUserInfoReplyData> getUserInfo(const RequestIn<TGetUserInfoRequestData> &in);
    RequestOut<TGetUserInfoAdminReplyData> getUserInfoAdmin(const RequestIn<TGetUserInfoAdminRequestData> &in);
    bool initializeRoot(QString *error = 0);
    bool isRootInitialized();
    bool isValid() const;
private:
    TGroupInfoList getGroups(const TIdList &ids, bool *ok = 0);
private:
    Q_DISABLE_COPY(UserService)
};

#endif // USERSERVICE_H
