#ifndef USERCONNECTION_H
#define USERCONNECTION_H

class BGenericSocket;
class BNetworkOperation;

class QObject;

#include "include/texsampleserver.h"

#include <bnetworkconnection.h>

#include <QByteArray>
#include <QMap>
#include <QString>
#include <QDataStream>

class UserConnection : public BNetworkConnection
{
    Q_OBJECT
public:
    explicit UserConnection(BGenericSocket *socket, QObject *parent = 0);
private:
    typedef void (UserConnection::*Handler)(BNetworkOperation *);
    //
    static const int CriticalBufferSize;
    static const int AuthorizationTimeout;
    static const int IdLength;
    static const QString SampleBaseName;
    static const QString SamplePrefix;
    static const QString SamplePostfix;
    static const QStringList ExtraFiles;
    static const QString CompilerCommand;
    static const QStringList CompilerArguments;
    static const int CompilerStartTimeout;
    static const int CompilerFinishTimeout;
    //
    static bool checkId(const QString &id);
    static QString expandId(const QString &id);
    static bool readFiles(const QString &dir, QString &sample, TexSampleServer::FilePairList &list);
    static bool readStream(QDataStream &in, QString &sample, QString &title, QString &tags,
                           QString &comment, TexSampleServer::FilePairList &list);
    static bool writeFiles(const QString &dir, const QString &sample, const TexSampleServer::FilePairList &list);
    static bool checkSample(const QString &dir, QString &resultString, QString &logText);
    //
    bool mauthorized;
    QString mlogin;
    QMap<QString, Handler> mreplyHandlers;
    QMap<QString, Handler> mrequestHandlers;
    //
    //reply handlers
    void handleReplyAuthorization(BNetworkOperation *operation);
    void handleReplyGetVersion(BNetworkOperation *operation);
    //resuest handlers
    void handleRequestGetPdf(BNetworkOperation *operation);
    void handleRequestGetSample(BNetworkOperation *operation);
    void handleRequestSendSample(BNetworkOperation *operation);
    void handleRequestDeleteSample(BNetworkOperation *operation);
    //other
    bool standardCheck(QDataStream &out);
    void sendSample(BNetworkOperation *operation, bool pdf);
    void mySendReply( BNetworkOperation *operation, const QByteArray &data = QByteArray() );
private slots:
    void replyReceivedSlot(BNetworkOperation *operation);
    void requestReceivedSlot(BNetworkOperation *operation);
    void checkAuthorization();
};

#endif // USERCONNECTION_H
