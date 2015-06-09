set -e
if [ $# -lt 3 ]; then
  echo "Usage: $0 <sentence index> <db-name> <config_file>"
  exit 1
fi

source $(dirname $0)/functions.sh
DB_NAME=$2
CONFIG_FILE=$3

BASE=$(readlink -f $(dirname $0)/..)

TEMP_F=$(readlink -f `mktemp --suffix=baseline`)
SENT="$(echo "$1" | tr ' ' '_')"

SYNTH_DB=~/SpeechSynthesis/db-synth-${DB_NAME}.bin
TEST_DB=~/SpeechSynthesis/db-test-${DB_NAME}.bin

echo "Concat temp file: $TEMP_F"
time $BASE/src/main-opt --mode baseline --synth-database $SYNTH_DB --test-database $TEST_DB --input $1 --textgrid $BASE/concat.TextGrid < $CONFIG_FILE > "$TEMP_F"

fix_synth_output "$TEMP_F"
time praat "$(os_path $BASE/scripts/concat.praat)" "$(os_path $TEMP_F)" baseline.wav
