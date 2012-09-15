#ifndef USERSERVER_H
#define USERSERVER_H

class BNetworkServerWorker;

class QObject;

#include <bnetworkserver.h>

class UserServer : public BNetworkServer
{
    Q_OBJECT
public:
    explicit UserServer(QObject *parent = 0);
private:
    BNetworkServerWorker *createWorker();
};

/*
#include <QTcpServer>
#include <QString>
#include <QList>
#include <QStringList>
#include <QMap>
#include <QTimer>

class Server : public QTcpServer
{
    Q_OBJECT
public:
    static bool checkAdmin();
    //
    explicit Server(QObject *parent = 0);
public slots:
    void saveSettings();
protected:
    void incomingConnection(int handle);
private:
    typedef bool (Server::*Handler)(const QStringList &);
    //
    QMap<QString, Handler> mHandlerMap;
    QList<ConnectionThread *> mConnections;
    QStringList mbanList;
    QTimer *mtimerAutosave;
    //
    void loadSettings();
    bool restart();
    bool handleSaveSettings(const QStringList &arguments);
    bool handleBanned(const QStringList &arguments);
    bool handleBan(const QStringList &arguments);
    bool handleUnban(const QStringList &arguments);
    bool handleConnections(const QStringList &arguments);
    bool handleRestart(const QStringList &arguments);
    bool handleExit(const QStringList &arguments);
private slots:
    void read(const QString &text);
    void finished();
    void updateNotify();
};
*/

#endif // USERSERVER_H
