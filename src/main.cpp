#include "terminaliohandler.h"
#include "server.h"
#include "registrationserver.h"

#include <TeXSampleGlobal>

#include <BCoreApplication>
#include <BDirTools>
#include <BTranslator>
#include <BLogger>

#include <QObject>
#include <QString>
#include <QStringList>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>

#include <QDebug>

bool testDatabase();

int main(int argc, char *argv[])
{
    tRegister();
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("TeXSample Server");
    QCoreApplication::setApplicationVersion("1.0.0-beta1");
    QCoreApplication::setOrganizationName("TeXSample Team");
    QCoreApplication::setOrganizationDomain("https://github.com/TeXSample-Team/TeXSample-Server");
#if defined(BUILTIN_RESOURCES)
    Q_INIT_RESOURCE(texsample_server);
    Q_INIT_RESOURCE(texsample_server_translations);
#endif
#if defined(Q_OS_UNIX)
    QCoreApplication::addLibraryPath( QDir( QCoreApplication::applicationDirPath() +
                                            "../lib/texsample-server" ).absolutePath() );
#endif
    int ret = 0;
    BCoreApplication bapp;
    BCoreApplication::logger()->setDateTimeFormat("yyyy.MM.dd hh:mm:ss");
    BCoreApplication::logger()->setFileName(BCoreApplication::location(BCoreApplication::DataPath,
                                                                       BCoreApplication::UserResources) + "/log.txt");
    BCoreApplication::installTranslator( new BTranslator("beqt") );
    BCoreApplication::installTranslator( new BTranslator("texsample-server") );
    BDirTools::createUserLocations(QStringList() << "samples" << "tmp" << "users");
    BCoreApplication::loadSettings();
    if ( !testDatabase() )
        return 0;
    Server server;
    TerminalIOHandler handler(&server);
    server.listen("0.0.0.0", 9041);
    RegistrationServer rserver;
    rserver.listen("0.0.0.0", 9042);
    ret = app.exec();
    BCoreApplication::saveSettings();
    return ret;
}

bool testDatabase()
{
    bLog("Testing database existence");
    QString fn = BDirTools::findResource("texsample.sqlite", BDirTools::UserOnly);
    QFileInfo fi(fn);
    if ( fn.isEmpty() || !fi.exists() || !fi.isFile() || !fi.size() )
    {
        bLog("Database does not exist", BLogger::FatalLevel);
        return false;
    }
    bLog("Database exists, OK");
    return true;
}
