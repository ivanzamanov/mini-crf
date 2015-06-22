#!/bin/bash

if [ $# -lt 3 ]; then
    echo "Usage: `basename $0` <wav> <textgrid> <output>"
fi

set -e

PRAAT_SCRIPT=`dirname $0`/extract.praat
WAV_FILE="$1"
TEXT_GRID="$2"
OUTPUT_FILE="$3"

TIME_STEP=0.01
MFCC_COUNT=12
MFCC_WINDOW=0.015

CMD="praat $PRAAT_SCRIPT $WAV_FILE $TEXT_GRID $OUTPUT_FILE"
echo $CMD
$CMD
