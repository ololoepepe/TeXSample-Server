#include "registrationserver.h"
#include "registrationconnection.h"

#include <BNetworkServer>
#include <BGenericServer>
#include <BNetworkConnection>
#include <BGenericSocket>
#include <BSpamNotifier>
#include <BeQt>

#include <QObject>

/*============================================================================
================================ RegistrationServer ==========================
============================================================================*/

/*============================== Public constructors =======================*/

RegistrationServer::RegistrationServer(QObject *parent) :
    BNetworkServer(BGenericServer::TcpServer, parent)
{
    spamNotifier()->setCheckInterval(BeQt::Second);
    spamNotifier()->setEventLimit(5);
    spamNotifier()->setEnabled(true);
}

/*============================== Protected methods =========================*/

BNetworkConnection *RegistrationServer::createConnection(BGenericSocket *socket)
{
    return new RegistrationConnection(this, socket);
}
