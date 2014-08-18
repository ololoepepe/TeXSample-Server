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
public:
    enum UserMatchType
    {
        MatchLogin = 0x01,
        MatchUniqueId = 0x02,
        MatchLoginAndUniqueId = MatchLogin | MatchUniqueId
    };
private:
    const QString Location;
public:
    explicit Server(const QString &location, QObject *parent = 0);
public:
    TUserConnectionInfoList userConnections(const QString &matchPattern, UserMatchType type = MatchLoginAndUniqueId,
                                            int *total = 0) const;
protected:
    BNetworkConnection *createConnection(BGenericSocket *socket, const QString &serverAddress, quint16 serverPort);
    BGenericSocket *createSocket();
};

#endif // SERVER_H
