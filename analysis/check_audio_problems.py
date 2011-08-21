#!/usr/bin/env python

import sys

# read lines from log file
f = open(sys.argv[1], "r")
lines = f.readlines()
f.close()

# find number of instance of "dropping buffer"
found = 0
for line in lines:
    if string.find(line, "dropping buffer") >= 0:
        found += 1

print(" ****     check audio problems script     ****\n");
print("VLC log contains %d lines.\n" % len(lines))
print("Audio problems noted %d times.\n" % found)

