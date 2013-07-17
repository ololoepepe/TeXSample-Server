#include "global.h"
#include "storage.h"
#include "terminaliohandler.h"
#include "translator.h"

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

#include <QDebug>

namespace Global
{

struct TrStruct
{
    const char *source;
    const char *comment;
};

static const TrStruct Messages[] =
{
    QT_TRANSLATE_NOOP3("Global", "Referred to invalid storage instance", "message"),
    QT_TRANSLATE_NOOP3("Global", "Failed to commit transaction or execute internal operation", "message"),
    QT_TRANSLATE_NOOP3("Global", "Failed to execute query", "message"),
    QT_TRANSLATE_NOOP3("Global", "Failed to perform file system read or write operation", "message"),
    QT_TRANSLATE_NOOP3("Global", "Received invalid parameters", "message"),
    QT_TRANSLATE_NOOP3("Global", "Attempted to perform operation while not authorized", "message"),
    QT_TRANSLATE_NOOP3("Global", "Attempted to perform operation while not enough rights", "message"),
    QT_TRANSLATE_NOOP3("Global", "Attempted to perform operation while on another user's account", "message"),
    QT_TRANSLATE_NOOP3("Global", "Attempted to perform operation while on another user's sample", "message"),
    QT_TRANSLATE_NOOP3("Global", "Attempted to perform operation while on read-only sample", "message"),
    QT_TRANSLATE_NOOP3("Global", "Attempted to perform operation on nonexistent user", "message"),
    QT_TRANSLATE_NOOP3("Global", "Attempted to perform operation on nonexistent sample", "message"),
    QT_TRANSLATE_NOOP3("Global", "Attempted to register using occupied login or e-mail", "message"),
    QT_TRANSLATE_NOOP3("Global", "Attempted to register using invalid invite code", "message"),
    QT_TRANSLATE_NOOP3("Global", "Attempted to perform write operation in read-only mode", "message")
};

bool readOnly()
{
    init_once(bool, ro, false)
        ro = QCoreApplication::arguments().contains("--read-only");
    return ro;
}

QString string(Message msg, Translator *t)
{
    bool b = bRangeD(0, NOMESSAGE - 1).contains(msg);
    const char *source = b ? Messages[msg].source : 0;
    const char *comment = b ? Messages[msg].comment : 0;
    return t ? t->translate("Global", source, comment) : QString(source);
}

TOperationResult result(Message msg, Translator *t)
{
    return TOperationResult(0); //TODO
}

TCompilationResult compilationResult(Message msg, Translator *t)
{
    return TCompilationResult(0); //TODO
}

TOperationResult sendEmail(const QString &receiver, const QString &subject, const QString &body, Translator *t)
{
    if (receiver.isEmpty() || subject.isEmpty() || body.isEmpty())
        return result(InvalidParameters, t);
    BSmtpSender smtp;
    smtp.setServer(bSettings->value("Mail/server_address").toString(),
                   bSettings->value("Mail/server_port", 25).toUInt());
    smtp.setLocalHostName(bSettings->value("Mail/local_host_name").toString());
    smtp.setSocketType(bSettings->value("Mail/ssl_required").toBool() ? BGenericSocket::SslSocket :
                                                                        BGenericSocket::TcpSocket);
    smtp.setUser(bSettings->value("Mail/login").toString(), TerminalIOHandler::mailPassword());
    BEmail email;
    email.setSender("TeXSample Team");
    email.setReceiver(receiver);
    email.setSubject(subject);
    email.setBody(body);
    smtp.setEmail(email);
    smtp.send();
    bool b = smtp.waitForFinished();
    return TOperationResult(b, 0); //TODO
}

TCompilationResult compileProject(const CompileParameters &p, Translator *t)
{
    static const QStringList Suffixes = QStringList() << "*.aux" << "*.dvi" << "*.idx" << "*.ilg" << "*.ind"
                                                      << "*.log" << "*.out" << "*.pdf" << "*.toc";
    if (p.path.isEmpty() || !p.project.isValid())
        return compilationResult(NotAuthorized, t);
    if (!BDirTools::mkpath(p.path))
        return compilationResult(FileSystemError, t);
    QString codecName = p.compiledProject ? p.param.codecName() : QString("UTF-8");
    if (!p.project.save(p.path, codecName) || !Storage::copyTexsample(p.path, codecName))
    {
        BDirTools::rmdir(p.path);
        return compilationResult(FileSystemError, t);
    }
    QString fn = p.project.rootFileName();
    QString tmpfn = BeQt::pureUuidText(QUuid::createUuid()) + ".tex";
    QString baseName = QFileInfo(fn).baseName();
    TCompilationResult r;
    QString command = !p.compiledProject ? "pdflatex" : p.param.compilerString();
    QStringList args = QStringList() << "-interaction=nonstopmode";
    if (!p.compiledProject)
    {
        if (!QFile::rename(p.path + "/" + fn, p.path + "/" + tmpfn))
            return compilationResult(FileSystemError, t);
        args << ("-jobname=" + baseName)
             << ("\\input texsample.tex \\input " + tmpfn + " \\end{document}");
    }
    else
    {
        args << p.param.commands() << (p.path + "/" + fn) << p.param.options();
    }
    QString log;
    int code = BeQt::execProcess(p.path, command, args, 5 * BeQt::Second, 2 * BeQt::Minute, &log);
    if (!p.compiledProject && !QFile::rename(p.path + "/" + tmpfn, p.path + "/" + fn))
        return compilationResult(FileSystemError, t);
    if (!Storage::removeTexsample(p.path))
        return compilationResult(FileSystemError, t);
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
            code = BeQt::execProcess(p.path, p.param.compilerString(), args, 5 * BeQt::Second, 5 * BeQt::Minute, &log);
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
}

}
