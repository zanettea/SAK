/***************************************************************************
 *   Copyright (C) 2007 by Arrigo Zanette                                  *
 *   zanettea@gmail.com                                                    *
 ***************************************************************************/


#include <QtGui>
#include <QSettings>

#include "sak.h"
#include "backupper.h"
#include "piechart.h"


//END Task <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<


// GView
#include <QtOpenGL>

#if defined(Q_WS_X11)
#include <QX11Info>
namespace X11
{
#include <X11/Xlib.h>
#undef KeyPress
#undef KeyRelease
static Window CurrentFocusWindow;
static int CurrentRevertToReturn;
}
#endif



class MyGL : public QGLWidget
{
    public:
        MyGL() {}
    protected:
        virtual void updateGL () {QTime t; t.start(); QGLWidget::updateGL(); qDebug() << "updatedGL in " << t.elapsed(); }
        virtual void updateOverlayGL () {QTime t; t.start(); QGLWidget::updateOverlayGL(); qDebug() << "updatedOverlayGL in " << t.elapsed(); }
        virtual void glDraw () {QTime t; t.start(); QGLWidget::glDraw(); qDebug() << "glDrawGL in " << t.elapsed(); }
        virtual void glInit () {QTime t; t.start(); QGLWidget::glInit(); qDebug() << "glinit in " << t.elapsed(); }
        virtual void initializeGL () {QTime t; t.start(); QGLWidget::initializeGL(); qDebug() << "initializeGL in " << t.elapsed(); }
        virtual void initializeOverlayGL () {QTime t; t.start(); QGLWidget::initializeOverlayGL(); qDebug() << "initializeOverlayGL in " << t.elapsed(); }
        virtual void paintEvent ( QPaintEvent * event ) {QTime t; t.start(); QGLWidget::paintEvent(event); qDebug() << "paintevnt in " << t.elapsed(); }
        virtual void paintGL () {QTime t; t.start(); QGLWidget::paintGL(); qDebug() << "paintGL in " << t.elapsed(); }
        virtual void paintOverlayGL () {QTime t; t.start(); QGLWidget::paintOverlayGL(); qDebug() << "paintOverlayGL in " << t.elapsed(); }
        virtual void resizeEvent ( QResizeEvent * event ) {QTime t; t.start(); QGLWidget::resizeEvent(event); qDebug() << "resizeevent in " << t.elapsed(); }
        virtual void resizeGL ( int width, int height ) {QTime t; t.start(); QGLWidget::resizeGL(width, height); qDebug() << "resizeGL in " << t.elapsed(); }
        virtual void resizeOverlayGL ( int width, int height ) {QTime t; t.start(); QGLWidget::resizeOverlayGL(width, height); qDebug() << "resizeoverlyGL in " << t.elapsed(); }
};

class GView : public QGraphicsView
{
    public:
        GView() { 
            if (QGLFormat::hasOpenGL()) { 
                qDebug() << "Using OpenGL";
                setViewport(new QGLWidget);
            }
        }
        ~GView() {
            delete this->viewport();
        }
        void drawBackground(QPainter* p, const QRectF & rect) {
            QBrush brush(QColor(0,0,0,200));
            p->setCompositionMode(QPainter::CompositionMode_Source);
            p->fillRect(rect, brush);
        }
};



//BEGIN Sak basic >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

Sak::Sak(QObject* parent)
    : QObject(parent)
    , m_timerId(0)
    , m_timeoutPopup(0)
    , m_settings(0)
    , m_changedHit(false)
{
    summaryList = hitsList = 0;
#ifndef Q_OS_LINUX
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, "%APPDATA%");
#endif
    init();

    if (QCoreApplication::arguments().contains("--clear")) {
        QHash<QString, Task>::iterator itr = m_tasks.begin();
        while(itr != m_tasks.end()) {
            itr->hits.clear();
            itr++;
        }
    }

    if (m_tasks.count() <= 0)
        m_settings->show();

    trayIcon->installEventFilter(this);


    m_previewing = false;
    m_changedHit = false;
    m_timerId = 0;
    start();
}

void Sak::init()
{
    m_backupper = new Backupper;
    m_incremental = new Incremental;

    // load the data model
    QSettings settings("ZanzaSoft", "SAK");
    QByteArray tasksArray = settings.value("tasks").toByteArray();
    QDataStream stream(&tasksArray, QIODevice::ReadWrite);
    stream.setVersion(QDataStream::Qt_4_3);
    stream >> m_tasks;

    m_editedTasks = m_tasks;

    //merge piecies
    interactiveMergeHits();

    m_editedTasks = m_tasks;

    // check consistency
    QHash<QString, Task>::iterator itr = m_tasks.begin();
    while(itr != m_tasks.end()) {
        if ( !(*itr).checkConsistency() ) {
            QMessageBox::warning(0, "Task inconsistency!", QString("The task %1 has incosistents records (maybe recorded with different clocks). The routine to recover a valid database in under development. Please contact the author zanettea at gmail dot com").arg((*itr).title));
        }
        itr++;
    }

    setupSettingsWidget();

    m_settings->installEventFilter(this);
    hitsList->installEventFilter(this);
    tasksTree->installEventFilter(this);
    tasksTree->setUniformRowHeights(false);
    QTreeWidgetItem* headerItem = new QTreeWidgetItem;
    headerItem->setSizeHint(0 , QSize(0,0));
    headerItem->setSizeHint(1 , QSize(0,0));
    headerItem->setSizeHint(2 , QSize(0,0));
    tasksTree->setHeaderItem(headerItem);
    
    connect(bgColorButton, SIGNAL(clicked()), this, SLOT(selectColor()));
    connect(fgColorButton, SIGNAL(clicked()), this, SLOT(selectColor()));
    connect(previewButton, SIGNAL(clicked()), this, SLOT(popup()));
    connect(tasksTree, SIGNAL(itemSelectionChanged()), this, SLOT(selectedTask()));
    connect(tasksTree, SIGNAL(itemClicked(QTreeWidgetItem*,int)), this, SLOT(selectedTask()));
    populateTasks();
    
    connect(cal1, SIGNAL(clicked(QDate)), this, SLOT(selectedStartDate(QDate)));
    connect(cal2, SIGNAL(clicked(QDate)), this, SLOT(selectedEndDate(QDate)));
    connect(cal3, SIGNAL(clicked(QDate)), this, SLOT(selectedStartDate(QDate)));
    connect(cal4, SIGNAL(clicked(QDate)), this, SLOT(selectedEndDate(QDate)));
    connect(hitsList, SIGNAL(itemChanged(QTreeWidgetItem*,int)), this, SLOT(hitsListItemChanged(QTreeWidgetItem*,int)));
    selectedTask();
    
    m_view = new GView;
    m_view->setScene(new QGraphicsScene);
    m_view->scene()->setSceneRect(QDesktopWidget().geometry());
    
//    QPalette pal;
//    pal.setColor(QPalette::Window, Qt::yellow);
//    pal.setColor(QPalette::Base,  Qt::yellow);
//    pal.setColor(QPalette::Background,  Qt::yellow);
//    m_view->setPalette(pal);
//    m_view->viewport()->setPalette(pal);

    m_view->installEventFilter(this);
    m_view->setFrameStyle(QFrame::NoFrame);
    m_view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_view->setWindowFlags(m_view->windowFlags() | Qt::WindowStaysOnTopHint | Qt::ToolTip );
    //m_view->setWindowModality(Qt::ApplicationModal);
    m_view->setAttribute(Qt::WA_QuitOnClose, false);
    m_view->setWindowIcon( QIcon(":/images/icon.png") );
    m_view->setWindowTitle("SaK - Sistema Anti Kazzeggio");

    int v = durationSpinBox->value();
    v = qMax(1, qMin(1440, v));
    m_currentInterval = Task::value( (double)v/60.0 );
    qDebug() << "SAK: pinging interval " <<  v << Task::hours(m_currentInterval) << " hours ";

    populateHitsList(createHitsList(QDateTime(cal1->selectedDate()), QDateTime(cal2->selectedDate())));
}

void Sak::start()
{
    if (!m_timerId) {
        int msecs = (int)(Task::hours(m_currentInterval)*3600.0*1000.0 / 2);
        m_timerId = startTimer( msecs );
        m_nextTimerEvent = QDateTime::currentDateTime().addMSecs(msecs);
        startAction->setEnabled(false);
        stopAction->setEnabled(true);
    } else {
        startAction->setEnabled(true);
        stopAction->setEnabled(false);
    }
}

void Sak::stop()
{
    if (m_timerId) {
        killTimer(m_timerId);
        m_timerId = 0;
        stopAction->setEnabled(false);
        startAction->setEnabled(true);
    } else {

        stopAction->setEnabled(true);
        startAction->setEnabled(false);
    }
}

void Sak::flush()
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
    m_incremental->clearAddedPieces();
}

void Sak::saveAsDb()
{
    if (!m_settings) return;
    QString fileName = QFileDialog::getSaveFileName();
    QFile file(fileName);
    file.remove();
    flush();
    QSettings settings("ZanzaSoft", "SAK");
    QFile file1(settings.fileName());
    if (!file1.copy(fileName)) {
        qWarning() << "Error copying " << settings.fileName() << " to " << fileName << file1.errorString();
    }
}

void Sak::exportDbCsv()
{
    if (!m_settings) return;
    QString fileName = QFileDialog::getSaveFileName();
    QFile file(fileName);
    if (!file.open(QIODevice::ReadWrite|QIODevice::Truncate)) {
        QMessageBox::warning(0, "Error saving", QString("Error saving to file %1").arg(fileName));
        return;
    }
    QTextStream stream(&file);
    foreach(const Task& t, m_tasks) {
        QList< QPair<QDateTime, quint8> >::const_iterator itr = t.hits.begin();
        while(itr != t.hits.end()) {
            stream << t.title << ";" << (*itr).first.toString(DATETIMEFORMAT) << ";" << Task::min((*itr).second) << ";\n";
            itr++;
        }
    }
    file.close();
}

void Sak::sendByEmail()
{
    if (!m_settings) return;
    flush();
    QSettings settings("ZanzaSoft", "SAK");
    QProcess::startDetached("kmail -s \"Sak db backup\" --body \"Enjoy the send by email feature of Sak. Bye. \" --attach \"" + settings.fileName() + "\"");
}

void Sak::open()
{
    QString fileName = QFileDialog::getOpenFileName(0, "Open db", "Select a new db");
    QFile file(fileName);
    if (!file.exists()) {
        QMessageBox::warning(0, "Cannot find db", QString("Cannot find db file %1").arg(fileName));
    }
    if (QMessageBox::Ok == QMessageBox::question(0, "Save current db", "Current db will be overwritten by the new db: do you want to backup it to file before?", QMessageBox::Ok | QMessageBox::No, QMessageBox::No) ) {
        saveAsDb();
    }
    destroy();

    file.copy(QSettings().fileName());
    init();
    m_settings->show();
    start();
}

void Sak::destroy()
{
    stop();
    if (!m_settings) return;
    flush();
    m_settings->deleteLater();
    m_view->scene()->deleteLater();
    m_view->deleteLater();
    delete m_backupper;
    delete m_incremental;
    m_previewing = false;
    m_changedHit = false;
    m_timerId = 0;
}


Sak::~Sak()
{
    destroy();
}


int Sak::taskCounter =  0;

bool Sak::eventFilter(QObject* obj, QEvent* e)
{
    if (obj == tasksTree) {
        return settingsEventFilter(e);
    } else if (obj == hitsList || obj == summaryList) {
        return hitsListEventFilter(e);
    }/* else if (obj == summaryList && e->type() == QEvent::Show) {
        m_editedTasks = m_tasks;
        populateHitsList(createHitsList(QDateTime(cal1->selectedDate()), QDateTime(cal2->selectedDate())));
        return false;
    }*/ else if (obj == m_settings && e->type() == QEvent::Close) {
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
    } else if (obj == m_view && e->type() == QEvent::KeyPress ) {
        QKeyEvent* ke = dynamic_cast<QKeyEvent*>(e);
        if ((ke->modifiers() & Qt::AltModifier) && (ke->modifiers() &  Qt::ControlModifier) ) {
            clearView();
            return true;
        } else if (ke->key() == Qt::Key_Left) {
            if (m_widgetsIterator == m_widgets.end()) return false;
            SakWidget* currentShowing = m_widgetsIterator != m_widgets.end() ? m_widgetsIterator.value() : 0;
            if (m_widgetsIterator == m_widgets.begin()) m_widgetsIterator = m_widgets.end();
            m_widgetsIterator--;
            if (m_widgetsIterator != m_widgets.end()) {
                currentShowing->showDetails(false);
                m_widgetsIterator.value()->showDetails(true);
            }
            return true;
        } else if (ke->key() == Qt::Key_Right) {
            if (m_widgetsIterator == m_widgets.end()) return false;
            SakWidget* currentShowing = m_widgetsIterator.value();
            m_widgetsIterator++;
            if (m_widgetsIterator == m_widgets.end()) m_widgetsIterator = m_widgets.begin();
            if (m_widgetsIterator != m_widgets.end()) {
                currentShowing->showDetails(false);
                m_widgetsIterator.value()->showDetails(true);
            }
            return true;
        } else { // forward events to current widget
            if (m_widgetsIterator == m_widgets.end()) return false;
            SakWidget* currentShowing = m_widgetsIterator.value();
            currentShowing->keyPressEvent(ke);
            return true;
        }
    } else if (obj == m_view && e->type() == QEvent::Show) {
#if defined(Q_WS_X11)
        // make sure the application has focus to accept keyboard inputs
        XGetInputFocus((X11::Display*)QX11Info::display(), &X11::CurrentFocusWindow, &X11::CurrentRevertToReturn);
        X11::XSetInputFocus((X11::Display*)QX11Info::display(), QX11Info::appRootWindow(QX11Info::appScreen()),  RevertToParent, CurrentTime);
        X11::XFlush((X11::Display*)QX11Info::display());
#endif
        m_view->grabKeyboard();
    } else if (obj == m_view && e->type() == QEvent::Close) {
        if (trayIcon->isVisible()) {
            return true;
        }
    } else if (obj && obj == trayIcon && e->type() == QEvent::ToolTip) {
        QDateTime last = m_incremental->lastTimeStamp;
        int seconds = QDateTime::currentDateTime().secsTo(m_nextTimerEvent);
        int hours = seconds / 3600;
        int minutes = (seconds / 60) % 60;
        seconds %= 60;
        trayIcon->setToolTip(tr(qPrintable(QString("<h2>Sistema Anti Kazzeggio</h2>Last registered hit at <b>%1</b>.<br />%2").arg(last.toString()).arg(m_timerId > 0 ? QString("Next hit in <b>%2:%3:%4</b>").arg(hours).arg(minutes).arg(seconds) : QString("<b>Paused</b>")))));
        return false;
    }
    return false;
}

//END  basic >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>


//BEGIN Tasks >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
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

void Sak::populateTasks()
{
    tasksTree->clear();
    foreach(const Task& t, m_tasks.values()) {
        QTreeWidgetItem* item = new QTreeWidgetItem(QStringList(t.title));
        item->setData(0, Qt::UserRole, QVariant(QMetaType::VoidStar, &t));
        QIcon icon;
        icon.addPixmap(t.icon);
        item->setSizeHint(0, QSize(32,32));
        item->setIcon(0, icon);
        item->setForeground(0,t.fgColor);
        item->setBackground(0,t.bgColor);
        //item->setCheckState(1, t.active ? Qt::Checked : Qt::Unchecked);
        //item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setIcon(1,QIcon(t.active ? ":/images/active.png" : ":/images/inactive.png"));
        item->setForeground(1,t.fgColor);
        item->setBackground(1,t.bgColor);
        item->setForeground(2,t.fgColor);
        item->setBackground(2,t.bgColor);
        item->setText(2,QString("%1 hours worked till now (overestimated %2)").arg(t.totHours).arg(t.totOverestimation));
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

void Sak::doubleClickedTask(QTreeWidgetItem* i, int column)
{
    if (column == 1) {
        QHash<QString, Task>::iterator itr = m_tasks.find(i->text(0));
        Q_ASSERT(itr != m_tasks.end());
        bool& active ( itr.value().active );
        active = !active;
        i->setIcon(column, active ? QIcon(":/images/active.png") : QIcon(":/images/inactive.png"));
        ((QTreeWidget*)sender())->update();
    }
}



void Sak::timerEvent(QTimerEvent* e)
{
    if (e->timerId() == m_timerId) {
        if (!m_view->isVisible() && !m_settings->isVisible() && m_tasks.count() > 0) {
            popup();
            // close timer
            killTimer(m_timeoutPopup);
            m_timeoutPopup = startTimer((int)(qMax( 5000.0, Task::hours(m_currentInterval)*3600.0*1000.0/10.0))); // 5 secmin
            // restart timer
            killTimer(m_timerId);
            int msecs = (int)(Task::hours(m_currentInterval)*3600.0*1000.0);
            m_timerId = startTimer(msecs);
            m_nextTimerEvent = QDateTime::currentDateTime().addMSecs(msecs);
        } else {
            qDebug() << "SAK: wait 5 seconds";
            killTimer(m_timerId);
            m_timerId = startTimer(5000);
            m_nextTimerEvent = QDateTime::currentDateTime().addMSecs(5000);
        }
    } else if (e->timerId() == m_timeoutPopup) {
        clearView();
        killTimer(m_timeoutPopup);
    }
}

void Sak::clearView()
{
    if (!m_view) return;
    QGraphicsScene* s = m_view->scene();
    QList<QGraphicsItem*> items = m_view->items();
    s->deleteLater();
    m_view->close();
    m_view->setScene(new QGraphicsScene);
    m_view->scene()->setSceneRect(QDesktopWidget().geometry());
    m_previewing = false;
    m_view->releaseKeyboard();
#if defined(Q_WS_X11)
    X11::XSetInputFocus((X11::Display*)QX11Info::display(), X11::CurrentFocusWindow, X11::CurrentRevertToReturn, CurrentTime);
    X11::XFlush((X11::Display*)QX11Info::display());
#endif

}

void Sak::workingOnTask()
{
    if (!m_previewing) {
        SakWidget* w = dynamic_cast<SakWidget*>(sender());
        if (w && m_tasks.contains(w->objectName())) {
            Task& t = m_tasks[w->objectName()];

            int historyIndex = m_taskSelectionHistory.indexOf(t.title);
            if (historyIndex != -1) {
                m_taskSelectionHistory.takeAt(historyIndex);
            }
            m_taskSelectionHistory.push_back(t.title);

            QDateTime now = QDateTime::currentDateTime();
            QHash<QString, Task>::iterator itr = m_tasks.begin();
            while( itr != m_tasks.end() ) {
                Task& ot = *itr;
                if ( ot.hits.count() ) {
                    uint realElapsed = now.toTime_t() - ot.hits.last().first.toTime_t();
                    quint8 lastdt = ot.hits.last().second;
                    qint64 diff = (qint64)realElapsed - (quint64)((Task::hours(lastdt)+Task::hours(m_currentInterval))*3600/2); // seconds
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
    int maxw = qMin(350, w/2);
    QSize size(maxw, maxw);
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
        sortedWidgets[0]->setGeometry(QRect(rect.topLeft() + off1, size));
        sortedWidgets[1]->setGeometry(QRect(rect.topLeft() + off2, size));
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
        sortedWidgets[0]->setGeometry(QRect(rect.topLeft() + off1, size));
        sortedWidgets[1]->setGeometry(QRect(rect.topLeft() + off2, size));
        sortedWidgets[2]->setGeometry(QRect(rect.topLeft() + off3, size));
    } else if (sortedWidgets.count() == 4) {
        QPoint off1(0,0);
        QPoint off2(0,w/2);
        QPoint off3(w/2,0);
        QPoint off4(w/2,w/2);
        sortedWidgets[0]->setGeometry(QRect(rect.topLeft() + off1, size));
        sortedWidgets[1]->setGeometry(QRect(rect.topLeft() + off2, size));
        sortedWidgets[2]->setGeometry(QRect(rect.topLeft() + off3, size));
        sortedWidgets[3]->setGeometry(QRect(rect.topLeft() + off4, size));
    } else {
        Q_ASSERT(sortedWidgets.count() <= 4);
    }
}

void layoutInRect( QList<SakWidget*> sortedWidgets, QRect rect, char attractor)
{
    if (sortedWidgets.count() == 0) return;
    int h = rect.height();
    int w = rect.width();
    int maxh = qMin(350, h);
    int maxw = qMin(350, w);
    if (sortedWidgets.count() == 1) {
        if (w>h) {
            sortedWidgets[0]->setGeometry(QRect(rect.topLeft() + QPoint((w-maxh)/2,(h-maxh)/2), QSize(maxh,maxh)));
        } else {
            sortedWidgets[0]->setGeometry(QRect(rect.topLeft() + QPoint((w-maxw)/2,(h-maxw)/2), QSize(maxw,maxw)));
        }
        return;
    } else if (sortedWidgets.count() == 2) {
        if (w>h) {
            sortedWidgets[0]->setGeometry(QRect(rect.topLeft() + QPoint(w/2-maxh,(h-maxh)/2), QSize(maxh,maxh)));
            sortedWidgets[1]->setGeometry(QRect(rect.topLeft() + QPoint(w/2,(h-maxh)/2), QSize(maxh,maxh)));
        } else {
            sortedWidgets[0]->setGeometry(QRect(rect.topLeft() + QPoint((h-maxw)/2,w-maxw), QSize(w,w)));
            sortedWidgets[1]->setGeometry(QRect(rect.topLeft() + QPoint((h-maxw)/2,w/2), QSize(maxw,maxw)));
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
        QRect rightRect(rect.topLeft() + QPoint((int)(0.75*w),0), QSize(h/2, h));
        layoutInRect(rightList, rightRect, 'L');
    } else {
        QRect square(rect.topLeft() + QPoint(0,w/2), QSize(w,w));
        layoutInSquare(sortedWidgets.mid(0,4), square, attractor);

        QRect leftRect(rect.topLeft(), QSize(w, w/2));
        layoutInRect(leftList, leftRect, 'B');
        QRect rightRect(rect.topLeft() + QPoint(0,(int)(0.75*h)), QSize(w, w/2));
        layoutInRect(rightList, rightRect, 'T');
    }
}

QRect Layouting( const QList<SakWidget*>& sortedWidgets)
{
    QRect r = QDesktopWidget().geometry();
    int height = (int)(0.75 * r.height());
    int width = r.width();

    int firstW = width / 2 < height ? width : height * 2;
    int firstH = firstW / 2;
    QRect firstRect (r.x() + (width - firstW) / 2, r.y() + (height - firstH) / 2 + (int)(r.height() * 0.25), firstW, firstH);

    layoutInRect(sortedWidgets, firstRect, 'C');
    return QRect(QPoint(r.x(), r.y()), QSize(r.width(), (int)(0.25 * r.height())));
}

void Sak::popup()
{
#if 0
#ifdef Q_OS_LINUX
    // fix interaction with yakuake
    QPushButton* focusCatcher = new QPushButton("Polling...", 0);
    focusCatcher->show();
    //focusCatcher->move(-1000,-1000);
    focusCatcher->setWindowFlags(focusCatcher->windowFlags() | Qt::FramelessWindowHint);
    QMetaObject::invokeMethod(focusCatcher, "deleteLater", Qt::QueuedConnection);
#endif
#endif
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
        if (t.active) {
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
    }

    m_widgets.clear();
    foreach(const Task& t,  m_tasks.values()) {
        if (!t.active) continue;
        SakWidget* test = new SakWidget(t);
        test->setVisible(false);
        double d = dayStats[t.title];
        double w = weekStats[t.title];
        double m = monthStats[t.title];
        test->setStatistics(d, w, m, d/dayHits * 100.0, w/weekHits * 100.0, m/monthHits * 100.0);
        test->setObjectName(t.title);
        connect (test, SIGNAL(clicked()), this, SLOT(workingOnTask()));
        int historyPosition = 1 + m_taskSelectionHistory.indexOf(t.title);
        int rank = historyPosition != 0 ? -1000000 * historyPosition : -d;
        m_widgets.insertMulti( rank, test);
    }

    qDebug() << m_taskSelectionHistory;

    m_widgetsIterator = m_widgets.begin();
    if (m_widgetsIterator != m_widgets.end()) {
        m_widgetsIterator.value()->showDetails(true);
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

    // add the exit item
    SakExitItem* exitItem = new SakExitItem(QPixmap(":/images/exit.png"));
    QRect r = QDesktopWidget().geometry();
    connect(exitItem, SIGNAL(exit()), this, SLOT(clearView()));
    exitItem->setPos(r.width() - exitItem->boundingRect().width(), 0);
    m_view->scene()->addItem(exitItem);
    exitItem->setZValue(1e8);
    exitItem->show();

    m_view->setGeometry( QRect(qApp->desktop()->screenGeometry()) );
    m_view->show();
    m_view->raise();
    m_view->setFocus();
    qApp->alert(m_view, 5000);
}

//END   Tasks >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>


//BEGIN settings ================================


void Sak::setupSettingsWidget()
{
    m_settings = new QMainWindow();
    m_settings->setMinimumHeight(500);
    m_settings->setMinimumWidth(700);
    QWidget* centralWidget = new QWidget;
    m_settings->setCentralWidget(centralWidget);
    
    QVBoxLayout* theMainLayout =  new QVBoxLayout;
    centralWidget->setLayout(theMainLayout);

    tabs = new QTabWidget;
    theMainLayout->addWidget(tabs);
    previewButton = new QPushButton("Preview");
    theMainLayout->addWidget(previewButton);

    tab1 = new QWidget;
    tab2 = new QWidget;
    tab3 = new QWidget;
    tab4 = new QWidget;
    tabs->addTab(tab1, "Tasks");
    tabs->addTab(tab2, "General");
    tabs->addTab(tab4, "Statistics");
    tabs->addTab(tab3, "Advanced");

    createActions();
    QMenuBar* mainMenu = new QMenuBar;
    m_settings->setMenuBar(mainMenu);
    QMenu* programMenu =  mainMenu->addMenu("Program");
    programMenu->addAction(quitAction);
    programMenu->addAction(minimizeAction);
    QMenu* dbMenu =  mainMenu->addMenu("Db");
    dbMenu->addAction(flushAction);
    dbMenu->addAction(openAction);
    dbMenu->addAction(saveAsDbAction);
    dbMenu->addAction(exportDbCsvAction);
    dbMenu->addAction(sendByEmailAction);
    QMenu* actionsMenu =  mainMenu->addMenu("Actions");
    actionsMenu->addAction(startAction);
    actionsMenu->addAction(stopAction);

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
    //trayIconMenu->addAction(minimizeAction);
    //trayIconMenu->addAction(maximizeAction);
    //trayIconMenu->addAction(restoreAction);
    //trayIconMenu->addSeparator();
    trayIconMenu->addAction(startAction);
    trayIconMenu->addAction(stopAction);
    trayIconMenu->addAction(flushAction);
    trayIconMenu->addSeparator();
    trayIconMenu->addAction(quitAction);
    trayIcon = new QSystemTrayIcon(m_settings);
    trayIcon->setContextMenu(trayIconMenu);
    trayIcon->setIcon( QIcon(":/images/icon.png") );
    m_settings->setWindowIcon( QIcon(":/images/icon.png") );
    m_settings->setWindowTitle("SaK - Sistema Anti Kazzeggio");
    trayIcon->setToolTip( tr("Sistema Anti Kazzeggio") );
    trayIcon->show();
    connect(trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(trayIconActivated(QSystemTrayIcon::ActivationReason)));
    
    mainLayout = new QVBoxLayout;
    tab1->setLayout(mainLayout);
    // create tree view
    tasksTree = new QTreeWidget;
    tasksTree->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    tasksTree->setColumnCount(3);
    tasksTree->setColumnWidth(0, 300);
    tasksTree->setColumnWidth(1, 65);
    tasksTree->setIconSize(QSize(32,32));
    connect(tasksTree, SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)), this, SLOT(doubleClickedTask(QTreeWidgetItem*,int)));

    
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

    QVBoxLayout* tab4MainLayout = new QVBoxLayout(tab4);
    //taskSelector = new QComboBox;
    summaryList = newTaskSummaryList();
    QTabWidget* tabs = new QTabWidget;
    tabs->setTabPosition(QTabWidget::East);
    tabs->addTab(summaryList, "List");
    QGraphicsView* summaryView = new QGraphicsView;
    summaryView->setScene(new QGraphicsScene);
    TaskSummaryPieChart* chart = new TaskSummaryPieChart;
    summaryView->scene()->addItem(new TaskSummaryPieChart);
    summaryView->centerOn(chart);
    tabs->addTab(summaryView, "Chart");
    tab4MainLayout->addWidget(tabs);
    cal3 = new QCalendarWidget;
    cal3->setMinimumSize(QSize(250,200));
    cal4 = new QCalendarWidget;
    cal4->setMinimumSize(QSize(250,200));
    cal3->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
    cal4->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
    QHBoxLayout* calsLayout = new QHBoxLayout;
    calsLayout->addWidget(cal3);
    calsLayout->addWidget(cal4);
    cal4->setSelectedDate(QDate::currentDate().addDays(1));
    tab4MainLayout->addLayout(calsLayout);


    QVBoxLayout* tab3MainLayout = new QVBoxLayout(tab3);
    //taskSelector = new QComboBox;
    hitsList = newHitsList();
    cal1 = new QCalendarWidget;
    cal1->setMinimumSize(QSize(250,200));
    cal2 = new QCalendarWidget;
    cal2->setMinimumSize(QSize(250,200));
    cal1->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
    cal2->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
    //tab3MainLayout->addWidget(taskSelector);

    calsLayout = new QHBoxLayout;
    calsLayout->addWidget(cal1);
    calsLayout->addWidget(cal2);
    cal2->setSelectedDate(QDate::currentDate().addDays(1));
    tab3MainLayout->addWidget(hitsList);
    tab3MainLayout->addLayout(calsLayout);



    m_settings->setWindowTitle(tr("SaK"));
    m_settings->resize(400, 300);
    
}

QTreeWidget* Sak::newHitsList()
{
    QTreeWidget* hitsList = new QTreeWidget;
    hitsList->setColumnCount(4);
    hitsList->setColumnWidth(0, 250);
    hitsList->setColumnWidth(1, 150);
    hitsList->setColumnWidth(2, 150);
    hitsList->setColumnWidth(3, 150);
    hitsList->setIconSize(QSize(24,24));
    hitsList->setSortingEnabled(true);
    hitsList->setItemDelegateForColumn(0, new MyDateItemDelegate);
    hitsList->setItemDelegateForColumn(1, new TaskItemDelegate(this));
    QTreeWidgetItem* header = new QTreeWidgetItem;
    header->setText(0, "Date/Time");
    header->setText(1, "Task");
    header->setText(2, "Duration (min)");
    header->setText(3, "Overestimation");
    hitsList->setHeaderItem(header);
    hitsList->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    return hitsList;
}

QTreeWidget* Sak::newTaskSummaryList()
{
    QTreeWidget* taskSummaryList = new QTreeWidget;
    taskSummaryList->setColumnCount(4);
    taskSummaryList->setColumnWidth(0, 250);
    taskSummaryList->setColumnWidth(1, 150);
    taskSummaryList->setIconSize(QSize(24,24));
    taskSummaryList->setSortingEnabled(true);
    QTreeWidgetItem* header = new QTreeWidgetItem;
    header->setText(0, "Task");
    header->setText(1, "Hours");
    taskSummaryList->setHeaderItem(header);
    taskSummaryList->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    taskSummaryList->setEnabled(true);
    return taskSummaryList;
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

    startAction = new QAction(tr("Start polling"), m_settings);
    connect(startAction, SIGNAL(triggered()), this, SLOT(start()));

    stopAction = new QAction(tr("Stop polling"), m_settings);
    connect(stopAction, SIGNAL(triggered()), this, SLOT(stop()));

    flushAction = new QAction(tr("&Flush data/settings to disk"), m_settings);
    connect(flushAction, SIGNAL(triggered()), this, SLOT(flush()));

    saveAsDbAction = new QAction(tr("Backup as"), m_settings);
    connect(saveAsDbAction, SIGNAL(triggered()), this, SLOT(saveAsDb()));

    exportDbCsvAction = new QAction(tr("Export hits in CSV format"), m_settings);
    connect(exportDbCsvAction, SIGNAL(triggered()), this, SLOT(exportDbCsv()));

    sendByEmailAction = new QAction(tr("Send by email (with kmail)"), m_settings);
    connect(sendByEmailAction, SIGNAL(triggered()), this, SLOT(sendByEmail()));

    openAction = new QAction(tr("Open a db backupt"), m_settings);
    connect(openAction, SIGNAL(triggered()), this, SLOT(open()));

    m_addHitAction = new QAction("Add hit", m_settings);
    m_addHitMenu = new QMenu(m_settings);
    m_addHitAction->setText("Add hit");
    m_addHitMenu->addAction(m_addHitAction);
    connect(m_addHitAction, SIGNAL(triggered()), this, SLOT(addDefaultHit()));

    m_exportDataAction = new QAction("Export data", m_settings);
    m_exportDataAction->setText("Export data");
    m_addHitMenu->addAction(m_exportDataAction);
    connect(m_exportDataAction, SIGNAL(triggered()), this, SLOT(exportHits()));

    m_addTaskAction = new QAction("Add task", m_settings);
    m_addTaskMenu = new QMenu(m_settings);
    m_addTaskAction->setText("Add task");
    m_addTaskMenu->addAction(m_addTaskAction);
    connect(m_addTaskAction, SIGNAL(triggered()), this, SLOT(addDefaultTask()));
}

void Sak::trayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::DoubleClick) {
        setVisible(true);
    }
}

//END Settings <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
