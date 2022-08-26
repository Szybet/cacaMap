#!/bin/bash
echo what

mv cacamap exec | EXIT_CODE=0

servername="root@192.168.2.2"
passwd="root"

sshpass -p $passwd ssh $servername "bash -c \"ifsctl mnt rootfs rw\""
sshpass -p $passwd ssh $servername "bash -c \"rm /kobo/tmp/exec\""
sshpass -p $passwd scp exec $servername:/kobo/tmp/

sshpass -p $passwd ssh $servername "bash -c \"sync\""

sshpass -p $passwd ssh $servername "bash -c \"/kobo/launch_app.sh\""
