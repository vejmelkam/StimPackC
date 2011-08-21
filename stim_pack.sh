#!/usr/bin/env bash

if [ $# -ne 6 ] 
then
    echo "Usage: stim_pack.sh subj_id cal_video v1 v2 v3 v4"
    exit
fi

DATECODE="`date +%Y%m%d%H%M`"
LOGBASE="$1-$DATECODE.log"
SYSLOG="sys_logs/syslog-$LOGBASE"
VLCLOG="vlc_logs/vlclog-$LOGBASE"

./stim_pack $1 $2 $3 $4 $5 $6 2> $VLCLOG | tee $SYSLOG

