#ifndef SERVER_H
#define SERVER_H

class BGenericSocket;
class BNetworkConnection;

#include <BLogger>
#include <BNetworkServer>

#include <QObject>
#include <QString>

#define sServer Server::instance()

/*============================================================================
================================ Server ======================================
============================================================================*/

class Server : public BNetworkServer
{
    Q_OBJECT
private:
    const QString Location;
public:
    explicit Server(const QString &location, QObject *parent = 0);
protected:
    BNetworkConnection *createConnection(BGenericSocket *socket);
    BGenericSocket *createSocket();
public:
    static Server *instance();
    static void sendLogRequest(const QString &text, BLogger::Level lvl = BLogger::NoLevel);
};

#endif // SERVER_H
