#ifndef GMAILPYINTERFACE_H
#define GMAILPYINTERFACE_H

#include <QString>

class GmailPyInterface
{
public:
    GmailPyInterface();
    ~GmailPyInterface();
    bool isValid() { return m_gs != 0; }
    void storeTaskFiles(const QStringList& filePaths);
    bool login();
    // clear cached data
    bool forceLogin();
private:
    struct _object* m_gs;
    struct _object* m_user;
    struct _object* m_pass;
    class sendByEmailAction;
};

#endif // GMAILPYINTERFACE_H
