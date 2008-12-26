#include "sakmessageitem.h"

SakMessageItem::SakMessageItem(const QString& message) : QGraphicsItem(0) {
    m_rect = QRectF(QPointF(0, 0), QSizeF(800, 200));

    setAcceptsHoverEvents(true);
    QPixmap p(":/images/whip.png");
    p = p.scaled(200, 600, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    m_p=new QGraphicsPixmapItem(p,this);
    m_p->setPos(0,0);

    m_t = new QTextDocument;
    m_t->setHtml(message);
    m_t->setTextWidth(500);

    setZValue(-1);
    //setFlag(ItemIsMovable, true);
    //setFlag(ItemIsSelectable, true);
}

SakMessageItem::~SakMessageItem()
{
     delete m_p;
     delete m_t;
}

void SakMessageItem::setGeometry(const QRect& r) {
    prepareGeometryChange();
    double xratio = r.width() / 800.0;
    double yratio = r.height() / 200.0;
    double ratio = qMin(xratio, yratio);
    QTransform t;
    t.scale(ratio, ratio);
    setTransform(t);
    setPos( 0, 0 );
    m_cachedPixmap  = QPixmap();
}

void SakMessageItem::paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * )
{
    if (m_cachedPixmap.isNull()) {
        m_cachedPixmap = QPixmap(QSize((int)m_rect.width(), (int)m_rect.height()));
        m_cachedPixmap.fill(QColor(255,255,255,0));
        QPainter p(&m_cachedPixmap);

        int m_border=20;
        QPen pen;
        pen.setWidth(4);
        pen.setColor(QColor(Qt::white).darker(150));
        p.setPen(pen);
        p.setBrush(Qt::white);

        QPainterPath path;
        int curve = 15;
        qreal w = 300 + qMin(600.0, m_t->documentLayout()->documentSize().width());
        qreal h = qMax(80.0+m_border+curve, qMin(200.0, m_t->documentLayout()->documentSize().height() + 20));
        path.moveTo(200 + m_border, 75);
        path.lineTo(200, 75);
        path.lineTo(200+m_border, 40);
        path.lineTo(200+m_border, m_border + curve);
        path.cubicTo(QPointF(200+m_border, m_border), QPointF(200+m_border, m_border), QPointF(200+m_border+curve, m_border));
        path.lineTo(w-m_border-curve, m_border);
        path.cubicTo(QPointF(w-m_border, m_border), QPointF(w-m_border, m_border), QPointF(w-m_border, m_border+curve));
        path.lineTo(w-m_border, h-m_border-curve);
        path.cubicTo(QPointF(w-m_border,h-m_border), QPointF(w-m_border,h-m_border), QPointF(w-m_border-curve,h-m_border));
        path.lineTo(200+m_border+curve,h-m_border);
        path.cubicTo(QPointF(200+m_border,h-m_border),QPointF(200+m_border,h-m_border),QPointF(200+m_border,h-m_border-curve));
        path.closeSubpath();
        p.drawPath(path);

        p.translate(240,40);
        m_t->drawContents(&p);
    }

    painter->save();
    QRectF exposed = option->exposedRect.adjusted(-1,-1,1,1);
    exposed &= QRectF(0,0, m_cachedPixmap.width(), m_cachedPixmap.height());
    painter->drawPixmap(exposed, m_cachedPixmap, exposed);
    painter->restore();

}
