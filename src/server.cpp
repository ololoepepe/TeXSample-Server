#include "server.h"
#include "connection.h"

#include <BNetworkServer>
#include <BGenericServer>
#include <BNetworkConnection>
#include <BGenericSocket>
#include <BSpamNotifier>
#include <BeQt>

#include <QObject>

/*============================================================================
================================ Server ======================================
============================================================================*/

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
