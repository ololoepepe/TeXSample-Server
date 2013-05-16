#include "remotecontrolserver.h"
#include "remotecontrolconnection.h"

#include <BNetworkServer>
#include <BGenericServer>
#include <BNetworkConnection>
#include <BGenericSocket>
#include <BSpamNotifier>
#include <BeQt>

#include <QObject>
#include <QString>
#include <QMetaObject>
#include <QList>

/*============================================================================
================================ RemoteControlServer =========================
============================================================================*/

/*============================== Public constructors =======================*/

RemoteControlServer::RemoteControlServer(QObject *parent) :
    BNetworkServer(BGenericServer::TcpServer, parent)
{
    spamNotifier()->setCheckInterval(BeQt::Second);
    spamNotifier()->setEventLimit(5);
    spamNotifier()->setEnabled(true);
    minstance = this;
}

/*============================== Public methods ============================*/

RemoteControlConnection *RemoteControlServer::connection() const
{
    foreach (BNetworkConnection *c, connections())
    {
        RemoteControlConnection *cc = static_cast<RemoteControlConnection *>(c);
        if (cc->isAuthorized())
            return cc;
    }
    return 0;
}

/*============================== Protected methods =========================*/

BNetworkConnection *RemoteControlServer::createConnection(BGenericSocket *socket)
{
    return new RemoteControlConnection(this, socket);
}

/*============================== Static private variables ==================*/

RemoteControlServer *RemoteControlServer::minstance = 0;
