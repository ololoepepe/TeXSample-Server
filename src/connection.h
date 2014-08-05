#ifndef CONNECTION_H
#define CONNECTION_H

class DataSource;
class UserService;

class TReply;

class BNetworkServer;
class BGenericSocket;
class BNetworkOperation;

class QString;

#include <TClientInfo>
#include <TUserInfo>

#include <BLogger>
#include <BNetworkConnection>

#include <QDateTime>
#include <QElapsedTimer>
#include <QLocale>
#include <QMap>
#include <QObject>
#include <QTimer>

/*============================================================================
================================ Connection ==================================
============================================================================*/

class Connection : public BNetworkConnection
{
    Q_OBJECT
public:
    enum Subscription
    {
        LogSubscription = 1
    };
private:
    enum LogTarget
    {
        NoTarget = 0,
        LocalOnly,
        RemoteOnly,
        LocalAndRemote
    };
private:
    DataSource * const Source;
    UserService * const UserServ;
private:
    TClientInfo mclientInfo;
    QDateTime mconnectionDT;
    QLocale mlocale;
    QMap<Subscription, bool> msubscriptions;
    QTimer mtimer;
    QElapsedTimer muptimeTimer;
    TUserInfo muserInfo;
public:
    explicit Connection(BNetworkServer *server, BGenericSocket *socket, const QString &location);
    ~Connection();
public:
    TClientInfo clientInfo() const;
    QDateTime connectionDT(Qt::TimeSpec spec = Qt::UTC) const;
    bool isSubscribed(Subscription subscription) const;
    QLocale locale() const;
    void sendLogRequest(const QString &text, BLogger::Level lvl = BLogger::NoLevel);
    qint64 uptime() const;
    TUserInfo userInfo() const;
protected:
    void log(const QString &text, BLogger::Level lvl = BLogger::NoLevel);
    void logLocal(const QString &text, BLogger::Level lvl = BLogger::NoLevel);
    void logRemote(const QString &text, BLogger::Level lvl = BLogger::NoLevel);
private:
    bool handleAddGroupRequest(BNetworkOperation *op);
    bool handleAddLabRequest(BNetworkOperation *op);
    bool handleAddSampleRequest(BNetworkOperation *op);
    bool handleAddUserRequest(BNetworkOperation *op);
    bool handleAuthorizeRequest(BNetworkOperation *op);
    bool handleChangeEmailRequest(BNetworkOperation *op);
    bool handleChangePasswordRequest(BNetworkOperation *op);
    bool handleCheckEmailAvailabilityRequest(BNetworkOperation *op);
    bool handleCheckLoginAvailabilityRequest(BNetworkOperation *op);
    bool handleCompileTexProjectRequest(BNetworkOperation *op);
    bool handleConfirmRegistrationRequest(BNetworkOperation *op);
    bool handleDeleteGroupRequest(BNetworkOperation *op);
    bool handleDeleteInvitesRequest(BNetworkOperation *op);
    bool handleDeleteLabRequest(BNetworkOperation *op);
    bool handleDeleteSampleRequest(BNetworkOperation *op);
    bool handleDeleteUserRequest(BNetworkOperation *op);
    bool handleEditGroupRequest(BNetworkOperation *op);
    bool handleEditLabRequest(BNetworkOperation *op);
    bool handleEditSampleRequest(BNetworkOperation *op);
    bool handleEditSampleAdminRequest(BNetworkOperation *op);
    bool handleEditSelfRequest(BNetworkOperation *op);
    bool handleEditUserRequest(BNetworkOperation *op);
    bool handleGenerateInvitesRequest(BNetworkOperation *op);
    bool handleGetGroupInfoListRequest(BNetworkOperation *op);
    bool handleGetInviteInfoListRequest(BNetworkOperation *op);
    bool handleGetLabDataRequest(BNetworkOperation *op);
    bool handleGetLabExtraFileRequest(BNetworkOperation *op);
    bool handleGetLabInfoListRequest(BNetworkOperation *op);
    bool handleGetLatestAppVersionRequest(BNetworkOperation *op);
    bool handleGetSampleInfoListRequest(BNetworkOperation *op);
    bool handleGetSamplePreviewRequest(BNetworkOperation *op);
    bool handleGetSampleSourceRequest(BNetworkOperation *op);
    bool handleGetSelfInfoRequest(BNetworkOperation *op);
    bool handleGetUserAvatarRequest(BNetworkOperation *op);
    bool handleGetUserInfoAdminRequest(BNetworkOperation *op);
    bool handleGetUserInfoRequest(BNetworkOperation *op);
    bool handleRecoverAccountRequest(BNetworkOperation *op);
    bool handleRegisterRequest(BNetworkOperation *op);
    bool handleRequestRecoveryCodeRequest(BNetworkOperation *op);
    bool handleSubscribeRequest(BNetworkOperation *op);
    void initHandlers();
    bool sendReply(BNetworkOperation *op, const TReply &reply);
    bool sendReply(BNetworkOperation *op, const QString &message, bool success = false);
private slots:
    void keepAlive();
    void restartTimer(BNetworkOperation *op = 0);
    void sendLogRequestInternal(const QString &text, int lvl);
    void testAuthorization();
};

#endif // CONNECTION_H
