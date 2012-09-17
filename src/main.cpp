#include "src/userserver.h"
#include "src/databaseinteractor.h"
#include "include/texsampleserver.h"

#include <bterminaliohandler.h>
#include <bcore.h>

#include <QCoreApplication>
#include <QStringList>
#include <QString>
#include <QObject>
#include <QDir>

#include <QDebug>

#define connect QObject::connect

int main(int argc, char **argv)
{
    //initializing application
    QCoreApplication *app = new QCoreApplication(argc, argv);
    QCoreApplication::setApplicationName("TexSample Server");
    QCoreApplication::setApplicationVersion("1.0.0pa1");
    QCoreApplication::setOrganizationName("Andrey Bogdanov");
    QCoreApplication::setOrganizationDomain("https://github.com/the-dark-angel/TeXSample-Server");
    //initializing BCore
#if defined(Q_OS_UNIX)
    QCoreApplication::addLibraryPath("/usr/lib/texsample-server/qt4/plugins");
#endif
    BCore::init();
    BCore::setPath("samples", "samples");
    BCore::createUserPath("samples");
    BCore::loadSettings();
    BCore::setLocale( QLocale::system() );
    //starting server
    BTerminalIOHandler::writeLine( QObject::tr("This is TeXSample Server version", "stdout text") +
                                   " " + QCoreApplication::applicationVersion() );
    BTerminalIOHandler::write(QObject::tr("Login:", "stdout text") + " ");
    QString login = BTerminalIOHandler::readLine();
    if ( login.isEmpty() )
    {
        BTerminalIOHandler::writeLine( QObject::tr("Invalid login", "stdout text") );
        return 0;
    }
    BTerminalIOHandler::write(QObject::tr("Password:", "stdout text") + " ");
    BTerminalIOHandler::setStdinEchoEnabled(false);
    QString password = BTerminalIOHandler::readLine();
    BTerminalIOHandler::setStdinEchoEnabled(true);
    BTerminalIOHandler::write("\n");
    if ( password.isEmpty() )
    {
        BTerminalIOHandler::writeLine( QObject::tr("Invalid password", "stdout text") );
        return 0;
    }
    DatabaseInteractor::setAdminInfo(login, password);
    if ( !DatabaseInteractor::checkAdmin() )
    {
        BTerminalIOHandler::writeLine( QObject::tr("Failed to connect to database", "stdout text") );
        return 0;
    }
    UserServer *srv = new UserServer;
    int ret = 0;
    if ( srv->listen("0.0.0.0", TexSampleServer::ServerPort) )
    {
        BTerminalIOHandler::writeLine( QObject::tr("Server successfully started", "stdout text") );
        ret = app->exec();
    }
    else
    {
        BTerminalIOHandler::writeLine( QObject::tr("Failed to start server", "stdout text") );
        ret = 1;
    }
    srv->deleteLater();
    BCore::saveSettings();
    return ret;
}
