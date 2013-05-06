#ifndef CONNECTION_H
#define CONNECTION_H

class Database;
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
    static QString sampleSourceFileName(quint64 id);
    static QString samplePreviewFileName(quint64 id);
    static QStringList sampleAuxiliaryFileNames(quint64 id);
    static bool addFile(QVariantMap &target, const QString &fileName);
    static bool addFile(QVariantList &target, const QString &fileName);
    static int addFiles(QVariantMap &target, const QStringList &fileNames);
    static bool addTextFile(QVariantMap &target, const QString &fileName);
    static QString tmpPath(const QUuid &uuid);
    static bool testUserInfo(const QVariantMap &m, bool isNew = false);
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
    bool checkRights(TAccessLevel minLevel = TAccessLevel::UserLevel) const;
    quint64 userId(const QString &login);
    QString userLogin(quint64 id);
    void retOk(BNetworkOperation *op, const QVariantMap &out, const QString &msg = QString());
    void retOk(BNetworkOperation *op, QVariantMap &out, const QString &key, const QVariant &value,
               const QString &msg = QString());
    void retErr(BNetworkOperation *op, const QVariantMap &out, const QString &msg = QString());
    void retErr(BNetworkOperation *op, QVariantMap &out, const QString &key, const QVariant &value,
                const QString &msg = QString());
private slots:
    void testAuthorization();
private:
    static const int MaxAvatarSize;
private:
    Storage *mstorage;
    QString mlogin;
    quint64 muserId;
    TAccessLevel maccessLevel;
    TClientInfo mclientInfo;
    bool mauthorized;
    Database *mdb;
};

#endif // CONNECTION_H
