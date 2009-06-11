#ifndef PIXMAPVIEVWER_H_
#define PIXMAPVIEVWER_H_

#include <QtGui>

class PixmapViewer : public QWidget
{
    Q_OBJECT;
    QPixmap m_p;
public:
    PixmapViewer(QWidget* parent = 0) : QWidget(parent) {
        setMinimumWidth(150);
        setMinimumHeight(150);
    }
    void setPixmap(const QPixmap& p) {
        m_p = p.scaled(qMin(p.width(), 400),qMin(p.height(),400),Qt::KeepAspectRatio);
        update();
    }
    QPixmap pixmap() const { return m_p; }
    void paintEvent(QPaintEvent* );
signals:
    void changed();
protected:
    void mousePressEvent(QMouseEvent* e);
};




#endif
