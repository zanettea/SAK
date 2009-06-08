#include "task.h"
#include <math.h>


QXmlStreamWriter & operator<< ( QXmlStreamWriter & out, const Task & task )
{
    out.writeStartElement("Task");

    out.writeAttribute("title", task.title);
    out.writeAttribute("active", QString("%1").arg(task.active));
    if (task.dueDate.isValid())
        out.writeAttribute("due", task.dueDate.toString("dd/MM/yyyy"));
    out.writeAttribute("estimated", QString("%1").arg(task.estimatedHours));
    out.writeAttribute("bgcolor", task.bgColor.name());
    out.writeAttribute("fgcolor", task.fgColor.name());

    out.writeStartElement("Description");
    out.writeCharacters(task.description);
    out.writeEndElement();

    out.writeStartElement("Icon");
    QByteArray bytes;
    QBuffer buffer(&bytes);
    buffer.open(QIODevice::WriteOnly);
    task.icon.save(&buffer, "PNG"); // writes pixmap into bytes in PNG format
    out.writeCharacters(bytes.toBase64());
    out.writeEndElement();

    out.writeStartElement("Subtasks");
    QHash< QString, QList< Task::Hit > >::const_iterator itr = task.hits.begin(), end = task.hits.end();
    while(itr != end) {
        out.writeStartElement("Subtask");
        out.writeAttribute("title", itr.key());
        QHash< QString, Task::SubTask >::const_iterator sitr = task.subTasks.find(itr.key());
        if (sitr != task.subTasks.end()) {
            const Task::SubTask& st(sitr.value());
            if (st.bgColor.isValid())
                out.writeAttribute("bgcolor", st.bgColor.name());
            if (st.fgColor.isValid())
                out.writeAttribute("fgcolor", st.fgColor.name());
            out.writeAttribute("active", QString("%1").arg(st.active));
            out.writeStartElement("Description");
            out.writeCharacters(st.description);
            out.writeEndElement();
        }
        out.writeStartElement("Hits");
        const QList<Task::Hit>&  h(itr.value());
        for(int i=h.count()-1; i>=0; i--) {
            if (h[i].timestamp.isValid()) {
                out.writeEmptyElement("h");
                out.writeAttribute("t", h[i].timestamp.toString(DATETIMEFORMAT));
                out.writeAttribute("d", QString("%1").arg(h[i].duration));
            }
        }
        out.writeEndElement();
        out.writeEndElement();
        itr++;
    }
    out.writeEndElement();

    out.writeEndElement();
    return out;
}



void parseHits(QXmlStreamReader & in, Task & task, const QString subtask)
{
    QMultiMap<unsigned int, Task::Hit> sortedHits;
    while(!in.atEnd()) {
        QXmlStreamReader::TokenType token = in.readNext();
        if (token == QXmlStreamReader::StartElement) {
            QString elementName = in.name().toString();
            if (0) {
            } else if (elementName.compare("h", Qt::CaseInsensitive) == 0){
                Task::Hit hit;
                QXmlStreamAttributes attrs = in.attributes();
                hit.timestamp = QDateTime::fromString(attrs.value("t").toString(), DATETIMEFORMAT);
                hit.duration = attrs.value("d").toString().toUInt();
                sortedHits.insertMulti(hit.timestamp.toTime_t(), hit);
            }
        } else if (token == QXmlStreamReader::EndElement && in.name().toString().compare("hits", Qt::CaseInsensitive) == 0) {
            break;
        }
    }

    // remove duplicates
    QMultiMap<unsigned int, Task::Hit>::iterator itr =  sortedHits.begin();
    Task::Hit prev;
    while(itr != sortedHits.end()) {
        if (itr.value() == prev)
            itr = sortedHits.erase(itr);
        else {
            prev = itr.value();
            itr++;
        }
    }
    task.hits[subtask] = sortedHits.values();
}



void parseSubtask(QXmlStreamReader & in, Task & task)
{
    QXmlStreamAttributes attrs = in.attributes();
    QString title = attrs.value("title").toString();
    Task::SubTask& st(task.subTasks[title]);
    QStringRef bgColor = attrs.value("bgcolor");
    QStringRef fgColor = attrs.value("fgcolor");
    QStringRef active = attrs.value("active");
    st.title = title;
    if (!bgColor.isEmpty())
        st.bgColor = QColor(bgColor.toString());
    if (!fgColor.isEmpty())
        st.fgColor = QColor(fgColor.toString());
    if (!active.isEmpty())
        st.active = active.toString().toUInt();

    // read subtask
    while(!in.atEnd()) {
        QXmlStreamReader::TokenType token = in.readNext();
        if (token == QXmlStreamReader::StartElement) {
            QString elementName = in.name().toString();
            if (0) {
            } else if (elementName.compare("description", Qt::CaseInsensitive) == 0){
                st.description = in.readElementText();
            } else if (elementName.compare("hits", Qt::CaseInsensitive) == 0) {
                parseHits(in, task, st.title);
            }
        } else if (token == QXmlStreamReader::EndElement && in.name().toString().compare("subtask", Qt::CaseInsensitive) == 0) {
            return;
        }
    }
}

void parseSubtasks(QXmlStreamReader & in, Task & task)
{
    while(!in.atEnd()) {
        QXmlStreamReader::TokenType token = in.readNext();
        if (token == QXmlStreamReader::StartElement ) {
            QString elementName = in.name().toString();
            if (0) {
            } else if (elementName.compare("subtask", Qt::CaseInsensitive) == 0) {
                parseSubtask(in, task);
            }
        } else {
            //qDebug() << " > token type " << token;
        }
    }
}


QXmlStreamReader & operator>> ( QXmlStreamReader & in, Task & task )
{
    QXmlStreamReader::TokenType token = in.readNext();
    if (!in.atEnd() && token != QXmlStreamReader::StartElement) {
        in.raiseError("Expected start of Task element, got " + QString("%1").arg(token));
    }
    if (!in.atEnd() && in.name().toString().compare("Task",Qt::CaseInsensitive)) {
        in.raiseError("Expected token name \"Task\", got " + in.name().toString());
    }
    if (!in.atEnd()) {
        QXmlStreamAttributes attrs = in.attributes();
        task.title = attrs.value("title").toString();
        QStringRef bgColor = attrs.value("bgcolor");
        QStringRef fgColor = attrs.value("fgcolor");
        QStringRef active = attrs.value("active");
        QStringRef due = attrs.value("due");
        QStringRef estimated = attrs.value("estimated");
        if (!bgColor.isEmpty())
            task.bgColor = QColor(bgColor.toString());
        if (!fgColor.isEmpty())
            task.fgColor = QColor(fgColor.toString());
        if (!active.isEmpty())
            task.active = active.toString().toUInt();
        if (!estimated.isEmpty())
            task.estimatedHours = estimated.toString().toUInt();
        if (!due.isEmpty())
            task.dueDate = QDate::fromString(due.toString(), "dd/MM/yyyy");

        QXmlStreamReader::TokenType token;
        while(!in.atEnd()) {
            token = in.readNext();
            if (token == QXmlStreamReader::StartElement) {
                QString elementName = in.name().toString();
                if (0) {
                } else if (elementName.compare("description", Qt::CaseInsensitive) == 0) {
                    task.description = in.readElementText();
                } else if (elementName.compare("icon", Qt::CaseInsensitive) == 0) {
                    task.icon.loadFromData(QByteArray::fromBase64(in.readElementText().toAscii()), "PNG");
                } else if (elementName.compare("subtasks", Qt::CaseInsensitive) == 0) {
                    parseSubtasks(in, task);
                }
            } else {
                //qDebug() << "token type " << token;
            }
        }
    }
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


void Task::updateSubTasks()
{
    const QList<QString>& subtasks( hits.keys() );
    for(int i=0; i<subtasks.count(); i++) {
        if (!subTasks.contains(subtasks[i])) {
            subTasks[subtasks[i]].title = subtasks[i];
        }
    }
}

bool Task::checkConsistency()
{
    QHash<QString, QList<Task::Hit > >::const_iterator hitr = hits.begin(), hend = hits.end();
    totHours = 0;
    totOverestimation = 0;
    // ensure tasks are in list
    updateSubTasks();
    while(hitr != hend) {
        const QList<Task::Hit>& l(hitr.value());
        QList<HitElement> hitlist;
        for(int i=0; i<l.count(); i++) {
            hitlist << HitElement(this, hitr.key(), l[i].timestamp, l[i].duration);
        }
        double subtaskHours, subtaskOverestimation;
        QVector<double> o;
        subtaskHours = HitElement::overestimations(hitlist, o, subtaskOverestimation);
        subTasks[hitr.key()].totHours = subtaskHours;
        totHours += subtaskHours;
        totOverestimation +=subtaskOverestimation;
        hitr++;
    }
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
    double totHours = 0;
    qSort(sortedList);

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


