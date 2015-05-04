set -e
set -x
source $(dirname $0)/functions.sh
BASE=$(readlink -f $(dirname $0))

TEMP_F=$(mktemp)
SENT="$(echo "$1" | tr ' ' '_')"

SYNTH_DB=~/SpeechSynthesis/db-synth-preemph.bin
TEST_DB=~/SpeechSynthesis/db-test-preemph.bin

pushd $BASE/../src

SYNTH_TMP=$(mktemp)
./main --mode query --synth-database $TEST_DB --sentence --input $SENT > $SYNTH_TMP
echo "Synth temp file: $SYNTH_TMP"
echo "Concat temp file: $TEMP_F"
./main-opt --mode synth --synth-database $SYNTH_DB --input - --textgrid $BASE/concat.TextGrid > "$TEMP_F" < $SYNTH_TMP
popd

set +x
fix_synth_output "$TEMP_F"
set -x
praat "$(os_path $BASE/concat.praat)" "$(os_path $TEMP_F)" concat-output.wav
