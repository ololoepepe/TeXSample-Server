#ifndef REMOTECONTROLCONNECTION_H
#define REMOTECONTROLCONNECTION_H

class Storage;

class BNetworkServer;
class BGenericSocket;
class BNetworkOperation;

class QString;

#include <BNetworkConnection>

#include <QObject>

/*============================================================================
================================ RemoteControlConnection =====================
============================================================================*/

class RemoteControlConnection : public BNetworkConnection
{
    Q_OBJECT
public:
    explicit RemoteControlConnection(BNetworkServer *server, BGenericSocket *socket);
    ~RemoteControlConnection();
public:
    bool isAuthorized() const;
public slots:
    void sendLogRequest(const QString &text, bool stderrLevel);
    void sendWriteRequest(const QString &text);
private:
    void handleAuthorizeRequest(BNetworkOperation *op);
    void handleExecuteCommandRequest(BNetworkOperation *op);
private slots:
    void testAuthorization();
private:
    Storage *mstorage;
    quint64 muserId;
};

#endif // REMOTECONTROLCONNECTION_H
