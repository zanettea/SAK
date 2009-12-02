#!/bin/bash

cd `dirname $0`

DIR=`pwd`

#echo r > /tmp/gdbinit

#screen -d -m  gdb $DIR/sak 

#screen -d -m  valgrind --log-file=/tmp/sak.valgrind.log $DIR/sak -x /tmp/gdbinit


echo -e "ulimit -c unlimited && cd $DIR && $DIR/sak" > /tmp/sakscript
chmod a+x /tmp/sakscript

screen -d -m /tmp/sakscript
