#!/bin/bash

if [ $# -lt 2 ]; then
  echo "Usage: <cmd> <template> <log-file>"
  exit 1
fi

TEMPLATE=$1
LOG=$2

cat $TEMPLATE
if [ -n "`cat $LOG | grep trans-baseline`" ]; then
  tail -n 10 $LOG | head -n 7
else
  tail -n 9 $LOG | head -n 6
fi
