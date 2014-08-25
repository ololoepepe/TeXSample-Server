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

#include "sampleservice.h"

#include "datasource.h"
#include "entity/sample.h"
#include "repository/samplerepository.h"

/*============================================================================
================================ SampleService ===============================
============================================================================*/

/*============================== Public constructors =======================*/

SampleService::SampleService(DataSource *source) :
    SampleRepo(new SampleRepository(source)), Source(source)
{
    //
}

SampleService::~SampleService()
{
    //
}

/*============================== Public methods ============================*/

DataSource *SampleService::dataSource() const
{
    return Source;
}

bool SampleService::isValid() const
{
    return Source && Source->isValid() && SampleRepo->isValid();
}

/*TCompilationResult compileProject(const CompileParameters &p)
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
