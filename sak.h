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

#include "task.h"


class SakWidget;
class SakSubWidget;
class SakMessageItem;
class PixmapViewer;
class Timeline;
class HitItem;

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
    bool taskTreeEventFilter(QEvent* e);
    bool hitsListEventFilter(QEvent* e);
public slots:
    void start();
    void stop();
    Task loadTaskFromFile(const QString& filePath);
    void flush();
    void open(const QStringList& filePaths = QStringList());
//    void saveAsDb();
    void exportDbCsv();
    void logInGmail();
    void saveToGmail();
    void importFromGmail();
protected slots:
    void init();
    void destroy();
    // tasks
    void addDefaultTask();
    void exportHits();
    void populateTasks();
    // commit changes on current tasks for temporary previews
    void commitCurrentTask();
    // permanentely save changes on tasks
    void saveTaskChanges();
    // permanentely save changes on hits
    void saveHitChanges();
    void selectedTask();
    void doubleClickedTask(QTreeWidgetItem* i, int column);
    void selectColor();
    void grabKeyboard(); // delayed grab of keayboard
    // popup task selection
    void popup();
    // popup subtask selection
    void popupSubtasks(const QString& taskname);
    // save current selected hit (after popup)
    void workingOnTask(const QString& taskname, const QString& subtask);
    void focusedSubTask();
    void clearView();
    void trayIconActivated(QSystemTrayIcon::ActivationReason);
    // create a list of hits merging tasks
    QList<HitElement> createHitsList(const QDateTime& from = QDateTime(), const QDateTime& to = QDateTime());
    QMap<double, QPair<Task*, QString> > createSummaryList(const QList<HitElement>& hits);
    void selectedStartDate(const QDate& date);
    void selectedEndDate(const QDate& date);
    void hitsListItemChanged(QTreeWidgetItem*, int column);
    void hitsSelectedInList(QTreeWidgetItem*, QTreeWidgetItem*);
    void hitsSelectedInTimeline(HitItem*);
    void populateHitsList(const QList<HitElement>&, QTreeWidget* t = 0);
    void populateHitsTimeline(const QList<HitElement>&, Timeline* t = 0);
    void addDefaultHit();
    void interactiveMergeHits();
private:
    int m_timerId, m_timeoutPopup, m_autoSaveTimer;
    QDateTime m_nextTimerEvent;
    QMainWindow* m_settings;
    QHash<QString, Task> m_tasks;
    QHash<QString, Task> m_editedTasks;
    QMultiMap<int, SakWidget*> m_widgets;
    QMultiMap<int, SakSubWidget*> m_subwidgets;
    class QGraphicsEllipseItem* m_marker;
    QMap<int, SakWidget*>::iterator m_widgetsIterator;
    QMap<int, SakSubWidget*>::iterator m_subwidgetsIterator;
    int m_subWidgetRank;
    QList<QString> m_taskSelectionHistory;
    QHash< QString, QList< QString > > m_subtaskSelectionHistory;
    QCompleter* m_subtaskCompleter;
    int m_currentInterval;

    bool m_previewing;
    bool m_changedHit, m_changedTask;
    bool m_subtaskView;
    static int taskCounter;

    QAction* m_addTaskAction;
    QMenu* m_addTaskMenu;

    QMenu* m_addHitMenu;
    QAction* m_addHitAction;
    QAction* m_exportDataAction;
    QString currentTask, currentSubtask;
    class GView* m_view;
    class Backupper* m_backupper;
    class Incremental* m_incremental;
    class GmailPyInterface* m_gmail;

    //BEGIN Settings data:
public:
    void setVisible(bool visible);
private:
    QTreeWidget* newHitsList();
    QTreeWidget* newTaskSummaryList();
    void setupSettingsWidget();
    void createActions();

    void scrollTasks(int npos);
    void scrollSubTasks(int npos);

    QTabWidget* tabs;
    QWidget* tab1;
    QWidget* tab2;
    QWidget* tab3;
    QWidget* tab4;
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
    QTreeWidget* hitsList, *summaryList;
    Timeline* hitsTimeline;
    QCalendarWidget* cal1, *cal2, *cal3, *cal4;
    PixmapViewer* taskPixmapViewer;
    QTextEdit* taskTextEditor;
    QLineEdit* taskTitleEditor;
    QSpinBox* estimatedHoursEditor;
    QDateEdit* dueEditor;

    QAction *minimizeAction;
    QAction *maximizeAction;
    QAction *restoreAction;
    QAction *startAction;
    QAction *stopAction;
    QAction *flushAction;
    QAction *quitAction;
    QAction *saveAsDbAction;
    QAction *openAction;
    QAction *exportDbCsvAction;
    QAction *gmailLoginAction;
    QAction *saveToGmailAction;
    QAction *sendByEmailAction;

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


class SubTaskItemDelegate : public QItemDelegate
{
public:
    SubTaskItemDelegate(Sak* sak, QObject *parent = 0);

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
