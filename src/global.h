#ifndef GLOBAL_H
#define GLOBAL_H

class Storage;
class Translator;

class TOperationResult;

#include <TProject>
#include <TCompiledProject>
#include <TCompilerParameters>
#include <TCompilationResult>

#include <BNetworkOperation>
#include <BNetworkConnection>
#include <BTranslator>

#include <QString>
#include <QStringList>
#include <QVariantMap>

#define DECLARE_TRANSLATE_FUNCTION \
static QString translate(const char *context, const char *sourceText, BTranslator *translator) \
{ \
    return translate(context, sourceText, translator); \
} \
static QString translate(const char *context, const char *sourceText, const char *disambiguation, \
                         BTranslator *translator) \
{ \
    return translator ? translator->translate(context, sourceText, disambiguation) : \
                        QCoreApplication::translate(context, sourceText, disambiguation); \
}

namespace Global
{

enum Message
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
    NOMESSAGE
};

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

QString string(Message msg, Translator *t = 0);

TOperationResult result(Message msg, Translator *t = 0);

TCompilationResult compilationResult(Message msg, Translator *t = 0);

TCompilationResult compileProject(const CompileParameters &p, Translator *t = 0);

TOperationResult sendEmail(const QString &receiver, const QString &subject, const QString &body, Translator *t = 0);

template<typename T> void sendReply(BNetworkOperation *op, QVariantMap out, const QString &key, const T &t)
{
    if (!op || !op->connection() || key.isEmpty())
        return;
    out.insert(key, t);
    op->connection()->sendReply(op, out);
}

template<typename T> void sendReply(BNetworkOperation *op, const QString &key, const T &t)
{
    QVariantMap out;
    sendReply(op, out, key, t);
}

template<typename T> void sendReply(BNetworkOperation *op, QVariantMap out, const T &t)
{
    sendReply(op, out, "operation_result", t);
}

template<typename T> void sendReply(BNetworkOperation *op, const T &t)
{
    sendReply(op, "operation_result", t);
}

}

#endif // GLOBAL_H
