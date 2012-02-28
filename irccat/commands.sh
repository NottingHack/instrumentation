#!/bin/bash
#if [ "$2" != 'null' ]; then
#    echo "$1, I have pm'd you the list of available commands."
#fi

COMMANDS=`ls -m /home/instrumentation/bin/irccat | sed -r -e 's/[a-z0-9]+\.(pyc|pck),\s+//g' | sed -r -e 's/\.[a-z0-9]+//g'`
echo -en "The available commands are:\n$COMMANDS" 

