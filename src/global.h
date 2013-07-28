#ifndef GLOBAL_H
#define GLOBAL_H

class TOperationResult;
class TMessage;

class BSettingsNode;

class QLocale;
class QVariant;

#include <TTexProject>
#include <TCompilerParameters>
#include <TCompilationResult>
#include <TCompiledProject>

#include <QString>
#include <QMap>

namespace Global
{

typedef QMap<QString, QString> StringMap;

struct CompileParameters
{
    QString path;
    mutable TTexProject project;
    TCompilerParameters param;
    mutable TCompiledProject *compiledProject;
    mutable TCompilationResult *makeindexResult;
    mutable TCompilationResult *dvipsResult;
public:
    CompileParameters()
    {
        compiledProject = 0;
        makeindexResult = 0;
        dvipsResult = 0;
    }
    ~CompileParameters()
    {
        delete compiledProject;
        delete makeindexResult;
        delete dvipsResult;
    }
};

bool readOnly();
bool noMail();
bool initMail(QString *errs = 0);
bool setMailPassword(const BSettingsNode *n, const QVariant &v = QVariant());
bool showMailPassword(const BSettingsNode *n, const QVariant &);
bool setLoggingMode(const BSettingsNode *n, const QVariant &v = QVariant());
void resetLoggingMode();
QString mailPassword();
TOperationResult sendEmail(const QString &receiver, const QString &templateName, const QLocale &locale,
                           const StringMap &replace = StringMap());
TCompilationResult compileProject(const CompileParameters &p);

}

#endif // GLOBAL_H
