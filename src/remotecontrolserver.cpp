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

/*============================================================================
================================ RemoteControlServer =========================
============================================================================*/

/*============================== Static public methods =====================*/

RemoteControlServer *RemoteControlServer::instance()
{
    return minstance;
}

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

void RemoteControlServer::sendOutput(const QString &text)
{
    if (text.isEmpty())
        return;
    foreach (BNetworkConnection *c, connections())
        QMetaObject::invokeMethod(c, "sendOutputRequest", Qt::QueuedConnection, Q_ARG(QString, text));
}

/*============================== Protected methods =========================*/

BNetworkConnection *RemoteControlServer::createConnection(BGenericSocket *socket)
{
    return new RemoteControlConnection(this, socket);
}

/*============================== Static private variables ==================*/

RemoteControlServer *RemoteControlServer::minstance = 0;
