#ifndef TEXSAMPLESERVER_H
#define TEXSAMPLESERVER_H

#include <bcore.h>

#include <QString>
#include <QDataStream>
#include <QRegExp>
#include <QPair>
#include <QByteArray>
#include <QList>

namespace TexSampleServer
{

typedef QPair<QString, QByteArray> FilePair;
typedef QList<FilePair> FilePairList;
//
const QString DefaultHostName = "texsample-server.no-ip.org";
const quint16 ServerPort = 9022;
const int DBPort = 3306;
const QString DBType = "QMYSQL";
const QString DBName = "texsample";
const QString DBTable = "samples";
const int MaximumPackageSize = 100 * BCore::Megabyte;
//
enum SampleType
{
    Unverified = 0,
    Approved = 1,
    Disapproved = 2,
    AddedByCurrentUser
};
enum TableTexSampleColumn
{
    IdColumn = 0, //BIGINT(20) (Primary, Not NULL, Auto Increment, UNSIGNED, ZEROFILL)
    TypeColumn = 1, //TINYINT(3) (Not NULL, UNSIGNED) [SampleType: 0-3]
    TitleColumn = 2, //TEXT (Not NULL, Character Set: UTF-8)
    AuthorColumn = 3, //TEXT (Not NULL, Character Set: UTF-8)
    TagsColumn = 4, //TEXT (Not NULL, Character Set: UTF-8)
    CommentColumn = 5, //TEXT (Not NULL, Character Set: UTF-8)
    RatingColumn = 6, //TINYINT(3) (Not NULL, UNSIGNED) [0-100]
    RemarkColumn = 7 //TEXT (Not NULL, Character Set: UTF-8)
};
const QRegExp TagSeparator = QRegExp("\\,\\s*");
//
const QString AuthorizeOperation = "authorize";
const QString GetVersionOperation = "get version";
const QString GetSampleOperation = "get sample";
const QString GetPdfOperation = "get pdf";
const QString SendSampleOperation = "send sample";
const QString DeleteSampleOperation = "delete sample";
//
const QDataStream::Version DataStreamVersion = QDataStream::Qt_4_8;

}

#endif // TEXSAMPLESERVER_H
