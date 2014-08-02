class BSettingsNode;

#include "global.h"
#include "storage.h"

//#include <TCompilationResult>
//#include <TCompilerParameters>
#include <TTexProject>
//#include <TCompiledProject>
#include <TMessage>
//#include <TOperationResult>

#include <BeQt>
#include <BDirTools>
#include <BGenericSocket>
#include <BSmtpSender>
#include <BEmail>

#include <QString>
#include <QFileInfo>
#include <QStringList>
#include <QFile>
#include <QCoreApplication>
#include <QSslSocket>
#include <QTextStream>
#include <QSettings>
#include <QLocale>
#include <QFileInfo>
#include <QLocale>
#include <QMap>
#include <QVariant>

#include <QDebug>

B_DECLARE_TRANSLATE_FUNCTION

namespace Global
{

QString mmailPassword;

bool readOnly()
{
    init_once(bool, ro, false)
        ro = QCoreApplication::arguments().contains("--read-only");
    return ro;
}

bool noMail()
{
    init_once(bool, nm, false)
        nm = QCoreApplication::arguments().contains("--no-mail");
    return nm;
}

bool initMail()
{
    /*static bool initialized = false;
    if (initialized)
    {
        bWriteLine(translate("Global", "E-mail already initialized"));
        return true;
    }
    bWriteLine(translate("Global", "Initializing e-mail..."));
    QString address = bSettings->value("Mail/server_address").toString();
    if (address.isEmpty())
    {
        address = bReadLine(translate("Global", "Enter e-mail server address:") + " ");
        if (address.isEmpty())
            return bRet(errs, translate("Global", "Invalid address"), false);
    }
    QString port;
    if (!bSettings->contains("Mail/server_port"))
        port = bReadLine(translate("Global", "Enter e-mail server port (default 25):") + " ");
    QVariant vport(!port.isEmpty() ? port : bSettings->value("Mail/server_port", QString("25")).toString());
    if (!vport.convert(QVariant::UInt))
        return bRet(errs, translate("Global", "Invalid port"), false);
    QString name;
    if (!bSettings->contains("Mail/local_host_name"))
        name = bReadLine(translate("Global", "Enter local host name or empty string:") + " ");
    else
        name = bSettings->value("Mail/local_host_name").toString();
    QString ssl;
    if (!bSettings->contains("Mail/ssl_required"))
        ssl = bReadLine(translate("Global", "Enter SSL mode [true|false] (default false):") + " ");
    QVariant vssl(!ssl.isEmpty() ? ssl : bSettings->value("Mail/ssl_required", QString("false")).toString());
    if (!vssl.convert(QVariant::Bool))
        return bRet(errs, translate("Global", "Invalid value"), false);
    QString login = bSettings->value("Mail/login").toString();
    if (login.isEmpty())
    {
        login = bReadLine(translate("Global", "Enter e-mail login:") + " ");
        if (login.isEmpty())
            return bRet(errs, translate("Global", "Invalid login"), false);
    }
    mmailPassword = bReadLineSecure(translate("Global", "Enter e-mail password:") + " ");
    if (mmailPassword.isEmpty())
        return bRet(errs, translate("Global", "Invalid password"), false);
    bSettings->setValue("Mail/server_address", address);
    bSettings->setValue("Mail/server_port", vport);
    bSettings->setValue("Mail/local_host_name", name);
    bSettings->setValue("Mail/ssl_required", vssl);
    bSettings->setValue("Mail/login", login);
    initialized = true;
    bWriteLine(translate("Global", "Done!"));*/
    return true;
}

bool setMailPassword(const BSettingsNode *, const QVariant &v)
{
    //mmailPassword = !v.isNull() ? v.toString() : bReadLineSecure(translate("Global", "Enter e-mail password:") + " ");
    //return !mmailPassword.isEmpty();
}

bool showMailPassword(const BSettingsNode *, const QVariant &)
{
    /*if (!QVariant(bReadLine(translate("Global", "Printing password is unsecure! Do tou want to continue?") +
                            " [yes|No] ")).toBool())
        return false;
    bWriteLine(translate("Global", "Value for") + " \"password\": " + mmailPassword);
    return true;*/
}

bool setLoggingMode(const BSettingsNode *, const QVariant &v)
{
    /*QString s = !v.isNull() ? v.toString() : bReadLine(translate("Global", "Enter logging mode:") + " ");
    if (s.isEmpty())
        return false;
    bool ok = false;
    int m = s.toInt(&ok);
    if (!ok)
        return false;
    bSettings->setValue("Log/mode", m);
    resetLoggingMode();
    return true;*/
}

void resetLoggingMode()
{
    int m = bSettings->value("Log/mode", 2).toInt();
    if (m <= 0)
    {
        bLogger->setLogToConsoleEnabled(false);
        bLogger->setLogToFileEnabled(false);
    }
    else if (1 == m)
    {
        bLogger->setLogToConsoleEnabled(true);
        bLogger->setLogToFileEnabled(false);
    }
    else if (2 == m)
    {
        bLogger->setLogToConsoleEnabled(false);
        bLogger->setLogToFileEnabled(true);
    }
    else if (m >= 3)
    {
        bLogger->setLogToConsoleEnabled(true);
        bLogger->setLogToFileEnabled(true);
    }
}

QString mailPassword()
{
    return mmailPassword;
}

/*TOperationResult sendEmail(const QString &receiver, const QString &templateName, const QLocale &locale,
                           const StringMap &replace)
{
    if (receiver.isEmpty() || templateName.isEmpty())
        return TOperationResult(TMessage::InternalParametersError);
    QString templatePath = BDirTools::findResource("templates/" + templateName, BDirTools::GlobalOnly);
    if (!QFileInfo(templatePath).isDir())
        return TOperationResult(TMessage::InternalParametersError);
    QString subject = BDirTools::localeBasedFileName(templatePath + "/subject.txt", locale);
    QString body = BDirTools::localeBasedFileName(templatePath + "/body.txt", locale);
    bool ok = false;
    subject = BDirTools::readTextFile(subject, "UTF-8", &ok);
    if (!ok)
        return TOperationResult(TMessage::InternalFileSystemError);
    body = BDirTools::readTextFile(body, "UTF-8", &ok);
    if (!ok)
        return TOperationResult(TMessage::InternalFileSystemError);
    foreach (const QString &k, replace.keys())
    {
        QString v = replace.value(k);
        subject.replace(k, v);
        body.replace(k, v);
    }
    BSmtpSender smtp;
    smtp.setServer(bSettings->value("Mail/server_address").toString(),
                   bSettings->value("Mail/server_port", 25).toUInt());
    smtp.setLocalHostName(bSettings->value("Mail/local_host_name").toString());
    smtp.setSocketType(bSettings->value("Mail/ssl_required").toBool() ? BGenericSocket::SslSocket :
                                                                        BGenericSocket::TcpSocket);
    smtp.setUser(bSettings->value("Mail/login").toString(), mmailPassword);
    BEmail email;
    email.setSender("TeXSample Team");
    email.setReceiver(receiver);
    email.setSubject(subject);
    email.setBody(body);
    smtp.setEmail(email);
    smtp.send();
    bool b = smtp.waitForFinished();
    return TOperationResult(b, b ? TMessage::NoMessage : TMessage::InternalNetworkError);
}

TCompilationResult compileProject(const CompileParameters &p)
{
    static const QStringList Suffixes = QStringList() << "*.aux" << "*.dvi" << "*.idx" << "*.ilg" << "*.ind"
                                                      << "*.log" << "*.out" << "*.pdf" << "*.toc";
    if (!p.project.isValid())
        return TCompilationResult(TMessage::InternalParametersError);
    if (p.path.isEmpty() || !BDirTools::mkpath(p.path))
        return TCompilationResult(TMessage::InternalFileSystemError);
    QString codecName = p.compiledProject ? p.param.codecName() : QString("UTF-8");
    if (!p.project.save(p.path, codecName) || !Storage::copyTexsample(p.path, codecName))
    {
        BDirTools::rmdir(p.path);
        return TCompilationResult(TMessage::InternalFileSystemError);
    }
    QString fn = p.project.rootFileName();
    QString tmpfn = BeQt::pureUuidText(QUuid::createUuid()) + ".tex";
    QString baseName = QFileInfo(fn).baseName();
    TCompilationResult r;
    QString command = !p.compiledProject ? "pdflatex" : p.param.compilerCommand();
    QStringList args = QStringList() << "-interaction=nonstopmode";
    if (!p.compiledProject)
    {
        if (!QFile::rename(p.path + "/" + fn, p.path + "/" + tmpfn))
            return TCompilationResult(TMessage::InternalFileSystemError);
        args << ("-jobname=" + baseName) << ("\\input texsample.tex \\input " + tmpfn + " \\end{document}");
    }
    else
    {
        args << p.param.commands() << (p.path + "/" + fn) << p.param.options();
    }
    QString log;
    int code = BeQt::execProcess(p.path, command, args, 5 * BeQt::Second, 2 * BeQt::Minute, &log);
    if (!p.compiledProject && !QFile::rename(p.path + "/" + tmpfn, p.path + "/" + fn))
        return TCompilationResult(TMessage::InternalFileSystemError);
    if (!Storage::removeTexsample(p.path))
        return TCompilationResult(TMessage::InternalFileSystemError);
    r.setSuccess(!code);
    r.setExitCode(code);
    r.setLog(log);
    if (p.compiledProject && r && p.param.makeindexEnabled())
    {
        QString mlog;
        int mcode = BeQt::execProcess(p.path, "makeindex", QStringList() << (p.path + "/" + baseName),
                                      5 * BeQt::Second, BeQt::Minute, &mlog);
        if (p.makeindexResult)
        {
            p.makeindexResult->setSuccess(!mcode);
            p.makeindexResult->setExitCode(mcode);
            p.makeindexResult->setLog(mlog);
        }
        if (!mcode)
        {
            code = BeQt::execProcess(p.path, p.param.compilerCommand(), args, 5 * BeQt::Second, 5 * BeQt::Minute, &log);
            r.setSuccess(!code);
            r.setExitCode(code);
            r.setLog(log);
        }
    }
    if (p.compiledProject && r && p.param.dvipsEnabled())
    {
        QString dlog;
        int dcode = BeQt::execProcess(p.path, "dvips", QStringList() << (p.path + "/" + baseName),
                                      5 * BeQt::Second, BeQt::Minute, &dlog);
        if (p.dvipsResult)
        {
            p.dvipsResult->setSuccess(!dcode);
            p.dvipsResult->setExitCode(dcode);
            p.dvipsResult->setLog(dlog);
        }
    }
    if (r && p.compiledProject)
        p.compiledProject->load(p.path, Suffixes);
    if (p.compiledProject || !r)
        BDirTools::rmdir(p.path);
    return r;
}*/

}
