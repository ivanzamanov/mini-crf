set -e
set -x
source $(dirname $0)/functions.sh
BASE=$(readlink -f $(dirname $0))

TEMP_F=$(mktemp)
SENT="$(echo "$1" | tr ' ' '_')"

pushd $BASE/../src
SYNTH_TMP=$(mktemp)
./main --mode query --synth-database db-test.bin --sentence --input $SENT > $SYNTH_TMP
echo "Synth temp file: $SYNTH_TMP"
echo "Concat temp file: $TEMP_F"
./main --mode synth --synth-database db-synth.bin --input - --textgrid $BASE/concat.TextGrid > "$TEMP_F" < $SYNTH_TMP
popd

fix_synth_output "$TEMP_F"
praat "$(os_path $BASE/concat.praat)" "$(os_path $TEMP_F)" concat-output.wav
