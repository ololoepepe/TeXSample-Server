#include "terminaliohandler.h"
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
    QCoreApplication::setApplicationVersion("1.0.1");
    QCoreApplication::setOrganizationName("TeXSample Team");
    QCoreApplication::setOrganizationDomain("https://github.com/TeXSample-Team/TeXSample-Server");
#if defined(BUILTIN_RESOURCES)
    Q_INIT_RESOURCE(texsample_server);
    Q_INIT_RESOURCE(texsample_server_translations);
#endif
    int ret = 0;
    BCoreApplication bapp;
    Q_UNUSED(bapp);
    TerminalIOHandler::writeLine(QCoreApplication::translate("main", "This is", "") + " "
                                 + QCoreApplication::applicationName() +
                                 " v" + QCoreApplication::applicationVersion());
    BDirTools::createUserLocations(QStringList() << "samples" << "users" << "logs");
    BCoreApplication::logger()->setDateTimeFormat("yyyy.MM.dd hh:mm:ss");
    QString logfn = BCoreApplication::location(BCoreApplication::DataPath, BCoreApplication::UserResources) + "/logs/";
    logfn += QDateTime::currentDateTime().toString("yyyy.MM.dd-hh.mm.ss") + ".txt";
    BCoreApplication::logger()->setFileName(logfn);
    BCoreApplication::installTranslator(new BTranslator("qt"));
    BCoreApplication::installTranslator(new BTranslator("beqt"));
    BCoreApplication::installTranslator(new BTranslator("texsample"));
    BCoreApplication::installTranslator(new BTranslator("texsample-server"));
    TerminalIOHandler::write(QCoreApplication::translate("main", "Initializing storage...", "") + " ");
    TerminalIOHandler handler;
    Q_UNUSED(handler)
    QString errs;
    if (!Storage::initStorage(&errs))
    {
        TerminalIOHandler::writeLine(QCoreApplication::translate("main", "Error:", "") + " " + errs);
        return 0;
    }
    TerminalIOHandler::writeLine(QCoreApplication::translate("main", "Success!", ""));
    TerminalIOHandler::writeLine(QCoreApplication::translate("main", "Please, don't forget to set e-mail password "
                                                             "and start the server"));
    ret = app.exec();
    return ret;
}
