#include "src/userconnection.h"
#include "src/databaseinteractor.h"
#include "include/texsampleserver.h"

#include <bnetworkconnection.h>
#include <bnetworkoperation.h>
#include <bcore.h>

#include <QTimer>
#include <QByteArray>
#include <QMap>
#include <QString>
#include <QDataStream>
#include <QMutex>
#include <QMutexLocker>
#include <QFile>
#include <QTextStream>
#include <QIODevice>
#include <QProcess>

const int UserConnection::CriticalBufferSize = 100 * BCore::Megabyte;
const int UserConnection::AuthorizationTimeout = 15 * BCore::Second;
const int UserConnection::IdLength = 20;
const QString UserConnection::SampleBaseName = "___sample___";
const QString UserConnection::SamplePrefix = "\\input texsample.tex\n\n";
const QString UserConnection::SamplePostfix = "\n\n\\end{document}";
const QStringList UserConnection::ExtraFiles = QStringList() << UserConnection::SampleBaseName + ".aux"
    << UserConnection::SampleBaseName + ".idx" << UserConnection::SampleBaseName + ".log"
    << UserConnection::SampleBaseName + ".out" << UserConnection::SampleBaseName + ".pdf"
    << UserConnection::SampleBaseName + ".tex";
const QString UserConnection::CompilerCommand = "pdflatex";
const QStringList UserConnection::CompilerArguments = QStringList() << "-interaction=nonstopmode";
const int UserConnection::CompilerStartTimeout = 10 * BCore::Second;
const int UserConnection::CompilerFinishTimeout = 2 * BCore::Minute;

//

bool UserConnection::checkId(const QString &id)
{
    bool ok = false;
    int iid = id.toInt(&ok);
    return ok && iid > 0 && id.length() <= IdLength;
}

QString UserConnection::expandId(const QString &id)
{
    QString eid = id;
    int len = eid.length();
    if (len < IdLength)
        eid.prepend( QString().fill('0', IdLength - len) );
    return eid;
}

bool UserConnection::readFiles(const QString &dir, QString &sample, TexSampleServer::FilePairList &list)
{
    //Reading sample
    QFile f(dir + "/" + SampleBaseName + ".tex");
    if ( !f.open(QFile::ReadOnly) )
        return false;
    QTextStream fin(&f);
    fin.setCodec("UTF-8");
    sample = fin.readAll().remove(SamplePrefix).remove(SamplePostfix);
    f.close();
    //Reading auxiliary files
    QStringList sl = QDir(dir).entryList(QDir::Files);
    for (int i = 0; i < ExtraFiles.size(); ++i)
        sl.removeAll( ExtraFiles.at(i) );
    for (int i = 0; i < sl.size(); ++i)
    {
        TexSampleServer::FilePair fp;
        fp.first = sl.at(i);
        QFile f(dir + "/" + fp.first);
        if ( !f.open(QFile::ReadOnly) )
            return false;
        fp.second = f.readAll();
        f.close();
        if ( fp.second.isEmpty() )
            continue;
        list << fp;
    }
    return true;
}

bool UserConnection::readStream(QDataStream &in, QString &sample, QString &title, QString &tags,
                                QString &comment, TexSampleServer::FilePairList &list)
{
    if ( !in.device() || !in.device()->isReadable() )
        return false;
    in >> sample;
    in >> title;
    in >> tags;
    in >> comment;
    TexSampleServer::FilePair fp;
    in >> fp.first;
    in >> fp.second;
    while ( !fp.first.isEmpty() && !fp.second.isEmpty() )
    {
        fp.first = fp.first.toLower();
        list << fp;
        fp.first.clear();
        fp.second.clear();
        in >> fp.first;
        in >> fp.second;
    }
    return !sample.isEmpty() && !title.isEmpty();
}

bool UserConnection::writeFiles(const QString &dir, const QString &sample, const TexSampleServer::FilePairList &list)
{
    QFile f(dir + "/" + SampleBaseName + ".tex");
    if ( !f.open(QFile::WriteOnly) )
        return false;
    QTextStream fout(&f);
    fout.setCodec("UTF-8");
    fout << SamplePrefix + sample + SamplePostfix;
    f.close();
    for (int i = 0; i < list.size(); ++i)
    {
        const TexSampleServer::FilePair &fp = list.at(i);
        QFile af(dir + "/" + fp.first);
        if ( !af.open(QFile::WriteOnly) )
            return false;
        af.write(fp.second);
        af.close();
    }
    return true;
}

bool UserConnection::checkSample(const QString &dir, QString &resultString, QString &logText)
{
    QString fn = dir + "/" + SampleBaseName + ".tex";
    if ( !QFile(fn).exists() )
    {
        resultString = tr("Internal error", "reply text");
        return false;
    }
    QProcess p;
    QStringList args;
    args << CompilerArguments;
    args << "\"" + fn + "\"";
    p.setWorkingDirectory(dir);
    p.start(CompilerCommand, args);
    bool b = true;
    if ( !p.waitForStarted(CompilerStartTimeout) || !p.waitForFinished(CompilerFinishTimeout) )
    {
        p.kill();
        b = false;
    }
    if (b)
    {
        QFile lf(dir + "/" + SampleBaseName + ".log");
        if ( lf.open(QFile::ReadOnly) )
        {
            QTextStream out(&lf);
            logText = out.readAll();
            lf.close();
        }
        else
        {
            QTextStream out(&p);
            logText = out.readAll();
        }
    }
    if (!b)
    {
        resultString = tr("Internal error", "reply text");
        return false;
    }
    if ( p.exitCode() )
    {
        resultString = tr("Compilation failed", "reply text");
        return false;
    }
    resultString = tr("Compilation was successful", "reply text");
    return true;
}

//

UserConnection::UserConnection(BGenericSocket *socket, QObject *parent) :
    BNetworkConnection(socket, parent)
{
    if ( !isConnected() )
        return;
    //initializing members
    mauthorized = false;
    //adding reply handlers
    mreplyHandlers.insert(TexSampleServer::AuthorizeOperation, &UserConnection::handleReplyAuthorization);
    mreplyHandlers.insert(TexSampleServer::GetVersionOperation, &UserConnection::handleReplyGetVersion);
    //adding request handlers
    mrequestHandlers.insert(TexSampleServer::GetPdfOperation, &UserConnection::handleRequestGetPdf);
    mrequestHandlers.insert(TexSampleServer::GetSampleOperation, &UserConnection::handleRequestGetSample);
    mrequestHandlers.insert(TexSampleServer::SendSampleOperation, &UserConnection::handleRequestSendSample);
    //other
    setCriticalBufferSize(CriticalBufferSize);
    setCloseOnCriticalBufferSize(true);
    connect( this, SIGNAL( replyReceived(BNetworkOperation *) ),
             this, SLOT( replyReceivedSlot(BNetworkOperation *) ) );
    connect( this, SIGNAL( requestReceived(BNetworkOperation *) ),
             this, SLOT( requestReceivedSlot(BNetworkOperation *) ) );
    QTimer::singleShot( AuthorizationTimeout, this, SLOT( checkAuthorization() ) );
    sendRequest(TexSampleServer::AuthorizeOperation);
}

//reply handlers

void UserConnection::handleReplyAuthorization(BNetworkOperation *operation)
{
    QDataStream in( operation->data() );
    in.setVersion(TexSampleServer::DataStreamVersion);
    QString login;
    QString password;
    in >> login;
    in >> password;
    mauthorized = DatabaseInteractor::checkUser(login, password);
    if (!mauthorized)
    {
        log( tr("Authorization failed. Now closing connection...", "log text") );
        close();
        return;
    }
    mlogin = login;
    log(tr("Authorized as:", "log text") + " " + login);
    sendRequest(TexSampleServer::GetVersionOperation);
}

void UserConnection::handleReplyGetVersion(BNetworkOperation *operation)
{
    QDataStream in( operation->data() );
    in.setVersion(TexSampleServer::DataStreamVersion);
    QString editor;
    QString plugin;
    QString beqt;
    in >> editor;
    in >> plugin;
    in >> beqt;
    log(tr("Editor:", "log text") + " " + editor + ". " + tr("Plugin:", "log text") + " " + plugin + ". BeQt: " + beqt);
}

//request handlers

void UserConnection::handleRequestGetPdf(BNetworkOperation *operation)
{
    sendSample(operation, true);
}

void UserConnection::handleRequestGetSample(BNetworkOperation *operation)
{
    sendSample(operation, false);
}

void UserConnection::handleRequestSendSample(BNetworkOperation *operation)
{
    QDataStream in( operation->data() );
    in.setVersion(TexSampleServer::DataStreamVersion);
    QByteArray data;
    QDataStream out(&data, QIODevice::WriteOnly);
    out.setVersion(TexSampleServer::DataStreamVersion);
    if ( !standardCheck(out) )
        return mySendReply(operation, data);
    //Reading data
    QString sample;
    QString title;
    QString tags;
    QString comment;
    TexSampleServer::FilePairList fpl;
    if ( !readStream(in, sample, title, tags, comment, fpl) )
    {
        out << false;
        out << tr("Invalid data", "reply text");
        return;
    }
    //Creating temporary dir and writing files
    QString tdir = BCore::user("samples") + "/" + uniqueId().toString();
    if ( !QDir(tdir).mkpath(tdir) || !writeFiles(tdir, sample, fpl) )
    {
        out << false;
        out << tr("Internal error", "reply text");
        BCore::removeDir(tdir);
        return;
    }
    //Checking sample
    QString resultString;
    QString logText;
    if ( !checkSample(tdir, resultString, logText) )
    {
        out << false;
        out << resultString;
        out << logText;
        BCore::removeDir(tdir);
        return;
    }
    //Executing query
    QString id = DatabaseInteractor::insertSample(title, tags, comment, mlogin);
    QString dir = BCore::user("samples") + "/" + expandId(id);
    //Checking id and creating directory
    if ( !checkId(id) || !QDir(tdir).rename(tdir, dir) )
    {
        out << false;
        out << tr("Internal error", "reply text");
        DatabaseInteractor::deleteSample(id);
        BCore::removeDir(tdir);
        return;
    }
    //Writing reply
    out << true;
    out << logText;
}

//other

bool UserConnection::standardCheck(QDataStream &out)
{
    if (!mauthorized)
    {
        out << false;
        out << tr("Not authorized", "reply text");
        return false;
    }
    return true;
}

void UserConnection::sendSample(BNetworkOperation *operation, bool pdf)
{
    QDataStream in( operation->data() );
    in.setVersion(TexSampleServer::DataStreamVersion);
    QByteArray data;
    QDataStream out(&data, QIODevice::WriteOnly);
    out.setVersion(TexSampleServer::DataStreamVersion);
    if ( !standardCheck(out) )
        return mySendReply(operation, data);
    QString id;
    in >> id;
    if ( !checkId(id) )
    {
        out << false;
        out << tr("Invalid id", "reply text");
        return mySendReply(operation, data);
    }
    QString dir = BCore::user("samples") + "/" + expandId(id);
    if ( !QDir(dir).exists() )
    {
        out << false;
        out << tr("No such sample", "reply text");
        return mySendReply(operation, data);
    }
    if (pdf)
    {
        QFile f(dir + "/" + SampleBaseName + ".pdf");
        if ( !f.open(QFile::ReadOnly) )
        {
            out << false;
            out << tr("Internal error", "reply text");
            return mySendReply(operation, data);
        }
        out << true;
        out << f.readAll();
        f.close();
    }
    else
    {
        QString sample;
        TexSampleServer::FilePairList fpl;
        if ( !readFiles(dir, sample, fpl) )
        {
            out << false;
            out << tr("Internal error", "reply text");
            return mySendReply(operation, data);
        }
        out << true;
        out << sample;
        for (int i = 0; i < fpl.size(); ++i)
        {
            const TexSampleServer::FilePair &fp = fpl.at(i);
            out << fp.first;
            out << fp.second;
        }
    }
    sendReply(operation, data);
}

void UserConnection::mySendReply(BNetworkOperation *operation, const QByteArray &data)
{
    sendReply(operation, data);
}

//

void UserConnection::replyReceivedSlot(BNetworkOperation *operation)
{
    Handler h = operation ? mreplyHandlers.value( operation->metaData().operation() ) : 0;
    if (h)
        (this->*h)(operation);
    if (operation)
        operation->deleteLater();
}

void UserConnection::requestReceivedSlot(BNetworkOperation *operation)
{
    Handler h = operation ? mrequestHandlers.value( operation->metaData().operation() ) : 0;
    if (h && mauthorized)
        (this->*h)(operation);
    else if (operation)
        operation->deleteLater();
}

void UserConnection::checkAuthorization()
{
    if (!mauthorized)
    {
        log( tr("Authorization timeout. Closing connection...", "log text") );
        close();
    }
}
