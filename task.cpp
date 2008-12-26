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

double Task::workedHours(const QDateTime& from, const QDateTime& to) const
{
    double worked = 0;
    for(int i=hits.count()-1; i>=0; i--) {
        const QPair<QDateTime, quint8>& hit = hits[i];
        if ( hit.first < from) return worked;
        if ( hits[i].first > to ) continue;
        else {
            worked += hours(hit.second); // multiple of quarter of hour
        }
    }
    return worked;
}


bool Task::checkConsistency()
{
    QList<Hit> hitlist;
    for(int i=0; i<hits.count(); i++) {
        hitlist << Hit(this, hits[i].first, hits[i].second);
    }
    QVector<double> o;
    totHours = Hit::overestimations(hitlist, o, totOverestimation);
    return totHours >= 0;
    /*
    totHours = 0;
    QDateTime t = QDateTime::currentDateTime();
    int lastdt=0;
    totOverestimation=0;
    for (int i=hits.count()-1; i>=0; i--) {
        if (hits[i].first > t) {
            //inconsistencies[i] = ( hits[i].first.toTime_t() - t.toTime_t() ) / 60;
            qWarning() << "SAK: inconsistency in task " << title << ": " << hits[i].first << " > " << t;
            totHours = -1;
            return false;
        } else {
            totHours += hours(hits[i].second);
            qint64 expected = (t.toTime_t() - hits[i].first.toTime_t());
            qint64 got = (hours(hits[i].second)+hours(lastdt))*3600/2; //seconds
            qint64 diff = expected - got;
            if (diff < 0)
                totOverestimation -= diff;
            if (diff < -59) {
                //inconsistencies[i] = diff/60;
                qDebug() << "SAK: task " << title << ": overestimated time from hit " << hits[i].first << " and hit " << t << ". Expected " << expected/60 << " minutes, got " << got/60 << " (diff = " << diff/60 << ")";
            }
            t=hits[i].first;
        }
    }
    if (totOverestimation > 60) {
        totOverestimation /= 3600.0;
        qDebug() << "SAK: task " << title << " total overestimation of " << totOverestimation << " hours ";
    }
    return true;
    */
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
double Hit::overestimations(const QList<Hit>& hits, QVector<double>& overestimation, double& totOverestimation)
{
    QList<Hit> sortedList(hits);
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


