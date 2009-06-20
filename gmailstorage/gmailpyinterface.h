#ifndef GMAILPYINTERFACE_H
#define GMAILPYINTERFACE_H

#include <QString>
#include <QStringList>
#include "gmailinterface.h"

class GmailPyInterface : public GmailInterface
{
public:
    GmailPyInterface();
    ~GmailPyInterface();
    bool isValid() { return m_gs != 0; }
    void storeTaskFiles(const QStringList& filePaths);
    bool login();
    // clear cached data
    bool forceLogin();
    QStringList fetchLatestTasks() { return QStringList(); };
private:
    struct _object* m_gs;
    struct _object* m_userObj;
    struct _object* m_passObj;
};

#endif // GMAILPYINTERFACE_H
