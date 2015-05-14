set -e
set -x
source $(dirname $0)/functions.sh
BASE=$(readlink -f $(dirname $0))

TEMP_F=$(readlink -f concat-input.txt)
SENT="$(echo "$1" | tr ' ' '_')"

SYNTH_DB=~/SpeechSynthesis/db-synth.bin
TEST_DB=~/SpeechSynthesis/db-test.bin

pushd $BASE/../src

echo "Concat temp file: $TEMP_F"
time ./main-opt --mode resynth --synth-database $SYNTH_DB --test-database $TEST_DB --input $1 --textgrid $BASE/concat.TextGrid > "$TEMP_F"
popd

set +x
fix_synth_output "$TEMP_F"
set -x
praat "$(os_path $BASE/concat.praat)" "$(os_path $TEMP_F)" concat-output.wav
