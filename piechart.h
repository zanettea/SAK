#ifndef PIECHART_H_
#define PIECHART_H_

#include "task.h"

class TaskSummaryPieChart : public QGraphicsItem
{
public:
    QRectF boundingRect() const { return QRectF(-0.61,-0.6,1.2,1.2);}
    void setHits(const QMap<double, Task*>& hits)  { m_hits = hits; }
    virtual void paint ( QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget ) {
        painter->save();
        QMap<double,Task*>::const_iterator itr = m_hits.begin(), end=m_hits.end();
        /*
        while(itr != end) {
            double startAngle = (start - m_min) / (m_max - m_min) * 16 * 360;
            double spanAngle = itr.duration / (m_max - m_min) * 16 * 360;
            QPainterPath p;
            p.moveTo(0,0);
            p.arcTo(QRectF(0,0,1,1), spanAngle + startAngle, spanAngle);
            painter->setBrush(itr.value().second);
            painter->setPen(Qt::NoPen);
            painter->drawPath(p);
            itr++;
        }
        */
        painter->restore();
    }
protected:
    QMap<double, Task*> m_hits;
};

#endif
