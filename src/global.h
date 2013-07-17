#ifndef GLOBAL_H
#define GLOBAL_H

class TOperationResult;
class TMessage;

class BNetworkOperation;

class QLocale;

#include <TProject>
#include <TCompilerParameters>
#include <TCompilationResult>
#include <TCompiledProject>

#include <QString>
#include <QMap>
#include <QVariantMap>

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
TOperationResult sendEmail(const QString &receiver, const QString &templateName, const QLocale &locale,
                           const StringMap &replace = StringMap());
TCompilationResult compileProject(const CompileParameters &p);
bool sendReply(BNetworkOperation *op, QVariantMap out, const TOperationResult &r);
bool sendReply(BNetworkOperation *op, QVariantMap out, const TCompilationResult &r);
bool sendReply(BNetworkOperation *op, QVariantMap out, const TMessage msg);
bool sendReply(BNetworkOperation *op, const TOperationResult &r);
bool sendReply(BNetworkOperation *op, const TCompilationResult &r);
bool sendReply(BNetworkOperation *op, const TMessage msg);
bool sendReply(BNetworkOperation *op, QVariantMap out);
bool sendReply(BNetworkOperation *op);

}

#endif // GLOBAL_H
