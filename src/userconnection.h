#ifndef USERCONNECTION_H
#define USERCONNECTION_H

class BGenericSocket;
class BNetworkOperation;

class QObject;

#include <bnetworkconnection.h>

#include <QByteArray>
#include <QMap>
#include <QString>
#include <QDataStream>

class UserConnection : public BNetworkConnection
{
    Q_OBJECT
public:
    explicit UserConnection(BGenericSocket *socket, QObject *parent = 0);
private:
    typedef void (UserConnection::*Handler)(BNetworkOperation *);
    //
    bool mauthorized;
    QMap<QString, Handler> mreplyHandlers;
    QMap<QString, Handler> mrequestHandlers;
    //
    //reply handlers
    void handleReplyAuthorization(BNetworkOperation *operation);
    void handleReplyGetVersion(BNetworkOperation *operation);
    //resuest handlers
    void handleRequestGetPdf(BNetworkOperation *operation);
    void handleRequestGetSample(BNetworkOperation *operation);
    //other
    bool standardCheck(const QString &id, QDataStream &out);
    void sendSample(BNetworkOperation *operation, bool pdf);
    void mySendReply( BNetworkOperation *operation, const QByteArray &data = QByteArray() );
private slots:
    void replyReceivedSlot(BNetworkOperation *operation);
    void requestReceivedSlot(BNetworkOperation *operation);
    void checkAuthorization();
};

/*
#include <QObject>
#include <QDataStream>
#include <QString>
#include <QSqlDatabase>
#include <QStringList>

class Connection : public BNetworkConnection
{
    Q_OBJECT
public:
    static void setAdminLogin(const QString &login);
    static void setAdminPassword(const QString &password);
    static void addToBanList(const QString &value, bool address = false);
    static QString adminLogin();
    static QString adminPassword();
    static QStringList banList(bool address = false);
    static QSqlDatabase *createDatabase(const QString &uniqueId);
    static void removeDatabase(QSqlDatabase *db);
    //
    Connection(QTcpSocket *socket, QObject *parent = 0);
    //
    const QString &uniqueId() const;
public slots:
    void requestUpdateNotify();
protected:
    bool authorize(const QString &login, const QString &password);
private:
    const QString mCUniqueId;
    //
    QSqlDatabase *mDatabase;
    QString mlogin;
    //
    void handleGetSampleRequest(QDataStream &in, QDataStream &out);
    void handleGetPdfRequest(QDataStream &in, QDataStream &out);
    void handleSendSampleRequest(QDataStream &in, QDataStream &out);
    void handleUpdateNotifyReply(bool success, QDataStream &in);
    void handleSendVersionReply(bool success, QDataStream &in);
    void logger(const QString &text);
private slots:
    void replySentSlot(const QString &operation);
    void emitUpdateNotify();
signals:
    void updateNotify();
};
*/

#endif // USERCONNECTION_H
