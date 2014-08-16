#include "application.h"

#include "connection.h"
#include "datasource.h"
#include "global.h"
#include "server.h"
#include "service/applicationversionservice.h"
#include "service/userservice.h"

#include <TAccessLevel>
#include <TClientInfo>
#include <TCoreApplication>
#include <TeXSample>
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
    setApplicationVersion("2.2.2-beta");
    setOrganizationDomain("https://github.com/TeXSample-Team/TeXSample-Server");
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
    QString title = Application::applicationName() + " v" + applicationVersion();
    if (Global::readOnly())
        title += " (" + translate("main", "read-only mode") + ")";
    BTerminal::setTitle(title);
    bWriteLine(translate("main", "This is") + " " + title);
    logger()->setDateTimeFormat("yyyy.MM.dd hh:mm:ss");
    Global::resetLoggingMode();
    QString logfn = location(DataPath, UserResource) + "/logs/";
    logfn += QDateTime::currentDateTime().toString("yyyy.MM.dd-hh.mm.ss") + ".txt";
    logger()->setFileName(logfn);
    mserver = new Server(location(DataPath, UserResource), this);
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
        bWriteLine(tr("Error: Failed to load texsample.sty", "error"));
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

Server *Application::server()
{
    return mserver;
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
    if (!bApp->ApplicationVersionServ->setApplicationVersion(client, os, arch, portable, version, url)) {
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

bool Application::handleUserCommand(const QString &, const QStringList &args)
{
    static const int MatchLogin = 0x01;
    static const int MatchUniqueId = 0x02;
    typedef QMap<QString, int> IntMap;
    init_once(IntMap, matchTypeMap, IntMap()) {
        matchTypeMap.insert("login-and-unique-id", MatchLogin | MatchUniqueId);
        matchTypeMap.insert("a", MatchLogin | MatchUniqueId);
        matchTypeMap.insert("login", MatchLogin);
        matchTypeMap.insert("l", MatchLogin);
        matchTypeMap.insert("unique-id", MatchUniqueId);
        matchTypeMap.insert("u", MatchUniqueId);
    }
    QMap<QString, QString> result;
    QString errorData;
    QString options = "[type:--match-type|-m=login-and-unique-id|a|login|l|unique-id|u],pattern:--match-pattern|-p=";
    BTextTools::OptionsParsingError error = BTextTools::parseOptions(args, options, result, errorData);
    if (!checkParsingError(error, errorData))
        return false;
    int matchType = result.contains("type") ? matchTypeMap.value(result.value("type")) : (MatchLogin | MatchUniqueId);
    QRegExp rx = QRegExp(result.value("pattern"), Qt::CaseSensitive, QRegExp::WildcardUnix);
    if (!rx.isValid()) {
        bWriteLine(tr("Invalid pattern", "error"));
        return false;
    }
    QMutexLocker locker(&serverMutex);
    Q_UNUSED(locker)
    if (!bApp) {
        bWriteLine(tr("No Application instance", "error"));
        return false;
    }
    TUserConnectionInfoList list;
    bApp->mserver->lock();
    int total = 0;
    foreach (BNetworkConnection *c, bApp->mserver->connections()) {
        ++total;
        Connection *cc = static_cast<Connection *>(c);
        TUserInfo userInfo = cc->userInfo();
        BUuid uniqueId = cc->uniqueId();
        if ((!(MatchLogin | matchType) || !rx.exactMatch(userInfo.login()))
                && (!(MatchUniqueId | matchType) || !rx.exactMatch(cc->uniqueId().toString(true)))) {
            continue;
        }
        TUserConnectionInfo info;
        info.setClientInfo(cc->clientInfo());
        info.setConnectionDateTime(cc->connectionDateTime());
        info.setPeerAddress(cc->peerAddress());
        info.setUniqueId(uniqueId);
        info.setUptime(cc->uptime());
        info.setUserInfo(userInfo);
        list << info;
    }
    bApp->mserver->unlock();
    if (list.isEmpty()) {
        bWriteLine(tr("No user matched. Total:", "message") + " " + QString::number(total));
        return true;
    }
    bWriteLine(tr("Matched users:", "message") + " " + QString::number(list.size()) + "/" + QString::number(total));
    foreach (const TUserConnectionInfo &info, list) {
        QString s = "\n";
        s += "[" + info.userInfo().login() + "] [" + info.peerAddress() + "] [" + info.uniqueId().toString(true) + "]";
        s += info.userInfo().accessLevel().toString() + "; " + info.clientInfo().os() + "\n";
        s += info.clientInfo().toString("%n v%v; TeXSample v%t; BeQt v%b; Qt v%q") + "\n";
        s += tr("Connected at:", "message") + " "
                + info.connectionDateTime().toLocalTime().toString("yyyy.MM.dd hh:mm:ss");
        s += tr("Uptime:", "message") + " " + msecsToString(info.uptime());
    }
    return true;
}

void Application::initTerminal()
{
    BTerminal::setMode(BTerminal::StandardMode);
    BTerminal::installHandler(BTerminal::QuitCommand);
    BTerminal::installHandler(BTerminal::SetCommand);
    BTerminal::installHandler(BTerminal::HelpCommand);
    BTerminal::installDefaultHandler(&handleUnknownCommand);
    BTerminal::installHandler("set-app-version", &handleSetAppVersionCommand);
    BTerminal::installHandler("start", &handleStartCommand);
    BTerminal::installHandler("stop", &handleStopCommand);
    BTerminal::installHandler("uptime", &handleUptimeCommand);
    BTerminal::installHandler("user", &handleUserCommand);
    BSettingsNode *root = new BSettingsNode;
    BSettingsNode *n = new BSettingsNode("Mail", root);
    BSettingsNode *nn = new BSettingsNode("server_address", n);
    nn->setDescription(BTranslation::translate("BSettingsNode", "E-mail server address used for e-mail delivery."));
    nn = new BSettingsNode(QVariant::UInt, "server_port", n);
    nn->setDescription(BTranslation::translate("BSettingsNode", "E-mail server port."));
    nn = new BSettingsNode("local_host_name", n);
    nn->setDescription(BTranslation::translate("BSettingsNode", "Name of local host passed to the e-mail server."));
    nn = new BSettingsNode("ssl_required", n);
    nn->setDescription(BTranslation::translate("BSettingsNode",
                                               "Determines wether the e-mail server requires SSL connection."));
    nn = new BSettingsNode("login", n);
    nn->setDescription(BTranslation::translate("BSettingsNode", "Identifier used to log into the e-mail server."));
    nn = new BSettingsNode("password", n);
    nn->setUserSetFunction(&Global::setMailPassword);
    nn->setUserShowFunction(&Global::showMailPassword);
    nn->setDescription(BTranslation::translate("BSettingsNode", "Password used to log into the e-mail server."));
    BTerminal::createBeQtSettingsNode(root);
    n = new BSettingsNode("Log", root);
    nn = new BSettingsNode("mode", n);
    nn->setUserSetFunction(&Global::setLoggingMode);
    nn->setDescription(BTranslation::translate("BSettingsNode", "Logging mode. Possible values:\n"
                                               "  0 or less - don't log\n"
                                               "  1 - log to console only\n"
                                               "  2 - log to file only\n"
                                               "  3 and more - log to console and file\n"
                                               "  The default is 2"));
    nn = new BSettingsNode("noop", n);
    nn->setDescription(BTranslation::translate("BSettingsNode", "Logging the \"keep alive\" operations. "
                                               "Possible values:\n"
                                               "  0 or less - don't log\n"
                                               "  1 - log locally\n"
                                               "  2 and more - log loaclly and remotely\n"
                                               "  The default is 0"));
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
    //Compatibility
    if (bSettings->value("Global/version").value<BVersion>() < BVersion("2.2.1-beta")) {
        const QStringList Names = QStringList() << "cloudlab-client" << "tex-creator" << "texsample-console";
        const QStringList Platforms = QStringList() << "lin" << "mac" << "win";
        foreach (const QString &name, Names) {
            QString s = "AppVersion/" + name + "/";
            foreach (const QString &pl, Platforms) {
                QString ss = s + pl + "/";
                QVariant ver = bSettings->value(ss + "version");
                QVariant url = bSettings->value(ss + "url");
                bSettings->remove(ss + "version");
                bSettings->remove(ss + "url");
                if (ver.isNull() || url.isNull())
                    continue;
                ss += "normal/";
                bSettings->setValue(ss + "version", ver);
                bSettings->setValue(ss + "url", url.toString());
            }
        }
    }
    bSettings->setValue("Global/version", BVersion(applicationVersion()));
}
