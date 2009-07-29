# #####################################################################
# Automatically generated by qmake (2.01a) Tue Dec 11 11:09:55 2007
# #####################################################################
TEMPLATE = app
TARGET = 
DEPENDPATH += .
INCLUDEPATH += .
CONFIG += qt
QT += gui opengl network



# Input
HEADERS += sak.h \
    sakwidget.h \
    sakmessageitem.h \
    pixmapviewer.h \
    backupper.h \
    piechart.h \
    task.h \
    saksubwidget.h \
    timeline.h \
    gmailstorage/gmailinterface.h gmailstorage/gmailpyinterface.h
SOURCES += main.cpp \
    sak.cpp \
    sakhits.cpp \
    sakwidget.cpp \
    sakmessageitem.cpp \
    pixmapviewer.cpp \
    task.cpp \
    saksubwidget.cpp \
    backupper.cpp \
    timeline.cpp \

!win32 {
	DEFINES += USEGMAIL
}

contains(DEFINES, USEGMAIL) {
	contains(DEFINES, USELIBGMAIL) {
	    LIBS += -lpython2.5
	    HEADERS += gmailstorage/gmailpyinterface.h
	    SOURCES += gmailstorage/gmailpyinterface.cpp
	} else {
	    HEADERS += gmailstorage/mailsender.h   gmailstorage/gmailmyinterface.h
	    SOURCES += gmailstorage/mailsender.cpp   gmailstorage/gmailmyinterface.cpp
	}
}


RESOURCES += sak.qrc
