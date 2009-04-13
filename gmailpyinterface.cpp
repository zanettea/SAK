#include "gmailpyinterface.h"

#include <python2.5/Python.h>

#include <QDebug>
#include <QFile>
#include <QResource>

GmailPyInterface::GmailPyInterface()
{
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
    PyObject* gs = PyObject_CallObject(gsClass, NULL);
    if (gs) {
        qWarning() << PyObject_CallMethod(gs, "fetchLatest", 0);
    } else {

    }

    Py_Finalize();
}
