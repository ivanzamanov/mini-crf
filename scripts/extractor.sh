#!/bin/bash

if [ $# -lt 3 ]; then
    echo "Usage: `basename $0` <wav> <textgrid> <output> <laryngograph signal>"
fi

set -e

#PRAAT_SCRIPT=$(readlink -f `dirname $0`/extract.praat)
PRAAT_SCRIPT=$(`dirname $0`/praat/extract-new.praat)

if [ ! -f $PRAAT_SCRIPT ]; then
  echo "No such file $PRAAT_SCRIPT"
  exit 1
fi

WAV_FILE="$1"
TEXT_GRID="$2"
OUTPUT_FILE="$3"
LARYNGOGRAPH="$4"

#echo "PRAAT: $(which praat)"
#echo `pwd`

EXE="$(which praat) --run"
EXE_EXTR=$(dirname $0)/praat/py-extract.py

#CMD="$EXE $PRAAT_SCRIPT $WAV_FILE $TEXT_GRID $OUTPUT_FILE $LARYNGOGRAPH"
CMD="$EXE $PRAAT_SCRIPT $WAV_FILE $TEXT_GRID $LARYNGOGRAPH"
echo "Exec: $CMD"
#echo "Script: $PRAAT_SCRIPT, WAVE: $WAV_FILE, TextGrid: $TEXT_GRID, Output: $OUTPUT_FILE"
$CMD | $EXE_EXTR - $OUTPUT_FILE
