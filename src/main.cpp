/****************************************************************************
**
** Copyright (C) 2012-2014 Andrey Bogdanov
**
** This file is part of TeXSample Server.
**
** TeXSample Server is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** TeXSample Server is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with TeXSample Server.  If not, see <http://www.gnu.org/licenses/>.
**
****************************************************************************/

#include "application.h"

#include <BApplicationServer>
#include <BTerminal>
#include <BTextTools>

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
    BApplicationServer s(9920 + qHash(home) % 10, AppName + "3" + home);
    int ret = 0;
    if (!s.testServer()) {
        Application app(argc, argv, AppName, "Andrey Bogdanov");
        s.listen();
        if (!app.initializeEmail() || !app.initializeStorage())
            return 0;
        bWriteLine(translate("main", "Enter \"help --commands\" to see the list of available commands"));
        QStringList args = app.arguments().mid(1);
        if (!args.isEmpty()) {
            BTerminal::writeLine(BTextTools::mergeArguments(args));
            BTerminal::emulateCommand(args.first(), args.mid(1));
        }
        ret = app.exec();
    } else {
        bWriteLine(translate("main", "Another instance of") + " "  + AppName + " "
                   + translate("main", "is already running. Quitting..."));
    }
    return ret;
}
