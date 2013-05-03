#include "global.h"
#include "storage.h"

#include <TCompilationResult>
#include <TCompilerParameters>
#include <TProject>
#include <TCompiledProject>

#include <BeQt>
#include <BDirTools>

#include <QString>
#include <QFileInfo>
#include <QStringList>
#include <QFile>

namespace Global
{

bool copyTexsample(const QString &path, const QString &codecName = "UTF-8")
{
    QString cn = (!codecName.isEmpty() ? codecName : QString("UTF-8")).toLower();
    QString sty = BDirTools::findResource("texsample/" + cn + "/texsample.sty");
    QString tex = BDirTools::findResource("texsample/" + cn + "/texsample.tex");
    return !sty.isEmpty() && !tex.isEmpty() && QFile::copy(sty, path + "/texsample.sty")
            && QFile::copy(tex, path + "/texsample.tex");
}

bool removeTexsample(const QString &path)
{
    return BDirTools::removeFilesInDir(path, QStringList() << "texsample.sty" << "texsample.tex");
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
    if (!project.save(path, codecName) || !copyTexsample(path, codecName))
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
    if (!removeTexsample(path))
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
