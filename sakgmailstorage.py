#!/usr/bin/env python
# -*- coding: iso-8859-1 -*-
#

import os
import sys
import logging
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
            return 1
        self.setError("Login failed. (Wrong username/password?)\n");
        return 0;


    def store(self, filePath, subject, label):
        msg = libgmail.GmailComposedMessage(to="", subject=subject, body="", filenames=[filePath]) 
        draftMsg = self.ga.sendMessage(msg, asDraft = True) 
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

        for thread in self.ga.getMessagesByQuery(" label:SAK1", allPages=True):
            print thread
            print thread.date
            print len(thread)
            for message in thread:
                if len(message.attachments):
                    print message.attachments[0].content

        print "Done."

