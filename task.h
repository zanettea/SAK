#ifndef TASK_H_
#define TASK_H_

#include <QtGui>
#include <math.h>

#define DATETIMEFORMAT "yyyy/MM/dd hh:mm:ss"

struct Task
{
    Task() : bgColor(Qt::white), fgColor(Qt::black), active(true), totHours(0) {}
    QString title;
    QPixmap icon;
    QString description;
    QColor bgColor, fgColor;
    QList< QPair<QDateTime, quint8> > hits;
    bool active;
//     int findHit(QDateTime, quint8);
    double totHours, totOverestimation;
    double workedHours(const QDateTime& from, const QDateTime& to) const;
    // also set totHours
    bool checkConsistency();
    // get hours from value
    static double hours(quint8 v) { return (v < 120.0 ? ((double)v) : (120.0 + (v-120.0)*10.0)) / 60.0; };
    static int min(quint8 v) { return (int)round(60.0 * hours(v)); };
    // get a value from a hour
    static quint8 value(double h) { Q_ASSERT(h <= 24); double v = h*60.0; return (quint8)(v < 120.0 ? v : 120.0 + (v-120.0)/10.0); }
};

struct Hit
{
    Task* task;
    QDateTime timestamp;
    quint8 duration;
    bool editable;
    Hit(Task* t = 0, const QDateTime& tt = QDateTime(), quint8 d=0, bool edit=true) : task(t), timestamp(tt), duration(d), editable(edit) {};
    static double overestimations(const QList<Hit>&, QVector<double>&, double&);
    bool operator<(const Hit& o) const {
        return timestamp < o.timestamp;
    }
};
Q_DECLARE_METATYPE(Hit)


QDataStream & operator<< ( QDataStream & out, const Task & task );
QDataStream & operator>> ( QDataStream & in, Task & task );


#endif
