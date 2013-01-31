#ifndef TERMINALIOHANDLER_H
#define TERMINALIOHANDLER_H

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
    explicit TerminalIOHandler(QObject *parent = 0);
private:
    void handleQuit(const QString &cmd, const QStringList &args);
};

#endif // TERMINALIOHANDLER_H
