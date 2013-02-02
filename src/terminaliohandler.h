#ifndef TERMINALIOHANDLER_H
#define TERMINALIOHANDLER_H

class Server;

class QString;
class QStringList;

#include <BTerminalIOHandler>

#include <QObject>

/*============================================================================
================================ TerminalIOHandler ===========================
============================================================================*/

class TerminalIOHandler : public BTerminalIOHandler
{
    Q_OBJECT
public:
    explicit TerminalIOHandler(Server *server, QObject *parent = 0);
private:
    void handleQuit(const QString &cmd, const QStringList &args);
    void handleUser(const QString &cmd, const QStringList &args);
private:
    Server *mserver;
};

#endif // TERMINALIOHANDLER_H
