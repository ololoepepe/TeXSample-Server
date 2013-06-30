#include "global.h"
#include "storage.h"
#include "terminaliohandler.h"

#include <TCompilationResult>
#include <TCompilerParameters>
#include <TProject>
#include <TCompiledProject>

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

namespace Global
{

TOperationResult notAuthorizedResult()
{
    return TOperationResult(QCoreApplication::translate("Global", "Not authorized", "errorString"));
}

TOperationResult sendEmail(const QString &receiver, const QString &subject, const QString &body, int timeout)
{
    if (receiver.isEmpty() || subject.isEmpty() || body.isEmpty())
        return Storage::invalidParametersResult();
    BSmtpSender smtp;
    smtp.setServer(bSettings->value("Mail/server_address").toString(),
                   bSettings->value("Mail/server_port", 25).toUInt());
    smtp.setLocalHostName("texsample-server.no-ip.org");
    smtp.setSocketType(bSettings->value("Mail/ssl_required").toBool() ? BGenericSocket::SslSocket :
                                                                        BGenericSocket::TcpSocket);
    smtp.setUser(bSettings->value("Mail/login").toString(), TerminalIOHandler::mailPassword());
    BEmail email;
    email.setSender("TeXSample Team");
    email.setReceiver(receiver);
    email.setSubject(subject);
    email.setBody(body);
    bool b = smtp.waitForFinished(timeout);
    return TOperationResult(b, smtp.lastTransferError());
}

TCompilationResult compileProject(const QString &path, TProject project, const TCompilerParameters &param,
                                  TCompiledProject *compiledProject, TCompilationResult *makeindexResult,
                                  TCompilationResult *dvipsResult)
{
    static const QStringList Suffixes = QStringList() << "*.aux" << "*.dvi" << "*.idx" << "*.ilg" << "*.ind"
                                                      << "*.log" << "*.out" << "*.pdf" << "*.toc";
    if (path.isEmpty() || !project.isValid())
        return Storage::invalidParametersResult();
    if (!BDirTools::mkpath(path))
        return Storage::fileSystemErrorResult();
    QString codecName = compiledProject ? param.codecName() : QString("UTF-8");
    if (!project.save(path, codecName) || !Storage::copyTexsample(path, codecName))
    {
        BDirTools::rmdir(path);
        return Storage::fileSystemErrorResult();
    }
    QString fn = project.rootFileName();
    QString tmpfn = BeQt::pureUuidText(QUuid::createUuid()) + ".tex";
    QString baseName = QFileInfo(fn).baseName();
    TCompilationResult r;
    QString command = !compiledProject ? "pdflatex" : param.compilerString();
    QStringList args = QStringList() << "-interaction=nonstopmode";
    if (!compiledProject)
    {
        if (!QFile::rename(path + "/" + fn, path + "/" + tmpfn))
            return Storage::fileSystemErrorResult();
        args << ("-jobname=" + baseName)
             << ("\\input texsample.tex \\input " + tmpfn + " \\end{document}");
    }
    else
    {
        args << param.commands() << (path + "/" + fn) << param.options();
    }
    QString log;
    int code = BeQt::execProcess(path, command, args, 5 * BeQt::Second, 2 * BeQt::Minute, &log);
    if (!compiledProject && !QFile::rename(path + "/" + tmpfn, path + "/" + fn))
        return Storage::fileSystemErrorResult();
    if (!Storage::removeTexsample(path))
        return Storage::fileSystemErrorResult();
    r.setSuccess(!code);
    r.setExitCode(code);
    r.setLog(log);
    if (compiledProject && r && param.makeindexEnabled())
    {
        QString mlog;
        int mcode = BeQt::execProcess(path, "makeindex", QStringList() << (path + "/" + baseName),
                                      5 * BeQt::Second, BeQt::Minute, &mlog);
        if (makeindexResult)
        {
            makeindexResult->setSuccess(!mcode);
            makeindexResult->setExitCode(mcode);
            makeindexResult->setLog(mlog);
        }
        if (!mcode)
        {
            code = BeQt::execProcess(path, param.compilerString(), args, 5 * BeQt::Second, 5 * BeQt::Minute, &log);
            r.setSuccess(!code);
            r.setExitCode(code);
            r.setLog(log);
        }
    }
    if (compiledProject && r && param.dvipsEnabled())
    {
        QString dlog;
        int dcode = BeQt::execProcess(path, "dvips", QStringList() << (path + "/" + baseName),
                                      5 * BeQt::Second, BeQt::Minute, &dlog);
        if (dvipsResult)
        {
            dvipsResult->setSuccess(!dcode);
            dvipsResult->setExitCode(dcode);
            dvipsResult->setLog(dlog);
        }
    }
    if (r && compiledProject)
        compiledProject->load(path, Suffixes);
    if (compiledProject || !r)
        BDirTools::rmdir(path);
    return r;
}

}
