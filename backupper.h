#ifndef BACKUPPER_H_
#define BACKUPPER_H_

const char BACKUPDATEFORMAT [] = "ddMMyyyyhhmmss";

class Backupper
{
public:
    static const int m_nCyclicBackups = 10;
    static const char  m_format [14];
    QMap<QDateTime, QString> m_cyclicBackups;
    QMap<QDateTime, QString> m_regularBackups;
    QDir m_dir;
    Backupper() {
        // search for backup dir
        QString path = QFileInfo(QSettings("ZanzaSoft", "SAK").fileName()).path() + QDir::separator() + "sakbcks";
        m_dir =QDir(path);
        m_dir.mkpath(path);
        m_dir.cd(path);
        QStringList list =  m_dir.entryList();
        foreach(QString s, list) {
            QString fullSuffix = QFileInfo(s).completeSuffix();
            if (fullSuffix.count() == 19 && fullSuffix[14]=='.' && fullSuffix.mid(15) == "cbck") {
                QDateTime time = QDateTime::fromString(fullSuffix.left(14), BACKUPDATEFORMAT);
                //qDebug() << "SAK: found cyclic backup " << s << " on date " << time;
                m_cyclicBackups[time]= s;
            } else if (fullSuffix.count() == 18 && fullSuffix[14]=='.' && fullSuffix.mid(15) == "bck") {
                QDateTime time = QDateTime::fromString(fullSuffix.left(14), BACKUPDATEFORMAT);
                //qDebug() << "SAK: found regular backup " << s << " on date " << time;
                m_regularBackups[time]= s;
            }
        }
    }

    void doCyclicBackup() {
        QDateTime now = QDateTime::currentDateTime();
        QString fileName = m_dir.filePath(QFileInfo(QSettings("ZanzaSoft", "SAK").fileName()).baseName() + "." + now.toString(BACKUPDATEFORMAT) + ".cbck");
        QList<QDateTime> list = m_cyclicBackups.keys();
        if (list.size() >= m_nCyclicBackups) {
            int toBeRemoved = list.size() - m_cyclicBackups.count() + 1;
            for(int i=0; i< toBeRemoved; i++) {
                const QDateTime& d = list[i];
                qWarning() << "SAK: remove old circular backup file " << m_cyclicBackups[d];
                QFile(m_dir.filePath(m_cyclicBackups[d])).remove();
                m_cyclicBackups.remove(d);
            }
        }
        if (!QFile(QSettings("ZanzaSoft", "SAK").fileName()).copy(fileName)) {
            qWarning() << "SAK: failed to copy " << QSettings("ZanzaSoft", "SAK").fileName() << " to cyclic backup file " <<  fileName;
        } else {
            qWarning() << "SAK: backup to " << fileName;
        }
    };
    void doBackup() {
        QDateTime now = QDateTime::currentDateTime();
        QString fileName = m_dir.filePath(QFileInfo(QSettings("ZanzaSoft", "SAK").fileName()).baseName() + "." + now.toString(BACKUPDATEFORMAT) + ".bck");
        qDebug() << m_cyclicBackups;
        if (!QFile(QSettings("ZanzaSoft", "SAK").fileName()).copy(fileName)) {
            qWarning() << "SAK: failed to copy " << QSettings("ZanzaSoft", "SAK").fileName() << " to backup file " <<  fileName;
        }  else {
            qWarning() << "SAK: backup to " << fileName;
        }
    }
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
    Incremental() {
        // search for backup dir
        QString path = QFileInfo(QSettings("ZanzaSoft", "SAK").fileName()).path() + QDir::separator() + "sakincr";
        m_dir =QDir(path);
        m_dir.mkpath(path);
        m_dir.cd(path);
        QStringList list =  m_dir.entryList();
        foreach(QString s, list) {
            QString fullSuffix = QFileInfo(s).completeSuffix();
            QString task, subtask;
            int divisor = s.lastIndexOf(":");
            if (divisor>=0) {
                task =  s.mid(0,divisor);
                subtask = QFileInfo(s.mid(divisor+1)).baseName();
            } else {
                task = QFileInfo(s).baseName();
                subtask = QString();
            }
            if (fullSuffix.count() == 19 && fullSuffix[14]=='.' && fullSuffix.mid(15) == "incr") {
                QDateTime time = QDateTime::fromString(fullSuffix.left(14), BACKUPDATEFORMAT);
                QFile f(m_dir.filePath(s));
                if (!f.open(QIODevice::ReadOnly)) {
                    qDebug() << "SAK: error opening file " << f.fileName() << ": " << f.errorString();
                }
                QDataStream stream(&f);
                stream.setVersion(QDataStream::Qt_4_3);
                Hit& p = foundPieces[time];
                p.task = task;
                p.subtask = subtask;
                stream >> p.duration;
                qDebug() << "SAK: found piece from task \"" << task << "\" subtask \"" << subtask << "\" timestamp " << time << " duration " << Task::hours(p.duration) <<  " hours";
                foundFiles << m_dir.filePath(s);
            }
        }
        lastTimeStamp = QDateTime::currentDateTime();
    }
    void addPiece(const QString& task, const QString& subTask, const QDateTime& now, unsigned int value) {
        QString filename= m_dir.filePath(task + ":" + subTask + "." + now.toString(BACKUPDATEFORMAT) + ".incr");
        addedFiles << filename;
        QFile f(filename);
        f.open(QIODevice::ReadWrite);
        QDataStream stream(&f);
        stream.setVersion(QDataStream::Qt_4_3);
        stream << value;
        qDebug() << "SAK: write piece of task " << task << " of duration " << Task::hours(value) << " in file " << filename;
        addedPieces[now] = Hit(task, subTask, value);
        lastTimeStamp = now;
    }
    void clearAddedPieces() {
        foreach(const QString& file, addedFiles) {
            QFile::remove(file);
        }
    }
    void clearMergedPieces() {
        foreach(const QString& file, foundFiles) {
            if (!QFile::remove(file))
                qDebug() << "Failed removing file " << file;
        }
        foundPieces.clear();
    }
};

#endif
