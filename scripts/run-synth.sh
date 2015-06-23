set -e
if [ $# -lt 3 ]; then
  echo "Usage: $0 <sentence index> <db-name> <config_file>"
  exit 1
fi

DB_NAME=$2
CONFIG_FILE=$3

TEMP_F=$(mktemp /tmp/temp-synth.XXXXXX)
SENT="$(echo "$1" | tr ' ' '_')"

SYNTH_DB=~/SpeechSynthesis/db-synth-${DB_NAME}.bin
TEST_DB=~/SpeechSynthesis/db-test-${DB_NAME}.bin

echo "Concat temp file: $TEMP_F"
time src/main-opt --mode resynth --synth-database $SYNTH_DB --test-database $TEST_DB --input $1 < $CONFIG_FILE > "$TEMP_F"

praat scripts/concat.praat $TEMP_F $(pwd)/synth.wav
