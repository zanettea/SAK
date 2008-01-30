/***************************************************************************
 *   Copyright (C) 2007 by Arrigo Zanette                                  *
 *   zanettea@gmail.com                                                    *
 ***************************************************************************/


#ifndef __SAK_H__
#define __SAK_H__

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

struct Task
{
    Task() : bgColor(Qt::white), fgColor(Qt::black), totHours(0) {}
    QString title;
    QPixmap icon;
    QString description;
    QColor bgColor, fgColor;
    QList< QPair<QDateTime, quint8> > hits;
    double totHours, totOverestimation;
    double workedHours(const QDateTime& from, const QDateTime& to) const;
    // also set totHours
    bool checkConsistency();
    // get hours from value
    static double hours(quint8 v) { return (v < 120.0 ? ((double)v) : (120.0 + (v-120.0)*10.0)) / 60.0; };
    // get a value from a hour
    static quint8 value(double h) { Q_ASSERT(h <= 24); double v = h*60.0; return v < 120.0 ? v : 120.0 + (v-120.0)/10.0; }
};

QDataStream & operator<< ( QDataStream & out, const Task & task );
QDataStream & operator>> ( QDataStream & in, Task & task );

class SakMessageItem : public QGraphicsItem
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

    void paint ( QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget = 0 );
    QRectF m_rect, m_boundingRect;
    static QBitmap* m_mask;
    int m_border;
    static int m_maxzvalue;
    QTextDocument* m_text;
    QPixmap m_cachedPixmap;
	QPixmap m_cachedPixmapB;
    int m_flipTimer;
    int m_showDetailsTimer;
	// is painting side B??
    bool m_sideB;
	// is animating??
    bool m_showingDetails;
    double m_animItr;
    double m_dailyWorked, m_weeklyWorked, m_monthlyWorked, m_dailyPercentage, m_weeklyPercentage, m_monthlyPercentage;
};

class PixmapViewer : public QWidget
{
    QPixmap m_p;
public:
    PixmapViewer(QWidget* parent = 0) : QWidget(parent) {
        setMinimumWidth(150);
        setMinimumHeight(150);
    }
    void setPixmap(const QPixmap& p) {
        m_p = p;
        update();
    }
    QPixmap pixmap() const { return m_p; }
    void paintEvent(QPaintEvent* );
protected:
    void mousePressEvent(QMouseEvent* e);
};



class Sak : public QObject
{
    Q_OBJECT;
public:
    Sak(QObject* parent = 0);
    virtual ~Sak();
protected:
    void timerEvent(QTimerEvent* e);
    bool eventFilter(QObject* obj, QEvent* e);
    bool settingsEventFilter(QEvent* e);
    bool hitsListEventFilter(QEvent* e);
    void commitCurrentTask();
public slots:
    void start();
    void stop();
    void flush();
protected slots:
    // tasks
    void addDefaultTask();
    void populateTasks();
    void selectedTask();
    void selectColor();
    void popup();
    void workingOnTask();
    void clearView();
    void trayIconActivated(QSystemTrayIcon::ActivationReason);
    // hits
    void selectedStartDate(const QDate& date);
    void selectedEndDate(const QDate& date);
    void hitsListItemChanged(QTreeWidgetItem*, int column);
    void populateHitsList();
    void addDefaultHit();
private:
    int m_timerId, m_timeoutPopup;
    QDateTime m_nextTimerEvent;
    QWidget* m_settings;
    QHash<QString, Task> m_tasks;
    QHash<QString, Task> m_editedTasks;
    QMultiMap<int, SakWidget*> m_widgets;
    quint8 m_currentInterval;

    bool m_previewing;
    static int taskCounter;

    QAction* m_addTaskAction;
    QMenu* m_addTaskMenu;

    QMenu* m_addHitMenu;
    QAction* m_addHitAction;
    QString currentTask;
    class GView* m_view;
    class Backupper* m_backupper;
    class Incremental* m_incremental; 

    //BEGIN Settings data: 
public:
    void setVisible(bool visible);
private:
    void setupSettingsWidget();
    void createActions();

    QTabWidget* tabs;
    QWidget* tab1;
    QWidget* tab2;
    QWidget* tab3;
    QPushButton* previewButton;
    QPushButton* bgColorButton;
    QPushButton* fgColorButton;

    QLabel *durationLabel, *durationLabel1;
    QLabel *durationWarningLabel;
    QLabel *bodyLabel;
    QSpinBox *durationSpinBox;
    QTextEdit *bodyEdit;

    QTreeWidget* tasksTree;
    QComboBox* taskSelector;
    QTreeWidget* hitsList;
    QCalendarWidget* cal1, *cal2;
    PixmapViewer* taskPixmapViewer;
    QTextEdit* taskTextEditor;
    QLineEdit* taskTitleEditor;

    QAction *minimizeAction;
    QAction *maximizeAction;
    QAction *restoreAction;
    QAction *startAction;
    QAction *stopAction;
    QAction *flushAction;
    QAction *quitAction;

    QSystemTrayIcon *trayIcon;
    QMenu *trayIconMenu;
    
};

#endif
