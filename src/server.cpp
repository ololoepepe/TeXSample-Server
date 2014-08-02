#include "server.h"

#include "application.h"
#include "connection.h"

#include <BGenericServer>
#include <BGenericSocket>
#include <BLogger>
#include <BNetworkConnection>
#include <BNetworkServer>

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

Server *Server::instance()
{
    return Application::server();
}

void Server::sendLogRequest(const QString &text, BLogger::Level lvl)
{
    Server *s = instance();
    if (!s)
        return;
    s->lock();
    foreach (BNetworkConnection *c, s->connections()) {
        Connection *cc = static_cast<Connection *>(c);
        cc->sendLogRequest(text, lvl);
    }
    s->unlock();
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
