#include "terminaliohandler.h"
#include "server.h"
#include "registrationserver.h"
#include "remotecontrolserver.h"
#include "logger.h"
#include "storage.h"

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
    bool local = !args.contains("--remote") && !args.contains("-R");
    BCoreApplication bapp;
    TerminalIOHandler::writeLine(QCoreApplication::translate("main", "This is", "") + " "
                                 + QCoreApplication::applicationName() +
                                 " v" + QCoreApplication::applicationVersion());
    TerminalIOHandler::writeLine(local ? QCoreApplication::translate("main", "Mode: normal", "") :
                                         QCoreApplication::translate("main", "Mode: remote", ""));
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
        TerminalIOHandler::write(QCoreApplication::translate("main", "Initializing storage...", "") + " ");
        QString errs;
        if (!Storage::initStorage(BCoreApplication::location(BCoreApplication::DataPath,
                                                             BCoreApplication::UserResources), &errs))
        {
            TerminalIOHandler::writeLine(QCoreApplication::translate("main", "Error:", "") + " " + errs);
            return 0;
        }
        else
        {
            TerminalIOHandler::writeLine(QCoreApplication::translate("main", "Success!", ""));
        }
        BCoreApplication::setLogger(new Logger);
    }
    TerminalIOHandler handler(local);
    if (!local)
    {
        int ind = args.indexOf("--remote");
        if (ind < 0)
            ind = args.indexOf("-R");
        if (ind >= 0 && ind < args.size() - 1)
            handler.connectToHost(args.at(ind + 1));
    }
    ret = app.exec();
    BCoreApplication::saveSettings();
    return ret;
}
