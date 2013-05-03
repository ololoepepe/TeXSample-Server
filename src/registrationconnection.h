#ifndef REGISTRATIONCONNECTION_H
#define REGISTRATIONCONNECTION_H

class Storage;

class BNetworkServer;
class BGenericSocket;
class BNetworkOperation;

class QString;

#include <BNetworkConnection>

#include <QObject>

/*============================================================================
================================ RegistrationConnection ======================
============================================================================*/

class RegistrationConnection : public BNetworkConnection
{
    Q_OBJECT
public:
    explicit RegistrationConnection(BNetworkServer *server, BGenericSocket *socket);
    ~RegistrationConnection();
private:
    void handleRegisterRequest(BNetworkOperation *op);
private:
    Storage *mstorage;
};

#endif // REGISTRATIONCONNECTION_H
