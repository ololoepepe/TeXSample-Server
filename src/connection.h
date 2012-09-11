#ifndef CONNECTION_H
#define CONNECTION_H

class QTcpSocket;

#include <bnetworkconnection.h>

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

#endif // CONNECTION_H
