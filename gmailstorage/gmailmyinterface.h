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
    bool isValid() { return true; }
    void storeTaskFiles(const QStringList& filePaths);
    bool login();
    QStringList fetchLatestTasks() { return QStringList(); }
private:
    class MailSender* m_sender;
};

#endif // GMAILPYINTERFACE_H
