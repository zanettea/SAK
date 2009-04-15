#ifndef GMAILPYINTERFACE_H
#define GMAILPYINTERFACE_H

#include <QString>

class GmailPyInterface
{
public:
    GmailPyInterface();
    ~GmailPyInterface();
    void storeTaskFiles(const QStringList& filePaths);
    bool login();
private:
    struct _object* m_gs;
    struct _object* m_user;
    struct _object* m_pass;
    class sendByEmailAction;
};

#endif // GMAILPYINTERFACE_H
