#ifndef PIECHART_H_
#define PIECHART_H_

class PieChart : public QGraphicsItem
{
public:
    void setLimits(double min, double max) {m_min=min; m_max=max;};
    QRectF boundingRect() const { return QRectF(-0.61,-0.6,1.2,1.2);}
    virtual void paint ( QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget ) {
        painter->save();
        QHash<double, QPair<double,QColor> >::const_iterator itr = m_hits.begin(), end=m_hits.end();
        while(itr != end) {
            double start = itr.key();
            Q_ASSERT(start > m_min && start < m_max);
            double startAngle = (start - m_min) / (m_max - m_min) * 16 * 360;
            double spanAngle = itr.value().first / (m_max - m_min) * 16 * 360;
            QPainterPath p;
            p.moveTo(0,0);
            p.arcTo(QRectF(0,0,1,1), spanAngle + startAngle, spanAngle);
            painter->setBrush(itr.value().second);
            painter->setPen(Qt::NoPen);
            painter->drawPath(p);
            itr++;
        }
        painter->restore();
    }
protected:
    QHash<double, QPair<double,QColor> > m_hits;
    double m_min;
    double m_max;
};

#endif