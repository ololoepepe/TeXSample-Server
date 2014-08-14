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
#include <TUserInfo>

#include <BDirTools>
#include <BeQt>
#include <BLocationProvider>
#include <BLogger>
#include <BSettingsNode>
#include <BTerminal>
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
    setApplicationVersion("2.2.2-beta");
    setOrganizationDomain("https://github.com/TeXSample-Team/TeXSample-Server");
    setApplicationCopyrightPeriod("2012-2014");
#if defined(BUILTIN_RESOURCES)
    Q_INIT_RESOURCE(texsample_server);
    Q_INIT_RESOURCE(texsample_server_translations);
#endif
    BLocationProvider *prov = new BLocationProvider;
    prov->addLocation("logs");
    prov->addLocation("users");
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
    if (!bRangeD(5, 6).contains(args.size())) {
        bWriteLine(tr("Invalid argument count. This command accepts 5-6 arguments", "error"));
        return false;
    }
    Texsample::ClientType client = Texsample::UnknownClient;
    BeQt::OSType os = BeQt::UnknownOS;
    BeQt::ProcessorArchitecture arch = BeQt::UnknownArchitecture;
    bool portable = false;
    bool bclient = false;
    bool bos = false;
    bool barch = false;
    bool bportable = false;
    foreach (int i, bRangeD(0, 2 + (args.size() == 6 ? 1 : 0))) {
        QString a = args.at(i);
        if (a.startsWith("--client=") || a.startsWith("-c=")) {
            if (bclient) {
                bWriteLine(tr("Repeating argument", "error"));
                return false;
            }
            QStringList sl = a.split('=');
            if (sl.size() != 2) {
                bWriteLine(tr("Invalid argument", "error"));
                return false;
            }
            if (!clientMap.contains(sl.last())) {
                bWriteLine(tr("Unknown client type", "error"));
                return false;
            }
            client = clientMap.value(sl.last());
            bclient = true;
        } else if (a.startsWith("--os=") || a.startsWith("-o=")) {
            if (bos) {
                bWriteLine(tr("Repeating argument", "error"));
                return false;
            }
            QStringList sl = a.split('=');
            if (sl.size() != 2) {
                bWriteLine(tr("Invalid argument", "error"));
                return false;
            }
            if (!osMap.contains(sl.last())) {
                bWriteLine(tr("Unknown OS type", "error"));
                return false;
            }
            os = osMap.value(sl.last());
            bos = true;
        } else if (a.startsWith("--arch=") || a.startsWith("-a=")) {
            if (barch) {
                bWriteLine(tr("Repeating argument", "error"));
                return false;
            }
            QStringList sl = a.split('=');
            if (sl.size() != 2) {
                bWriteLine(tr("Invalid argument", "error"));
                return false;
            }
            if (!archMap.contains(sl.last())) {
                bWriteLine(tr("Unknown processor architecture", "error"));
                return false;
            }
            arch = archMap.value(sl.last());
            barch = true;
        } else if (a == "--portable" || a == "-p") {
            if (bportable) {
                bWriteLine(tr("Repeating argument", "error"));
                return false;
            }
            portable = true;
            bportable = true;
        } else {
            bWriteLine(tr("Unknown argument", "error"));
            return false;
        }
    }
    if (!bclient || !bos || !barch) {
        bWriteLine(tr("Not enough arguments", "error"));
        return false;
    }
    BVersion version = BVersion(args.at(args.size() - 2));
    QUrl url = QUrl::fromUserInput(args.at(args.size() - 1));
    if (!version.isValid() || !url.isValid()) {
        bWriteLine(tr("Invalid argument", "error"));
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
    if (args.size() > 2) {
        bWriteLine(tr("Invalid argument count. This command accepts 0-2 arguments", "error"));
        return false;
    }
    QMutexLocker locker(&serverMutex);
    Q_UNUSED(locker)
    if (!bApp) {
        bWriteLine(tr("No Application instance", "error"));
        return false;
    }
    Server *srv = bApp->mserver;
    if (args.isEmpty()) {
        srv->lock();
        int count = srv->currentConnectionCount();
        srv->unlock();
        bWriteLine(tr("User count:", "message") + " " + count);
    } else if (args.first() == "--list") {
        QStringList list;
        srv->lock();
        foreach (BNetworkConnection *c, srv->connections()) {
            TUserInfo info = static_cast<Connection *>(c)->userInfo();
            list << ("[" + info.login() + "] [" + c->peerAddress() + "] [" + c->uniqueId().toString(true) + "]");
        }
        srv->unlock();
        bWriteLine(tr("Listing connected users", "message") + " (" + list.size() + "):\n" + list.join("\n"));
    } else if (args.size() == 2) {
        QList<Connection *> users;
        BUuid uuid(args.at(1));
        srv->lock();
        if (uuid.isNull()) {
            foreach (BNetworkConnection *c, srv->connections()) {
                Connection *cc = static_cast<Connection *>(c);
                if (cc->userInfo().login() == args.at(1))
                    users << cc;
            }
        } else {
            foreach (BNetworkConnection *c, srv->connections()) {
                Connection *cc = static_cast<Connection *>(c);
                if (cc->uniqueId() == uuid) {
                    users << cc;
                    break;
                }
            }
        }
        if (args.first() == "--kick") {
            foreach (Connection *c, users)
                QMetaObject::invokeMethod(c, "abort", Qt::QueuedConnection);
            srv->unlock();
            bWriteLine(tr("Kicked users:", "message") + " " + users.size());
            return true;
        } else if (args.first() == "--info") {
            QStringList list;
            foreach (Connection *c, users) {
                TUserInfo info = c->userInfo();
                TClientInfo client = c->clientInfo();
                QString s = "[" + info.login() + "] [" + c->peerAddress() + "] [" + c->uniqueId().toString(true) + "]";
                s += "\n" + info.accessLevel().toStringNoTr() + "; " + client.toString("%o (%a)");
                s += " [" + c->locale().name() + "]";
                s += "\n" + client.toString("%n v%v; TeXSample %t; BeQt v%b; Qt v%q");
                list << s;
            }
            bWriteLine(list.join("\n"));
        } else if (args.first() == "--uptime") {
            QStringList list;
            foreach (Connection *c, users) {
                TUserInfo info = c->userInfo();
                QString s = "[" + info.login() + "] [" + c->peerAddress() + "] [" + c->uniqueId().toString(true) + "]";
                s += " " + msecsToString(c->uptime());
                list << s;
            }
            bWriteLine(tr("Uptime:", "message") + "\n" + list.join("\n"));
        } else if (args.first() == "--connected-at") {
            QStringList list;
            foreach (Connection *c, users) {
                TUserInfo info = c->userInfo();
                QString s = "[" + info.login() + "] [" + c->peerAddress() + "] [" + c->uniqueId().toString(true) + "]";
                s += " " + c->connectionDateTime().toString("yyyy.MM.dd hh:mm:ss");
                list << s;
            }
            bWriteLine(tr("Connection time:", "message") + "\n" + list.join("\n"));
        }
        srv->unlock();
    } else {
        bWriteLine(tr("Invalid arguments", "error"));
        return false;
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
    nn->setDescription(BTranslation::translate("BSettingsNode", "E-mail server address used for e-mail delivery"));
    nn = new BSettingsNode(QVariant::UInt, "server_port", n);
    nn->setDescription(BTranslation::translate("BSettingsNode", "E-mail server port"));
    nn = new BSettingsNode("local_host_name", n);
    nn->setDescription(BTranslation::translate("BSettingsNode", "Name of local host passed to the e-mail server"));
    nn = new BSettingsNode("ssl_required", n);
    nn->setDescription(BTranslation::translate("BSettingsNode",
                                               "Determines wether the e-mail server requires SSL connection"));
    nn = new BSettingsNode("login", n);
    nn->setDescription(BTranslation::translate("BSettingsNode", "Identifier used to log into the e-mail server"));
    nn = new BSettingsNode("password", n);
    nn->setUserSetFunction(&Global::setMailPassword);
    nn->setUserShowFunction(&Global::showMailPassword);
    nn->setDescription(BTranslation::translate("BSettingsNode", "Password used to log into the e-mail server"));
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
    BTerminal::setHelpDescription(BTranslation::translate("BTerminalIOHandler", "This is TeXSample Server. "
                                                          "Enter \"help --all\" to see full Help"));
    BTerminal::CommandHelpList chl;
    BTerminal::CommandHelp ch;
    ch.usage = "user-info [--match-type=<match_type>] <match_pattern>";
    ch.description = BTranslation::translate("BTerminalIOHandler", "Show information about connected users matching"
                                             "<match_pattern>, which is to be a wildcard.\n"
                                             "<match_type> may be one of the following:\n"
                                             "  login-and-unique-id - attempt to match both login and uinque id "
                                             "(default)\n"
                                             "  login - match login only\n"
                                             "  unique-id - match unique id only");
    chl << ch;
    BTerminal::setCommandHelp("user-info", chl);
    ch.usage = "start [address]";
    ch.description = BTranslation::translate("BTerminalIOHandler", "Start the server. "
                                             "If [address] is passed, server will listen on that address. "
                                             "Otherwise it will listen on all available addresses");
    BTerminal::setCommandHelp("start", ch);
    ch.usage = "stop";
    ch.description = BTranslation::translate("BTerminalIOHandler", "Stop the server. "
                                             "Note: Users are not disconnected.");
    BTerminal::setCommandHelp("stop", ch);
    ch.usage = "set-app-version --client|-c=<client> --os|-o=<os> --arch|-a=<arch> [--portable|-p] <version> <url>";
    ch.description = BTranslation::translate("BTerminalIOHandler",
                                             "Set the latest version of an application along with the download URL\n"
                                             "  client must be either cloudlab-client (clab), tex-creator (tcrt), "
                                             "or texsample-console (tcsl)\n"
                                             "  os must be either linux (lin, l), macos (mac, m), "
                                             "or windows (win, w)\n"
                                             "  arch must be either alpha, amd64, arm, arm64, blackfin, convex, "
                                             "epiphany, risc, x86, itanium, motorola, mips, powerpc, "
                                             "pyramid, rs6000, sparc, superh, systemz, tms320, tms470");
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
