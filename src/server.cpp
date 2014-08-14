#include "server.h"

#include "connection.h"

#include <TUserConnectionInfo>
#include <TUserConnectionInfoList>
#include <TUserInfo>

#include <BGenericServer>
#include <BGenericSocket>
#include <BNetworkServer>
#include <BUuid>

#include <QDateTime>
#include <QObject>
#include <QString>
#include <QTcpSocket>

/*============================================================================
================================ Server ======================================
============================================================================*/

/*============================== Public constructors =======================*/

Server::Server(const QString &location, QObject *parent) :
    BNetworkServer(BGenericServer::TcpServer, parent), Location(location)
{
    //
}

/*============================== Static public methods =====================*/

TUserConnectionInfoList Server::userConnections() const
{
    TUserConnectionInfoList list;
    const_cast<Server *>(this)->lock();
    foreach (BNetworkConnection *c, connections()) {
        Connection *cc = static_cast<Connection *>(c);
        TUserConnectionInfo info;
        info.setClientInfo(cc->clientInfo());
        info.setConnectionDateTime(cc->connectionDateTime());
        info.setPeerAddress(cc->peerAddress());
        info.setUniqueId(cc->uniqueId());
        info.setUptime(cc->uptime());
        info.setUserInfo(cc->userInfo());
        list << info;
    }
    const_cast<Server *>(this)->unlock();
    return list;
}

/*============================== Protected methods =========================*/

BNetworkConnection *Server::createConnection(BGenericSocket *socket)
{
    return new Connection(this, socket, Location);
}

BGenericSocket *Server::createSocket()
{
    BGenericSocket *s = new BGenericSocket(BGenericSocket::TcpSocket);
    s->tcpSocket()->setSocketOption(QTcpSocket::KeepAliveOption, 1);
    return s;
}
