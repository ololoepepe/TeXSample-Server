#include "terminaliohandler.h"

#include <QObject>
#include <QString>
#include <QStringList>
#include <QCoreApplication>

/*============================================================================
================================ TerminalIOHandler ===========================
============================================================================*/

/*============================== Public constructors =======================*/

TerminalIOHandler::TerminalIOHandler(QObject *parent) :
    BTerminalIOHandler(parent)
{
    installHandler("quit", (InternalHandler) &TerminalIOHandler::handleQuit);
    installHandler("exit", (InternalHandler) &TerminalIOHandler::handleQuit);
}

/*============================== Private methods ===========================*/

void TerminalIOHandler::handleQuit(const QString &, const QStringList &)
{
    QCoreApplication::quit();
}
