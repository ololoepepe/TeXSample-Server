#include "terminaliohandler.h"
#include "storage.h"
#include "application.h"
#include "global.h"

#include <TeXSampleGlobal>

#include <BCoreApplication>
#include <BDirTools>
#include <BTranslator>
#include <BApplicationServer>

#include <QObject>
#include <QString>
#include <QStringList>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QDateTime>
#include <QRegExp>
#include <QHash>

#include <QDebug>

B_DECLARE_TRANSLATE_FUNCTION

int main(int argc, char *argv[])
{
    tInit();
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("TeXSample Server");
    QCoreApplication::setApplicationVersion("2.0.0-a1");
    QCoreApplication::setOrganizationName("TeXSample Team");
    QCoreApplication::setOrganizationDomain("https://github.com/TeXSample-Team/TeXSample-Server");
    QString home = QDir::home().dirName();
    BApplicationServer s(9930 + qHash(home) % 10, QCoreApplication::applicationName() + home);
    int ret = 0;
    if (Global::readOnly() || !s.testServer())
    {
        if (!Global::readOnly())
            s.listen();
#if defined(BUILTIN_RESOURCES)
        Q_INIT_RESOURCE(texsample_server);
        Q_INIT_RESOURCE(texsample_server_translations);
#endif
        Application bapp;
        Q_UNUSED(bapp);
        Application::installTranslator(new BTranslator("qt"));
        Application::installTranslator(new BTranslator("beqt"));
        Application::installTranslator(new BTranslator("texsample"));
        Application::installTranslator(new BTranslator("texsample-server"));
        QString msg = QCoreApplication::applicationName() + " v" + QCoreApplication::applicationVersion();
        if (Global::readOnly())
            msg += " (" + translate("main", "read-only mode") + ")";
        BTerminalIOHandler::setTerminalTitle(msg);
        bWriteLine(translate("main", "This is") + " " + msg);
        BDirTools::createUserLocations(QStringList() << "samples" << "labs" << "users" << "logs");
        Application::logger()->setDateTimeFormat("yyyy.MM.dd hh:mm:ss");
        Global::resetLoggingMode();
        QString logfn = Application::location(Application::DataPath, Application::UserResources) + "/logs/";
        logfn += QDateTime::currentDateTime().toString("yyyy.MM.dd-hh.mm.ss") + ".txt";
        Application::logger()->setFileName(logfn);
        TerminalIOHandler handler;
        Q_UNUSED(handler)
        QString errs;
        if ((!Global::noMail() && !Global::initMail(&errs)) || !Storage::initStorage(&errs))
        {
            bWriteLine(translate("main", "Error:") + " " + errs);
            return 0;
        }
        bWriteLine(translate("main", "Enter \"help --commands\" to see the list of available commands"));
        ret = app.exec();
#if defined(BUILTIN_RESOURCES)
        Q_CLEANUP_RESOURCE(texsample_server);
        Q_CLEANUP_RESOURCE(texsample_server_translations);
#endif
    }
    else
    {
        bWriteLine(translate("main", "Another instance of") + " "  + QCoreApplication::applicationName() + " "
                   + translate("main", "is already running. Quitting..."));
    }
    tCleanup();
    return ret;
}
