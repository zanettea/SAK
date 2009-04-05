#include "task.h"
#include <math.h>

QDataStream & operator<< ( QDataStream & out, const Task & task )
{
    out << task.title;
    out << task.description;
    out << task.icon;
    out << task.bgColor;
    out << task.fgColor;
    out << task.active;
    out << task.hits;
    return out;
}

QDataStream & operator>> ( QDataStream & in, Task & task )
{
    in >> task.title;
    in >> task.description;
    in >> task.icon;
    in >> task.bgColor;
    in >> task.fgColor;
    in >> task.active;
    in >> task.hits;
    return in;
}


QDataStream & operator<< ( QDataStream & out, const Task::Hit & hit )
{
    out << hit.timestamp;
    out << hit.duration;
    return out;
}

QDataStream & operator>> ( QDataStream & in, Task::Hit & hit )
{
    in >> hit.timestamp;
    in >> hit.duration;
    return in;
}


double Task::workedHours(const QDateTime& from, const QDateTime& to) const
{
    double worked = 0;
    QHash<QString, QList<Task::Hit > >::const_iterator hitr = hits.begin(), hend = hits.end();
    while(hitr != hend) {
        const QList<Task::Hit>& l(hitr.value());
        for(int i=l.count()-1; i>=0; i--) {
            const Task::Hit& hit ( l[i] );
            if ( hit.timestamp < from) return worked;
            if ( hit.timestamp > to ) continue;
            else {
                worked += hours(hit.duration); // multiple of quarter of hour
            }
        }
        hitr++;
    }
    return worked;
}


bool Task::checkConsistency()
{
    QList<HitElement> hitlist;
    QHash<QString, QList<Task::Hit > >::const_iterator hitr = hits.begin(), hend = hits.end();
    while(hitr != hend) {
        const QList<Task::Hit>& l(hitr.value());
        for(int i=0; i<l.count(); i++) {
            hitlist << HitElement(this, hitr.key(), l[i].timestamp, l[i].duration);
        }
        hitr++;
    }
    QVector<double> o;
    totHours = HitElement::overestimations(hitlist, o, totOverestimation);
    return totHours >= 0;
}

// QPair<QDateTime, quint8>* Task::findHit(QDateTime t, quint8 d)
// {
//     QList< QPair<QDateTime, quint8> >::const_iterator itr = hits.begin(), end = hits.end();
//     for (int i=0; i<hits.count(); i++) {
//         if (hits[i].first == t && hits[i].second == d)
//             return i;
//     }
//     return -1;
// }


// returns tot time
double HitElement::overestimations(const QList<HitElement>& hits, QVector<double>& overestimation, double& totOverestimation)
{
    QList<HitElement> sortedList(hits);
    qSort(sortedList);

    double totHours = 0;
    QDateTime t = QDateTime::currentDateTime();
    overestimation.resize(sortedList.count());
    totOverestimation=0;
    int count = sortedList.count()-1;
    for (int i=count; i>=0; i--) {
        if (sortedList[i].timestamp > t) {
            overestimation[i] = ( sortedList[i].timestamp.toTime_t() - t.toTime_t() ) / 60;
            //qWarning() << "SAK: inconsistency in task " << sortedList[i].task->title << ": " << sortedList[i].timestamp << " > " << t;
            totHours = -1;
            return totHours;
        } else {
            totHours += Task::hours(sortedList[i].duration);
            qint64 expected = (t.toTime_t() - sortedList[i].timestamp.toTime_t());
            qint64 got = Task::hours(sortedList[i].duration)*3600; //seconds
            qint64 diff = expected - got;
            if (diff < 0) {
                overestimation[i] = 0;
                totOverestimation -= diff;
            } if (diff < -59) {
                if (i < count) {
                    overestimation[i+1] = ceil(-diff/120);
                    overestimation[i] = ceil(-diff/120);
                }
                overestimation[i] = ceil(-diff/120);
                //qDebug() << "SAK: task " << sortedList[i].task->title << ": overestimated time from hit " << sortedList[i].timestamp << " and hit " << t << ". Expected " << expected/60 << " minutes, got " << got/60 << " (diff = " << diff/60 << ")";
            }
            t=sortedList[i].timestamp;
        }
    }
    if (totOverestimation > 60) {
        totOverestimation /= 3600.0;
    }
    return totHours;
}


