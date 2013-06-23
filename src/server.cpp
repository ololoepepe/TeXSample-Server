#include "server.h"
#include "connection.h"
#include "terminaliohandler.h"

#include <BNetworkServer>
#include <BGenericServer>
#include <BNetworkConnection>
#include <BGenericSocket>
#include <BSpamNotifier>
#include <BeQt>

#include <QObject>
#include <QTcpSocket>

/*============================================================================
================================ Server ======================================
============================================================================*/

/*============================== Static public methods =====================*/

void Server::sendWriteRequest(const QString &text)
{
    if (!TerminalIOHandler::instance())
        return;
    Server *s = static_cast<TerminalIOHandler *>(TerminalIOHandler::instance())->server();
    if (!s)
        return;
    foreach (BNetworkConnection *c, s->connections())
    {
        Connection *cc = static_cast<Connection *>(c);
        cc->sendWriteRequest(text);
    }
}

void Server::sendWriteLineRequest(const QString &text)
{
    sendWriteRequest(text + "\n");
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
