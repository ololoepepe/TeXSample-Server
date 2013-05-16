#include "terminaliohandler.h"
#include "server.h"
#include "registrationserver.h"
#include "remotecontrolserver.h"
#include "logger.h"

#include <TeXSampleGlobal>

#include <BCoreApplication>
#include <BDirTools>
#include <BTranslator>

#include <QObject>
#include <QString>
#include <QStringList>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QDateTime>
#include <QRegExp>

#include <QDebug>

bool testDatabase();

int main(int argc, char *argv[])
{
    tRegister();
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("TeXSample Server");
    QCoreApplication::setApplicationVersion("1.0.0-beta2");
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
    QStringList args = QCoreApplication::arguments().mid(1);
    bool local = !args.contains("--remote-server") && !args.contains("-R");
    BCoreApplication bapp;
    if (local)
        BDirTools::createUserLocations(QStringList() << "samples" << "tmp" << "users" << "logs");
    BCoreApplication::logger()->setDateTimeFormat("yyyy.MM.dd hh:mm:ss");
    QString logfn = BCoreApplication::location(BCoreApplication::DataPath, BCoreApplication::UserResources) + "/logs/";
    logfn += QDateTime::currentDateTime().toString("yyyy.MM.dd-hh.mm.ss") + ".txt";
    BCoreApplication::logger()->setFileName(logfn);
    BCoreApplication::installTranslator( new BTranslator("beqt") );
    BCoreApplication::installTranslator( new BTranslator("texsample-server") );
    BCoreApplication::loadSettings();
    if (local)
    {
        if (!testDatabase())
            return 0;
        BCoreApplication::setLogger(new Logger);
    }
    TerminalIOHandler handler(local);
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
