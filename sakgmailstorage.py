#!/usr/bin/env python
# -*- coding: iso-8859-1 -*-
#

import os
import sys
import logging
import libgmail


class SAKGmailStorage:

    def retInt(self):
        print "\nciao\n"
        return 2

    def store(self, filePath, label):
        if ga.storeFile(filePath, label=label):
            print "File `%s` stored successfully in `%s`." % (filename, label)
        else:
            print "Could not store file."

    def fetchLatest(self):
        print "\nFetch latest tasks..."
        ga = libgmail.GmailAccount("zanettea", "bear4ever")

        print "\nPlease wait, logging in..."

        try:
            ga.login()
        except libgmail.GmailLoginFailure:
            print "\nLogin failed. (Wrong username/password?)"
        else:
            print "Log in successful.\n"

        for thread in ga.getMessagesByQuery(" label:SAK1", allPages=True):
            print thread
            print thread.date
            print len(thread)
            for message in thread:
                if len(message.attachments):
                    print message.attachments[0].content

        print "Done."

