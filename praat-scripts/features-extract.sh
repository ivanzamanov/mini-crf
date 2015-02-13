
CORPUS_PATH="/home/ivo/SpeechSynthesis/corpus"
OUTPUT_PATH="/home/ivo/corpus-features"

PRAAT=praat
EXTRACTOR_SCRIPT="`dirname $0`/extractor.sh"

mkdir -p OUTPUT_PATH
if [ -f "$OUTPUT" ]; then
    rm $OUTPUT
fi

WAVS=0
GRIDS=0
for WAV in `find $CORPUS_PATH | grep wav$`
do
    WAVS=`expr $WAVS + 1`
    BASE=`basename $WAV .wav`
    GRID_NAME=`echo $BASE | tr . _`
    GRID=`dirname $WAV`/$GRID_NAME.TextGrid
    if [ -f $GRID ]; then
        echo "Extracting from $WAV"
        $EXTRACTOR_SCRIPT "$WAV" "$GRID" $OUTPUT_PATH/"$GRID_NAME.Features"
        GRIDS=`expr $GRIDS + 1`
    fi
done

echo "WAVS: $WAVS"
echo "Grids: $GRIDS"
