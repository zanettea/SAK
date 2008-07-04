#include "pixmapviewer.h"


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
        int w = (int)(ratio*m_p.width());
        int h = (int)(ratio*m_p.height());
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
