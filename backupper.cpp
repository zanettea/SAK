#include <QSettings>
#include <QDebug>

#include "task.h"
#include "backupper.h"


Backupper::Backupper() {
    // search for backup dir
    QString path = QFileInfo(QSettings(QSettings::IniFormat, QSettings::UserScope, "ZanzaSoft", "SAK").fileName()).path() + QDir::separator() + "sakbcks";
    m_dir =QDir(path);
    m_dir.mkpath(path);
    m_dir.cd(path);
    QStringList list =  m_dir.entryList();
    foreach(QString s, list) {
        QString fullSuffix = QFileInfo(s).completeSuffix();
        QString baseName = QFileInfo(s).baseName();
        if (fullSuffix.count() >= 19)
            fullSuffix = fullSuffix.mid(fullSuffix.count()-19);
        if (fullSuffix.count() == 19 && fullSuffix[14]=='.' && fullSuffix.mid(15) == "cbck") {
            QDateTime time = QDateTime::fromString(fullSuffix.left(14), BACKUPDATEFORMAT);
            qDebug() << "SAK: found cyclic backup " << s << " on date " << time;
            m_cyclicBackups[baseName][time]= s;
        }
    }
}

void Backupper::doCyclicBackup() {
    // build the list of files to be backupped
    QStringList backupPaths;
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "ZanzaSoft", "SAK");
    backupPaths << m_dir.filePath(QFileInfo(settings.fileName()).baseName());

    QDir saveDir(QFileInfo(settings.fileName()).dir());
    saveDir.mkdir("SakTasks");
    saveDir.cd("SakTasks");
    QStringList nameFilters;
    nameFilters << "*.xml";
    QStringList files = saveDir.entryList( nameFilters, QDir::Files);
    foreach(QString file, files) {
        backupPaths << m_dir.filePath(file);
    }

    foreach(QString filePath, backupPaths) {
        QDateTime now = QDateTime::currentDateTime();
        QString fileName = filePath + "." + now.toString(BACKUPDATEFORMAT) + ".cbck";
        QString baseName = QFileInfo(fileName).baseName();
        QMap<QDateTime, QString>& map = m_cyclicBackups[baseName];
        QList<QDateTime> list = map.keys();
        if (list.size() >= m_nCyclicBackups) {
            int toBeRemoved = list.size() - map.count() + 1;
            for(int i=0; i< toBeRemoved; i++) {
                const QDateTime& d = list[i];
                qWarning() << "SAK: remove old circular backup file " << map[d];
                QFile(m_dir.filePath(map[d])).remove();
                map.remove(d);
            }
        }
        QString origFile = saveDir.filePath(QFileInfo(filePath).fileName());
        if (!QFile(origFile).copy(fileName) ) {
            qWarning() << "SAK: failed to copy " << origFile << " to cyclic backup file " <<  fileName;
        } else {
            qWarning() << "SAK: backup " << origFile << " to " << fileName;
        }
    }
}

//BEGIN: INCREMENTAL

Incremental::Incremental() {
    // search for backup dir
    QString path = QFileInfo(QSettings(QSettings::IniFormat, QSettings::UserScope, "ZanzaSoft", "SAK").fileName()).path() + QDir::separator() + "sakincr";
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

void Incremental::writePiece(const QString& task, const QString& subTask, const QDateTime& now, int value) {
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

void Incremental::addPiece(const QString& task, const QString& subTask, const QDateTime& now, int value) {
    foundPieces.insertMulti(now, Hit(task, subTask, value));
    lastTimeStamp = now;
}

void Incremental::clearAddedPieces() {
    foreach(const QString& file, addedFiles) {
        QFile::remove(file);
    }
}

void Incremental::clearMergedPieces() {
    foreach(const QString& file, foundFiles) {
        if (!QFile::remove(file))
            qDebug() << "Failed removing file " << file;
    }
    foundPieces.clear();
}

