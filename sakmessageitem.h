#ifndef SAKMESSAGEITEM_H_
#define SAKMESSAGEITEM_H_

#include <QtGui>

class SakMessageItem : public QObject, public QGraphicsItem
{
public:
    SakMessageItem(const QString& message);
    ~SakMessageItem();
    //void setPixmap(const QPixmap &p);
    //QPixmap pixmap();
    void setGeometry(const QRect&);
    QRectF boundingRect() const { return m_rect; }
    void paint( QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget = 0 );
private:
    QGraphicsPixmapItem* m_p;
    QTextDocument* m_t;
    QPixmap m_cachedPixmap;
    QRectF m_rect;
};

class SakExitItem : public QObject, public QGraphicsPixmapItem
{
    Q_OBJECT;
public:
    SakExitItem(const QPixmap&p) : QGraphicsPixmapItem(p) {}
signals:
    void exit();
protected:
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *) { emit exit(); };
};

#endif
