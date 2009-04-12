#include "saksubwidget.h"

static const char * message = "<insert a new, meaningful, subtask title here>";

SakSubWidget::SakSubWidget(const Task& task, Task::SubTask subtask, bool editable) : m_task(task), m_subtask(subtask)
{

    m_editable = editable;

    QColor bgColor = subtask.bgColor.isValid() ? subtask.bgColor : task.bgColor;
    QColor fgColor = subtask.fgColor.isValid() ? subtask.fgColor : task.fgColor;

    m_palette.setColor(QPalette::Inactive, QPalette::Button, bgColor);
    m_palette.setColor(QPalette::Active, QPalette::Button, fgColor);

    m_palette.setColor(QPalette::Inactive, QPalette::ButtonText, fgColor);
    m_palette.setColor(QPalette::Active, QPalette::ButtonText, bgColor);

    m_showingDetails=false;

    QWidget* b = editable ? (QWidget*)new QLineEdit(subtask.title) : (QWidget*)new QPushButton(subtask.title);
    if (editable) {
        ((QLineEdit*)b)->setText(message);
        ((QLineEdit*)b)->setAlignment(Qt::AlignCenter);
    }
    setWidget(b);
    b->setPalette(m_palette);

    QFont f(font());
    f.setBold(true);
    f.setPixelSize(20);
    b->setFont(f);

    b->setEnabled(true);
    setAcceptsHoverEvents(true);
}

SakSubWidget::~SakSubWidget()
{
    //delete m_text;
}


void SakSubWidget::showDetails(bool show)
{
}

void SakSubWidget::focusOutEvent ( QFocusEvent * event )
{
    setPalette(m_palette);
    QGraphicsProxyWidget::focusOutEvent(event);
}

void SakSubWidget::focusInEvent ( QFocusEvent * event )
{
    if (m_editable)
        ((QLineEdit*)widget())->setSelection(0, 99999);
    setPalette(m_palette);
    QGraphicsProxyWidget::focusInEvent(event);
    emit focused();
}


void SakSubWidget::keyPressEvent ( QKeyEvent * event ) {
    if (event->key() == Qt::Key_Return &&  ( (event->modifiers() &  Qt::ControlModifier) || (event->modifiers() &  Qt::ShiftModifier)) ) {
        event->accept();
        emit clicked(m_task.title, !m_editable ? m_subtask.title : ((QLineEdit*)widget())->text());
    } else if (!m_editable && event->key() == Qt::Key_Space) {
        event->accept();
        showDetails(!m_showingDetails);
    } else QGraphicsProxyWidget::keyPressEvent(event);
}


void SakSubWidget::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* e)
{
    QString text;
    text = m_editable ? ((QLineEdit*)widget())->text() : ((QPushButton*)widget())->text();
    text = text == message ? "" : text;
    emit clicked(m_task.title, !m_editable ? m_subtask.title : ((QLineEdit*)widget())->text());
    QGraphicsProxyWidget::mouseDoubleClickEvent(e);
}




