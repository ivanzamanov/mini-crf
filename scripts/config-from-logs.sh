#!/bin/bash

if [ $# -lt 2 ]; then
  echo "Usage: <cmd> <template> <log-file>"
  exit 1
fi

TEMPLATE=$1
LOG=$2

cat $TEMPLATE
tail -n 9 $LOG | head -n 6
