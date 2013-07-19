#ifndef GLOBAL_H
#define GLOBAL_H

class TOperationResult;
class TMessage;

class QLocale;
class QVariant;

#include <TProject>
#include <TCompilerParameters>
#include <TCompilationResult>
#include <TCompiledProject>

#include <QString>
#include <QMap>

namespace Global
{

/*enum Message
{
    StorageError = 0,
    DatabaseError,
    QueryError,
    FileSystemError,
    InvalidParameters,
    NotAuthorized,
    NotEnoughRights,
    NotYourAccount,
    NotYourSample,
    NotModifiableSample,
    NoSuchUser,
    NoSuchSample,
    LoginOrEmailOccupied,
    InvalidInvite,
    ReadOnly,
    NOMESSAGE
};*/

typedef QMap<QString, QString> StringMap;

struct CompileParameters
{
    QString path;
    mutable TProject project;
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
bool setMailPassword(const QVariant &v = QVariant());
bool showMailPassword(const QVariant &);
QString mailPassword();
TOperationResult sendEmail(const QString &receiver, const QString &templateName, const QLocale &locale,
                           const StringMap &replace = StringMap());
TCompilationResult compileProject(const CompileParameters &p);

}

#endif // GLOBAL_H
