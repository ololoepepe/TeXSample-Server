#ifndef REMOTECONTROLSERVER_H
#define REMOTECONTROLSERVER_H

class RemoteControlConnection;

class BNetworkConnection;
class BGenericSocket;

class QString;

#include <BNetworkServer>

#include <QObject>

/*============================================================================
================================ RemoteControlServer =========================
============================================================================*/

class RemoteControlServer : public BNetworkServer
{
    Q_OBJECT
public:
    explicit RemoteControlServer(QObject *parent = 0);
public:
    RemoteControlConnection *connection() const;
protected:
    BNetworkConnection *createConnection(BGenericSocket *socket);
private:
    static RemoteControlServer *minstance;
};

#endif // REMOTECONTROLSERVER_H
