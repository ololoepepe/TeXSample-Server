#ifndef TERMINALIOHANDLER_H
#define TERMINALIOHANDLER_H

class Server;
class RegistrationServer;

class BNetworkConnection;
class BNetworkOperation;

class QStringList;

#include <BTerminalIOHandler>

#include <QObject>
#include <QElapsedTimer>
#include <QString>
#include <QAbstractSocket>

/*============================================================================
================================ TerminalIOHandler ===========================
============================================================================*/

class TerminalIOHandler : public BTerminalIOHandler
{
    Q_OBJECT
public:
    static void write(const QString &text);
    static void writeLine(const QString &text = QString());
public:
    explicit TerminalIOHandler(bool local, QObject *parent = 0);
    ~TerminalIOHandler();
public:
    void executeCommand(const QString &cmd, const QStringList &args);
    void connectToHost(const QString &hostName);
    Server *server() const;
private:
    void handleQuit(const QString &cmd, const QStringList &args);
    void handleUser(const QString &cmd, const QStringList &args);
    void handleUptime(const QString &cmd, const QStringList &args);
    void handleConnect(const QString &cmd, const QStringList &args);
    void handleDisconnect(const QString &cmd, const QStringList &args);
    void handleRemote(const QString &cmd, const QStringList &args);
private slots:
    void connectToHost(const QString &hostName, const QString &login, const QString &password);
    void disconnectFromHost();
    void sendCommand(const QString &cmd, const QStringList &args);
    void disconnected();
    void error(QAbstractSocket::SocketError err);
    void remoteRequest(BNetworkOperation *op);
private:
    Server *mserver;
    RegistrationServer *mrserver;
    BNetworkConnection *mremote;
    quint64 muserId;
    QElapsedTimer melapsedTimer;
};

#endif // TERMINALIOHANDLER_H
