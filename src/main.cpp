#include "src/userserver.h"
#include "src/databaseinteractor.h"

#include <bstdio.h>
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
    QCoreApplication::setApplicationVersion("0.1.0pa1");
    QCoreApplication::setOrganizationName("Andrey Bogdanov");
    QCoreApplication::setOrganizationDomain("https://github.com/the-dark-angel/TeXSample-Server");
    //initializing BCore
#if defined(Q_OS_MAC)
    QCoreApplication::addLibraryPath("/usr/lib/texsample-server/qt4/plugins"); //TODO
    BCore::setSharedRoot("/usr/share/texsample-server"); //TODO
    BCore::setUserRoot(QDir::homePath() + "/Library/Application Support/TeXSample Server");
#elif defined(Q_OS_UNIX)
    QCoreApplication::addLibraryPath("/usr/lib/texsample-server/qt4/plugins");
    BCore::setSharedRoot("/usr/share/texsample-server");
    BCore::setUserRoot(QDir::homePath() + "/.texsample-server");
#elif defined(Q_OS_WIN)
    BCore::setSharedRoot( QCoreApplication::applicationDirPath() );
    BCore::setUserRoot(QDir::homePath() + "/TeXSample Server");
#endif
    BCore::setPath("translations", "translations");
    BCore::setPath("samples", "samples");
    BCore::createUserPath("translations");
    BCore::createUserPath("samples");
    BCore::addStandardTranslator(BCore::QtTranslator);
    BCore::addStandardTranslator(BCore::BCoreTranslator);
    BCore::addStandardTranslator(BCore::BNetworkTranslator);
    BCore::addTranslator(":/res/translations/texsample-server");
    BCore::addTranslator(BCore::user("translations") + "/texsample-server");
    BCore::loadSettings();
    BCore::setLocale( QLocale::system() );
    //starting server
    BStdIO::write(QObject::tr("Login:", "stdout text") + " ");
    QString login = BStdIO::readLine();
    if ( login.isEmpty() )
    {
        BStdIO::write(QObject::tr("Invalid login.", "stdout text") + "\n");
        return 0;
    }
    BStdIO::write(QObject::tr("Password:", "stdout text") + " ");
    BStdIO::setStdinEchoEnabled(false);
    QString password = BStdIO::readLine();
    BStdIO::setStdinEchoEnabled(true);
    BStdIO::write("\n");
    if ( password.isEmpty() )
    {
        BStdIO::write(QObject::tr("Invalid password.", "stdout text") + "\n");
        return 0;
    }
    DatabaseInteractor::setAdminInfo(login, password);
    if ( !DatabaseInteractor::checkAdmin() )
    {
        BStdIO::write(QObject::tr("Failed to connect to database.", "stdout text") + "\n");
        return 0;
    }
    UserServer *srv = new UserServer;
    int ret = app->exec();
    srv->deleteLater();
    BCore::saveSettings();
    return ret;
}
