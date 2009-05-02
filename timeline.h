#ifndef TIMELINE_H
#define TIMELINE_H

#include <QtGui>

class Task;

class HitItem : public QObject,  public QGraphicsItem {
    Q_OBJECT;
public:
    HitItem(const Task* t, const QDateTime& timestamp, unsigned int duration, const QString& subtask, QGraphicsItem* parent = 0);
    QRectF boundingRect() const { return m_rect; }
    void paint ( QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget = 0 );
    const Task* task() const { return m_task; }
    void commitChanges();
    QDateTime timestamp() const { return m_t; }
    QDateTime newTimestamp() const { return m_newT; }
    unsigned int duration() const { return m_d; }
    unsigned int newDuration() const { return m_newD; }
    QString subtask() const { return m_subtask; }
protected:
    void mousePressEvent(QGraphicsSceneMouseEvent * event);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent * event);
    void mouseMoveEvent(QGraphicsSceneMouseEvent * event);
    void hoverEnterEvent ( QGraphicsSceneHoverEvent * event );
    void hoverLeaveEvent ( QGraphicsSceneHoverEvent * event );
    void hoverMoveEvent ( QGraphicsSceneHoverEvent * event );
    void reshape();
signals:
    void changed();
private:
    const Task* m_task;
    QRectF m_rect;
    unsigned int m_d, m_newD;
    QDateTime m_t, m_newT;
    QString m_subtask;
    QPainterPath m_shape;
    QPen m_pen;
    QBrush m_bgbrush;
    QCursor m_savedCursor;
    QPointF m_lastPos;
    bool m_leftExtending, m_rightExtending, m_moving;
};

class Timeline : public QGraphicsView
{
    Q_OBJECT;
public:
    Timeline(QWidget* parent = 0);
    ~Timeline();
    void setPeriod(const QDateTime& from, const QDateTime& to);
signals:
    void hitSelected(HitItem*);
public slots:
    void selectionChanged();
protected:
    void keyPressEvent(QKeyEvent* e);
    void wheelEvent(QWheelEvent* e);
    void mousePressEvent(QMouseEvent* e);
    void mouseReleaseEvent(QMouseEvent* e);
    void mouseMoveEvent(QMouseEvent* e);
private:
    QGraphicsItemGroup* m_cursor;
    QGraphicsTextItem* m_cursorText;
    QGraphicsLineItem* m_cursorLine;
    QGraphicsRectItem* m_bg;
    QPoint m_pos;
    bool m_moving;
};

#endif // TIMELINE_H
