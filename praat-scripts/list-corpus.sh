
CORPUS_PATH="/home/ivo/SpeechSynthesis/corpus"
OUTPUT="wav-grid-list.txt"

if [ -f $OUTPUT ]; then
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
        echo $WAV $GRID >> $OUTPUT
        GRIDS=`expr $GRIDS + 1`
    fi
done

echo "WAVS: $WAVS"
echo "Grids: $GRIDS"
