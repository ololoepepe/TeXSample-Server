#ifndef REGISTRATIONSERVER_H
#define REGISTRATIONSERVER_H

class BNetworkConnection;
class BGenericSocket;

#include <BNetworkServer>

#include <QObject>

/*============================================================================
================================ RegistrationServer ==========================
============================================================================*/

class RegistrationServer : public BNetworkServer
{
    Q_OBJECT
public:
    explicit RegistrationServer(QObject *parent = 0);
protected:
    BNetworkConnection *createConnection(BGenericSocket *socket);
};

#endif // REGISTRATIONSERVER_H
