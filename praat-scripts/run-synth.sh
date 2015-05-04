set -e
set -x
source $(dirname $0)/functions.sh
BASE=$(readlink -f $(dirname $0))

TEMP_F=$(mktemp)
SENT="$(echo "$1" | tr ' ' '_')"

SYNTH_DB=~/SpeechSynthesis/db-synth-preemph-2.bin
TEST_DB=~/SpeechSynthesis/db-test-preemph-2.bin

pushd $BASE/../src

SYNTH_TMP=$(mktemp)
echo "Concat temp file: $TEMP_F"
./main-opt --mode resynth --synth-database $SYNTH_DB --test-database $TEST_DB --input $1 --textgrid $BASE/concat.TextGrid > "$TEMP_F"
popd

set +x
fix_synth_output "$TEMP_F"
set -x
praat "$(os_path $BASE/concat.praat)" "$(os_path $TEMP_F)" concat-output.wav
