#!/bin/bash

export DISPLAY=:0

while((1)); do

    pgrep X > /dev/null
    if [ $? == 0 ] ; then
        pgrep sak > /dev/null
        if [ $? == 1 ]; then
            xmessage -buttons ok,run:17 -c "SAK not running!";
            if [ $? == 17 ]; then
                xmessage -c "STARTING SAK!!!";
                start-stop-daemon --start /root/progetti/Sak/sak
            fi
        fi
    fi
    
    sleep 10;

done;