#!/usr/bin/env bash

if [ $# -ne 6 ] 
then
    echo "Usage: stim_pack.sh subj_id cal_video v1 v2 v3 v4"
    exit
fi

# check if log directories exist
for DIR in [ ev_logs sys_logs vlc_logs ]; do
    if [ ! -d "$DIR" ]; then
	echo "Log dir $DIR does not exist, creating ..."
	mkdir "$DIR"
    fi
done

export SDL_VIDEO_WINDOW_POS=0,0

DATECODE="`date +%Y%m%d%H%M`"
LOGBASE="$1-$DATECODE.log"
SYSLOG="sys_logs/syslog-$LOGBASE"
VLCLOG="vlc_logs/vlclog-$LOGBASE"

# clear disk caches to have defined startup conditions
sudo sync
sudo /sbin/sysctl vm.drop_caches=3

# start stim_pack
./stim_pack $1 $2 $3 $4 $5 $6 2> $VLCLOG | tee $SYSLOG

# find the logfile from this subject and run some simple tests on it
EVLOG=`find ev_logs -name $1-*.log`
analysis/check_audio_problems.py $VLCLOG

# analyze the log for basic data
analysis/sift_event_log.m $EVLOG
