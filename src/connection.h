#ifndef CONNECTION_H
#define CONNECTION_H

class Database;

class BNetworkServer;
class BGenericSocket;
class BNetworkOperation;

class QSqlDatabase;
class QUuid;
class QByteArray;

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
    QString info() const;
protected:
    void log(const QString &text, BLogger::Level lvl = BLogger::NoLevel);
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
    struct Info
    {
        QString osVersion;
        QString editorVersion;
        QString beqtVersion;
        QString qtVersion;
        //
        Info(const QVariantMap &map = QVariantMap())
        {
            osVersion = map.value("os_ver").toString();
            editorVersion = map.value("editor_ver").toString();
            beqtVersion = map.value("beqt_ver").toString();
            qtVersion = map.value("qt_ver").toString();
        }
    };
private:
    static QString sampleSourceFileName(quint64 id);
    static QString samplePreviewFileName(quint64 id);
    static QStringList sampleAuxiliaryFileNames(quint64 id);
    static bool addFile(QVariantMap &target, const QString &fileName);
    static bool addFile(QVariantList &target, const QString &fileName);
    static int addFiles(QVariantMap &target, const QStringList &fileNames);
    static bool addTextFile(QVariantMap &target, const QString &fileName);
    static QString tmpPath(const QUuid &uuid);
    static bool compileSample(const QString &path, const QVariantMap &in, QString *log = 0);
    static bool compile(const QString &path, const QVariantMap &in, int *exitCode = 0, QString *log = 0);
    static bool testUserInfo(const QVariantMap &m, bool isNew = false);
    static int execSampleCompiler(const QString &path, const QString &jobName, QString *log = 0);
    static int execProjectCompiler(const QString &path, const QString &fileName, const QString &cmd,
                                   const QStringList &commands, const QStringList &options, QString *log = 0);
    static int execTool(const QString &path, const QString &fileName, const QString &tool);
    static bool saveUserAvatar(quint64 id, const QByteArray &avatar);
    static QByteArray loadUserAvatar(quint64 id, bool *ok = 0);
    static bool loadUserAvatar(quint64 id, QByteArray &avatar);
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
    void handleGetInvitesListRequest(BNetworkOperation *op);
    void handleAddUserRequest(BNetworkOperation *op);
    void handleGetUserInfoRequest(BNetworkOperation *op);
    void handleCompileRequest(BNetworkOperation *op);
    bool checkRights(AccessLevel minLevel = UserLevel) const;
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
    bool mauthorized;
    QString mlogin;
    quint64 muserId;
    int maccessLevel;
    Database *mdb;
    Info minfo;
};

#endif // CONNECTION_H
