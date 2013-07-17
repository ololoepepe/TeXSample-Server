#ifndef SERVER_H
#define SERVER_H

class QString;

class BNetworkConnection;
class BGenericSocket;

#include <BNetworkServer>
#include <BLogger>

#include <QObject>

#define sServer Server::instance()

/*============================================================================
================================ Server ======================================
============================================================================*/

class Server : public BNetworkServer
{
    Q_OBJECT
public:
    static Server *instance();
    static void sendLogRequest(const QString &text, BLogger::Level lvl = BLogger::NoLevel);
public:
    explicit Server(QObject *parent = 0);
protected:
    BNetworkConnection *createConnection(BGenericSocket *socket);
    BGenericSocket *createSocket();
};

#endif // SERVER_H
