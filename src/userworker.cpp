#include "src/userworker.h"
#include "src/userconnection.h"

#include <bnetworkserverworker.h>
#include <bnetworkconnection.h>
#include <bgenericsocket.h>

UserWorker::UserWorker(QObject *parent) :
    BNetworkServerWorker(parent)
{
}

//

BNetworkConnection *UserWorker::createConnection(int socketDescriptor)
{
    BGenericSocket *socket = new BGenericSocket(BGenericSocket::TcpSocket);
    if ( !socket->setSocketDescriptor(socketDescriptor) || !socket->isValid() )
    {
        socket->deleteLater();
        return 0;
    }
    BNetworkConnection *connection = new UserConnection(socket);
    if ( !connection->isConnected() )
    {
        connection->deleteLater();
        connection = 0;
    }
    return connection;
}
