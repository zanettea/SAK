#ifndef TASK_H_
#define TASK_H_

#include <QtGui>
#include <math.h>

#define DATETIMEFORMAT "yyyy/MM/dd hh:mm:ss"


struct Task
{
    struct SubTask
    {
        SubTask(const QString& t=QString()) : title(t) {}
        QString title, description;
        QColor bgColor, fgColor;
    };

    struct Hit
    {
        Hit(const QDateTime& t=QDateTime::currentDateTime(), unsigned int d=0)
            : timestamp(t)
            , duration(d) {}
        QDateTime timestamp;
        unsigned int duration;
    };

    Task() : bgColor(Qt::white), fgColor(Qt::black), active(true), totHours(0) {}
    QString title, description;
    QPixmap icon;
    QColor bgColor, fgColor;
    QHash< QString, SubTask > subTasks;
    QHash< QString, QList< Hit > > hits; // hits for each subtask
    bool active;

    double totHours, totOverestimation;
    double workedHours(const QDateTime& from, const QDateTime& to) const;
    // also set totHours
    bool checkConsistency();
    // get hours from value
    static double hours(unsigned int v) { return (double)v/60.0; }
    static int min(unsigned int v) { return v; }
    // get a value from a hour
    static unsigned int value(double h) { return (unsigned int)(60.0 * h); }
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
    unsigned int duration;
    bool editable;
    HitElement(Task* t = 0, const QString& subtask = QString(), const QDateTime& tt = QDateTime(), unsigned int d=0, bool edit=true)
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


QDataStream & operator<< ( QDataStream & out, const Task & task );
QDataStream & operator>> ( QDataStream & in, Task & task );


QDataStream & operator<< ( QDataStream & out, const Task::Hit & hit );
QDataStream & operator>> ( QDataStream & in, Task::Hit & hit );


#endif
