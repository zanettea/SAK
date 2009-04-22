#include "timeline.h"

#include "task.h"

static const int TLH = 30;
static const int TLH2 = 15;

HitItem::HitItem(const Task* t, const QDateTime& timestamp, unsigned int duration, const QString& subtask, QGraphicsItem* parent)
    : QGraphicsItem(parent)
    , m_task(t)
    , m_d(duration)
    , m_t(timestamp)
    , m_subtask(subtask)
{
    setPos(timestamp.toTime_t() / 60.0, 0.0);
    m_rect = QRectF(-(int)duration / 2.0, -TLH2, duration, TLH );

    float halfd = duration/2.0;
    m_shape.moveTo(-halfd, 0);
    m_shape.lineTo(-halfd+1, -TLH2);
    m_shape.lineTo(halfd-1, -TLH2);
    m_shape.lineTo(halfd, 0);
    m_shape.lineTo(halfd-1, TLH2);
    m_shape.lineTo(-halfd+1, TLH2);
    m_shape.closeSubpath();
    QColor c(m_task->fgColor); c.setAlpha(240);
    m_pen.setColor(c);
    QColor d(m_task->bgColor); d.setAlpha(180);
    m_bgbrush.setStyle(Qt::SolidPattern);
    m_bgbrush.setColor(d);
    setVisible(true);
    setZValue(timestamp.toTime_t());
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setFlag(QGraphicsItem::ItemIsFocusable, true);
    setAcceptsHoverEvents(true);
    setToolTip( timestamp.toString(DATETIMEFORMAT) + "\n" + subtask + " @ " + t->title + "\n" + QString("%1").arg(duration) +  " minutes long");
}


void HitItem::paint ( QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget)
{
    painter->save();
    painter->setPen(m_pen);
    painter->setBrush(m_bgbrush);
    painter->drawPath(m_shape);
    painter->restore();
}

void HitItem::mousePressEvent(QGraphicsSceneMouseEvent * event)
{
    if (cursor().shape() != Qt::SizeHorCursor) {
        setCursor(QCursor(Qt::SizeAllCursor));
    }
}

void HitItem::mouseReleaseEvent(QGraphicsSceneMouseEvent * event)
{
    if (cursor().shape() == Qt::SizeAllCursor) {
        setCursor(m_savedCursor);
    }
}

void HitItem::mouseMoveEvent(QGraphicsSceneMouseEvent * event)
{
    qDebug() << "mouse move ";
}

void HitItem::hoverEnterEvent ( QGraphicsSceneHoverEvent * event )
{
    m_savedCursor = cursor();
}

void HitItem::hoverLeaveEvent ( QGraphicsSceneHoverEvent * event )
{
    setCursor(m_savedCursor);
}

void HitItem::hoverMoveEvent ( QGraphicsSceneHoverEvent * event )
{
    if (event->lastScreenPos().x() != event->screenPos().x()) {
        double dsx = event->scenePos().x() - event->lastScenePos().x();
        double dSx = event->screenPos().x() - event->lastScreenPos().x();
        double scale = sqrt((dSx*dSx) / (dsx*dsx));

        double one = qMin(1.0, 10.0 / scale);
        if (event->pos().x() <= m_rect.left()+one || event->pos().x() >= m_rect.right()-one )
            setCursor(QCursor(Qt::SizeHorCursor));
        else setCursor(m_savedCursor);
    }
}


Timeline::Timeline(QWidget* parent) : QGraphicsView(parent)
{
    setScene(new QGraphicsScene());
    scene()->setBackgroundBrush(Qt::gray);
    setFixedHeight(TLH+10);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    connect(scene(), SIGNAL(selectionChanged()), this, SLOT(selectionChanged()));

    setMouseTracking(true);


    m_cursorLine = 0;
    m_cursorText = 0;
    m_bg=0;
}


Timeline::~Timeline()
{
    scene()->deleteLater();
}


void Timeline::keyPressEvent(QKeyEvent* e)
{
    if (e->key() == Qt::Key_Up && matrix().m11()<20)
        scale(1.0 + 0.1, 1.0);
    else if (e->key() == Qt::Key_Down)
        scale(1.0 - 0.1, 1.0);


    QGraphicsView::keyPressEvent(e);
}


void Timeline::wheelEvent(QWheelEvent* e)
{
    if (matrix().m11()<20 || e->delta() < 0)
        scale(1.0 + 0.1 * e->delta() / 120, 1.0);
    QGraphicsView::wheelEvent(e);
}

void Timeline::mousePressEvent(QMouseEvent* e)
{
    m_moving=false;
    if (e->buttons() & Qt::LeftButton) {
        m_pos = e->globalPos();
    }
    QGraphicsView::mousePressEvent(e);
}

void Timeline::mouseReleaseEvent(QMouseEvent* e)
{
    if (m_moving) {
        int dx = e->globalPos().x() - m_pos.x();
        horizontalScrollBar()->setValue(horizontalScrollBar()->value()-dx);
        m_moving=false;
    }
    QGraphicsView::mouseReleaseEvent(e);
}

void Timeline::mouseMoveEvent(QMouseEvent* e)
{

    if (!m_cursorText) {
        m_cursorLine = new QGraphicsLineItem;
        m_cursorText = new QGraphicsTextItem(m_cursorLine);
        QPen redPen;
        redPen.setColor(Qt::red);
        redPen.setWidth(2);
        m_cursorLine->setPen(redPen);
        m_cursorLine->setLine(0, -TLH2, 0, TLH2);
        m_cursorLine->setFlag(QGraphicsItem::ItemIgnoresTransformations, true);
        m_cursorText->setFlag(QGraphicsItem::ItemIgnoresTransformations, true);

        scene()->addItem(m_cursorLine);

        m_cursorLine->setVisible(true);
        m_cursorLine->setZValue(1e21);

        m_cursorText->setVisible(true);
        m_cursorText->setZValue(1e21);
        m_cursorText->setPos(1,-10);
        m_cursorText->setDefaultTextColor(Qt::red);

        m_cursorText->setAcceptHoverEvents(false);
        m_cursorLine->setAcceptHoverEvents(false);
        m_cursorText->setEnabled(false);
        m_cursorLine->setEnabled(false);
        m_cursorLine->setAcceptedMouseButtons(Qt::NoButton);
        m_cursorText->setAcceptedMouseButtons(Qt::NoButton);
    }

    double pos = mapToScene(e->pos()).x();
    m_cursorLine->setPos(QPointF(pos, 0));
    m_cursorText->setPlainText(QDateTime::fromTime_t(pos*60.0).toString("hh:mm dd/MM/yy"));

    if (e->buttons() & Qt::LeftButton)
        m_moving=true;
    QGraphicsView::mouseMoveEvent(e);
}

void Timeline::setPeriod(const QDateTime& from, const QDateTime& to)
{
    scene()->setSceneRect(from.toTime_t()/60.0, -TLH2, to.toTime_t()/60.0 - from.toTime_t()/60.0, TLH);

    scene()->removeItem(m_bg);
    delete m_bg;
    m_bg = new QGraphicsRectItem(scene()->sceneRect());
    QLinearGradient g(QPointF(0, -TLH2), QPointF(0, TLH2));
    g.setColorAt(0.0, QColor(80,80,80));
    g.setColorAt(0.5, QColor(240,240,240));
    g.setColorAt(1.0, QColor(80,80,80));
    m_bg->setBrush(g);
    m_bg->setPen(Qt::NoPen);
    m_bg->setZValue(-999999);
    m_bg->setPos(0,0);
    m_bg->setVisible(true);
    m_bg->setAcceptHoverEvents(false);
    m_bg->setEnabled(false);
    m_bg->setAcceptedMouseButtons(Qt::NoButton);
    scene()->addItem(m_bg);
}

void Timeline::selectionChanged()
{
    if (scene()->selectedItems().count()) {
        HitItem* tmp = dynamic_cast<HitItem*>(scene()->selectedItems().first());
        emit hitSelected(tmp);
    }
}
