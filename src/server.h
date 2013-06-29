#ifndef SERVER_H
#define SERVER_H

class BNetworkConnection;
class BGenericSocket;

#include <BNetworkServer>

#include <QObject>

/*============================================================================
================================ Server ======================================
============================================================================*/

class Server : public BNetworkServer
{
    Q_OBJECT
public:
    explicit Server(QObject *parent = 0);
protected:
    BNetworkConnection *createConnection(BGenericSocket *socket);
    BGenericSocket *createSocket();
};

#endif // SERVER_H
