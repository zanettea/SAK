#ifndef SAKWIDGET_H_
#define SAKWIDGET_H_

#include <QtGui>
#include <QFont>
#include <QList>
#include <QHash>
#include <QPair>
#include <QObject>
#include <QDateTime>
#include <QPixmap>
#include <QRectF>
#include <QDebug>

#include "task.h"

class SakWidget : public QObject, public QGraphicsItem
{
    Q_OBJECT;
public:
    SakWidget(const Task& task);
    void setGeometry(const QRect&);
    void setStatistics(double dailyWorked, double weeklyWorked, double monthlyWorked, double dailyPercentage, double weeklyPercentage, double monthlyPercentage);
    QRectF boundingRect() const { return m_rect;  };
public slots:
    void showDetails(bool show = true);
signals:
    void clicked();
protected:
    void mousePressEvent(QGraphicsSceneMouseEvent* e);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* e);
    void mouseMoveEvent(QGraphicsSceneMouseEvent* e);
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* e);
    void timerEvent(QTimerEvent* e);
    
private:
    bool m_ismoving;
    QPointF m_position;
    qreal m_scale;
    QPoint m_lastPoint;
    QPalette m_palette;
    Task m_task;
    QString m_tooltipText;

    void drawBasicShape(QPixmap& p);
    void paint ( QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget = 0 );
    QRectF m_rect, m_boundingRect;
    static QBitmap* m_mask;
    int m_border;
    static int m_maxzvalue;
    QTextDocument* m_text;
    QPixmap m_cachedPixmap;
    QPixmap m_cachedPixmapB;
    QPixmap m_ic;
    bool m_redrawCachedPixmap, m_redrawCachedPixmapB;
    void redrawPixmaps();
    int m_flipTimer;
    // is painting side B??
    bool m_sideB;
    // is animating??
    bool m_showingDetails;
    double m_animItr;
    double m_dailyWorked, m_weeklyWorked, m_monthlyWorked, m_dailyPercentage, m_weeklyPercentage, m_monthlyPercentage;
};



#endif


