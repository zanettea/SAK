#include "sakwidget.h"


QBitmap* SakWidget::m_mask = 0;
int SakWidget::m_maxzvalue = 0;

SakWidget::SakWidget(const Task& task)
{
    m_task = task;

    m_palette.setColor(QPalette::Window, task.bgColor);
    m_palette.setColor(QPalette::WindowText, task.fgColor);

    m_flipTimer = 0;
    m_showingDetails=false;
    m_sideB = false;
    m_ismoving = false;

    setAcceptsHoverEvents(false);
    m_dailyWorked = m_weeklyWorked = m_dailyPercentage = m_weeklyPercentage = 0;

    m_text = 0;
    m_rect = QRectF(0,0,400,400);

    m_border = 5;
    if (m_mask == 0 ) {
        m_mask = new QBitmap(QSize(400,400));
        QPainter p(m_mask);
        m_mask->fill(Qt::white);
        QPen pen;
        pen.setWidth(4);
        pen.setColor(Qt::black);
        p.setPen(pen);
        p.setBrush(Qt::black);
        int border = m_border + 4;
        p.drawRoundRect(m_rect.adjusted(border,border, -border,-border), 25, 25 );
    }

    m_text = new QTextDocument(this);
    m_text->setHtml(m_tooltipText);
    m_text->setTextWidth(m_rect.width() - 20);

    m_cachedPixmap = QPixmap(QSize((int)m_rect.width(), (int)m_rect.height()));
    m_cachedPixmapB = QPixmap(QSize((int)m_rect.width(), (int)m_rect.height()));


    m_ic  = QPixmap(m_task.icon);
    if (!m_ic.isNull()) {
        m_ic = m_ic.scaled(m_mask->width()-14, m_mask->height()-14, Qt::KeepAspectRatio);
        QRect r = m_ic.rect();
        r.moveTopLeft(QPoint((m_mask->width() - m_ic.width())/2, (m_mask->height()-m_ic.height())/2));
        m_ic.setMask(m_mask->copy(r));
    }
    m_redrawCachedPixmap=true;
    m_redrawCachedPixmapB=true;
    redrawPixmaps();
}

SakWidget::~SakWidget()
{
    delete m_text;
}



void SakWidget::setStatistics(double dailyWorked, double weeklyWorked, double monthlyWorked, double dailyPercentage, double weeklyPercentage, double monthlyPercentage)
{
    m_dailyWorked = dailyWorked;
    m_weeklyWorked = weeklyWorked;
    m_monthlyWorked = monthlyWorked;
    m_dailyPercentage = dailyPercentage;
    m_weeklyPercentage = weeklyPercentage;
    m_monthlyPercentage = monthlyPercentage;
    m_tooltipText = QString("<div style=\"color:" + m_task.fgColor.name() + "\"><center><h1>%1</h1></center>%2%3<div>Today: <b>%4</b> h (%5%)<br />Last week: <b>%6</b> h (%7%)<br />Last month: <b>%8</b> h (%9%)</div></div>").arg(m_task.title).arg(m_task.description.isEmpty() ? "" : "<p>").arg(m_task.description).arg(m_dailyWorked).arg(m_dailyPercentage, 0, 'f', 2).arg(m_weeklyWorked).arg(m_weeklyPercentage, 0, 'f', 2).arg(m_monthlyWorked).arg(m_monthlyPercentage, 0, 'f', 2);
    m_text->setHtml(m_tooltipText);
    m_redrawCachedPixmapB=true;
}

#define ITERATIONS 5
void SakWidget::timerEvent(QTimerEvent* e)
{
    if (e->timerId() == m_flipTimer) {
        if (m_showingDetails ==  true) {
            QTransform t;
            t.translate( m_animItr * (m_rect.width() / 2 / ITERATIONS), 0);
            t.rotate( - m_animItr * 90.0 / ITERATIONS, Qt::YAxis);
            double newScale = m_scale + m_animItr * (qMax(1.0, m_scale) - m_scale) / 2 / ITERATIONS;
            t.scale( newScale, newScale );
            setTransform(t);
            setPos( m_position + m_animItr * ( scene()->sceneRect().center() - m_rect.center() - m_position ) / 2/ ITERATIONS );
            if (m_animItr == ITERATIONS+1) {
                m_sideB = true;
            } else if (m_animItr >= 2*ITERATIONS ) {
                killTimer(m_flipTimer);
                m_animItr=0;
            }
            m_animItr += 1;
        } else {
            QTransform t;
            t.translate( m_rect.width() -  m_animItr * (m_rect.width() / 2 / ITERATIONS), 0);
            t.rotate( - 180 - m_animItr * 90.0 / ITERATIONS, Qt::YAxis);
            double newScale = qMax(1.0, m_scale) + m_animItr * (m_scale - qMax(1.0, m_scale)) / 2 / ITERATIONS;
            t.scale(newScale, newScale);
            setTransform(t);
            setPos( scene()->sceneRect().center() - m_rect.center() + m_animItr * ( m_position - scene()->sceneRect().center() + m_rect.center()) / 2/ ITERATIONS );
            if (m_animItr == ITERATIONS+1) {
                m_sideB = false;
            } else if (m_animItr >= 2*ITERATIONS ) {
                killTimer(m_flipTimer);
                m_animItr=0;
                setZValue(0);
                m_maxzvalue--;
            }
            m_animItr += 1;
        }
    }
}


void SakWidget::showDetails(bool show)
{
    if (show == m_showingDetails) return;
    else {
        if (show) {
            setZValue( ++m_maxzvalue );
        }
        m_showingDetails = show;
        m_animItr = 0;
        m_flipTimer = startTimer(25);
    }
}

void SakWidget::setGeometry(const QRect& geom) {
    prepareGeometryChange ();

    setPos(geom.topLeft());
    m_position = geom.topLeft();
    m_scale = geom.width() / m_rect.width();

    QTransform t=transform();
    t.setMatrix(m_scale, t.m12(), t.m13(), t.m21(), m_scale, t.m23(), t.m31(), t.m32(), t.m33());
    setTransform(t);

    m_redrawCachedPixmap = m_redrawCachedPixmapB = true;
    redrawPixmaps();
}

void SakWidget::keyPressEvent ( QKeyEvent * event ) {
    if (event->key() == Qt::Key_Return && (event->modifiers() &  Qt::ControlModifier) ) {
        event->accept();
        emit clicked(m_task.title);
    } else  if (event->key() == Qt::Key_Return && (event->modifiers() &  Qt::ShiftModifier) ) {
        event->accept();
        emit clicked("");
    } else if (event->key() == Qt::Key_Space) {
        event->accept();
        showDetails(!m_showingDetails);
    }
}


void SakWidget::mousePressEvent(QGraphicsSceneMouseEvent* e)
{
    m_lastPoint = e->screenPos();
}

void SakWidget::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* )
{
    emit clicked(m_task.title);
}

void SakWidget::mouseReleaseEvent(QGraphicsSceneMouseEvent* )
{
    if (!m_ismoving)
        showDetails(!m_showingDetails);
    else if (!m_showingDetails)
        m_position = pos();
    m_ismoving = false;
}


void SakWidget::mouseMoveEvent(QGraphicsSceneMouseEvent* e)
{
    if (e->buttons() != Qt::NoButton && (m_lastPoint - e->screenPos()).manhattanLength() > 4) {
        m_ismoving = true;
        if (e->buttons() & Qt::LeftButton ) {
            setPos(pos() + (e->screenPos() - m_lastPoint));
            m_lastPoint = e->screenPos();
        }
    }
}

void SakWidget::drawBasicShape(QPixmap& basicShape)
{
    basicShape.fill(QColor(0,0,0,0));
    QPainter p(&basicShape);
    QPen pen;
    pen.setWidth(4);
    pen.setColor(m_task.bgColor.darker(130));
    p.setPen(pen);
    p.setBrush(m_task.bgColor);
    p.drawRoundRect(m_rect.adjusted(m_border,m_border, -m_border,-m_border), 25, 25 );

    pen.setColor(m_task.fgColor);
    p.setPen(pen);
}

void SakWidget::paint ( QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget *)
{
    QTime t;
    t.start();

    redrawPixmaps();

    painter->save();
    QRectF exposed = option->exposedRect.adjusted(-1,-1,1,1);
    exposed &= QRectF(0,0, m_cachedPixmap.width(), m_cachedPixmap.height());
    if (m_sideB)
        painter->drawPixmap(exposed, m_cachedPixmapB, exposed);
    else
        painter->drawPixmap(exposed, m_cachedPixmap, exposed);
    painter->restore();
}


void SakWidget::redrawPixmaps() {
    if (m_redrawCachedPixmap) {
        drawBasicShape(m_cachedPixmap);
        QPainter p(&m_cachedPixmap);
        p.setRenderHints(QPainter::Antialiasing, true);
        QPen pen;
        pen.setColor(m_task.fgColor);
        p.setPen(pen);
        if (!m_ic.isNull()) {
            p.drawPixmap((int)(m_rect.width() - m_ic.width())/2, (int)(m_rect.height() - m_ic.height())/2, m_ic);
        } else if ( m_text && !m_sideB) {
            QFont f = p.font();
            QFontMetricsF metrix(f);
            f.setPixelSize( (int)m_rect.width() /  m_task.title.count() );
            p.setFont(f);
            p.drawText(m_rect, Qt::AlignCenter, m_task.title);
        }

        // draw completion
        QPointF dateInfoTopLeft(m_rect.width() - 100, m_rect.height() - 100);
        if (m_task.dueDate.isValid()) {
            p.setBrush(Qt::red);
            int days = QDate::currentDate().daysTo(m_task.dueDate);
            p.setPen(Qt::NoPen);
            if (days <= 0) {
                p.setBrush(Qt::red);
            } else {
                QConicalGradient g(dateInfoTopLeft + QPointF(32,32), 90);
                g.setColorAt(0.0, Qt::red);
                g.setColorAt(1.0, Qt::green);
                p.setBrush(g);
                days = qMin(30, days);
                days *= 12;
            }

            p.drawPie(QRectF(dateInfoTopLeft, QSizeF(64, 64)), 90*16, -(360 - days)*16);
            p.drawPixmap(dateInfoTopLeft, QPixmap(":/images/hourglass.png"));
        }
        if (m_task.estimatedHours) {
            p.setPen(Qt::NoPen);
            QLinearGradient g(QPointF(dateInfoTopLeft.x(),0), QPointF(dateInfoTopLeft.x()+64,0));
            g.setColorAt(0.0, Qt::red);
            g.setColorAt(1.0, Qt::green);
            p.setBrush(Qt::gray);
            QRectF infoRect(dateInfoTopLeft + QPointF(0,66), QSizeF(64, 8));
            p.drawRoundedRect(infoRect,3,3);
            p.setBrush(g);
            double percentage = ((double)m_task.totHours / (double)m_task.estimatedHours) * 64.0;
            p.drawRoundedRect(infoRect.adjusted(0,0,percentage-64,0), 3,3);
        }

        m_redrawCachedPixmap = false;
    }
    
    if (m_redrawCachedPixmapB) {
        drawBasicShape(m_cachedPixmapB);
        QPainter p(&m_cachedPixmapB);
        p.setRenderHints(QPainter::Antialiasing, true);
        QPen pen;
        pen.setColor(m_task.fgColor);
        p.setPen(pen);
        if (!m_ic.isNull()) {
            p.drawPixmap(QPoint(25, (int)m_rect.height() - 120), m_ic.scaled(100, 100, Qt::KeepAspectRatio));
        }
        QTransform t;
        t.translate( m_rect.width() -10 , 10);
        t.rotate( -180 , Qt::YAxis);
        p.setTransform(t);
        m_text->drawContents(&p);
        m_redrawCachedPixmapB = false;
    }
}
