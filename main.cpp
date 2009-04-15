#include <QtGui>
#include <signal.h>
#include "sak.h"

void sighandler(int signum) {
    static bool quitting = false;
    qDebug() << "SAK: caught signal " << signum;
    signal(signum, SIG_IGN);
    if (!quitting) {
        quitting = true;
        qApp->quit();
    }
}


class MySplashScreen : public QSplashScreen
{
public:
     MySplashScreen(const QPixmap & px) : QSplashScreen(px) {}
protected:
     void timerEvent(QTimerEvent*) { hide(); deleteLater(); }
};


int main(int argc, char** argv)
{
    QApplication app (argc, argv);

#ifdef Q_OS_LINUX
    app.setGraphicsSystem("raster");
#endif

    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
        QMessageBox::critical(0, QObject::tr("Systray"),
                              QObject::tr("I couldn't detect any system tray "
                                          "on this system."));
        return 1;
    }

    signal(SIGINT, sighandler);
#ifdef Q_OS_LINUX
    signal(SIGQUIT, sighandler);
#endif
    signal(SIGILL, sighandler);
    signal(SIGABRT, sighandler);
    signal(SIGFPE, sighandler);
    signal(SIGSEGV, sighandler);
    signal(SIGTERM, sighandler);

    QFile lockFile(QDir::homePath() + QDir::separator() + ".sak.run");
    if (lockFile.exists()) {
        if ( QMessageBox::question(0, "Another instance of SAK is running ", "Another instance of SAK seems to be running right now. Proceeding with this new instance may results in data loss / corruptions. Do you want to proceed?", QMessageBox::Yes | QMessageBox::No , QMessageBox::No) == QMessageBox::No)
            return -1;
        else {
            QFile::remove(lockFile.fileName());
        }
    }
    lockFile.open(QIODevice::ReadWrite);

    QPixmap px(":/images/splash.png");
    MySplashScreen* splash = new MySplashScreen(px);
    splash->setMask(px.createMaskFromColor(QColor(0,0,0,0), Qt::MaskInColor));
    splash->show();
    splash->startTimer(2000);
    app.processEvents();
    Sak sak;
    bool rc = app.exec();

    lockFile.close();
    QFile::remove(lockFile.fileName());
    return rc;
}
