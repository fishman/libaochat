#!/bin/sh
cat<<X >aochat.def
LIBRARY aochat.dll

EXPORTS
X

grep -i '^[a-z0-9_ ]\+ \**[a-z0-9]\+(' aochat.h | \
	sed 's/^[a-z0-9_ ]\+ \**\([a-z0-9]\+\)(.*/.\1/i' | \
	tr '.' '\t' >>aochat.def
