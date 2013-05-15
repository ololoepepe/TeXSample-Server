#ifndef REMOTECONTROLSERVER_H
#define REMOTECONTROLSERVER_H

class BNetworkConnection;
class BGenericSocket;

class QString;

#include <BNetworkServer>

#include <QObject>

#define sRCServer RemoteControlServer::instance()

/*============================================================================
================================ RemoteControlServer =========================
============================================================================*/

class RemoteControlServer : public BNetworkServer
{
    Q_OBJECT
public:
    static RemoteControlServer *instance();
public:
    explicit RemoteControlServer(QObject *parent = 0);
public:
    void sendOutput(const QString &text);
protected:
    BNetworkConnection *createConnection(BGenericSocket *socket);
private:
    static RemoteControlServer *minstance;
};

#endif // REMOTECONTROLSERVER_H
