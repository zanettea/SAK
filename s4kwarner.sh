#!/bin/bash

export DISPLAY=:0

while((1)); do

    xhost +
    /usr/bin/pgrep X > /dev/null
    if [ $? == 0 ] ; then
        /usr/bin/pgrep sak > /dev/null
        if [ $? == 1 ]; then
            /usr/bin/xmessage -buttons ok,run:17 -c "SAK not running!";
            if [ $? == 17 ]; then
                /usr/bin/xmessage -c "STARTING SAK!!!";
                start-stop-daemon --start /root/progetti/Sak/sak.sh
            fi
        fi
    fi
    
    sleep 10;

done;