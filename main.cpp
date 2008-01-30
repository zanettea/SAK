#include <QtGui>
#include <signal.h>
#include "sak.h"

#if defined(Q_OS_LINUX) && defined(ARGB)
    #include <X11/extensions/Xrender.h>
#endif


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
#if defined(ARGB) && defined(Q_OS_LINUX)
    bool  argbVisual=false;
    Display *dpy = XOpenDisplay(0); // open default display
    if (!dpy) {
        qWarning("Cannot connect to the X server");
        exit(1);
    }

    int screen = DefaultScreen(dpy);
    Colormap colormap = 0;
    Visual *visual = 0;
    int eventBase, errorBase;

    if (XRenderQueryExtension(dpy, &eventBase, &errorBase)) {
        int nvi;
        XVisualInfo templ;
        templ.screen  = screen;
        templ.depth   = 32;
        templ.c_class = TrueColor;
        XVisualInfo *xvi = XGetVisualInfo(dpy, VisualScreenMask |
                                          VisualDepthMask |
                                          VisualClassMask, &templ, &nvi);

        for (int i = 0; i < nvi; ++i) {
            XRenderPictFormat *format = XRenderFindVisualFormat(dpy,
                                                                xvi[i].visual);
            if (format->type == PictTypeDirect && format->direct.alphaMask) {
                visual = xvi[i].visual;
                colormap = XCreateColormap(dpy, RootWindow(dpy, screen),
                                           visual, AllocNone);
                argbVisual=true;
                break;
            }
        }
    }
    if (argbVisual == true) {
        qWarning("Found ARGB visual. Starting app...");
    }
    else  {
        qWarning("Couldn't find ARGB visual... Exiting.");
        return -1;
    }

    QApplication app(dpy, argc, argv,
                     Qt::HANDLE(visual), Qt::HANDLE(colormap));
#else 
    QApplication app (argc, argv);
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
