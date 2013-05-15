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
    QString login() const;
    TClientInfo clientInfo() const;
    QString infoString(const QString &format = "") const; //"%l - login, %p - address, %u - id, %a - access level
protected:
    void log(const QString &text, BLogger::Level lvl = BLogger::NoLevel);
private:
    static TOperationResult notAuthorizedResult();
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
private slots:
    void testAuthorization();
private:
    Storage *mstorage;
    QString mlogin;
    quint64 muserId;
    TAccessLevel maccessLevel;
    TClientInfo mclientInfo;
};

#endif // CONNECTION_H
