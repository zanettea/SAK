/*************************-**************************************************
 *   Copyright (C) 2007 by Arrigo Zanette                                  *
 *   zanettea@gmail.com                                                    *
 ***************************************************************************/

#include <QtGui>
#include <QSettings>

#include "sak.h"
#include "backupper.h"


//BEGIN Hits  >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

//BEGIN MyDateItemDelegate


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


//BEGIN SubTaskItemDelegate

SubTaskItemDelegate::SubTaskItemDelegate(Sak* s, QObject *parent)
: QItemDelegate(parent), m_sak(s)
{
}

QWidget *SubTaskItemDelegate::createEditor(QWidget *parent,
                                          const QStyleOptionViewItem &/* option */,
                                          const QModelIndex & index ) const
{
    QComboBox *editor = new QComboBox(parent);
    editor->setEditable(true);

    int j = -1;
    int i=0;
    QString taskTitle = index.model()->index(index.row(), index.column()-1, index.parent()).data().toString();

    QHash<QString, Task>::iterator titr = m_sak->tasks()->find(taskTitle);
    if (titr == m_sak->tasks()->end()) {
        return editor;
    }
    const Task& task(titr.value());

    QString current = index.data().toString();
    QHash< QString, Task::SubTask >::const_iterator itr = task.subTasks.begin(), end = task.subTasks.end();
    for(int i=0; itr != end; itr++, i++) {
        editor->addItem(itr->title);
        if (itr->title == current)
            j = i;
    }

    editor->setCurrentIndex(j);
    return editor;
}

void SubTaskItemDelegate::setEditorData(QWidget *editor,
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

void SubTaskItemDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
                                      const QModelIndex &index) const
{
    QComboBox *e = static_cast<QComboBox*>(editor);
    model->setData(index, e->currentText(), Qt::EditRole);
    model->setData(index, e->itemIcon(e->currentIndex()), Qt::DecorationRole);
}

void SubTaskItemDelegate::updateEditorGeometry(QWidget *editor,
                                              const QStyleOptionViewItem &option, const QModelIndex &/* index */) const
{
    editor->setGeometry(option.rect);
}

//END TaskItemDelegate



void Sak::addDefaultHit()
{
    QTreeWidgetItem* i = new QTreeWidgetItem;
    Task::Hit p(QDateTime::currentDateTime(), 0);
    i->setText(0, p.timestamp.toString(DATETIMEFORMAT));
    i->setText(1, m_tasks.begin().key());
    i->setIcon(1, m_tasks.begin().value().icon);
    i->setSizeHint(0, QSize(32, 32));
    i->setText(3, QString("%1").arg(p.duration));
    i->setFlags(i->flags() | Qt::ItemIsEditable);
    hitsList->addTopLevelItem( i );
    m_changedHit=true;
    m_editedTasks[i->text(1)].hits[i->text(2)] << p;
    i->setData(1, Qt::UserRole, qVariantFromValue(HitElement(&m_editedTasks[i->text(1)], i->text(2), p.timestamp, p.duration)));
}

void Sak::exportHits()
{
    QString fileName = QFileDialog::getSaveFileName();
    QFile file(fileName);
    if (!file.open(QIODevice::Truncate|QIODevice::ReadWrite)) {
        QMessageBox::warning(0, QString("Error saving"), QString("Error opening file %1").arg(fileName));
        return;
    }
    QTextStream stream(&file);
    for (int i=0; i<hitsList->topLevelItemCount(); i++) {
        QTreeWidgetItem* w = hitsList->topLevelItem ( i );
        QString name(w->text(1));
        QDateTime timestamp(QDateTime::fromString(w->text(0), DATETIMEFORMAT));
        unsigned int duration = w->text(3).toInt();
        QHash<QString, Task>::iterator titr = m_tasks.find(name);
        stream << w->text(1) << ";" << w->text(0)  << ";" << w->text(2) << ";" << w->text(3) << ";\n";
    }
    file.flush();
    file.close();
}


QList<HitElement> Sak::createHitsList(const QDateTime& from , const QDateTime& to )
{
    QMap<QDateTime, HitElement> hits;
    foreach(const Task& t, m_editedTasks) {
        QHash<QString, QList< Task::Hit> > ::const_iterator hitr = t.hits.begin(), hend = t.hits.end();
        while(hitr != hend) {
            const QList< Task::Hit > & l(hitr.value());
            for (int i=0; i<l.count(); i++) {
                const QDateTime& d = l[i].timestamp;
                if ( (!from.isValid() || d >= from) && ( !to.isValid() || d <= to) ) {
                    hits.insert(d, HitElement((Task*)&t, hitr.key(), d, l[i].duration));
                }
            }
            hitr++;
        }
    }
    return hits.values();
}


QMap<double,Task*> Sak::createSummaryList(const QList<HitElement>& hits)
{
    QHash<Task*, double> summaryMap;
    foreach(const HitElement& hit, hits) {
        summaryMap[hit.task] += Task::hours(hit.duration);
    }
    QMap<double, Task*> summaryOrderedMap;
    QHash<Task*, double>::const_iterator itr = summaryMap.begin();
    while(itr != summaryMap.end()) {
       summaryOrderedMap.insertMulti(itr.value(), itr.key());
       itr++;
    }
    return summaryOrderedMap;
}



void Sak::saveHitChanges()
{
    if (m_changedHit) {
        if ( QMessageBox::question ( 0, "Hit list changed", "Hit list has changed: do you want to save changes?", QMessageBox::Save | QMessageBox::Discard,  QMessageBox::Discard) == QMessageBox::Save ) {
            qDebug() << "save changes!!!!";
            m_tasks = m_editedTasks;
        }
        m_changedHit=false;
        populateHitsList(createHitsList(QDateTime(cal1->selectedDate()), QDateTime(cal2->selectedDate())));
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
    } /*else if (e->type() == QEvent::Show) {
        m_editedTasks = m_tasks;
        populateHitsList(createHitsList(QDateTime(cal1->selectedDate()), QDateTime(cal2->selectedDate())));
    }*/
    return false;
}


void Sak::selectedStartDate(const QDate& date)
{
    if (sender() != cal1) {
        cal1->setSelectedDate(date);
    }
    if (sender() != cal3) {
        cal3->setSelectedDate(date);
    }
    if (cal2->selectedDate() < cal1->selectedDate()) {
        cal2->setSelectedDate(date);
        cal4->setSelectedDate(date);
    }
    cal2->setMinimumDate(date);
    cal4->setMinimumDate(date);
    populateHitsList(createHitsList(QDateTime(cal1->selectedDate()), QDateTime(cal2->selectedDate())));
}

void Sak::selectedEndDate(const QDate& date)
{
    if (sender() != cal2) {
        cal2->setSelectedDate(date);
    }
    if (sender() != cal4) {
        cal4->setSelectedDate(date);
    }
    if (cal1->selectedDate() > date) {
        cal2->setSelectedDate(cal1->selectedDate());
        cal4->setSelectedDate(cal1->selectedDate());
    }
    populateHitsList(createHitsList(QDateTime(cal1->selectedDate()), QDateTime(cal2->selectedDate())));
}


void Sak::hitsListItemChanged(QTreeWidgetItem* i, int column)
{
    hitsList->blockSignals(true);
    m_changedHit = true;
    // save change in structure m_editedTasks
    {
        const HitElement& origHit = i->data(1, Qt::UserRole).value<HitElement>();
        // find hit in m_editedTasks
        Q_ASSERT(origHit.task);
        Task& t(*origHit.task);
        Q_ASSERT(m_editedTasks.contains(t.title));
        Task& et(m_editedTasks[t.title]);

        QList< Task::Hit >& origList(et.hits[origHit.subtask]);

        int hitPosition = origList.indexOf(Task::Hit(origHit.timestamp, origHit.duration));
        Q_ASSERT(hitPosition != origList.size());
        if (!m_editedTasks.contains(i->text(1))) {
            i->setText(1, t.title);
            qDebug() << "Task " << i->text(1) << " does not exists -> undo change";
        } else if (column == -1) {
            qDebug() << "remove hit from task " << t.title;
            origList.takeAt(hitPosition);
        } else {
            qDebug() << "remove hit from task " << t.title;
            origList.takeAt(hitPosition);
            Task& nt(m_editedTasks[i->text(1)]);
            Task::Hit p(QDateTime::fromString(i->text(0), DATETIMEFORMAT), i->text(3).toUInt());
            i->setData(1, Qt::UserRole, qVariantFromValue(HitElement(&nt, i->text(2), p.timestamp, p.duration)));
            qDebug() << "insert hit into task " << i->text(1) << p.timestamp;
            nt.hits[i->text(2)] << p;
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

void Sak::populateHitsList(const QList<HitElement>& hits, QTreeWidget* theHitsList )
{
    if (theHitsList == 0)
        theHitsList = hitsList;
    Q_ASSERT(theHitsList);
    saveHitChanges();
    theHitsList->clear();

    QVector<double> o;
    double totOverestimation;
    HitElement::overestimations(hits, o, totOverestimation);
    QList<QTreeWidgetItem*> widgets;
    int i=0;
    foreach(const HitElement& hit, hits) {
        QTreeWidgetItem* w = new QTreeWidgetItem;
        w->setText(0, hit.timestamp.toString(DATETIMEFORMAT));
        w->setText(1, hit.task->title);
        w->setSizeHint(0, QSize(24, 24));
        w->setData(1, Qt::UserRole, qVariantFromValue(hit));
        w->setText(2, hit.subtask);
        w->setText(3, QString("%1").arg(hit.duration));
        if (o[i] != 0) {
            w->setBackground(0,Qt::red);
            w->setBackground(1,Qt::red);
            w->setBackground(2,Qt::red);
            w->setBackground(3,Qt::red);
            w->setBackground(4,Qt::red);
            w->setText(4,QString("%1").arg(o[i]));
        }
        if (hit.task->title == "<away>") {
            w->setForeground(0,Qt::gray);
            w->setForeground(1,Qt::gray);
            w->setForeground(2,Qt::gray);
            w->setForeground(3,Qt::gray);
            w->setForeground(4,Qt::gray);
        }
        w->setIcon(1, hit.task->icon);
        if (hit.editable)
            w->setFlags(w->flags() | Qt::ItemIsEditable);
        else w->setDisabled(true);
        widgets << w;
        i++;
    }
    theHitsList->addTopLevelItems(widgets);

    widgets.clear();

    if (summaryList) {
        theHitsList = summaryList;
        theHitsList->clear();
        const QMap<double, Task*>& map(createSummaryList(hits));
        QMap<double,Task*>::const_iterator itr = map.begin();
        while(itr != map.end()) {
            QTreeWidgetItem* w = new QTreeWidgetItem(QTreeWidgetItem::UserType);
            w->setText(0, itr.value()->title);
            w->setIcon(0, itr.value()->icon);
            w->setText(1, QString("%1").arg(itr.key(), 4, 'f', 1, ' '));
            //w->setFlags(w->flags() & (!Qt::ItemIsEditable));
            widgets<< w;
            itr++;
        }
        theHitsList->addTopLevelItems(widgets);
    }
}


void Sak::interactiveMergeHits()
{
    if (!m_incremental->foundPieces.count()) return;
    QDialog mergeDialog;
    mergeDialog.setWindowTitle("Merge sparse hits");
    mergeDialog.setModal(true);
    QTreeWidget* theHitsList = newHitsList();
    QMap<QDateTime, HitElement> hits;
    QMap<QDateTime, Incremental::Hit >::iterator itr = m_incremental->foundPieces.begin();
    while(itr != m_incremental->foundPieces.end()) {
        QHash<QString, Task>::iterator titr = m_tasks.find(itr.value().task);
        if (titr == m_tasks.end()) {
            qDebug() << "Discard piece for unknown task " << itr.value().task;
        } else {
            hits.insertMulti(itr.key(), HitElement(&titr.value(), itr.value().subtask, itr.key(), itr.value().duration));
        }
        itr++;
    }
    if (hits.count() == 0) return;

    qDebug() << "hits: " << hits.count() << hits.begin().key() << (--hits.end()).key();
    QList<HitElement> okHits (createHitsList(hits.begin().key(), (--hits.end()).key()));
    qDebug() << okHits.count();
    for(int i=0; i<okHits.count(); i++) {
        okHits[i].editable=false;
        hits.insertMulti(okHits[i].timestamp, okHits[i]);
    }

    qDebug() << "hits: " << hits.count();
    populateHitsList(hits.values(), theHitsList);
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
                unsigned int duration (w->text(3).toUInt());
                QHash<QString, Task>::iterator titr = m_tasks.find(name);
                Q_ASSERT(titr != m_tasks.end());
                (*titr).hits[w->text(2)] << Task::Hit(timestamp, duration);
            }
        }
        flush();
        m_incremental->clearMergedPieces();
    }
}
