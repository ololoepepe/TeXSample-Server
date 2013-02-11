#ifndef CONNECTION_H
#define CONNECTION_H

class BNetworkServer;
class BGenericSocket;
class BNetworkOperation;

class QSqlDatabase;

#include <BNetworkConnection>

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
private:
    enum AccessLevel
    {
        NoLevel = 0,
        UserLevel = 10,
        ModeratorLevel = 100,
        AdminLevel = 1000
    };
    enum SampleType
    {
        Unverified = 0,
        Approved,
        Rejected
    };
private:
    static QString sampleSourceFileName(quint64 id);
    static QString samplePreviewFileName(quint64 id);
    static QStringList sampleAuxiliaryFileNames(quint64 id);
    static bool addFile(QVariantMap &target, const QString &fileName);
    static bool addFile(QVariantList &target, const QString &fileName);
    static int addFiles(QVariantMap &target, const QStringList &fileNames);
    static bool addTextFile(QVariantMap &target, const QString &fileName);
    static QString userTmpPath(const QString &login);
    static bool compileSample(const QString &path, const QVariantMap &in, QString *log = 0);
    static bool testUserInfo(const QVariantMap &m, bool isNew = false);
private:
    void handleRegisterRequest(BNetworkOperation *op);
    void handleAuthorizeRequest(BNetworkOperation *op);
    void handleGetSamplesListRequest(BNetworkOperation *op);
    void handleGetSampleSourceRequest(BNetworkOperation *op);
    void handleGetSamplePreviewRequest(BNetworkOperation *op);
    void handleAddSampleRequest(BNetworkOperation *op);
    void handleUpdateSampleRequest(BNetworkOperation *op);
    void handleDeleteSampleRequest(BNetworkOperation *op);
    void handleUpdateAccountRequest(BNetworkOperation *op);
    void handleGenerateInviteRequest(BNetworkOperation *op);
    void handleAddUserRequest(BNetworkOperation *op);
    void handleGetUserInfoRequest(BNetworkOperation *op);
    bool checkRights(AccessLevel minLevel = UserLevel) const;
    void retOk( BNetworkOperation *op, const QVariantMap &out, const QString &msg = QString() );
    void retOk( BNetworkOperation *op, QVariantMap &out, const QString &key, const QVariant &value,
                const QString &msg = QString() );
    void retErr( BNetworkOperation *op, const QVariantMap &out, const QString &msg = QString() );
    void retErr( BNetworkOperation *op, QVariantMap &out, const QString &key, const QVariant &value,
                 const QString &msg = QString() );
    bool beginDBOperation();
    bool endDBOperation(bool success = true);
    bool execQuery(const QString &query, QVariantMap &values, const QVariantMap &boundValues = QVariantMap(),
                   QVariant *insertId = 0);
    bool execQuery(const QString &query, QVariantList &values, const QVariantMap &boundValues = QVariantMap(),
                   QVariant *insertId = 0);
    bool execQuery(const QString &query, QVariantMap &values, const QString &boundKey, const QVariant &boundValue,
                   QVariant *insertId = 0);
    bool execQuery(const QString &query, QVariantList &values, const QString &boundKey, const QVariant &boundValue,
                   QVariant *insertId = 0);
    bool execQuery( const QString &query, QVariant *insertId = 0, const QVariantMap &boundValues = QVariantMap() );
    bool execQuery(const QString &query, const QString &boundKey, const QVariant &boundValue, QVariant *insertId = 0);
private slots:
    void testAuthorization();
private:
    static const int MaxAvatarSize;
private:
    bool mauthorized;
    QString mlogin;
    int maccessLevel;
    QSqlDatabase *mdb;
};

#endif // CONNECTION_H
