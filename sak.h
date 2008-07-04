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

#include "sakwidget.h"
#include "sakmessageitem.h"
#include "pixmapviewer.h"
#include "task.h"


class Sak : public QObject
{
    Q_OBJECT;
public:
    Sak(QObject* parent = 0);
    virtual ~Sak();
    QHash<QString, Task>* tasks() { return &m_tasks; }
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
    void saveHitChanges();
    void selectedTask();
    void selectColor();
    void popup();
    void workingOnTask();
    void clearView();
    void trayIconActivated(QSystemTrayIcon::ActivationReason);
    // create a list of hits merging tasks
    QList<Hit> createHitsList(const QDateTime& from = QDateTime(), const QDateTime& to = QDateTime());
    void selectedStartDate(const QDate& date);
    void selectedEndDate(const QDate& date);
    void hitsListItemChanged(QTreeWidgetItem*, int column);
    void populateHitsList(QTreeWidget*, const QList<Hit>&);
    void addDefaultHit();
    void interactiveMergeHits();
private:
    int m_timerId, m_timeoutPopup;
    QDateTime m_nextTimerEvent;
    QWidget* m_settings;
    QHash<QString, Task> m_tasks;
    QHash<QString, Task> m_editedTasks;
    QMultiMap<int, SakWidget*> m_widgets;
    quint8 m_currentInterval;

    bool m_previewing;
    bool m_changedHit;
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
    QTreeWidget* newHitsList();
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

class TaskItemDelegate : public QItemDelegate
{
public:
    TaskItemDelegate(Sak* sak, QObject *parent = 0);

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                          const QModelIndex &index) const;

    void setEditorData(QWidget *editor, const QModelIndex &index) const;
    void setModelData(QWidget *editor, QAbstractItemModel *model,
                      const QModelIndex &index) const;

    void updateEditorGeometry(QWidget *editor,
                              const QStyleOptionViewItem &option, const QModelIndex &index) const;
private:
    Sak* m_sak;
};



#endif
