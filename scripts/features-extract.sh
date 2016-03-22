#!/bin/bash
set -e
set -x

CORPUS_PATH="/home/ivo/SpeechSynthesis/corpus-small"
OUTPUT_PATH="/home/ivo/corpus-features"

if [ -n "$1" ]; then
        CORPUS_PATH="$1"
fi

if [ -n "$2" ]; then
        OUTPUT_PATH="$2"
fi

LARYNGOGRAPH_DIR="$3"
if [ -d "$LARYNGOGRAPH_DIR" ]; then
    N_PARAMS=4
else
    N_PARAMS=3
fi

PRAAT=praat
EXTRACTOR_SCRIPT="$(dirname $0)/extractor.sh"

mkdir -p $OUTPUT_PATH
FILES_LIST=$OUTPUT_PATH/files-list
rm -f $FILES_LIST

WAVS=0
GRIDS=0
set +x
echo "Found $(find $CORPUS_PATH | grep wav$ | wc -l) wavs"

for WAV in $(find $CORPUS_PATH | grep wav$)
do
    WAVS=$(expr $WAVS + 1)
    BASE=$(basename $WAV .wav)
    GRID_NAME=$(echo $BASE | tr . _)
    GRID=$(dirname $WAV)/$GRID_NAME.TextGrid
    if [ -f $GRID ]; then
        #echo "Extracting from $WAV"
	      OUTPUT=$OUTPUT_PATH/"$GRID_NAME.Features"
        if [ -d "$LARYNGOGRAPH_DIR" ]; then
            LARYNGOGRAPH_FILE=$LARYNGOGRAPH_DIR/$(basename $WAV)
        fi
	      echo "$OUTPUT $WAV" >> $FILES_LIST
        echo "$WAV" "$GRID" "$OUTPUT $LARYNGOGRAPH_FILE"
        GRIDS=`expr $GRIDS + 1`
    fi
done | sort | xargs -P 8 -n $N_PARAMS -t $EXTRACTOR_SCRIPT

echo "WAVS: $WAVS"
echo "Grids: $GRIDS"
