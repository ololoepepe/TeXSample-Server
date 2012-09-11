#ifndef CONNECTIONTHREAD_H
#define CONNECTIONTHREAD_H

class Connection;

#include <QThread>
#include <QString>
#include <QStringList>

class ConnectionThread : public QThread
{
    Q_OBJECT
public:
    ConnectionThread(int socketDescriptor, QObject *parent = 0);
    //
    void requestUpdateNotify();
    Connection *connection() const;
protected:
    void run();
private:
    const int mCSocketDescriptor;
    //
    Connection *mconnection;
signals:
    void updateNotify();
};

#endif // CONNECTIONTHREAD_H
