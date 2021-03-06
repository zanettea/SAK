#ifndef TASK_H_
#define TASK_H_

#include <QtGui>
#include <QXmlStreamWriter>
#include <QXmlStreamReader>
#include <math.h>

#define DATETIMEFORMAT "yyyy/MM/dd hh:mm:ss"


struct Task
{
    struct SubTask
    {
        SubTask(const QString& t=QString()) : title(t), active(true) {}
        QString title, description;
        QColor bgColor, fgColor;
        double totHours;
       bool active;
    };

    struct Hit
    {
        Hit(const QDateTime& t=QDateTime::currentDateTime(), unsigned int d=0)
            : timestamp(t)
            , duration(d) {}
        QDateTime timestamp;
        int duration;
        bool operator<(const Hit& o) const {
            return timestamp < o.timestamp;
        }
    };

    Task() : estimatedHours(0), bgColor(Qt::white), fgColor(Qt::black), active(true), totHours(0) {}
    QString title, description;
    QDate dueDate;
    int estimatedHours;
    QPixmap icon;
    QColor bgColor, fgColor;
    QHash< QString, SubTask > subTasks;
    QHash< QString, QList< Hit > > hits; // hits for each subtask
    bool active;

    void updateSubTasks();
    double totHours, totOverestimation;
    double workedHours(const QDateTime& from, const QDateTime& to) const;
    // also set totHours
    bool checkConsistency();
    // get hours from value
    static double hours(int v) { return (double)v/60.0; }
};

static inline bool operator == (const Task::Hit& h1, const Task::Hit& h2) {
    return  h1.timestamp == h2.timestamp && h1.duration == h2.duration;
}

// Used by gui
struct HitElement
{
    Task* task;
    QString subtask;
    QDateTime timestamp;
    int duration;
    bool editable;
    HitElement(Task* t = 0, const QString& subtask = QString(), const QDateTime& tt = QDateTime(), int d=0, bool edit=true)
        : task(t)
        , subtask(subtask)
        , timestamp(tt)
        , duration(d)
        , editable(edit) {}
    static double overestimations(const QList<HitElement>&, QVector<double>&, double&);
    bool operator<(const HitElement& o) const {
        return timestamp < o.timestamp;
    }
};
Q_DECLARE_METATYPE(HitElement)


inline QDebug & operator << (QDebug & out, Task::Hit& taskHit)
{
    out << taskHit.timestamp << ":" << taskHit.duration;
    return out;
}

QDataStream & operator<< ( QDataStream & out, const Task & task );
QDataStream & operator>> ( QDataStream & in, Task & task );


QDataStream & operator<< ( QDataStream & out, const Task::Hit & hit );
QDataStream & operator>> ( QDataStream & in, Task::Hit & hit );


QXmlStreamWriter & operator<< ( QXmlStreamWriter & out, const Task & task );
QXmlStreamReader & operator>> ( QXmlStreamReader & in, Task & task );


#endif
