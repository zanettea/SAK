#include "gmailpyinterface.h"

#include <python2.5/Python.h>

#include <QFileInfo>
#include <QDebug>
#include <QFile>
#include <QResource>
#include <QtGui>

GmailPyInterface::GmailPyInterface()
{
    m_user = 0;
    m_pass = 0;

    Py_Initialize();

    PyObject* modname = PyString_FromString ("libgmail");
    PyObject* mod = PyImport_Import (modname);
    if (!mod) {
        qWarning() << "Failed initializing libgmail!!";
    }

    qWarning() << PyRun_SimpleString((char*)QResource(":/sakgmailstorage.py").data());

    PyObject* main_module = PyImport_AddModule("__main__");
    PyObject* global_dict = PyModule_GetDict(main_module);

    PyObject* gsClass = PyDict_GetItemString(global_dict, "SAKGmailStorage");
    m_gs = PyObject_CallObject(gsClass, NULL);
    if (m_gs) {
//        qWarning() << PyObject_CallMethod(gs, "fetchLatest", 0);
    } else {

    }

}

GmailPyInterface::~GmailPyInterface()
{
    Py_Finalize();
}

bool GmailPyInterface::login()
{
    bool goon=true;
    while(goon) {
        if (m_user == 0 || m_pass == 0) {
            QDialog d;
            d.setModal(true);
            QLineEdit* user(new QLineEdit);
            QLineEdit* pass(new QLineEdit);
            pass->setEchoMode(QLineEdit::Password);
            QVBoxLayout* layout(new QVBoxLayout);
            d.setLayout(layout);
            layout->addWidget(new QLabel("Enter you gmail account:"));
            QGridLayout* glayout(new QGridLayout());
            glayout->addWidget(new QLabel("Username:"),0,0);
            glayout->addWidget(user,0,1);
            glayout->addWidget(new QLabel("Password:"),1,0);
            glayout->addWidget(pass,1,1);
            layout->addLayout(glayout);
            QHBoxLayout* buttons(new QHBoxLayout);
            QPushButton* okbutton(new QPushButton("Ok"));
            QPushButton* cancelbutton(new QPushButton("Cancel"));
            QObject::connect(okbutton, SIGNAL(clicked()), &d, SLOT(accept()));
            QObject::connect(cancelbutton, SIGNAL(clicked()), &d, SLOT(reject()));
            buttons->addWidget(okbutton);
            buttons->addWidget(cancelbutton);
            layout->addLayout(buttons);
            d.exec();
            if (user->text().isEmpty() || pass->text().isEmpty()) continue;
            if (d.result() == QDialog::Accepted) {
                m_user = PyString_FromString(user->text().toAscii());
                m_pass = PyString_FromString(pass->text().toAscii());
            } else return false;
        }
        PyObject* result = PyObject_CallMethodObjArgs(m_gs, PyString_FromString("login"), m_user, m_pass, 0);
        if (!result || !PyObject_IsTrue(result)) {
            PyObject* error = PyObject_CallMethod(m_gs, "getError", 0);
            QMessageBox::warning(0, "Error!", PyString_AsString(error));
            m_user = 0;
            m_pass = 0;
        } else return true;
    }
    return false;
}


void GmailPyInterface::storeTaskFiles(const QStringList& filePaths)
{
    if (!m_gs) return;

    bool overide=false;
    foreach(const QString& filePath, filePaths) {
        QString taskName = QFileInfo(filePath).baseName();

        if (!login()) {
            if (overide)
                QApplication::restoreOverrideCursor();
            return;
        }

        if (!overide) {
            QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
            overide=true;
        }
        PyObject* filePathPy = PyString_FromString (filePath.toAscii());
        PyObject* subjectPy = PyString_FromString ((taskName + "@SAK").toAscii());
        PyObject* labelPy = PyString_FromString( "SAK" );
        PyObject* result = PyObject_CallMethodObjArgs(m_gs, PyString_FromString("store"), filePathPy, subjectPy, labelPy, 0);
        if (!result || !PyObject_IsTrue(result)) {
            PyObject* error = PyObject_CallMethod(m_gs, "getError", 0);
            QMessageBox::warning(0, "Error!", PyString_AsString(error));
        }
    }
    if (overide)
        QApplication::restoreOverrideCursor();
}


