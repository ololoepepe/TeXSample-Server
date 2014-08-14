#ifndef SERVER_H
#define SERVER_H

class TUserConnectionInfoList;

class BGenericSocket;
class BNetworkConnection;

#include <BLogger>
#include <BNetworkServer>

#include <QObject>
#include <QString>

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
public:
    TUserConnectionInfoList userConnections() const;
protected:
    BNetworkConnection *createConnection(BGenericSocket *socket);
    BGenericSocket *createSocket();
};

#endif // SERVER_H
