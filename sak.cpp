/***************************************************************************
 *   Copyright (C) 2007 by Arrigo Zanette                                  *
 *   zanettea@gmail.com                                                    *
 ***************************************************************************/

#include <QtGui>
#include <QSettings>

#include "sak.h"

const char BACKUPDATEFORMAT [] = "ddMMyyyyhhmmss";

class Backupper
{
public:
    static const int m_nCyclicBackups = 10;
    static const char  m_format [14];
    QMap<QDateTime, QString> m_cyclicBackups;
    QMap<QDateTime, QString> m_regularBackups;
    QDir m_dir;
    Backupper() {
        // search for backup dir
        QString path = QFileInfo(QSettings("ZanzaSoft", "SAK").fileName()).path() + QDir::separator() + "sakbcks";
        m_dir =QDir(path);
        m_dir.mkpath(path);
        m_dir.cd(path);
        QStringList list =  m_dir.entryList();
        foreach(QString s, list) {
            QString fullSuffix = QFileInfo(s).completeSuffix();
            if (fullSuffix.count() == 19 && fullSuffix[14]=='.' && fullSuffix.mid(15) == "cbck") {
                QDateTime time = QDateTime::fromString(fullSuffix.left(14), BACKUPDATEFORMAT);
                //qDebug() << "SAK: found cyclic backup " << s << " on date " << time;
                m_cyclicBackups[time]= s;
            } else if (fullSuffix.count() == 18 && fullSuffix[14]=='.' && fullSuffix.mid(15) == "bck") {
                QDateTime time = QDateTime::fromString(fullSuffix.left(14), BACKUPDATEFORMAT);
                //qDebug() << "SAK: found regular backup " << s << " on date " << time;
                m_regularBackups[time]= s;
            }
        }
    }
    
    void doCyclicBackup() {
        QDateTime now = QDateTime::currentDateTime();
        QString fileName = m_dir.filePath(QFileInfo(QSettings("ZanzaSoft", "SAK").fileName()).baseName() + "." + now.toString(BACKUPDATEFORMAT) + ".cbck");
        QList<QDateTime> list = m_cyclicBackups.keys();
        if (list.size() >= m_nCyclicBackups) {
            int toBeRemoved = list.size() - m_cyclicBackups.count() + 1;
            for(int i=0; i< toBeRemoved; i++) {
                const QDateTime& d = list[i];
                qWarning() << "SAK: remove old circular backup file " << m_cyclicBackups[d];
                QFile(m_dir.filePath(m_cyclicBackups[d])).remove();
                m_cyclicBackups.remove(d);
            }
        }
        if (!QFile(QSettings("ZanzaSoft", "SAK").fileName()).copy(fileName)) {
            qWarning() << "SAK: failed to copy " << QSettings("ZanzaSoft", "SAK").fileName() << " to cyclic backup file " <<  fileName;
        } else {
            qWarning() << "SAK: backup to " << fileName;
        }
    };
    void doBackup() {
        QDateTime now = QDateTime::currentDateTime();
        QString fileName = m_dir.filePath(QFileInfo(QSettings("ZanzaSoft", "SAK").fileName()).baseName() + "." + now.toString(BACKUPDATEFORMAT) + ".bck");
        qDebug() << m_cyclicBackups;
        if (!QFile(QSettings("ZanzaSoft", "SAK").fileName()).copy(fileName)) {
            qWarning() << "SAK: failed to copy " << QSettings("ZanzaSoft", "SAK").fileName() << " to backup file " <<  fileName;
        }  else {
            qWarning() << "SAK: backup to " << fileName;
        }
    }
};

class Incremental
{
public:
    QMap<QDateTime, QPair<QString, quint8> > foundPieces;
    QMap<QDateTime, QPair<QString, quint8> > addedPieces;
    QList<QString> addedFiles;
    QDir m_dir;
    Incremental() {
        // search for backup dir
        QString path = QFileInfo(QSettings("ZanzaSoft", "SAK").fileName()).path() + QDir::separator() + "sakincr";
        m_dir =QDir(path);
        m_dir.mkpath(path);
        m_dir.cd(path);
        QStringList list =  m_dir.entryList();
        foreach(QString s, list) {
            QString fullSuffix = QFileInfo(s).completeSuffix();
            QString task = QFileInfo(s).baseName();
            if (fullSuffix.count() == 19 && fullSuffix[14]=='.' && fullSuffix.mid(15) == "incr") {
                QDateTime time = QDateTime::fromString(fullSuffix.left(14), BACKUPDATEFORMAT);
                QFile (s);
                s.open(QIODevice::ReadOnly);
                QDataStream stream(&s);
                stream.setVersion(QDataStream::Qt_4_3);
                QPair<QString, quint8>& p = foundPieces[time];
                p.first = task;
                stream >> p.second;
                qDebug() << "SAK: found piece from task " << task << " timestamp " << time << " duration " << Task::hours(p.second) <<  " hours";
            }
        }
    }
    void addPiece(const QString& task, const QDateTime& now, quint8 value) {
        QString filename= m_dir.filePath(task + "." + now.toString(BACKUPDATEFORMAT) + ".incr");
        addedFiles << filename;
        QFile f(filename);
        f.open(QIODevice::ReadWrite);
        QDataStream stream(&f);
        stream.setVersion(QDataStream::Qt_4_3);
        stream << value;
        qDebug() << "SAK: write piece of task " << task << " of duration " << Task::hours(value) << " in file " << filename;
        addedPieces[now] = QPair<QString, quint8>(task, value);
    }
    void clearAddedPieces() {
        foreach(const QString& file, addedFiles) {
            QFile::remove(file);
        }
    }
};


//END: increamental


//BEGIN: date delagate
class MyDateItemDelegate : public QItemDelegate
{
public:
    MyDateItemDelegate(QObject *parent = 0);

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                          const QModelIndex &index) const;

    void setEditorData(QWidget *editor, const QModelIndex &index) const;
    void setModelData(QWidget *editor, QAbstractItemModel *model,
                      const QModelIndex &index) const;

    void updateEditorGeometry(QWidget *editor,
                              const QStyleOptionViewItem &option, const QModelIndex &index) const;
};

MyDateItemDelegate::MyDateItemDelegate(QObject *parent)
: QItemDelegate(parent)
{
}

QWidget *MyDateItemDelegate::createEditor(QWidget *parent,
                                       const QStyleOptionViewItem &/* option */,
                                       const QModelIndex &/* index */) const
{
    QDateTimeEdit *editor = new QDateTimeEdit(parent);
    editor->setCalendarPopup(true);
    editor->setDateTime(QDateTime::currentDateTime());
    return editor;
}

void MyDateItemDelegate::setEditorData(QWidget *editor,
                                    const QModelIndex &index) const
{
    const QDateTime& value = index.model()->data(index, Qt::DisplayRole).toDateTime();
    
    QDateTimeEdit *e = static_cast<QDateTimeEdit*>(editor);
    e->setDateTime(value);
}

void MyDateItemDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
                                   const QModelIndex &index) const
{
    QDateTimeEdit *e = static_cast<QDateTimeEdit*>(editor);
    e->interpretText();
    const QDateTime& value = e->dateTime();
    
    model->setData(index, value.toString(), Qt::EditRole);
}

void MyDateItemDelegate::updateEditorGeometry(QWidget *editor,
                                           const QStyleOptionViewItem &option, const QModelIndex &/* index */) const
{
    editor->setGeometry(option.rect);
}

//END: date delegate

QDataStream & operator<< ( QDataStream & out, const Task & task )
{
    out << task.title;
    out << task.description;
    out << task.icon;
    out << task.bgColor;
    out << task.fgColor;
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
    totHours = 0;
    QDateTime t = QDateTime::currentDateTime();
    int lastdt=0;
    totOverestimation=0;
    for (int i=hits.count()-1; i>=0; i--) {
        if (hits[i].first > t) {
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
}

//END Task <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<


// GView

class GView : public QGraphicsView
{
public:
	void drawBackground(QPainter* p, const QRectF & rect) {
		QBrush brush(QColor(0,0,0,40));
		p->setCompositionMode(QPainter::CompositionMode_Source);
		p->fillRect(rect, brush);
	}
};

//

//BEGIN PixmapViewer >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

void PixmapViewer::paintEvent(QPaintEvent* )
{
    QPainter p(this);
    QSize s = size();
    if (m_p.isNull()) {
        int edge = qMin(s.width(), s.height()) - 4;
        int offx = (s.width() - edge) / 2;
        int offy = (s.height() - edge) / 2;
        QRect r(offx, offy, edge, edge);
        p.fillRect(r, Qt::white);
        p.drawLine(edge + offx, offy, offx, offy+edge);
        p.drawLine(offx, offy, offx+edge, offy+edge);
        p.drawRect(r);
    } else {
        double ratiox = (double)s.width()/(double)m_p.width();
        double ratioy = (double)s.height()/(double)m_p.height();
        double ratio = qMin(ratiox, ratioy);
        int w = ratio*m_p.width();
        int h = ratio*m_p.height();
        int offx = (s.width() - w)/2;
        int offy = (s.height() - h)/2;
        QRect r(offx, offy, w, h);
        p.drawPixmap(r, m_p);
    }
}


void PixmapViewer::mousePressEvent(QMouseEvent* e)
{
    if (e->buttons()) {
        QString file = QFileDialog::getOpenFileName(0, "Select an image");
        QPixmap tmp;
        if (!file.isEmpty()) {
            if (tmp.load ( file ) ) {
                setPixmap(tmp);
            } else {
                QMessageBox::warning(0, "Failure loading image", QString("Failed load image file %1").arg(file));
            }
        }
    }
}

//END PixmapViewer <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

//BEGIN SakMessageItem <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

SakMessageItem::SakMessageItem(const QString& message) {
    m_rect = QRectF(QPointF(0, 0), QSizeF(800, 200));

    QPixmap p(":/images/whip.png");
    p = p.scaled(200, 600, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    m_p = new QGraphicsPixmapItem(p, this);
    m_p->setPos(0, 0);

    m_t = new QGraphicsTextItem(this);
    m_t->setHtml(message);
    m_t->setPos(240, 40);
    m_t->setTextWidth(520);

    setZValue(-1);
    //    setFlag(ItemIsMovable, true);
    //    setFlag(ItemIsSelectable, true);
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
}

void SakMessageItem::paint(QPainter * painter, const QStyleOptionGraphicsItem *, QWidget * )
{
    painter->save();
    int m_border=20;
    QPen pen;
    pen.setWidth(4);
    pen.setColor(QColor(Qt::white).darker(150));
    painter->setPen(pen);
    painter->setBrush(Qt::white);

    QPainterPath path;
    int curve = 15;
    qreal w = 200 + qMin(600.0, m_t->boundingRect().width());
    qreal h = qMax(80.0+m_border+curve, qMin(200.0, m_t->boundingRect().height()+40));
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
    painter->drawPath(path);
    painter->restore();
}


//END SakMessageItem >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>


//BEGIN SakWidget <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

QBitmap* SakWidget::m_mask = 0;
int SakWidget::m_maxzvalue = 0;

SakWidget::SakWidget(const Task& task)
{
    m_task = task;

    m_palette.setColor(QPalette::Window, task.bgColor);
    m_palette.setColor(QPalette::WindowText, task.fgColor);

    m_flipTimer = 0;
    m_showingDetails=false;
    m_ismoving = false;

    setAcceptsHoverEvents(true);
    m_dailyWorked = m_weeklyWorked = m_dailyPercentage = m_weeklyPercentage = 0;

    m_text = 0;
    m_image = m_image2 = 0;
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

    QPixmap ic (m_task.icon);
    if (!ic.isNull()) {
        ic = ic.scaled(m_mask->width()-14, m_mask->height()-14, Qt::KeepAspectRatio);
        QRect r = ic.rect();
        r.moveTopLeft(QPoint((m_mask->width() - ic.width())/2, (m_mask->height()-ic.height())/2));
        ic.setMask(m_mask->copy(r));
        m_image = new QGraphicsPixmapItem(ic, this);
        m_image->setPos((m_rect.width() - ic.width())/2, (m_rect.height() - ic.height())/2);

        m_image2 = new QGraphicsPixmapItem(ic.scaled(100, 100, Qt::KeepAspectRatio), this);
        m_image2->setPos(25, m_rect.height() - 120);
        m_image2->setZValue(-1);
        m_image2->hide();
    }
    
    m_text = new QGraphicsTextItem(this);
    //m_text->document()->setPageSize(QSize(m_rect.width() - 20, m_rect.height()-20));
    QTransform t = transform();
    t.scale(-1,1);
    t.translate(-m_rect.width() + 10, 10);
    m_text->setTransform(t);
    m_text->setHtml(m_tooltipText);
    m_text->setTextWidth(m_rect.width() - 20);
    m_text->setDefaultTextColor(m_task.fgColor);
    m_text->hide();

};


void SakWidget::setStatistics(double dailyWorked, double weeklyWorked, double monthlyWorked, double dailyPercentage, double weeklyPercentage, double monthlyPercentage)
{
    m_dailyWorked = dailyWorked;
    m_weeklyWorked = weeklyWorked;
    m_monthlyWorked = monthlyWorked;
    m_dailyPercentage = dailyPercentage;
    m_weeklyPercentage = weeklyPercentage;
    m_monthlyPercentage = monthlyPercentage;
    m_tooltipText = QString("<center><h1>%1</h1></center>%2%3<div>Today: <b>%4</b> h (%5%)<br />Last week: <b>%6</b> h (%7%)<br />Last month: <b>%8</b> h (%9%)</div>").arg(m_task.title).arg(m_task.description.isEmpty() ? "" : "<p>").arg(m_task.description).arg(m_dailyWorked).arg(m_dailyPercentage, 0, 'f', 2).arg(m_weeklyWorked).arg(m_weeklyPercentage, 0, 'f', 2).arg(m_monthlyWorked).arg(m_monthlyPercentage, 0, 'f', 2);
    m_text->setHtml(m_tooltipText);
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
            if (m_animItr == ITERATIONS + 1) {
                m_text->show();
                if (m_image) {
                    m_image->hide();
                    m_image2->show();
                }
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
            if (m_animItr == ITERATIONS + 1) {
                if (m_image) {
                    m_image->show();
                    m_image2->hide();
                }
                m_text->hide();
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
}


void SakWidget::mousePressEvent(QGraphicsSceneMouseEvent* e)
{
    m_lastPoint = e->screenPos();
}


void SakWidget::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* )
{
    emit clicked();
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

void SakWidget::paint ( QPainter * painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    painter->save();
    QPen pen;
    pen.setWidth(4);
    pen.setColor(m_task.bgColor.darker(130));
    painter->setPen(pen);
    painter->setBrush(m_task.bgColor);
    painter->drawRoundRect(m_rect.adjusted(m_border,m_border, -m_border,-m_border), 25, 25 );
    
    pen.setColor(m_task.fgColor);
    painter->setPen(pen);

    if (!m_image && m_text && !m_text->isVisible()) {
        QFont f = painter->font();
        QFontMetricsF metrix(f);
        f.setPixelSize( m_rect.width() /  m_task.title.count() );
        painter->setFont(f);
        painter->drawText(m_rect, Qt::AlignCenter, m_task.title);
    }

    painter->restore();
}

//END SakWidget <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<


//BEGIN Sak >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

Sak::Sak(QObject* parent)
: QObject(parent)
, m_timerId(0)
, m_timeoutPopup(0)
, m_settings(0)
{

    m_backupper = new Backupper;
    m_incremental = new Incremental;

    // load the data model
    QSettings settings("ZanzaSoft", "SAK");
    QByteArray tasksArray = settings.value("tasks").toByteArray();
    QDataStream stream(&tasksArray, QIODevice::ReadWrite);
    stream.setVersion(QDataStream::Qt_4_3);
    stream >> m_tasks;

    qDebug() << QCoreApplication::arguments();
    if (QCoreApplication::arguments().contains("--clear")) {
        QHash<QString, Task>::iterator itr = m_tasks.begin();
        while(itr != m_tasks.end()) {
           itr->hits.clear();
           itr++;
        }
    }

    setupSettingsWidget();

    if (m_tasks.count() <= 0)
        m_settings->show();

    // check consistency
    QHash<QString, Task>::iterator itr = m_tasks.begin();
    while(itr != m_tasks.end()) {
        if ( !(*itr).checkConsistency() ) {
            QMessageBox::warning(0, "Task inconsistency!", QString("The task %1 has incosistents records (maybe recorded with different clocks). The routine to recover a valid database in under development. Please contact the author zanettea at gmail dot com").arg((*itr).title));
        }
        itr++;
    }

    m_settings->installEventFilter(this);
    hitsList->installEventFilter(this);
    tasksTree->installEventFilter(this);
    tasksTree->setUniformRowHeights(false);
    QTreeWidgetItem* headerItem = new QTreeWidgetItem;
    headerItem->setSizeHint(0 , QSize(0,0));
    headerItem->setSizeHint(1 , QSize(0,0));
    tasksTree->setHeaderItem(headerItem);

    // add task action
    m_addTaskAction = new QAction("Add task", this);
    m_addTaskMenu = new QMenu;
    m_addTaskAction->setText("Add task");
    m_addTaskMenu->addAction(m_addTaskAction);
    connect(m_addTaskAction, SIGNAL(triggered()), this, SLOT(addDefaultTask()));
    connect(bgColorButton, SIGNAL(clicked()), this, SLOT(selectColor()));
    connect(fgColorButton, SIGNAL(clicked()), this, SLOT(selectColor()));
    connect(previewButton, SIGNAL(clicked()), this, SLOT(popup()));
    connect(tasksTree, SIGNAL(itemSelectionChanged()), this, SLOT(selectedTask()));
    connect(tasksTree, SIGNAL(itemClicked(QTreeWidgetItem*,int)), this, SLOT(selectedTask()));
    populateTasks();

    connect(cal1, SIGNAL(clicked(QDate)), this, SLOT(selectedStartDate(QDate)));
    connect(cal2, SIGNAL(clicked(QDate)), this, SLOT(selectedEndDate(QDate)));
    connect(hitsList, SIGNAL(itemChanged(QTreeWidgetItem*,int)), this, SLOT(hitsListItemChanged(QTreeWidgetItem*,int)));
    m_addHitAction = new QAction("Add hit", this);
    m_addHitMenu = new QMenu;
    m_addHitAction->setText("Add hit");
    m_addHitMenu->addAction(m_addHitAction);
    connect(m_addHitAction, SIGNAL(triggered()), this, SLOT(addDefaultHit()));

    populateHitsList();
    selectedTask();

    m_view = new GView;
    m_view->setScene(new QGraphicsScene);
    m_view->scene()->setSceneRect(QDesktopWidget().geometry());
    //QPalette pal;
    //pal.setColor(QPalette::Window, QColor(0,0,0,200));
    //pal.setColor(QPalette::Base, QColor(0,0,0,200));
    //pal.setColor(QPalette::Background, QColor(0,0,0,200));
    //m_view->setPalette(pal);
    //m_view->viewport()->setPalette(pal);
    m_view->setFrameStyle(QFrame::NoFrame);
    m_view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_view->installEventFilter(this);
    m_view->setWindowFlags(m_view->windowFlags() | Qt::WindowStaysOnTopHint );
    m_view->setAttribute(Qt::WA_QuitOnClose, false);
    m_view->setWindowIcon( QIcon(":/images/icon.png") );
    m_view->setWindowTitle("SaK - Sistema Anti Kazzeggio");

    int v = durationSpinBox->value();
    v = qMax(1, qMin(1440, v));
    m_currentInterval = Task::value( (double)v/60.0 );
    qDebug() << "SAK: pinging interval " <<  v << Task::hours(m_currentInterval) << " hours ";

    m_previewing = false;

    m_timerId = startTimer( Task::hours(m_currentInterval)*3600.0*1000.0 / 2 );
}


Sak::~Sak()
{
    if (!m_settings) return;
        m_backupper->doCyclicBackup();
    QSettings settings("ZanzaSoft", "SAK");
    QByteArray tasksArray;
    QDataStream stream(&tasksArray, QIODevice::ReadWrite);
    stream.setVersion(QDataStream::Qt_4_0);
    stream << m_tasks;
    settings.setValue("tasks", tasksArray);
    settings.setValue("Ping interval", durationSpinBox->value());
    settings.setValue("Message", bodyEdit->toPlainText());
    settings.sync();
    delete m_settings;
    m_view->scene()->deleteLater();
    delete m_view;
    delete m_addTaskAction;
    delete m_addTaskMenu;
    delete m_addHitAction;
    delete m_addHitMenu;
    delete m_backupper;
    m_incremental->clearAddedPieces();
    delete m_incremental;
}


int Sak::taskCounter =  0;

bool Sak::eventFilter(QObject* obj, QEvent* e)
{
    if (obj == tasksTree) {
        return settingsEventFilter(e);
    } else if (obj == hitsList) {
        return hitsListEventFilter(e);
    }else if (obj == m_settings && e->type() == QEvent::Close) {
        if (trayIcon->isVisible()) {
            QMessageBox::information(m_settings, tr("Systray"),
                                     tr("The program will keep running in the "
                                        "system tray. To terminate the program, "
                                        "choose <b>Quit</b> in the context menu "
                                        "of the system tray entry."));
            m_settings->hide();
            e->ignore();
            return true;
        }
    } else if (obj == m_view && e->type() == QEvent::KeyPress) {
        QKeyEvent* ke = dynamic_cast<QKeyEvent*>(e);
        if (ke->modifiers() & ( Qt::AltModifier + Qt::ControlModifier) ) {
            clearView();

            return true;
        }
    } else if (obj == m_view && e->type() == QEvent::Close) {
        if (trayIcon->isVisible()) {
            return true;
        }
    }
    return false;
}

void Sak::addDefaultTask()
{
    QString tentativeName;
    do {
        tentativeName = QString("task %1").arg(taskCounter++);
    } while(m_tasks.contains(tentativeName));

    Task& t = m_tasks[tentativeName];
    t.title = tentativeName;
    QTreeWidgetItem* item = new QTreeWidgetItem(QStringList(tentativeName));
    item->setData(0,Qt::UserRole, QVariant(QMetaType::VoidStar, &t));
    tasksTree->addTopLevelItem(item);
}

void Sak::addDefaultHit()
{
    QTreeWidgetItem* i = new QTreeWidgetItem;
    i->setText(0, "--");
    i->setText(1, "--");
    i->setText(2, "--");
    i->setFlags(i->flags() | Qt::ItemIsEditable);
    hitsList->addTopLevelItem( i );
    hitsList->setItemWidget(i, 0, new QDateTimeEdit);
}

void Sak::populateTasks()
{
    tasksTree->clear();
    foreach(const Task& t, m_tasks.values()) {
        QTreeWidgetItem* item = new QTreeWidgetItem(QStringList(t.title));
        item->setData(0, Qt::UserRole, QVariant(QMetaType::VoidStar, &t));
        QIcon icon;
        icon.addPixmap(t.icon);
        item->setSizeHint(0, QSize(32, 32));
        item->setIcon(0, icon);
        item->setForeground(0,t.fgColor);
        item->setBackground(0,t.bgColor);
        item->setForeground(1,t.fgColor);
        item->setBackground(1,t.bgColor);
        item->setText(1,QString("%1 hours worked till now (overestimated %2)").arg(t.totHours).arg(t.totOverestimation));
        tasksTree->addTopLevelItem(item);
    }
}

void Sak::selectColor() {
    if (tasksTree->selectedItems().isEmpty()) return;

    if (sender() == fgColorButton) {
        QColor c = QColorDialog::getColor(fgColorButton->palette().color(QPalette::ButtonText));
        if (!c.isValid()) return;
        QPalette p = fgColorButton->palette();
        p.setColor(QPalette::ButtonText, c);
        fgColorButton->setPalette(p);
        bgColorButton->setPalette(p);
    } else if (sender() == bgColorButton) {
        QColor c = QColorDialog::getColor(bgColorButton->palette().color(QPalette::Button));
        if (!c.isValid()) return;
        QPalette p = bgColorButton->palette();
        p.setColor(QPalette::Button, c);
        fgColorButton->setPalette(p);
        bgColorButton->setPalette(p);
    }

}

bool Sak::settingsEventFilter(QEvent* e)
{
    if (e->type() == QEvent::ContextMenu) {
        QContextMenuEvent* me = dynamic_cast<QContextMenuEvent*>(e);
        if (!me) return false;
        m_addTaskMenu->popup(me->globalPos());
        return true;
    } else if (e->type() == QEvent::KeyRelease) {
        QKeyEvent* ke = dynamic_cast<QKeyEvent*>(e);
        if (!ke) return false;
        if ( (ke->key() != Qt::Key_Delete && ke->key() != Qt::Key_Backspace) ) return false;
        m_tasks.remove(currentTask);
        QList<QTreeWidgetItem*> selected = tasksTree->selectedItems();
        if (selected.isEmpty()) return false;
        foreach(QTreeWidgetItem* ii, selected)
            tasksTree->takeTopLevelItem(tasksTree->indexOfTopLevelItem (ii));
        selectedTask();
        return true;
    }
    return false;
}

bool Sak::hitsListEventFilter(QEvent* e)
{
    if (e->type() == QEvent::ContextMenu) {
        QContextMenuEvent* me = dynamic_cast<QContextMenuEvent*>(e);
        if (!me) return false;
        m_addHitMenu->popup(me->globalPos());
        return true;
    } else if (e->type() == QEvent::KeyRelease) {
        QKeyEvent* ke = dynamic_cast<QKeyEvent*>(e);
        if (!ke) return false;
        if ( !hitsList->hasFocus() || (ke->key() != Qt::Key_Delete && ke->key() != Qt::Key_Backspace) ) return false;
        QList<QTreeWidgetItem*> selected = hitsList->selectedItems();
        if (selected.isEmpty()) return false;
        foreach(QTreeWidgetItem* ii, selected)
            hitsList->takeTopLevelItem(hitsList->indexOfTopLevelItem (ii));
        return true;
    }
    return false;
}


void Sak::commitCurrentTask()
{
    QString currentTitle = taskTitleEditor->text();
    // backup data
    if (!currentTitle.isEmpty()) {
        if (currentTitle != currentTask && m_tasks.contains(currentTitle)) {
            QMessageBox::warning(0, "Conflitto nei nomi dei tasks", "Conflitto nei nomi dei tasks");
            taskTitleEditor->setText(currentTitle);
            return;
        } else if (m_tasks.contains(currentTask)) {
            m_tasks[currentTitle] = m_tasks.take(currentTask);
            m_tasks[currentTitle].title = currentTitle;
        } else return;
    } else return;

    Task& t = m_tasks[currentTitle];
    t.bgColor = bgColorButton->palette().color(QPalette::Button);
    t.fgColor = fgColorButton->palette().color(QPalette::ButtonText);
    QList<QTreeWidgetItem *> items =  tasksTree->findItems(currentTask,Qt::MatchExactly,0);
    foreach(QTreeWidgetItem* ii, items) {
        ii->setText(0, currentTitle);
        ii->setIcon(0, taskPixmapViewer->pixmap());
        for (int i=0; i<2; i++) {
            ii->setForeground(i, QColor(t.fgColor));
            ii->setBackground(i, QColor(t.bgColor));
        }
    }
    currentTitle = taskTitleEditor->text();
    if (m_tasks.contains(currentTitle)) {
        Task& bt = m_tasks[taskTitleEditor->text()];
        bt.icon = taskPixmapViewer->pixmap();
        bt.description = taskTextEditor->document()->toPlainText();
    }
}

void Sak::selectedTask()
{
    if (tasksTree->selectedItems().isEmpty()) {
        taskPixmapViewer->setEnabled(false);
        taskTextEditor->setEnabled(false);
        taskTitleEditor->setEnabled(false);
        bgColorButton->setEnabled(false);
        fgColorButton->setEnabled(false);
        return;
    }
    commitCurrentTask();
    taskPixmapViewer->setEnabled(true);
    taskTextEditor->setEnabled(true);
    taskTitleEditor->setEnabled(true);
    bgColorButton->setEnabled(true);
    fgColorButton->setEnabled(true);

    QString tt = tasksTree->selectedItems().first()->text(0);
    if (!m_tasks.contains(tt)) return;
    const Task& t = m_tasks[tt];
    taskPixmapViewer->setPixmap(t.icon);
    taskTextEditor->setPlainText(t.description);
    taskTitleEditor->setText(t.title);
    QPalette p;
    p.setColor(QPalette::Button, t.bgColor);
    p.setColor(QPalette::ButtonText, t.fgColor);
    bgColorButton->setPalette(p);
    fgColorButton->setPalette(p);

    currentTask = t.title;
}

void Sak::timerEvent(QTimerEvent* e)
{
    if (e->timerId() == m_timerId) {
        if (!m_view->isVisible() && !m_settings->isVisible() && m_tasks.count() > 0) {
            popup();
            // close timer
            killTimer(m_timeoutPopup);
            m_timeoutPopup = startTimer(qMax( 5000.0, Task::hours(m_currentInterval)*3600.0*1000.0/10.0)); // 5 secmin
            // restart timer
            killTimer(m_timerId);
            m_timerId = startTimer(Task::hours(m_currentInterval)*3600*1000);
        } else {
            qDebug() << "SAK: wait 5 seconds";
            killTimer(m_timerId);
            m_timerId = startTimer(5000);
        }
    } else if (e->timerId() == m_timeoutPopup) {
        clearView();
        killTimer(m_timeoutPopup);
    }
}

void Sak::clearView()
{
    if (!m_view) return;
    m_view->close();
    QGraphicsScene* s = m_view->scene();
    QList<QGraphicsItem*> items = m_view->items();
    s->deleteLater();
    m_view->setScene(new QGraphicsScene);
    m_view->scene()->setSceneRect(QDesktopWidget().geometry());
    m_previewing = false;
}

void Sak::workingOnTask()
{
    if (!m_previewing) {
        SakWidget* w = dynamic_cast<SakWidget*>(sender());
        if (w && m_tasks.contains(w->objectName())) {
            Task& t = m_tasks[w->objectName()];

            QDateTime now = QDateTime::currentDateTime();
            QHash<QString, Task>::iterator itr = m_tasks.begin();
            while( itr != m_tasks.end() ) {
                Task& ot = *itr;
                if ( ot.hits.count() ) {
                    uint realElapsed = now.toTime_t() - ot.hits.last().first.toTime_t();
                    quint8 lastdt = ot.hits.last().second;
                    qint64 diff = (qint64)realElapsed - (Task::hours(lastdt)+Task::hours(m_currentInterval))*3600/2; // seconds
                    if ( diff < -59) {
                        qWarning() << "SAK: overestimated time elapsed from last record (" << ot.hits.last().first << ") of task " << ot.title << " and new record of task " << t.title << " (" << diff << " seconds)";
                        quint8 newdt = Task::value(qMin(24.0, qMax(0.0,  Task::hours(lastdt) + (double)diff/3600.0)));
                        qWarning() << "     -> adjusted duration of last record of task " << ot.title << " from " << lastdt << " to " << newdt;
                        ot.hits.back().second = newdt;
                    }
                }
                itr++;
            }

            t.hits << QPair<QDateTime, quint8>(now, m_currentInterval);
            m_incremental->addPiece(t.title, now, m_currentInterval);
            t.checkConsistency();
            QList<QTreeWidgetItem*> items = tasksTree->findItems (t.title, Qt::MatchExactly, 0);
            if (!items.isEmpty())
                items.first()->setText(1, QString("%1 hours worked till now (overestimated %2)").arg(t.totHours).arg(t.totOverestimation));
        }
    }
    clearView();
}


// attractor = 't' (top), 'b', 'l', 'r'
void layoutInSquare( QList<SakWidget*> sortedWidgets, QRect rect, char attractor)
{
    int w = rect.width();
    if (rect.width() < 64) return;
    if (sortedWidgets.count() == 0) {
        return;
    } else if (sortedWidgets.count() == 1) {
        sortedWidgets[0]->setGeometry(rect);
    } else if (sortedWidgets.count() == 2) {
        QPoint off1, off2;
        if (attractor == 'C') {
            off1 = QPoint(0,w/4);
            off2 = QPoint(w/2,w/4);
        } else if (attractor == 'T') {
            off1 = QPoint(0,0);
            off2 = QPoint(w/2,0);
        } else if (attractor == 'B') {
            off1 = QPoint(0,w/2);
            off2 = QPoint(w/2,w/2);
        } else if (attractor == 'R') {
            off1 = QPoint(w/2,0);
            off2 = QPoint(w/2,w/2);
        } else if (attractor == 'L') {
            off1 = QPoint(0,0);
            off2 = QPoint(0,w/2);
        }
        sortedWidgets[0]->setGeometry(QRect(rect.topLeft() + off1, QSize(w/2, w/2)));
        sortedWidgets[1]->setGeometry(QRect(rect.topLeft() + off2, QSize(w/2, w/2)));
    } else if (sortedWidgets.count() == 3) {
        QPoint off1, off2, off3;
        if (attractor == 'T' || attractor == 'C') {
            off1 = QPoint(0,0);
            off2 = QPoint(w/2,0);
            off3 = QPoint(w/4,w/2);
        } else if (attractor == 'B') {
            off1 = QPoint(0,w/2);
            off2 = QPoint(w/2,w/2);
            off3 = QPoint(w/4,0);
        } else if (attractor == 'R') {
            off1 = QPoint(w/2,0);
            off2 = QPoint(w/2,w/2);
            off3 = QPoint(0,w/4);
        } else if (attractor == 'L') {
            off1 = QPoint(0,0);
            off2 = QPoint(0,w/2);
            off3 = QPoint(w/2,w/4);
        }
        sortedWidgets[0]->setGeometry(QRect(rect.topLeft() + off1, QSize(w/2, w/2)));
        sortedWidgets[1]->setGeometry(QRect(rect.topLeft() + off2, QSize(w/2, w/2)));
        sortedWidgets[2]->setGeometry(QRect(rect.topLeft() + off3, QSize(w/2, w/2)));
    } else if (sortedWidgets.count() == 4) {
        QPoint off1(0,0);
        QPoint off2(0,w/2);
        QPoint off3(w/2,0);
        QPoint off4(w/2,w/2);
        sortedWidgets[0]->setGeometry(QRect(rect.topLeft() + off1, QSize(w/2, w/2)));
        sortedWidgets[1]->setGeometry(QRect(rect.topLeft() + off2, QSize(w/2, w/2)));
        sortedWidgets[2]->setGeometry(QRect(rect.topLeft() + off3, QSize(w/2, w/2)));
        sortedWidgets[3]->setGeometry(QRect(rect.topLeft() + off4, QSize(w/2, w/2)));
    } else {
        Q_ASSERT(sortedWidgets.count() <= 4);
    }
}

void layoutInRect( QList<SakWidget*> sortedWidgets, QRect rect, char attractor)
{
    if (sortedWidgets.count() == 0) return;
    int h = rect.height();
    int w = rect.width();
    if (sortedWidgets.count() == 1) {
        if (w>h) {
            sortedWidgets[0]->setGeometry(QRect(rect.topLeft() + QPoint(h/2,0), QSize(h,h)));
        } else {
            sortedWidgets[0]->setGeometry(QRect(rect.topLeft() + QPoint(0,w/2), QSize(w,w)));
        }
        return;
    } else if (sortedWidgets.count() == 2) {
        if (w>h) {
            sortedWidgets[0]->setGeometry(QRect(rect.topLeft(), QSize(h,h)));
            sortedWidgets[1]->setGeometry(QRect(rect.topLeft() + QPoint(h,0), QSize(h,h)));
        } else {
            sortedWidgets[0]->setGeometry(QRect(rect.topLeft(), QSize(w,w)));
            sortedWidgets[1]->setGeometry(QRect(rect.topLeft() + QPoint(0,w), QSize(w,w)));
        }
        return;
    }
    if (h < 64 || w < 64) return;
    QList<SakWidget*> leftList, rightList;
    for (int i=4; i<sortedWidgets.count(); i++) {
        if (i%2)
            rightList << sortedWidgets[i];
        else
            leftList << sortedWidgets[i];
    }
    if (w > h) {
        QRect square(rect.topLeft() + QPoint(h/2, 0), QSize(h,h));
        layoutInSquare(sortedWidgets.mid(0,4), square, attractor);

        QRect leftRect(rect.topLeft(), QSize(h/2, h));
        layoutInRect(leftList, leftRect, 'R');
        QRect rightRect(rect.topLeft() + QPoint(0.75*w,0), QSize(h/2, h));
        layoutInRect(rightList, rightRect, 'L');
    } else {
        QRect square(rect.topLeft() + QPoint(0,w/2), QSize(w,w));
        layoutInSquare(sortedWidgets.mid(0,4), square, attractor);

        QRect leftRect(rect.topLeft(), QSize(w, w/2));
        layoutInRect(leftList, leftRect, 'B');
        QRect rightRect(rect.topLeft() + QPoint(0,0.75*h), QSize(w, w/2));
        layoutInRect(rightList, rightRect, 'T');
    }
}

QRect Layouting( const QList<SakWidget*>& sortedWidgets)
{
    QRect r = QDesktopWidget().geometry();
    int height = 0.75 * r.height();
    int width = r.width();

    int firstW = width / 2 < height ? width : height * 2;
    int firstH = firstW / 2;
    QRect firstRect (r.x() + (width - firstW) / 2, r.y() + (height - firstH) / 2 + r.height() * 0.25, firstW, firstH);

    layoutInRect(sortedWidgets, firstRect, 'C');
    return QRect(QPoint(r.x(), r.y()), QSize(r.width(), 0.25 * r.height()));
}

void Sak::popup()
{
    if (sender() == previewButton) {
        m_previewing = true;
    }
    QDateTime now = QDateTime::currentDateTime();
    QDateTime fromWeek = now;
    fromWeek.setDate(now.date().addDays(-now.date().dayOfWeek() + 1));
    fromWeek.setTime(QTime(0,0,0));
    QDateTime fromMonth = now;
    fromMonth.setDate(now.date().addDays(-now.date().day()));
    fromMonth.setTime(QTime(0,0,0));
    QDateTime fromToday = now;
    fromToday.setTime(QTime(0,0,0));

    double weekHits = 0;
    double dayHits = 0;
    double monthHits = 0;
    QHash<QString, double> dayStats;
    QHash<QString, double> weekStats;
    QHash<QString, double> monthStats;

    foreach(const Task& t,  m_tasks.values()) {
        double m = t.workedHours(fromMonth, now);
        monthStats[t.title] = m;
        monthHits += m;
        double w = t.workedHours(fromWeek, now);
        weekStats[t.title] = w;
        weekHits += w;
        double d = t.workedHours(fromToday, now);
        dayStats[t.title] = d;
        dayHits += d;
    }

    QMultiMap<double, SakWidget*> m_widgets;
    foreach(const Task& t,  m_tasks.values()) {
        SakWidget* test = new SakWidget(t);
        test->setVisible(false);
        double d = dayStats[t.title];
        double w = weekStats[t.title];
        double m = monthStats[t.title];
        test->setStatistics(d, w, m, d/dayHits * 100.0, w/weekHits * 100.0, m/monthHits * 100.0);
        test->setObjectName(t.title);
        connect (test, SIGNAL(clicked()), this, SLOT(workingOnTask()));
        // check this criterion!!
        m_widgets.insertMulti( - w, test);
    }

    const QList<SakWidget*>& values = m_widgets.values();
    QRect messageRect = Layouting((const QList<SakWidget*>&)values);
    foreach(SakWidget* w, values) {
        m_view->scene()->addItem(w);
        w->show();
    }


    // add the message item
    SakMessageItem* sakMessage = new SakMessageItem(bodyEdit->toPlainText());
    sakMessage->setGeometry(messageRect);
    m_view->scene()->addItem(sakMessage);
    sakMessage->show();

    m_view->showFullScreen();
    m_view->raise();
    m_view->setFocus();
}

void Sak::selectedStartDate(const QDate& date)
{
    if (sender() != cal1) {
        cal1->setSelectedDate(date);
    }
    if (cal2->selectedDate() < cal1->selectedDate()) {
        cal2->setSelectedDate(date);
    }
    cal2->setMinimumDate(date);
    populateHitsList();
}

void Sak::selectedEndDate(const QDate& date)
{
    if (sender() != cal2) {
        cal2->setSelectedDate(date);
    }
    if (cal1->selectedDate() > date) {
        cal2->setSelectedDate(cal1->selectedDate());
    }
    populateHitsList();
}


void Sak::hitsListItemChanged(QTreeWidgetItem* i, int column)
{
    if (m_editedTasks.isEmpty()) {
        m_editedTasks = m_tasks;
    }
    if (column == 1) {
        hitsList->blockSignals(true);
        if (!i) return;
        if (m_tasks.contains(i->text(column))) {
            i->setIcon(column, m_tasks[i->text(column)].icon);
        } else {
            i->setIcon(column, QIcon());
        }
        hitsList->blockSignals(false);
    }
}

void Sak::populateHitsList()
{
    //
    hitsList->clear();
    QMap<QDateTime, QTreeWidgetItem*> widgets;
    foreach(const Task& t, m_tasks) {
        QList< QPair<QDateTime, quint8> >::const_iterator itr = t.hits.begin(), end = t.hits.end();
        while(itr != t.hits.end()) {
            const QDateTime& d = itr->first;
            if (d.date() >= cal1->selectedDate() && d.date() <= cal2->selectedDate()) {
                QTreeWidgetItem* w = new QTreeWidgetItem;
                w->setText(0, d.toString("dd/MM/yyyy hh:mm:ss"));
                w->setText(1, t.title);
                w->setText(2, QString("%1").arg(Task::hours(itr->second)));
                w->setIcon(1, t.icon);
                w->setFlags(w->flags() | Qt::ItemIsEditable);
                widgets[d] = w;            }
            itr++;
        }
    }
    hitsList->addTopLevelItems(widgets.values());
}


//BEGIN settings ================================


void Sak::setupSettingsWidget()
{
    m_settings = new QWidget();
    m_settings->setMinimumHeight(500);
    m_settings->setMinimumWidth(700);

    QVBoxLayout* theMainLayout =  new QVBoxLayout;
    m_settings->setLayout(theMainLayout);

    tabs = new QTabWidget;
    theMainLayout->addWidget(tabs);
    previewButton = new QPushButton("Preview");
    theMainLayout->addWidget(previewButton);

    tab1 = new QWidget;
    tab2 = new QWidget;
    tab3 = new QWidget;
    tabs->addTab(tab1, "Tasks");
    tabs->addTab(tab2, "General");
    tabs->addTab(tab3, "Advanced");

    createActions();

    QVBoxLayout *mainLayout = new QVBoxLayout;
    QGridLayout *messageLayout = new QGridLayout;
    durationLabel = new QLabel(tr("Interval:"));
    durationLabel1 = new QLabel(tr("(effective only after restart)"));
    
    QSettings settings("ZanzaSoft", "SAK");
    durationSpinBox = new QSpinBox;
    durationSpinBox->setSingleStep(15);
    durationSpinBox->setRange(1, 1440);
    durationSpinBox->setSuffix(" minutes");
    durationSpinBox->setValue(qMin(1440, qMax(1, settings.value("Ping interval", 15).toInt())));
    durationSpinBox->setCorrectionMode(QAbstractSpinBox::CorrectToNearestValue);
    
    bodyLabel = new QLabel(tr("Message:"));
    bodyEdit = new QTextEdit;
    bodyEdit->setPlainText( settings.value("Message", "<h1>Enter a message here!</h1>").toString() );
    
    messageLayout->addWidget(durationLabel, 1, 0);
    messageLayout->addWidget(durationSpinBox, 1, 1);
    messageLayout->addWidget(durationLabel1, 1, 2);
    messageLayout->addWidget(bodyLabel, 3, 0);
    messageLayout->addWidget(bodyEdit, 3, 1, 2, 4);
    messageLayout->setColumnStretch(3, 1);
    messageLayout->setRowStretch(4, 1);
    mainLayout->addLayout(messageLayout);
    tab2->setLayout(mainLayout);

    trayIconMenu = new QMenu(m_settings);
    trayIconMenu->addAction(minimizeAction);
    trayIconMenu->addAction(maximizeAction);
    trayIconMenu->addAction(restoreAction);
    trayIconMenu->addSeparator();
    trayIconMenu->addAction(quitAction);
    trayIcon = new QSystemTrayIcon(m_settings);
    trayIcon->setContextMenu(trayIconMenu);
    trayIcon->setIcon( QIcon(":/images/icon.png") );
    m_settings->setWindowIcon( QIcon(":/images/icon.png") );
    m_settings->setWindowTitle("SaK - Sistema Anti Kazzeggio");
    trayIcon->setToolTip( tr("Sistema Anti Kazzeggio") );
    trayIcon->show();
    
    mainLayout = new QVBoxLayout;
    tab1->setLayout(mainLayout);
    // create tree view
    tasksTree = new QTreeWidget;
    tasksTree->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    tasksTree->setColumnCount(2);
    tasksTree->setColumnWidth(0, 300);
    tasksTree->setIconSize(QSize(32,32));
    
    taskTextEditor = new QTextEdit;
    taskTextEditor->setFixedHeight(130);
    taskTextEditor->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    
    
    taskTitleEditor = new QLineEdit;
    taskTitleEditor->setFixedHeight(20);
    taskTitleEditor->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    
    taskPixmapViewer = new PixmapViewer;
    mainLayout->addWidget(tasksTree, 2);
    QHBoxLayout* detailsLayout = new QHBoxLayout;
    QVBoxLayout* editsLayout = new QVBoxLayout;
    detailsLayout->addWidget(taskPixmapViewer);
    editsLayout->addWidget(taskTitleEditor);
    editsLayout->addWidget(taskTextEditor);
    detailsLayout->addLayout(editsLayout);
    
    QVBoxLayout* colorsLayout = new QVBoxLayout;
    bgColorButton = new QPushButton("bg color");
    bgColorButton->setToolTip("Background color");
    bgColorButton->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::MinimumExpanding));

    fgColorButton = new QPushButton("fg color");
    fgColorButton->setToolTip("Foreground color");
    fgColorButton->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::MinimumExpanding));

    colorsLayout->addWidget(bgColorButton);
    colorsLayout->addWidget(fgColorButton);
    detailsLayout->addLayout(colorsLayout);
    
    mainLayout->addLayout(detailsLayout);


    QVBoxLayout* tab3MainLayout = new QVBoxLayout(tab3);
    //taskSelector = new QComboBox;
    hitsList = new QTreeWidget;
    hitsList->setColumnCount(3);
    hitsList->setColumnWidth(0, 250);
    hitsList->setColumnWidth(1, 150);
    hitsList->setIconSize(QSize(24,24));
    hitsList->setSortingEnabled(true);
    hitsList->setItemDelegateForColumn(0, new MyDateItemDelegate);
    QTreeWidgetItem* header = new QTreeWidgetItem;
    header->setText(0, "Date/Time");
    header->setText(1, "Task");
    header->setText(2, "Duration");
    hitsList->setHeaderItem(header);
    hitsList->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    cal1 = new QCalendarWidget;
    cal1->setMinimumSize(QSize(250,200));
    cal2 = new QCalendarWidget;
    cal2->setMinimumSize(QSize(250,200));
    cal1->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
    cal2->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
    //tab3MainLayout->addWidget(taskSelector);
    tab3MainLayout->addWidget(hitsList);
    QHBoxLayout* calsLayout = new QHBoxLayout;
    calsLayout->addWidget(cal1);
    calsLayout->addWidget(cal2);
    tab3MainLayout->addLayout(calsLayout);

    m_settings->setWindowTitle(tr("SaK"));
    m_settings->resize(400, 300);
    
}


void Sak::setVisible(bool visible)
{
    minimizeAction->setEnabled(visible);
    maximizeAction->setEnabled(!m_settings->isMaximized());
    restoreAction->setEnabled(m_settings->isMaximized() || !visible);
    m_settings->setVisible(visible);
}

void Sak::createActions()
{
    minimizeAction = new QAction(tr("Mi&nimize"), m_settings);
    connect(minimizeAction, SIGNAL(triggered()), m_settings, SLOT(hide()));

    maximizeAction = new QAction(tr("Ma&ximize"), m_settings);
    connect(maximizeAction, SIGNAL(triggered()), m_settings, SLOT(showMaximized()));

    restoreAction = new QAction(tr("&Restore"), m_settings);
    connect(restoreAction, SIGNAL(triggered()), m_settings, SLOT(showNormal()));

    quitAction = new QAction(tr("&Quit"), m_settings);
    connect(quitAction, SIGNAL(triggered()), qApp, SLOT(quit()));

}


//END Settings <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
