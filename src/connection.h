#ifndef CONNECTION_H
#define CONNECTION_H

class Storage;

class TOperationResult;
class TMessage;
class TCompilationResult;

class BNetworkServer;
class BGenericSocket;
class BNetworkOperation;

class QSqlDatabase;
class QUuid;
class QByteArray;

#include <TClientInfo>
#include <TAccessLevel>
#include <TServiceList>

#include <BNetworkConnection>
#include <BLogger>

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantMap>
#include <QVariantList>
#include <QVariant>
#include <QTimer>
#include <QElapsedTimer>
#include <QDateTime>
#include <QLocale>

/*============================================================================
================================ Connection ==================================
============================================================================*/

class Connection : public BNetworkConnection
{
    Q_OBJECT
public:
    explicit Connection(BNetworkServer *server, BGenericSocket *socket);
    ~Connection();
public:
    void sendLogRequest(const QString &text, BLogger::Level lvl = BLogger::NoLevel);
    void sendMessageRequest(const TMessage &msg);
    QString login() const;
    TClientInfo clientInfo() const;
    QString infoString(const QString &format = "") const;
    //%d - user id, "%u - login, %p - address, %i - id
    //%a - access level
    QDateTime connectedAt(Qt::TimeSpec spec = Qt::LocalTime) const;
    bool isSubscribed() const;
    qint64 uptime() const;
protected:
    void log(const QString &text, BLogger::Level lvl = BLogger::NoLevel);
    void logLocal(const QString &text, BLogger::Level lvl = BLogger::NoLevel);
    void logRemote(const QString &text, BLogger::Level lvl = BLogger::NoLevel);
private:
    enum LogTarget
    {
        NoTarget = 0,
        LocalOnly,
        RemoteOnly,
        LocalAndRemote
    };
private:
    bool handleRegisterRequest(BNetworkOperation *op);
    bool handleGetRecoveryCodeRequest(BNetworkOperation *op);
    bool handleRecoverAccountRequest(BNetworkOperation *op);
    bool handleAuthorizeRequest(BNetworkOperation *op);
    bool handleAddUserRequest(BNetworkOperation *op);
    bool handleEditUserRequest(BNetworkOperation *op);
    bool handleUpdateAccountRequest(BNetworkOperation *op);
    bool handleGetUserInfoRequest(BNetworkOperation *op);
    bool handleGenerateInvitesRequest(BNetworkOperation *op);
    bool handleGetInvitesListRequest(BNetworkOperation *op);
    bool handleSubscribeRequest(BNetworkOperation *op);
    bool handleChangeLocale(BNetworkOperation *op);
    bool handleStartServerRequest(BNetworkOperation *op);
    bool handleStopServerRequest(BNetworkOperation *op);
    bool handleUptimeRequest(BNetworkOperation *op);
    bool handleUserRequest(BNetworkOperation *op);
    bool handleAddSampleRequest(BNetworkOperation *op);
    bool handleEditSampleRequest(BNetworkOperation *op);
    bool handleUpdateSampleRequest(BNetworkOperation *op);
    bool handleDeleteSampleRequest(BNetworkOperation *op);
    bool handleGetSamplesListRequest(BNetworkOperation *op);
    bool handleGetSampleSourceRequest(BNetworkOperation *op);
    bool handleGetSamplePreviewRequest(BNetworkOperation *op);
    bool handleCompileProjectRequest(BNetworkOperation *op);
    bool handleEditClabGroupsRequest(BNetworkOperation *op);
    bool handleGetClabGroupsListRequest(BNetworkOperation *op);
    bool handleAddLabRequest(BNetworkOperation *op);
    bool handleEditLabRequest(BNetworkOperation *op);
    bool handleDeleteLabRequest(BNetworkOperation *op);
    bool handleGetLabsListRequest(BNetworkOperation *op);
    bool handleGetLabRequest(BNetworkOperation *op);
    bool sendReply(BNetworkOperation *op, QVariantMap out, const TOperationResult &r, LogTarget lt = LocalAndRemote,
                   const QString &prefix = QString());
    bool sendReply(BNetworkOperation *op, QVariantMap out, const TCompilationResult &r, LogTarget lt = LocalAndRemote,
                   const QString &prefix = QString());
    bool sendReply(BNetworkOperation *op, QVariantMap out, int msg, LogTarget lt = LocalAndRemote,
                   const QString &prefix = QString());
    bool sendReply(BNetworkOperation *op, const TOperationResult &r, LogTarget lt = LocalAndRemote,
                   const QString &prefix = QString());
    bool sendReply(BNetworkOperation *op, const TCompilationResult &r, LogTarget lt = LocalAndRemote,
                   const QString &prefix = QString());
    bool sendReply(BNetworkOperation *op, int msg, LogTarget lt = LocalAndRemote, const QString &prefix = QString());
    bool sendReply(BNetworkOperation *op, int msg, bool success, LogTarget lt = LocalAndRemote,
                   const QString &prefix = QString());
    bool sendReply(BNetworkOperation *op, QVariantMap out, LogTarget lt = LocalAndRemote,
                   const QString &prefix = QString());
    bool sendReply(BNetworkOperation *op, LogTarget lt = LocalAndRemote, const QString &prefix = QString());
private slots:
    void testAuthorization();
    void restartTimer(BNetworkOperation *op = 0);
    void keepAlive();
    void sendLogRequestInternal(const QString &text, int lvl);
    void sendMessageRequestInternal(int msg);
private:
    Storage *mstorage;
    QString mlogin;
    quint64 muserId;
    TAccessLevel maccessLevel;
    TServiceList mservices;
    TClientInfo mclientInfo;
    QLocale mlocale;
    bool msubscribed;
    QTimer mtimer;
    QElapsedTimer muptimeTimer;
    QDateTime mconnectedAt;
};

#endif // CONNECTION_H
