#include "server.h"
#include "connection.h"
#include "terminaliohandler.h"
#include "application.h"

#include <BNetworkServer>
#include <BGenericServer>
#include <BNetworkConnection>
#include <BGenericSocket>
#include <BSpamNotifier>
#include <BeQt>
#include <BLogger>

#include <QObject>
#include <QTcpSocket>
#include <QList>

/*============================================================================
================================ Server ======================================
============================================================================*/

/*============================== Static public methods =====================*/

Server *Server::instance()
{
    return Application::server();
}

void Server::sendLogRequest(const QString &text, BLogger::Level lvl)
{
    Server *s = instance();
    if (s)
        return;
    s->lock();
    foreach (BNetworkConnection *c, s->connections())
    {
        Connection *cc = static_cast<Connection *>(c);
        cc->sendLogRequest(text, lvl);
    }
    s->unlock();
}

/*============================== Public constructors =======================*/

Server::Server(QObject *parent) :
    BNetworkServer(BGenericServer::TcpServer, parent)
{
    spamNotifier()->setCheckInterval(BeQt::Second);
    spamNotifier()->setEventLimit(5);
    spamNotifier()->setEnabled(true);
}

/*============================== Protected methods =========================*/

BNetworkConnection *Server::createConnection(BGenericSocket *socket)
{
    return new Connection(this, socket);
}

BGenericSocket *Server::createSocket()
{
    BGenericSocket *s = new BGenericSocket(BGenericSocket::TcpSocket);
    s->tcpSocket()->setSocketOption(QTcpSocket::KeepAliveOption, 1);
    return s;
}
