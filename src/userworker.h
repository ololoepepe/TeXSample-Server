#ifndef USERWORKER_H
#define USERWORKER_H

class BNetworkConnection;

class QObject;

#include <bnetworkserverworker.h>

class UserWorker : public BNetworkServerWorker
{
    Q_OBJECT
public:
    explicit UserWorker(QObject *parent = 0);
protected:
    BNetworkConnection *createConnection(int socketDescriptor);
};

#endif // USERWORKER_H
