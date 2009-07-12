#ifndef SAKSUBWIDGET_H_
#define SAKSUBWIDGET_H_

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

class SakSubWidget : public QGraphicsProxyWidget
{
    Q_OBJECT;
public:
    SakSubWidget(const Task& task, Task::SubTask subtask, bool editable=false);
    ~SakSubWidget();
public slots:
    void showDetails(bool show = true);
    const Task& task () const {return m_task;}
    const Task::SubTask& subtask() const {return m_subtask;}
signals:
    void clicked(const QString& taskName, const QString& subtaskName);
    void focused();
public:
    void keyPressEvent (QKeyEvent * event);
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* e);
    void focusOutEvent ( QFocusEvent * event );
    void focusInEvent ( QFocusEvent * event );
private:
    const Task& m_task;
    const Task::SubTask m_subtask;
    // returns current text
    QString getMeaningfulText();

    QPointF m_position;
    qreal m_scale;
    QPoint m_lastPoint;
    QPalette m_palette;
    QString m_tooltipText;

    bool m_showingDetails;
    bool m_editable;
    double m_animItr;
};


#endif



