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

#include "connection.h"
#include "datasource.h"
#include "server.h"
#include "service/applicationversionservice.h"
#include "service/userservice.h"
#include "settings.h"

#include <TAccessLevel>
#include <TClientInfo>
#include <TCoreApplication>
#include <TeXSample>
#include <TGetUserConnectionInfoListRequestData>
#include <TInviteInfo>
#include <TUserConnectionInfo>
#include <TUserConnectionInfoList>
#include <TUserInfo>

#include <BDirTools>
#include <BeQt>
#include <BLocationProvider>
#include <BLogger>
#include <BSettingsNode>
#include <BTerminal>
#include <BTextTools>
#include <BTranslation>
#include <BUuid>
#include <BVersion>

#include <QDateTime>
#include <QElapsedTimer>
#include <QMap>
#include <QMutex>
#include <QMutexLocker>
#include <QObject>
#include <QSettings>
#include <QString>
#include <QStringList>
#include <QTimerEvent>
#include <QUrl>
#include <QVariant>

/*============================================================================
================================ Application =================================
============================================================================*/

/*============================== Static private variables ==================*/

QMutex Application::serverMutex(QMutex::Recursive);
QString Application::texsampleSty;
QString Application::texsampleTex;

/*============================== Public constructors =======================*/

Application::Application(int &argc, char **argv, const QString &applicationName, const QString &organizationName) :
    TCoreApplication(argc, argv, applicationName, organizationName),
    Source(new DataSource(location(DataPath, UserResource))),
    ApplicationVersionServ(new ApplicationVersionService(Source)), UserServ(new UserService(Source))
{
#if defined(BUILTIN_RESOURCES)
    Q_INIT_RESOURCE(texsample_server);
    Q_INIT_RESOURCE(texsample_server_translations);
#endif
    setApplicationVersion("3.0.0-a");
    setOrganizationDomain("https://github.com/ololoepepe/TeXSample-Server");
    setApplicationCopyrightPeriod("2012-2014");
    BLocationProvider *prov = new BLocationProvider;
    prov->addLocation("logs");
    installLocationProvider(prov);
    compatibility();
    installBeqtTranslator("qt");
    installBeqtTranslator("beqt");
    installBeqtTranslator("texsample");
    installBeqtTranslator("texsample-server");
    initTerminal();
    updateReadonly();
    bWriteLine(tr("This is") + " " + Application::applicationName() + " v" + applicationVersion());
    logger()->setDateTimeFormat("yyyy.MM.dd hh:mm:ss");
    updateLoggingMode();
    QString logfn = location(DataPath, UserResource) + "/logs/";
    logfn += QDateTime::currentDateTime().toString("yyyy.MM.dd-hh.mm.ss") + ".txt";
    logger()->setFileName(logfn);
    mserver = new Server(location(DataPath, UserResource));
    melapsedTimer.start();
    timerId = startTimer(BeQt::Hour);
}

Application::~Application()
{
    delete mserver;
#if defined(BUILTIN_RESOURCES)
    Q_CLEANUP_RESOURCE(texsample_server);
    Q_CLEANUP_RESOURCE(texsample_server_translations);
#endif
}

/*============================== Public methods ============================*/

bool Application::initializeStorage()
{
    static bool initialized = false;
    if (initialized) {
        bWriteLine(tr("Storage already initialized", "message"));
        return true;
    }
    bWriteLine(tr("Initializing storage...", "message"));
    QString sty = BDirTools::findResource("texsample-framework/texsample.sty", BDirTools::GlobalOnly);
    sty = BDirTools::readTextFile(sty, "UTF-8");
    if (sty.isEmpty()) {
        bWriteLine(tr("Failed to load texsample.sty", "error"));
        return false;
    }
    QString tex = BDirTools::findResource("texsample-framework/texsample.tex", BDirTools::GlobalOnly);
    tex = BDirTools::readTextFile(tex, "UTF-8");
    if (tex.isEmpty()) {
        bWriteLine(tr("Failed to load texsample.tex", "error"));
        return false;
    }
    QString err;
    if (!Source->initialize(&err)) {
        bWriteLine(tr("Error:", "error") + " " + err);
        return false;
    }
    if (!UserServ->checkOutdatedEntries()) {
        bWriteLine(tr("Failed to check for (or delete) outdated entries", "error"));
        return false;
    }
    if (UserServ->isRootInitialized()) {
        bWriteLine(tr("Done!", "message"));
        return true;
    }
    if (!UserServ->initializeRoot(&err)) {
        bWriteLine(tr("Error:", "error") + " " + err);
        return false;
    }
    texsampleSty = sty;
    texsampleTex = tex;
    initialized = true;
    bWriteLine(tr("Done!", "message"));
    return true;
}

bool Application::initializeEmail()
{
    static bool initialized = false;
    if (initialized) {
        bWriteLine(tr("E-mail already initialized"));
        return true;
    }
    bWriteLine(tr("Initializing e-mail..."));
    QString address = Settings::Email::serverAddress();
    if (address.isEmpty()) {
        address = bReadLine(tr("Enter e-mail server address:") + " ");
        if (address.isEmpty()) {
            bWriteLine(tr("Invalid address"));
            return false;
        }
    }
    QString port;
    if (!Settings::Email::hasServerPort())
        port = bReadLine(tr("Enter e-mail server port (default 25):") + " ");
    QVariant vport(!port.isEmpty() ? port : QString::number(Settings::Email::serverPort()));
    if (!vport.convert(QVariant::UInt)) {
        bWriteLine(tr("Invalid port"));
        return false;
    }
    QString name;
    if (!Settings::Email::hasLocalHostName())
        name = bReadLine(tr("Enter local host name or empty string:") + " ");
    else
        name = Settings::Email::localHostName();
    QString ssl;
    if (!Settings::Email::hasSslRequired())
        ssl = bReadLine(tr("Enter SSL mode [true|false] (default false):") + " ");
    QVariant vssl(!ssl.isEmpty() ? ssl : (Settings::Email::sslRequired() ? "true" : "false"));
    if (!vssl.convert(QVariant::Bool)) {
        bWriteLine(tr("Invalid value"));
        return false;
    }
    QString login = Settings::Email::login();
    if (login.isEmpty()) {
        login = bReadLine(tr("Enter e-mail login:") + " ");
        if (login.isEmpty()) {
            bWriteLine(tr("Invalid login"));
            return false;
        }
    }
    QString password = Settings::Email::password();
    if (password.isEmpty()) {
        password = bReadLineSecure(tr("Enter e-mail password:") + " ");
        if (password.isEmpty()) {
            bWriteLine(tr("Invalid password"));
            return false;
        }
    }
    Settings::Email::setServerAddress(address);
    Settings::Email::setServerPort(vport.toUInt());
    Settings::Email::setLocalHostName(name);
    Settings::Email::setSslRequired(vssl.toBool());
    Settings::Email::setLogin(login);
    Settings::Email::setPassword(password);
    initialized = true;
    bWriteLine(tr("Done!"));
    return true;
}

Server *Application::server()
{
    return mserver;
}

void Application::updateLoggingMode()
{
    int m = Settings::Log::loggingMode();
    if (m <= 0) {
        bLogger->setLogToConsoleEnabled(false);
        bLogger->setLogToFileEnabled(false);
    } else if (1 == m) {
        bLogger->setLogToConsoleEnabled(true);
        bLogger->setLogToFileEnabled(false);
    } else if (2 == m) {
        bLogger->setLogToConsoleEnabled(false);
        bLogger->setLogToFileEnabled(true);
    } else if (m >= 3)
    {
        bLogger->setLogToConsoleEnabled(true);
        bLogger->setLogToFileEnabled(true);
    }
}

void Application::updateReadonly()
{
    QString title = applicationName() + " v" + applicationVersion();
    if (Settings::Server::readonly())
        title += " (" + tr("read-only mode") + ")";
    BTerminal::setTitle(title);
}

qint64 Application::uptime() const
{
    return melapsedTimer.elapsed();
}

/*============================== Protected methods =========================*/

void Application::timerEvent(QTimerEvent *e)
{
    if (!e || e->timerId() != timerId)
        return;
    UserServ->checkOutdatedEntries();
}

/*============================== Static private methods ====================*/

bool Application::checkParsingError(BTextTools::OptionsParsingError error, const QString &errorData)
{
    switch (error) {
    case BTextTools::InvalidParametersError:
        bWriteLine(tr("Internal parsing error", "error"));
        return false;
    case BTextTools::MalformedOptionError:
        bWriteLine(tr("Malformed option:", "error") + " " + errorData);
        return false;
    case BTextTools::MissingOptionError:
        bWriteLine(tr("Missing option:", "error") + " " + errorData);
        return false;
    case BTextTools::RepeatingOptionError:
        bWriteLine(tr("Repeating option:", "error") + " " + errorData);
        return false;
    case BTextTools::UnknownOptionError:
        bWriteLine(tr("Unknown option:", "error") + " " + errorData);
        return false;
    case BTextTools::UnknownOptionValueError:
        bWriteLine(tr("Unknown option value:", "error") + " " + errorData);
        return false;
    case BTextTools::NoError:
    default:
        return true;
    }
}

bool Application::handleSetAppVersionCommand(const QString &, const QStringList &args)
{
    typedef QMap<QString, Texsample::ClientType> ClientMap;
    init_once(ClientMap, clientMap, ClientMap()) {
        clientMap.insert("cloudlab-client", Texsample::CloudlabClient);
        clientMap.insert("clab", Texsample::CloudlabClient);
        clientMap.insert("tex-creator", Texsample::TexCreator);
        clientMap.insert("tcrt", Texsample::TexCreator);
        clientMap.insert("texsample-console", Texsample::TexsampleConsole);
        clientMap.insert("tcsl", Texsample::TexsampleConsole);
    }
    typedef QMap<QString, BeQt::OSType> OsMap;
    init_once(OsMap, osMap, OsMap()) {
        osMap.insert("linux", BeQt::LinuxOS);
        osMap.insert("lin", BeQt::LinuxOS);
        osMap.insert("l", BeQt::LinuxOS);
        osMap.insert("macos", BeQt::MacOS);
        osMap.insert("mac", BeQt::MacOS);
        osMap.insert("m", BeQt::MacOS);
        osMap.insert("windows", BeQt::WindowsOS);
        osMap.insert("win", BeQt::WindowsOS);
        osMap.insert("w", BeQt::WindowsOS);
    }
    typedef QMap<QString, BeQt::ProcessorArchitecture> ArchMap;
    init_once(ArchMap, archMap, ArchMap()) {
        archMap.insert("alpha", BeQt::AlphaArchitecture);
        archMap.insert("amd64", BeQt::Amd64Architecture);
        archMap.insert("arm", BeQt::ArmArchitecture);
        archMap.insert("arm64", BeQt::Arm64Architecture);
        archMap.insert("blackfin", BeQt::BlackfinArchitecture);
        archMap.insert("convex", BeQt::ConvexArchitecture);
        archMap.insert("epiphany", BeQt::EpiphanyArchitecture);
        archMap.insert("risc", BeQt::HpPaRiscArchitecture);
        archMap.insert("x86", BeQt::IntelX86Architecture);
        archMap.insert("itanium", BeQt::IntelItaniumArchitecture);
        archMap.insert("motorola", BeQt::Motorola68kAArchitecture);
        archMap.insert("mips", BeQt::MipsArchitecture);
        archMap.insert("powerpc", BeQt::PowerPcArchitecture);
        archMap.insert("pyramid", BeQt::Pyramid9810Architecture);
        archMap.insert("rs6000", BeQt::Rs6000Architecture);
        archMap.insert("sparc", BeQt::SparcArchitecture);
        archMap.insert("superh", BeQt::SuperHArchitecture);
        archMap.insert("systemz", BeQt::SystemZArchitecture);
        archMap.insert("tms320", BeQt::Tms320Architecture);
        archMap.insert("tms470", BeQt::Tms470Architecture);
    }
    QMap<QString, QString> result;
    QString errorData;
    QString options = "client:-c|--client=" + QStringList(clientMap.keys()).join("|") + ",";
    options += "os:-o|--os=" + QStringList(osMap.keys()).join("|") + ",";
    options += "arch:-a|--arch=" + QStringList(archMap.keys()).join("|") + ",";
    options += "[portable:-p|--portable],version:-v|--version=,url:-u|--url=";
    BTextTools::OptionsParsingError error = BTextTools::parseOptions(args, options, result, errorData);
    if (!checkParsingError(error, errorData))
        return false;
    Texsample::ClientType client = clientMap.value(result.value("client"));
    BeQt::OSType os = osMap.value(result.value("os"));
    BeQt::ProcessorArchitecture arch = archMap.value(result.value("arch"));
    BVersion version = BVersion(result.value("version"));
    QUrl url = QUrl::fromUserInput(result.value("url"));
    bool portable = result.contains("portable");
    if (!version.isValid()) {
        bWriteLine(tr("Invalid version", "error"));
        return false;
    }
    if (!url.isValid()) {
        bWriteLine(tr("Invalid url", "error"));
        return false;
    }
    if (!bApp) {
        bWriteLine(tr("No Application instance", "error"));
        return false;
    }
    if (!bApp->ApplicationVersionServ->setLatestAppVersion(client, os, arch, portable, version, url)) {
        bWriteLine(tr("Failed to set application version", "error"));
        return true;
    }
    bWriteLine(tr("OK", "message"));
    return true;
}

bool Application::handleStartCommand(const QString &, const QStringList &args)
{
    QMutexLocker locker(&serverMutex);
    Q_UNUSED(locker)
    if (args.size() > 1) {
        bWriteLine(tr("Invalid argument count. This command accepts 0-1 arguments", "error"));
        return false;
    }
    if (!bApp) {
        bWriteLine(tr("No Application instance", "error"));
        return false;
    }
    if (bApp->mserver->isListening()) {
        bWriteLine(tr("The server is listening already", "error"));
        return false;
    }
    if (!bApp->mserver->listen(!args.isEmpty() ? args.first() : QString("0.0.0.0"), Texsample::MainPort)) {
        bWriteLine(tr("Failed to start server", "error"));
        return true;
    }
    bWriteLine(tr("OK", "message"));
    return true;
}

bool Application::handleStopCommand(const QString &, const QStringList &args)
{
    QMutexLocker locker(&serverMutex);
    Q_UNUSED(locker)
    if (!args.isEmpty()) {
        bWriteLine(tr("Invalid argument count. This command does not accept any arguments", "error"));
        return false;
    }
    if (!bApp) {
        bWriteLine(tr("No Application instance", "error"));
        return false;
    }
    if (!bApp->mserver->isListening()) {
        bWriteLine(tr("The server is not listening", "error"));
        return false;
    }
    bApp->mserver->close();
    bWriteLine(tr("OK", "message"));
    return true;
}

bool Application::handleUnknownCommand(const QString &, const QStringList &)
{
    bWriteLine(tr("Unknown command. Enter \"help --commands\" to see the list of available commands"));
    return false;
}

bool Application::handleUptimeCommand(const QString &, const QStringList &args)
{
    if (!args.isEmpty()) {
        bWriteLine(tr("Invalid argument count. This command does not accept any arguments", "error"));
        return false;
    }
    if (!bApp) {
        bWriteLine(tr("No Application instance", "error"));
        return false;
    }
    bWriteLine(tr("Uptime:", "message") + " " + msecsToString(bApp->melapsedTimer.elapsed()));
    return true;
}

bool Application::handleUserInfoCommand(const QString &, const QStringList &args)
{
    typedef QMap<QString, TGetUserConnectionInfoListRequestData::MatchType> IntMap;
    init_once(IntMap, matchTypeMap, IntMap()) {
        matchTypeMap.insert("login-and-unique-id", TGetUserConnectionInfoListRequestData::MatchLoginAndUniqueId);
        matchTypeMap.insert("a", TGetUserConnectionInfoListRequestData::MatchLoginAndUniqueId);
        matchTypeMap.insert("login", TGetUserConnectionInfoListRequestData::MatchLogin);
        matchTypeMap.insert("l", TGetUserConnectionInfoListRequestData::MatchLogin);
        matchTypeMap.insert("unique-id", TGetUserConnectionInfoListRequestData::MatchUniqueId);
        matchTypeMap.insert("u", TGetUserConnectionInfoListRequestData::MatchUniqueId);
    }
    QMap<QString, QString> result;
    QString errorData;
    QString options = "[type:--match-type|-m=login-and-unique-id|a|login|l|unique-id|u],pattern:--match-pattern|-p=";
    BTextTools::OptionsParsingError error = BTextTools::parseOptions(args, options, result, errorData);
    if (!checkParsingError(error, errorData))
        return false;
    TGetUserConnectionInfoListRequestData::MatchType matchType =
            result.contains("type") ? matchTypeMap.value(result.value("type")) :
                                      TGetUserConnectionInfoListRequestData::MatchLoginAndUniqueId;
    QString matchPattern = result.value("pattern");
    if (!QRegExp(matchPattern, Qt::CaseSensitive, QRegExp::WildcardUnix).isValid()) {
        bWriteLine(tr("Invalid pattern", "error"));
        return false;
    }
    QMutexLocker locker(&serverMutex);
    Q_UNUSED(locker)
    if (!bApp) {
        bWriteLine(tr("No Application instance", "error"));
        return false;
    }
    int total = 0;
    TUserConnectionInfoList list = bApp->mserver->userConnections(matchPattern, matchType, &total);
    if (list.isEmpty()) {
        bWriteLine(tr("No user matched. Total:", "message") + " " + QString::number(total));
        return true;
    }
    bWriteLine(tr("Matched users:", "message") + " " + QString::number(list.size()) + "/" + QString::number(total));
    foreach (const TUserConnectionInfo &info, list) {
        QString s = "\n";
        s += "[" + info.userInfo().login() + "] [" + info.peerAddress() + "] [" + info.uniqueId().toString(true) + "]";
        s += "\n" + info.userInfo().accessLevel().toString() + "; " + info.clientInfo().os() + "\n";
        s += info.clientInfo().toString("%n v%v; TeXSample v%t; BeQt v%b; Qt v%q") + "\n";
        s += tr("Connected at:", "message") + " "
                + info.connectionDateTime().toLocalTime().toString("yyyy.MM.dd hh:mm:ss") + "\n";
        s += tr("Uptime:", "message") + " " + msecsToString(info.uptime());
        bWriteLine(s);
    }
    return true;
}

void Application::initTerminal()
{
    BTerminal::setMode(BTerminal::StandardMode);
    BTerminal::installHandler(BTerminal::QuitCommand);
    BTerminal::installHandler(BTerminal::SetCommand);
    BTerminal::installHandler(BTerminal::HelpCommand);
    BTerminal::installHandler(BTerminal::LastCommand);
    BTerminal::installDefaultHandler(&handleUnknownCommand);
    BTerminal::installHandler("set-app-version", &handleSetAppVersionCommand);
    BTerminal::installHandler("start", &handleStartCommand);
    BTerminal::installHandler("stop", &handleStopCommand);
    BTerminal::installHandler("uptime", &handleUptimeCommand);
    BTerminal::installHandler("user-info", &handleUserInfoCommand);
    BSettingsNode *root = new BSettingsNode;
    BSettingsNode *n = new BSettingsNode(Settings::Email::RootPath, root);
    BSettingsNode *nn = new BSettingsNode(Settings::Email::ServerAddressSubpath, n);
    nn->setDescription(BTranslation::translate("Application", "E-mail server address used for e-mail delivery."));
    nn = new BSettingsNode(QVariant::UInt, Settings::Email::ServerPortSubpath, n);
    nn->setDescription(BTranslation::translate("Application", "E-mail server port."));
    nn = new BSettingsNode(Settings::Email::LocalHostNameSubpath, n);
    nn->setDescription(BTranslation::translate("Application", "Name of local host passed to the e-mail server."));
    nn = new BSettingsNode(QVariant::Bool, Settings::Email::SslRequiredSubpath, n);
    nn->setDescription(BTranslation::translate("Application",
                                               "Determines wether the e-mail server requires SSL connection."));
    nn = new BSettingsNode(Settings::Email::LoginSubpath, n);
    nn->setDescription(BTranslation::translate("Application", "Identifier used to log into the e-mail server."));
    nn = new BSettingsNode(Settings::Email::PasswordSubpath, n);
    nn->setUserSetFunction(&Settings::Email::setPassword);
    nn->setUserShowFunction(&Settings::Email::showPassword);
    nn->setDescription(BTranslation::translate("Application", "Password used to log into the e-mail server."));
    nn = new BSettingsNode(Settings::Email::StorePasswordSubpath, n);
    nn->setUserSetFunction(&Settings::Email::setStorePassword);
    nn->setDescription(BTranslation::translate("Application", "Determines wether e-mail password is stored on disk."));
    BTerminal::createBeQtSettingsNode(root);
    n = new BSettingsNode(Settings::Log::RootPath, root);
    nn = new BSettingsNode(QVariant::Int, Settings::Log::LoggingModeSubpath, n);
    nn->setUserSetFunction(&Settings::Log::setLoggingMode);
    nn->setDescription(BTranslation::translate("Application", "Logging mode. Possible values:\n"
                                               "  0 or less - don't log\n"
                                               "  1 - log to console only\n"
                                               "  2 - log to file only\n"
                                               "  3 and more - log to console and file\n"
                                               "  The default is 2"));
    nn = new BSettingsNode(QVariant::Int, Settings::Log::LogNoopSubpath, n);
    nn->setDescription(BTranslation::translate("Application", "Logging the \"keep alive\" operations. "
                                               "Possible values:\n"
                                               "  0 or less - don't log\n"
                                               "  1 - log locally\n"
                                               "  2 and more - log loaclly and remotely\n"
                                               "  The default is 0"));
    n = new BSettingsNode(Settings::Server::RootPath, root);
    nn = new BSettingsNode(QVariant::Bool, Settings::Server::ReadonlySubpath, n);
    nn->setDescription(BTranslation::translate("Application", "Read-only mode. Possible values:\n"
                                               "  true - read-only mode\n"
                                               "  false - normal mode (read and write)"));
    BTerminal::setRootSettingsNode(root);
    BTerminal::setHelpDescription(BTranslation::translate("BTerminalIOHandler",
        "This is TeXSample Server. Enter \"help --all\" to see full Help"));
    BTerminal::CommandHelpList chl;
    BTerminal::CommandHelp ch;
    ch.usage = "user-info [--match-type|-m=<match_type>] --match-pattern|-p=<match_pattern>";
    ch.description = BTranslation::translate("BTerminalIOHandler",
        "Show information about connected users matching <match_pattern>, which is to be a wildcard.\n"
        "<match_type> may be one of the following:\n"
        "  login-and-unique-id|a - attempt to match both login and uinque id (default)\n"
        "  login|l - match login only\n"
        "  unique-id|u - match unique id only");
    chl << ch;
    BTerminal::setCommandHelp("user-info", chl);
    ch.usage = "start [address]";
    ch.description = BTranslation::translate("BTerminalIOHandler", "Start the server.\n"
        "If [address] is passed, server will listen on that address.\n"
        "Otherwise it will listen on all available addresses.");
    BTerminal::setCommandHelp("start", ch);
    ch.usage = "stop";
    ch.description = BTranslation::translate("BTerminalIOHandler", "Stop the server.\n"
                                             "Note: Users are not disconnected.");
    BTerminal::setCommandHelp("stop", ch);
    ch.usage = "set-app-version <parameters>";
    ch.description = BTranslation::translate("BTerminalIOHandler",
        "Set the latest version of an application along with the download URL.\n"
        "The parameters are:\n"
        "  --client|-c=<client>, where <client> must be one of the following:\n"
        "    cloudlab-client|clab\n"
        "    tex-creator|tcrt\n"
        "    texsample-console|tcsl\n"
        "  --os|o=<os>, where <os> must be one of the following:\n"
        "    linux|lin|l\n"
        "    macos|mac|m\n"
        "    windows|win|w\n"
        "  --arch|-a=<arch>, where <arch> must be one of the following:\n"
        "    alpha, amd64, arm, arm64, blackfin, convex, epiphany,\n"
        "    risc, x86, itanium, motorola, mips, powerpc, pyramid,\n"
        "    rs6000, sparc, superh, systemz, tms320, tms470\n"
        "  --version|-v=<version>, where <version> must be in the following format:\n"
        "    <major>[.<minor>[.<patch>]][-<status>[extra]]\n"
        "  --url|-u<url> (optional), where <url> must be a url (schema may be omitted)\n"
        "  --portable|-p (optional)\n"
        "Example:\n"
        "  set-app-version -c=tex-creator -o=windows -a=x86 -p -v=3.5.0-beta2 -u=site.com/dl/install.exe");
    BTerminal::setCommandHelp("set-app-version", ch);
    ch.usage = "uptime";
    ch.description = BTranslation::translate("BTerminalIOHandler",
                                             "Show information about server state (uptime, listening state, etc.)");
    BTerminal::setCommandHelp("uptime", ch);
}

QString Application::msecsToString(qint64 msecs)
{
    QString days = QString::number(msecs / (24 * BeQt::Hour));
    msecs %= (24 * BeQt::Hour);
    QString hours = QString::number(msecs / BeQt::Hour);
    hours.prepend(QString().fill('0', 2 - hours.length()));
    msecs %= BeQt::Hour;
    QString minutes = QString::number(msecs / BeQt::Minute);
    minutes.prepend(QString().fill('0', 2 - minutes.length()));
    msecs %= BeQt::Minute;
    QString seconds = QString::number(msecs / BeQt::Second);
    seconds.prepend(QString().fill('0', 2 - seconds.length()));
    return days + " " + tr("days") + " " + hours + ":" + minutes + ":" + seconds;
}

/*============================== Private methods ===========================*/

void Application::compatibility()
{
    bSettings->setValue("Global/version", BVersion(applicationVersion()));
}
