#ifndef CONNECTION_H
#define CONNECTION_H

class Storage;

class TOperationResult;

class BNetworkServer;
class BGenericSocket;
class BNetworkOperation;

class QSqlDatabase;
class QUuid;
class QByteArray;

#include "translator.h"

#include <TClientInfo>
#include <TAccessLevel>

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
    void sendWriteRequest(const QString &text);
    QString translate(const char *context, const char *sourceText, const char *disambiguation = 0, int n = -1);
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
    void handleAuthorizeRequest(BNetworkOperation *op);
    void handleAddUserRequest(BNetworkOperation *op);
    void handleEditUserRequest(BNetworkOperation *op);
    void handleUpdateAccountRequest(BNetworkOperation *op);
    void handleGetUserInfoRequest(BNetworkOperation *op);
    void handleAddSampleRequest(BNetworkOperation *op);
    void handleEditSampleRequest(BNetworkOperation *op);
    void handleUpdateSampleRequest(BNetworkOperation *op);
    void handleDeleteSampleRequest(BNetworkOperation *op);
    void handleGetSamplesListRequest(BNetworkOperation *op);
    void handleGetSampleSourceRequest(BNetworkOperation *op);
    void handleGetSamplePreviewRequest(BNetworkOperation *op);
    void handleGenerateInvitesRequest(BNetworkOperation *op);
    void handleGetInvitesListRequest(BNetworkOperation *op);
    void handleCompileProjectRequest(BNetworkOperation *op);
    void handleSubscribeRequest(BNetworkOperation *op);
    void handleExecuteCommandRequest(BNetworkOperation *op);
    void handleChangeLocale(BNetworkOperation *op);
private slots:
    void testAuthorization();
    void restartTimer(BNetworkOperation *op = 0);
    void keepAlive();
    void sendLogRequestInternal(const QString &text, int lvl);
    void sendWriteRequestInternal(const QString &text);
private:
    Storage *mstorage;
    Translator mtranslator;
    QString mlogin;
    quint64 muserId;
    TAccessLevel maccessLevel;
    TClientInfo mclientInfo;
    bool msubscribed;
    QTimer mtimer;
    QElapsedTimer muptimeTimer;
    QDateTime mconnectedAt;
};

#endif // CONNECTION_H
