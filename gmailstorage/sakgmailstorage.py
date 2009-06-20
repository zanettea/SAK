#!/usr/bin/env python
# -*- coding: iso-8859-1 -*-
#

import os
import sys
import libgmail


class SAKGmailStorage:

    def __init__(self):
         self.error="No error"

    def setError(self, error):
         self.error = error
         sys.stderr.write(self.error)

    def getError(self):
        return self.error

    def login(self, user, passwd):
#        self.setError("Unknown error logging in as " % (user) )
        self.user = user
        self.ga = libgmail.GmailAccount(user, passwd)
        try:
            self.ga.login()
        except libgmail.GmailLoginFailure:
            self.setError("Login failed. (Wrong username/password?)\n")
            return 0
        except:
            self.setError("Network error\n")
            return 0;
        else:
            self.setError("Log in successful.\n")
            self.user = user
            return 1
        self.setError("Login failed. (Wrong username/password?)\n");
        return 0;


    def store(self, filePath, subject, label):
#        self.setError("Unknown error storing file %s in label %s " % (filePath, label) )
        msg = libgmail.GmailComposedMessage(to=self.user, subject=subject, body="SAK backup!", filenames=[filePath]) 
        try:
            draftMsg = self.ga.sendMessage(msg, asDraft = True) 
        except libgmail.GmailSendError, error:
            self.setError("Error sending file `%s` in `%s`: %s" % (filePath, label, error) )
            return 0
        if draftMsg and label: 
            draftMsg.addLabel(label) 
        if draftMsg:
            self.setError( "File `%s` stored successfully in `%s`." % (filePath, label) )
            return 1
        else:
            self.setError( "Could not store file `%s` in `%s`." % (filePath, label) )
            return 0

    def fetchLatest(self):
        print "\nFetch latest tasks..."

        tasks = {}


        for thread in self.ga.getMessagesByQuery(" label:SAK ", allPages=True):
            if len(thread) == 0: continue
            items = thread._account._parseSearchResult(libgmail.U_QUERY_SEARCH, 
                                                      view = libgmail.U_CONVERSATION_VIEW, 
                                                      th = thread.id, 
                                                       q = "in:anywhere") 
            attachments=None
            subject=None
            message=None
            # TODO: Handle this better? 
            # Note: This handles both draft & non-draft messages in a thread... 
            for key, isDraft in [(libgmail.D_MSGINFO, False), (libgmail.D_DRAFTINFO, True)]: 
                try: 
                    msgsInfo = items[key] 
                except KeyError: 
                    # No messages of this type (e.g. draft or non-draft) 
                    continue 
                else: 
                    msg = msgsInfo[0]
                    subject = msg[libgmail.MI_SUBJECT]
                    attachments = msg[16] # MI_ATTACHINFO per i draft pare non funzionare
                    message = libgmail.GmailMessage(thread, msg, isDraft = isDraft)
                break               

            if len(attachments):       
                print attachments[0]
                a = libgmail.GmailAttachment(message, attachments[0])
                if a.mimetype != "text/xml":
                    print "skip alien mime type"
                    continue
                if a.filename in tasks:
                    print "skip older ", a.filename
                    continue
                else:
                    tasks[a.filename] = None
                print a.filename
                print a.mimetype
                

        print "Done."

#ga = SAKGmailStorage();
#ga.login("zanetteatest", "test4test");
#ga.store("/tmp/prova.txt", "prova", "SAK");
#ga.fetchLatest();
