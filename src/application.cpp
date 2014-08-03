#include "application.h"

#include "connection.h"
#include "datasource.h"
#include "global.h"
#include "server.h"
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
    Source(new DataSource(location(DataPath, UserResource))), UserServ(new UserService(Source))
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

/*============================== Static public methods =====================*/

/*TExecuteCommandReplyData Application::executeSetAppVersion(const QStringList &args)
{
    init_once(QStringList, clientNames, QStringList()) {
        clientNames << "cloudlab-client";
        clientNames << "clab";
        clientNames << "tex-creator";
        clientNames << "tcrt";
        clientNames << "texsample-console";
        clientNames << "tcsl";
    }
    init_once(QStringList, osNames, QStringList()) {
        osNames << "linux";
        osNames << "lin";
        osNames << "l";
        osNames << "macos";
        osNames << "mac";
        osNames << "m";
        osNames << "windows";
        osNames << "win";
        osNames << "w";
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
    TExecuteCommandReplyData reply;
    if (!bRangeD(5, 6).contains(args.size())) {
        reply.setMessage(TCommandMessage::InvalidArgumentCountMessage);
        return reply;
    }
    QString client;
    QString os;
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
                reply.setMessage(TCommandMessage::InvalidArgumentsMessage);
                return reply;
            }
            QStringList sl = a.split('=');
            if (sl.size() != 2) {
                reply.setMessage(TCommandMessage::InvalidArgumentsMessage);
                return reply;
            }
            if (!clientNames.contains(sl.last())) {
                reply.setMessage(TCommandMessage::InvalidArgumentsMessage);
                return reply;
            }
            if (sl.last().startsWith("texsample"))
                client = "tcsl";
            else if (sl.last().startsWith("tex"))
                client = "tcrt";
            else
                client = "clab";
            bclient = true;
        } else if (a.startsWith("--os=") || a.startsWith("-o=")) {
            if (bos) {
                reply.setMessage(TCommandMessage::InvalidArgumentsMessage);
                return reply;
            }
            QStringList sl = a.split('=');
            if (sl.size() != 2) {
                reply.setMessage(TCommandMessage::InvalidArgumentsMessage);
                return reply;
            }
            if (!osNames.contains(sl.last())) {
                reply.setMessage(TCommandMessage::InvalidArgumentsMessage);
                return reply;
            }
            if (sl.last().startsWith("l"))
                os = "lin";
            else if (sl.last().startsWith("m"))
                os = "mac";
            else
                os = "win";
            bos = true;
        } else if (a.startsWith("--arch=") || a.startsWith("-a=")) {
            if (barch) {
                reply.setMessage(TCommandMessage::InvalidArgumentsMessage);
                return reply;
            }
            QStringList sl = a.split('=');
            if (sl.size() != 2) {
                reply.setMessage(TCommandMessage::InvalidArgumentsMessage);
                return reply;
            }
            if (!archMap.contains(sl.last())) {
                reply.setMessage(TCommandMessage::InvalidArgumentsMessage);
                return reply;
            }
            arch = archMap.value(sl.last());
            barch = true;
        } else if (a == "--portable" || a == "-p") {
            if (bportable) {
                reply.setMessage(TCommandMessage::InvalidArgumentsMessage);
                return reply;
            }
            portable = true;
            bportable = true;
        } else {
            reply.setMessage(TCommandMessage::InvalidArgumentsMessage);
            return reply;
        }
    }
    if (!bclient || !bos || !barch) {
        reply.setMessage(TCommandMessage::InvalidArgumentsMessage);
        return reply;
    }
    BVersion version = BVersion(args.at(args.size() - 2));
    QUrl url = QUrl::fromUserInput(args.at(args.size() - 1));
    if (!version.isValid() || !url.isValid()) {
        reply.setMessage(TCommandMessage::InvalidArgumentsMessage);
        return reply;
    }
    QString s = "AppVersion/" + client + "/" + os + "/" + QString::number(int(arch)) + "/";
    s += portable ? "portable/" : "normal/";
    bSettings->setValue(s + "version", version);
    bSettings->setValue(s + "url", url.toString());
    reply.setMessage(TCommandMessage::OkMessage);
    reply.setSuccess(true);
    return reply;
}

TExecuteCommandReplyData Application::executeStartServer(const QStringList &args)
{
    QMutexLocker locker(&serverMutex);
    Q_UNUSED(locker)
    TExecuteCommandReplyData reply;
    if (args.size() > 1) {
        reply.setMessage(TCommandMessage::InvalidArgumentCountMessage);
        return reply;
    }
    if (!server()) {
        reply.setMessage(TCommandMessage::InternalErrorMessage);
        return reply;
    }
    if (server()->isListening()) {
        reply.setMessage(TCommandMessage::ServerAlreadyListeningMessage);
        return reply;
    }
    if (!server()->listen(!args.isEmpty() ? args.first() : QString("0.0.0.0"), Texsample::MainPort)) {
        reply.setMessage(TCommandMessage::FailedToStartServerMessage);
        return reply;
    }
    reply.setSuccess(true);
    reply.setMessage(TCommandMessage::OkMessage);
    return reply;
}

TExecuteCommandReplyData Application::executeStopServer(const QStringList &args)
{
    QMutexLocker locker(&serverMutex);
    Q_UNUSED(locker)
    TExecuteCommandReplyData reply;
    if (!args.isEmpty()) {
        reply.setMessage(TCommandMessage::InvalidArgumentCountMessage);
        return reply;
    }
    if (!server()) {
        reply.setMessage(TCommandMessage::InternalErrorMessage);
        return reply;
    }
    if (!server()->isListening()) {
        reply.setMessage(TCommandMessage::ServerNotListeningMessage);
        return reply;
    }
    server()->close();
    reply.setSuccess(true);
    reply.setMessage(TCommandMessage::OkMessage);
    return reply;
}

TExecuteCommandReplyData Application::executeUptime(const QStringList &args)
{
    TExecuteCommandReplyData reply;
    if (!args.isEmpty()) {
        reply.setMessage(TCommandMessage::InvalidArgumentCountMessage);
        return reply;
    }
    if (!tApp) {
        reply.setMessage(TCommandMessage::InternalErrorMessage);
        return reply;
    }
    reply.setMessage(TCommandMessage::UptimeMessage);
    reply.setArguments(QStringList() << msecsToString(tApp->melapsedTimer.elapsed()));
    reply.setSuccess(true);
    return reply;
}

TExecuteCommandReplyData Application::executeUser(const QStringList &args)
{
    TExecuteCommandReplyData reply;
    if (args.size() > 2) {
        reply.setMessage(TCommandMessage::InvalidArgumentCountMessage);
        return reply;
    }
    QMutexLocker locker(&serverMutex);
    Q_UNUSED(locker)
    if (!server()) {
        reply.setMessage(TCommandMessage::InternalErrorMessage);
        return reply;
    }
    if (args.isEmpty()) {
        reply.setMessage(TCommandMessage::UserCountMessage);
        server()->lock();
        int count = server()->currentConnectionCount();
        server()->unlock();
        reply.setArguments(QStringList() << QString::number(count));
    } else if (args.first() == "--list") {
        QStringList list;
        server()->lock();
        foreach (BNetworkConnection *c, server()->connections()) {
            TUserInfo info = static_cast<Connection *>(c)->userInfo();
            list << ("[" + info.login() + "] [" + c->peerAddress() + "] [" + c->uniqueId().toString(true) + "]");
        }
        server()->unlock();
        reply.setMessage(TCommandMessage::UserInfoListMessage);
        reply.setArguments(QStringList() << QString::number(list.size()) << list.join("\n"));
    } else if (args.size() == 2) {
        QList<Connection *> users;
        BUuid uuid(args.at(1));
        server()->lock();
        if (uuid.isNull()) {
            foreach (BNetworkConnection *c, sServer->connections()) {
                Connection *cc = static_cast<Connection *>(c);
                if (cc->userInfo().login() == args.at(1))
                    users << cc;
            }
        } else {
            foreach (BNetworkConnection *c, sServer->connections()) {
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
            server()->unlock();
            reply.setMessage(TCommandMessage::UserKickedMessage);
            reply.setArguments(QStringList() << args.at(1) << QString::number(users.size()));
            reply.setSuccess(true);
            return reply;
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
            reply.setMessage(TCommandMessage::UserInfoListMessage);
            reply.setArguments(QStringList() << QString::number(list.size()) << list.join("\n"));
        } else if (args.first() == "--uptime") {
            QStringList list;
            foreach (Connection *c, users) {
                TUserInfo info = c->userInfo();
                QString s = "[" + info.login() + "] [" + c->peerAddress() + "] [" + c->uniqueId().toString(true) + "]";
                s += " " + msecsToString(c->uptime());
                list << s;
            }
            reply.setMessage(TCommandMessage::UsersUptimeMessage);
            reply.setArguments(QStringList() << QString::number(list.size()) << list.join("\n"));
        } else if (args.first() == "--connected-at") {
            QStringList list;
            foreach (Connection *c, users) {
                TUserInfo info = c->userInfo();
                QString s = "[" + info.login() + "] [" + c->peerAddress() + "] [" + c->uniqueId().toString(true) + "]";
                s += " " + c->connectionDT().toString("yyyy.MM.dd hh:mm:ss");
                list << s;
            }
            reply.setMessage(TCommandMessage::UsersConnectedAtMessage);
            reply.setArguments(QStringList() << QString::number(list.size()) << list.join("\n"));
        }
        server()->unlock();
    } else {
        reply.setMessage(TCommandMessage::InvalidArgumentCountMessage);
        return reply;
    }
    reply.setSuccess(true);
    return reply;
}*/

bool Application::initializeStorage()
{
    static bool initialized = false;
    if (initialized) {
        bWriteLine(tr("Storage already initialized", "message"));
        return true;
    }
    bWriteLine(tr("Initializing storage...", "message"));
    Application *app = tApp;
    if (!app) {
        bWriteLine(tr("Error: No application instance", "error"));
        return false;
    }
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
    if (!app->Source->initialize(&err)) {
        bWriteLine(tr("Error:", "error") + " " + err);
        return false;
    }
    if (app->UserServ->isRootInitialized()) {
        bWriteLine(tr("Done!", "message"));
        return true;
    }
    if (!app->UserServ->initializeRoot(&err)) {
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
    return bApp ? bApp->mserver : 0;
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
    //TExecuteCommandReplyData reply = executeSetAppVersion(args);
    //writeCommandMessage(reply.message(), reply.arguments());
    //return reply.success();
}

bool Application::handleStartCommand(const QString &, const QStringList &args)
{
    //TExecuteCommandReplyData reply = executeStartServer(args);
    //writeCommandMessage(reply.message(), reply.arguments());
    //return reply.success();
}

bool Application::handleStopCommand(const QString &, const QStringList &args)
{
    //TExecuteCommandReplyData reply = executeStopServer(args);
    //writeCommandMessage(reply.message(), reply.arguments());
    //return reply.success();
}

bool Application::handleUnknownCommand(const QString &, const QStringList &)
{
    bWriteLine(tr("Unknown command. Enter \"help --commands\" to see the list of available commands"));
    return false;
}

bool Application::handleUptimeCommand(const QString &, const QStringList &args)
{
    //TExecuteCommandReplyData reply = executeUptime(args);
    //writeCommandMessage(reply.message(), reply.arguments());
    //return reply.success();
}

bool Application::handleUserCommand(const QString &, const QStringList &args)
{
    //TExecuteCommandReplyData reply = executeUser(args);
    //writeCommandMessage(reply.message(), reply.arguments());
    //return reply.success();
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
    ch.usage = "uptime";
    ch.description = BTranslation::translate("BTerminalIOHandler",
                                             "Show for how long the application has been running");
    BTerminal::setCommandHelp("uptime", ch);
    ch.usage = "user [--list]";
    ch.description = BTranslation::translate("BTerminalIOHandler", "Show connected user count or list them all");
    chl << ch;
    ch.usage = "user --connected-at|--info|--uptime <id|login>";
    ch.description = BTranslation::translate("BTerminalIOHandler", "Show information about the user. "
                                             "The user may be specified by id or by login. Options:\n"
                                             "  --connected-at - time when the user connected\n"
                                             "  --info - detailed user information\n"
                                             "  --uptime - for how long the user has been connected");
    chl << ch;
    ch.usage = "user --kick <id|login>";
    ch.description = BTranslation::translate("BTerminalIOHandler", "Disconnect the specified user. "
                                             "If login is specified, all connections of this user will be closed");
    chl << ch;
    BTerminal::setCommandHelp("user", chl);
    ch.usage = "start [address]";
    ch.description = BTranslation::translate("BTerminalIOHandler", "Start the server. "
                                             "If address is specified, the server will only listen on that address, "
                                             "otherwise it will listen on available all addresses.");
    BTerminal::setCommandHelp("start", ch);
    ch.usage = "stop";
    ch.description = BTranslation::translate("BTerminalIOHandler", "Stop the server. Users are NOT disconnected");
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
