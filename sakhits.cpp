/***************************************************************************
 *   Copyright (C) 2007 by Arrigo Zanette                                  *
 *   zanettea@gmail.com                                                    *
 ***************************************************************************/

#include <QtGui>
#include <QSettings>

#include "sak.h"
#include "backupper.h"


//BEGIN Hits  >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

//BEGIN MyDateItemDelegate

#define DATETIMEFORMAT "yyyy/MM/dd hh:mm:ss"

MyDateItemDelegate::MyDateItemDelegate(QObject *parent)
: QItemDelegate(parent)
{
}

QWidget *MyDateItemDelegate::createEditor(QWidget *parent,
                                          const QStyleOptionViewItem &/* option */,
                                          const QModelIndex & index ) const
{
    QDateTimeEdit *editor = new QDateTimeEdit(parent);
    editor->setCalendarPopup(true);
    editor->setDisplayFormat(DATETIMEFORMAT);
    editor->setDateTime(QDateTime::fromString(index.data().toString(), DATETIMEFORMAT));
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

    model->setData(index, value.toString(DATETIMEFORMAT), Qt::EditRole);
}

void MyDateItemDelegate::updateEditorGeometry(QWidget *editor,
                                              const QStyleOptionViewItem &option, const QModelIndex &/* index */) const
{
    editor->setGeometry(option.rect);
}

//END MyDateItemDelegate

//BEGIN TaskItemDelegate

TaskItemDelegate::TaskItemDelegate(Sak* sak, QObject *parent)
: QItemDelegate(parent), m_sak(sak)
{
}

QWidget *TaskItemDelegate::createEditor(QWidget *parent,
                                          const QStyleOptionViewItem &/* option */,
                                          const QModelIndex & index ) const
{
    QComboBox *editor = new QComboBox(parent);
    int j = -1;
    int i=0;
    QString current = index.data().toString();
    foreach(const Task& t, *m_sak->tasks()) {
        editor->addItem(t.icon, t.title);
        if (t.title == current)
            j = i;
    }
    Q_ASSERT(j = -1);
    editor->setCurrentIndex(j);
    return editor;
}

void TaskItemDelegate::setEditorData(QWidget *editor,
                                       const QModelIndex &index) const
{
    const QString& value ( index.data().toString() );
    
    QComboBox *e = static_cast<QComboBox*>(editor);
    for(int i=0; i<e->count(); i++) {
        if (e->itemText(i) == value) {
            e->setCurrentIndex(i);
            break;
        }
    }
}

void TaskItemDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
                                      const QModelIndex &index) const
{
    QComboBox *e = static_cast<QComboBox*>(editor);
    model->setData(index, e->currentText(), Qt::EditRole);
    model->setData(index, e->itemIcon(e->currentIndex()), Qt::DecorationRole);
}

void TaskItemDelegate::updateEditorGeometry(QWidget *editor,
                                              const QStyleOptionViewItem &option, const QModelIndex &/* index */) const
{
    editor->setGeometry(option.rect);
}

//END TaskItemDelegate

void Sak::addDefaultHit()
{
    QTreeWidgetItem* i = new QTreeWidgetItem;
    QPair<QDateTime, quint8> p(QDateTime::currentDateTime(), 0);
    i->setText(0, p.first.toString(DATETIMEFORMAT));
    i->setText(1, m_tasks.begin().key());
    i->setIcon(1, m_tasks.begin().value().icon);
    i->setText(2, QString("%1").arg(p.second));
    i->setFlags(i->flags() | Qt::ItemIsEditable);
    hitsList->addTopLevelItem( i );
    m_changedHit=true;
    m_editedTasks[i->text(1)].hits << p;
    i->setData(1, Qt::UserRole, qVariantFromValue(Hit(&m_editedTasks[i->text(1)], p.first, p.second)));
}


QList<Hit> Sak::createHitsList(const QDateTime& from , const QDateTime& to )
{
    QMap<QDateTime, Hit> hits;
    foreach(const Task& t, m_editedTasks) {
        QList< QPair<QDateTime, quint8> >::const_iterator itr = t.hits.begin(), end = t.hits.end();
        for (int i=0; i<t.hits.count(); i++) {
            const QDateTime& d = t.hits[i].first;
            if ( (!from.isValid() || d >= from) && ( !to.isValid() || d <= to) ) {
                hits.insert(d, Hit((Task*)&t, t.hits[i].first, t.hits[i].second));
            }
        }
    }
    return hits.values();
}



void Sak::saveHitChanges()
{
    if (m_changedHit) {
        if ( QMessageBox::question ( 0, "Hit list changed", "Hit list has changed: do you want to save changes?", QMessageBox::Save | QMessageBox::Discard,  QMessageBox::Discard) == QMessageBox::Save ) {
            qDebug() << "save changes!!!!";
            m_tasks = m_editedTasks;
        }
        m_changedHit=false;
    }
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
        foreach(QTreeWidgetItem* ii, selected) {
            hitsListItemChanged(hitsList->takeTopLevelItem(hitsList->indexOfTopLevelItem (ii)), -1);
        }
        return true;
    } else if (e->type() == QEvent::Hide) {
        saveHitChanges();
    } else if (e->type() == QEvent::Show) {
        m_editedTasks = m_tasks;
    }
    return false;
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
    populateHitsList(hitsList, createHitsList(QDateTime(cal1->selectedDate()), QDateTime(cal2->selectedDate())));
}

void Sak::selectedEndDate(const QDate& date)
{
    if (sender() != cal2) {
        cal2->setSelectedDate(date);
    }
    if (cal1->selectedDate() > date) {
        cal2->setSelectedDate(cal1->selectedDate());
    }
    populateHitsList(hitsList, createHitsList(QDateTime(cal1->selectedDate()), QDateTime(cal2->selectedDate())));
}


void Sak::hitsListItemChanged(QTreeWidgetItem* i, int column)
{
    hitsList->blockSignals(true);
    m_changedHit = true;
    // save change in structure m_editedTasks
    {
        const Hit& origHit = i->data(1, Qt::UserRole).value<Hit>();
        // find hit in m_editedTasks
        Q_ASSERT(origHit.task);
        Task& t(*origHit.task);
        Q_ASSERT(m_editedTasks.contains(t.title));
        Task& et(m_editedTasks[t.title]);
        int hitPosition = et.hits.indexOf(QPair<QDateTime, quint8>(origHit.timestamp, origHit.duration));
        Q_ASSERT(hitPosition != et.hits.size());
        if (!m_editedTasks.contains(i->text(1))) {
            i->setText(1, t.title);
            qDebug() << "Task " << i->text(1) << " does not exists -> undo change";
        } else if (i->text(1) != t.title) {
            qDebug() << "remove hit from task " << t.title;
            et.hits.takeAt(hitPosition);
            qDebug() << "insert hit into task " << i->text(1);
            Task& nt(m_editedTasks[i->text(1)]);
            QPair<QDateTime,quint8> p( QDateTime::fromString(i->text(0), DATETIMEFORMAT), Task::value(i->text(2).toFloat()) );
            i->setData(1, Qt::UserRole, qVariantFromValue(Hit(&nt, p.first, p.second)));
            nt.hits << p;
        } else if (column == -1) {
            qDebug() << "remove hit from task " << t.title;
            et.hits.takeAt(hitPosition);
        } else {
            qDebug() << "modify hit on task " << t.title;
            et.hits[hitPosition] = QPair<QDateTime,quint8>( QDateTime::fromString(i->text(0), DATETIMEFORMAT), Task::value(i->text(2).toFloat()) );
            i->setData(1, Qt::UserRole, qVariantFromValue(Hit(&et, et.hits[hitPosition].first, et.hits[hitPosition].second)));
        }
    }
    if (column == 1) {
        if (!i) return;
        if (m_editedTasks.contains(i->text(column))) {
            i->setIcon(column, m_editedTasks[i->text(column)].icon);
        } else {
            i->setIcon(column, QIcon());
        }
    }
    hitsList->blockSignals(false);
}

//static int HitMetatype = qRegisterMetaType<Hit>("Hit");

void Sak::populateHitsList(QTreeWidget* theHitsList, const QList<Hit>& hits)
{
    saveHitChanges();
    theHitsList->clear();

    QVector<double> o;
    double totOverestimation;
    Hit::overestimations(hits, o, totOverestimation);
    QList<QTreeWidgetItem*> widgets;
    int i=0;
    foreach(const Hit& hit, hits) {
        QTreeWidgetItem* w = new QTreeWidgetItem;
        w->setText(0, hit.timestamp.toString(DATETIMEFORMAT));
        w->setText(1, hit.task->title);
        w->setData(1, Qt::UserRole, qVariantFromValue(hit));
        w->setText(2, QString("%1").arg(Task::min(hit.duration)));
        if (o[i] != 0) {
            w->setBackground(0,Qt::red);
            w->setBackground(1,Qt::red);
            w->setBackground(2,Qt::red);
            w->setBackground(3,Qt::red);
            w->setText(3,QString("%1").arg(o[i]));
        }
        w->setIcon(1, hit.task->icon);
        if (hit.editable)
            w->setFlags(w->flags() | Qt::ItemIsEditable);
        else w->setDisabled(true);
        widgets << w;
        i++;
    }
    theHitsList->addTopLevelItems(widgets);
}

void Sak::interactiveMergeHits()
{
    if (!m_incremental->foundPieces.count()) return;
    m_editedTasks=m_tasks;
    QDialog mergeDialog;
    mergeDialog.setWindowTitle("Merge sparse hits");
    mergeDialog.setModal(true);
    QTreeWidget* theHitsList = newHitsList();
    QMap<QDateTime, Hit> hits;
    QMap<QDateTime, QPair<QString, quint8> >::iterator itr = m_incremental->foundPieces.begin();
    while(itr != m_incremental->foundPieces.end()) {
        QHash<QString, Task>::iterator titr = m_tasks.find(itr.value().first);
        if (titr == m_tasks.end()) {
            qDebug() << "Discard piece for unknown task " << itr.value().first;
        } else {
            hits.insertMulti(itr.key(), Hit(&titr.value(), itr.key(), itr.value().second));
        }
        itr++;
    }

    qDebug() << "hits: " << hits.count() << hits.begin().key() << (--hits.end()).key();
    QList<Hit> okHits (createHitsList(hits.begin().key(), (--hits.end()).key()));
    qDebug() << okHits.count();
    for(int i=0; i<okHits.count(); i++) {
        okHits[i].editable=false;
        hits.insertMulti(okHits[i].timestamp, okHits[i]);
    }

    qDebug() << "hits: " << hits.count();
    populateHitsList(theHitsList, hits.values());
    mergeDialog.setMinimumWidth(750);
    mergeDialog.setMinimumHeight(600);
    QVBoxLayout mainLayout(&mergeDialog);
    mainLayout.addWidget(theHitsList);
    QPushButton* ok = new QPushButton("Do merge!");
    QPushButton* cancel = new QPushButton("Cancel");
    QHBoxLayout* buttons = new QHBoxLayout;
    buttons->addWidget(ok);
    buttons->addWidget(cancel);
    mainLayout.addLayout(buttons);
    connect(ok, SIGNAL(clicked()), &mergeDialog, SLOT(accept()));
    connect(cancel, SIGNAL(clicked()), &mergeDialog, SLOT(reject()));
    // do auto merging
    if ( mergeDialog.exec() ) {
        qDebug() << "Do merge!!!";
        for (int i=0; i<theHitsList->topLevelItemCount(); i++) {
            QTreeWidgetItem* w = theHitsList->topLevelItem ( i );
            if (!w->isDisabled()) {
                QString name(w->text(1));
                QDateTime timestamp(QDateTime::fromString(w->text(0), DATETIMEFORMAT));
                quint8 duration (Task::value(w->text(2).toFloat()/60.0));
                QHash<QString, Task>::iterator titr = m_tasks.find(name);
                Q_ASSERT(titr != m_tasks.end());
                (*titr).hits << QPair<QDateTime, quint8>(timestamp, duration);
            }
        }
        flush();
        m_incremental->clearMergedPieces();
    }
}
