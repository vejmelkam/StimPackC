#!/usr/bin/env python

import sys
import string

# read lines from log file
f = open(sys.argv[1], "r")
lines = f.readlines()
f.close()

# find number of instance of "dropping buffer"
found = 0
for line in lines:
    if string.find(line, "dropping buffer") >= 0:
        found += 1

print("\n ****     check audio problems script     ****");
print("VLC log contains %d lines." % len(lines))
if found < 20:
    print("Audio problems noted %d times, no problem for 4 videos." % found)
else:
    print("Audio problems noted %d times !!! Check audio log and question subject." % found)


