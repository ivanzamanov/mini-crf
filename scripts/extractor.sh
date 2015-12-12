#!/bin/bash

if [ $# -lt 3 ]; then
    echo "Usage: `basename $0` <wav> <textgrid> <output> <laryngograph signal>"
fi

set -e

PRAAT_SCRIPT=$(readlink -f `dirname $0`/extract.praat)

if [ ! -f $PRAAT_SCRIPT ]; then
  echo "No such file $PRAAT_SCRIPT"
  exit 1
fi

WAV_FILE="$1"
TEXT_GRID="$2"
OUTPUT_FILE="$3"
LARYNGOGRAPH="$4"

TIME_STEP=0.01
MFCC_COUNT=12
MFCC_WINDOW=0.015

#echo "PRAAT: $(which praat)"
#echo `pwd`
EXE="$(which praat)"
CMD="$EXE $PRAAT_SCRIPT $WAV_FILE $TEXT_GRID $OUTPUT_FILE $LARYNGOGRAPH"
#echo "$CMD"
#echo "Script: $PRAAT_SCRIPT, WAVE: $WAV_FILE, TextGrid: $TEXT_GRID, Output: $OUTPUT_FILE"
$CMD > /dev/null
