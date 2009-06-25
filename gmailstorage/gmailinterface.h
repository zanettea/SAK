#ifndef GMAILINTERFACE_H
#define GMAILINTERFACE_H

#include <QString>
#include <QStringList>

class GmailInterface
{
public:
    GmailInterface() {}
    virtual ~GmailInterface() {}
    virtual bool isValid() { return false; }
    virtual void storeTaskFiles(const QStringList& filePaths) = 0;
    virtual bool login() = 0;
    // clear cached data
    virtual bool forceLogin() { m_user=""; m_pass=""; login(); }
    virtual QStringList fetchLatestTasks() { return QStringList(); }
protected:
    QString m_user;
    QString m_pass;
    class sendByEmailAction;
};

#endif // GMAILPYINTERFACE_H
