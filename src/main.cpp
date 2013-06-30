#include "terminaliohandler.h"
#include "storage.h"
#include "application.h"

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

QString translate(const char *context, const char *key, const char *disambiguation = 0)
{
    return QCoreApplication::translate(context, key, disambiguation);
}

int main(int argc, char *argv[])
{
    tRegister();
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("TeXSample Server");
    QCoreApplication::setApplicationVersion("1.0.3");
    QCoreApplication::setOrganizationName("TeXSample Team");
    QCoreApplication::setOrganizationDomain("https://github.com/TeXSample-Team/TeXSample-Server");
#if (QT_VERSION < QT_VERSION_CHECK(5, 0, 0))
    BApplicationServer s(9930 + qHash(QDir::home().dirName()) % 10);
#else
    BApplicationServer s(QCoreApplication::applicationName() + QDir::home().dirName());
#endif
    int ret = 0;
    if (!s.testServer())
    {
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
    TerminalIOHandler::writeLine(translate("main", "This is") + " " + QCoreApplication::applicationName() +
                                 " v" + QCoreApplication::applicationVersion());
    BDirTools::createUserLocations(QStringList() << "samples" << "users" << "logs");
    Application::logger()->setDateTimeFormat("yyyy.MM.dd hh:mm:ss");
    QString logfn = Application::location(Application::DataPath, Application::UserResources) + "/logs/";
    logfn += QDateTime::currentDateTime().toString("yyyy.MM.dd-hh.mm.ss") + ".txt";
    Application::logger()->setFileName(logfn);
    TerminalIOHandler::write(QCoreApplication::translate("main", "Initializing storage...", "") + " ");
    TerminalIOHandler handler;
    Q_UNUSED(handler)
    QString errs;
    if (!Storage::initStorage(&errs))
    {
        TerminalIOHandler::writeLine(translate("main", "Error:") + " " + errs);
        return 0;
    }
    TerminalIOHandler::writeLine(translate("main", "Success!"));
    TerminalIOHandler::writeLine(translate("main", "Please, don't forget to configure e-mail and start the server"));
    TerminalIOHandler::writeLine(translate("main", "Enter \"help\" to see commands list"));
    ret = app.exec();
#if defined(BUILTIN_RESOURCES)
     Q_CLEANUP_RESOURCE(texsample_server);
     Q_CLEANUP_RESOURCE(texsample_server_translations);
#endif
    }
    else
    {
        BTerminalIOHandler::writeLine(translate("main", "Another instance of") + " "
                                      + QCoreApplication::applicationName() + " "
                                      + translate("main", "is already running. Quitting..."));
    }
    tCleanup();
    return ret;
}
