#ifndef BACKUPPER_H_
#define BACKUPPER_H_

#include <QMap>
#include <QList>
#include <QDir>
#include <QDateTime>

const char BACKUPDATEFORMAT [] = "ddMMyyyyhhmmss";

class Backupper
{
public:
    static const int m_nCyclicBackups = 10;
    static const char  m_format [14];
    QMap<QString, QMap<QDateTime, QString> > m_cyclicBackups;
    QDir m_dir;

    Backupper();
    void doCyclicBackup();
};


class Incremental
{
public:
    struct Hit {
        Hit(const QString& t="", const QString& st="", unsigned int d=0)
            : task(t)
            , subtask(st)
            , duration(d) {}
        QString task, subtask;
        unsigned int duration;
    };
    QMap<QDateTime, Hit > foundPieces; // to be merged
    QMap<QDateTime, Hit > addedPieces; // to be removed
    QDateTime lastTimeStamp;
    QList<QString> foundFiles;
    QList<QString> addedFiles;
    QDir m_dir;
    Incremental();
    void addPiece(const QString& task, const QString& subTask, const QDateTime& now, unsigned int value);
    void clearAddedPieces();
    void clearMergedPieces();
};

#endif
