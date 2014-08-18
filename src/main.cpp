#include "application.h"
#include "global.h"

#include <BApplicationServer>
#include <BTerminal>

#include <QDebug>
#include <QDir>
#include <QHash>
#include <QRegExp>
#include <QString>
#include <QStringList>

B_DECLARE_TRANSLATE_FUNCTION

int main(int argc, char *argv[])
{
    static const QString AppName = "TeXSample Server";
    QString home = QDir::home().dirName();
    BApplicationServer s(9920 + qHash(home) % 10, AppName + home);
    int ret = 0;
    QStringList args;
    foreach (int i, bRangeD(1, argc - 1))
        args << argv[i];
    if (args.contains("--read-only") || !s.testServer()) {
        s.listen();
        Application app(argc, argv, AppName, "Andrey Bogdanov");
        if ((!Global::noMail() && !Global::initMail()) || !app.initializeStorage())
            return 0;
        bWriteLine(translate("main", "Enter \"help --commands\" to see the list of available commands"));
        ret = app.exec();
    } else {
        bWriteLine(translate("main", "Another instance of") + " "  + AppName + " "
                   + translate("main", "is already running. Quitting..."));
    }
    return ret;
}
