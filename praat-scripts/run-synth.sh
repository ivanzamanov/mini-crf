BASE=$(readlink -f $(dirname $0))
MAIN=$BASE/../src/main

TEMP_F=$(mktemp)
SENT=$(echo "$1" | tr ' ' '_')
$MAIN "$SENT" > $TEMP_F
praat $BASE/concat.praat $TEMP_F /home/ivo/concat-output.wav
