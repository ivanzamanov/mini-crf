#!/bin/bash
set -e
source $(dirname $0)/functions.sh

CORPUS_PATH="/home/ivo/SpeechSynthesis/corpus-small"
OUTPUT_PATH="/home/ivo/corpus-features"

if [ -n "$1" ]; then
        CORPUS_PATH="$1"
fi

if [ -n "$2" ]; then
        OUTPUT_PATH="$2"
fi

PRAAT=praat
EXTRACTOR_SCRIPT="$(dirname $0)/extractor.sh"

mkdir -p $OUTPUT_PATH
FILES_LIST=$OUTPUT_PATH/files-list

WAVS=0
GRIDS=0
set +x
for WAV in $(find $CORPUS_PATH | grep wav$)
do
    WAVS=$(expr $WAVS + 1)
    BASE=$(basename $WAV .wav)
    GRID_NAME=$(echo $BASE | tr . _)
    GRID=$(dirname $WAV)/$GRID_NAME.TextGrid
    if [ -f $GRID ]; then
        #echo "Extracting from $WAV"
				OUTPUT=$(os_path $OUTPUT_PATH/"$GRID_NAME.Features")
				echo "$OUTPUT $WAV" >> $FILES_LIST
        echo "$WAV" "$GRID" "$OUTPUT"
        GRIDS=`expr $GRIDS + 1`
    fi
done | xargs -P 4 -n 3 -t $EXTRACTOR_SCRIPT

echo "WAVS: $WAVS"
echo "Grids: $GRIDS"
