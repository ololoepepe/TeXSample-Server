#include "src/connectionthread.h"
#include "src/connection.h"

#include <bstdio.h>

#include <QThread>
#include <QTcpSocket>
#include <QString>
#include <QEventLoop>
#include <QMetaObject>
#include <QStringList>
#include <QHostAddress>

ConnectionThread::ConnectionThread(int socketDescriptor, QObject *parent) :
    QThread(parent), mCSocketDescriptor(socketDescriptor)
{
    mconnection = 0;
}

//

Connection *ConnectionThread::connection() const
{
    return mconnection;
}

//

void ConnectionThread::requestUpdateNotify()
{
    QMetaObject::invokeMethod(mconnection, "requestUpdateNotify", Qt::QueuedConnection);
}

//

void ConnectionThread::run()
{
    QEventLoop *loop = new QEventLoop;
    QTcpSocket *socket = new QTcpSocket;
    socket->setSocketDescriptor(mCSocketDescriptor);
    QString addr = socket->peerAddress().toString();
    if ( addr.isEmpty() || Connection::banList(true).contains(addr) )
    {
        loop->deleteLater();
        socket->deleteLater();
        //TODO
        return;
    }
    mconnection = new Connection(socket);
    if ( mconnection->isConnected() )
    {
        connect( mconnection, SIGNAL( disconnected() ), loop, SLOT( quit() ) );
        connect(mconnection, SIGNAL( updateNotify() ),
                this, SIGNAL( updateNotify() ), Qt::QueuedConnection);
        loop->exec();
    }
    mconnection->deleteLater();
    loop->deleteLater();
}
