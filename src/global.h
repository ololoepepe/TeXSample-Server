#ifndef GLOBAL_H
#define GLOBAL_H

class Storage;

class TOperationResult;
class TCompilationResult;
class TProject;
class TCompiledProject;

#include <TCompilerParameters>

#include <BNetworkOperation>
#include <BNetworkConnection>

#include <QString>
#include <QVariantMap>

namespace Global
{

TCompilationResult compileProject(const QString &path, TProject project,
                                  const TCompilerParameters &param = TCompilerParameters(),
                                  TCompiledProject *compiledProject = 0, TCompilationResult *makeindexResult = 0,
                                  TCompilationResult *dvipsResult = 0);

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
