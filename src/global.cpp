#include "global.h"
#include "storage.h"

#include <TCompilationResult>
#include <TCompilerParameters>
#include <TProject>
#include <TCompiledProject>

#include <BeQt>
#include <BDirTools>
#include <BGenericSocket>

#include <QString>
#include <QFileInfo>
#include <QStringList>
#include <QFile>
#include <QCoreApplication>
#include <QSslSocket>
#include <QTextStream>

namespace Global
{

TOperationResult notAuthorizedResult()
{
    return TOperationResult(QCoreApplication::translate("Global", "Not authorized", "errorString"));
}

TOperationResult sendMail(Host host, User user, Mail mail, bool ssl)
{
    mail.to.removeAll("");
    mail.to.removeDuplicates();
    if (host.address.isEmpty() || !host.port || (user.login.isEmpty() && mail.from.isEmpty()) || mail.to.isEmpty()
            || mail.body.isEmpty())
        return TOperationResult(QCoreApplication::translate("Global", "Invalid parameters", "errorString"));
    if (!host.port)
        host.port = 25;
    if (mail.from.isEmpty())
        mail.from = user.login;
    if (mail.subject.isEmpty())
        mail.subject = "No subject";
    BGenericSocket s(ssl ? BGenericSocket::SslSocket : BGenericSocket::TcpSocket);
    if (ssl)
        s.sslSocket()->connectToHostEncrypted(host.address, host.port);
    else
        s.connectToHost(host.address, host.port);
    if ((ssl && !s.sslSocket()->waitForEncrypted(5 * BeQt::Second)) || (!ssl && !s.waitForConnected(5 * BeQt::Second)))
        return TOperationResult(QCoreApplication::translate("Global", "Connection timeout", "errorString"));
    QTextStream t(s.ioDevice());
    QString text;
    QString line;
    int stage = 0;
    int authStage = 0;
    int currTo = 0;
    TOperationResult err = TOperationResult(QCoreApplication::translate("Global", "Communication failed",
                                                                        "errorString"));
    if (s.bytesAvailable() <= 0 && !s.waitForReadyRead(5 * BeQt::Second))
        return TOperationResult(QCoreApplication::translate("Global", "No response", "errorString"));
    do
    {
        if (!t.device()->canReadLine())
            return TOperationResult(QCoreApplication::translate("Global", "Failed to read line", "errorString"));
        do
        {
            line = t.readLine();
            text += line;
        }
        while (t.device()->canReadLine() && line.length() > 3 && line.at(3) != ' ');
        line.truncate(3);
        switch (stage)
        {
        case 0:
            if (line.at(0) != '2')
                return err;
            t << ("HELO" + (!host.name.isEmpty() ? host.name : QString()));
            break;
        case 1:
            --stage;
            switch (authStage)
            {
            case 0:
                t << "AUTH LOGIN";
                break;
            case 1:
                t << QString(user.login.toAscii().toBase64());
                break;
            case 2:
            default:
                t << QString(user.password.toAscii().toBase64());
                ++stage;
                break;
            }
            ++authStage;
            break;
        case 2:
            if (line.at(0) != '2')
                return err;
            t << ("MAIL FROM: <" + mail.from + ">");
            break;
        case 3:
            if (line.at(0) != '2')
                return err;
            t << ("RCPT TO: <" + mail.to.at(currTo) + ">");
            ++currTo;
            if (currTo < mail.to.size())
                --stage;
            break;
        case 4:
            if (line.at(0) != '2')
                return err;
            t << "DATA";
            break;
        case 5:
            if (line.at(0) != '3')
                return err;
            t << ("from: " + mail.from + "\nto: " + mail.to.first() +
                  "\nsubject: " + mail.subject + "\n\n" + mail.body + "\r\n.\r");
            break;
        case 6:
            if (line.at(0) != '2')
                return err;
            t << "QUIT";
            break;
        case 7:
            if (line.at(0) != '2')
                return err;
            s.disconnectFromHost();
            return TOperationResult(s.waitForDisconnected(5 * BeQt::Second));
            break;
        default:
            return err;
        }
        t << "\n";
        t.flush();
        if (!s.waitForBytesWritten(5 * BeQt::Second))
            return err;
        ++stage;
    }
    while (s.bytesAvailable() > 0 || s.waitForReadyRead(5 * BeQt::Second));
    return TOperationResult(err);
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
