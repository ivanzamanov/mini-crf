set -e
source $(dirname $0)/functions.sh
BASE=$(readlink -f $(dirname $0))

TEMP_F=$(mktemp)
SENT="$(echo "$1" | tr ' ' '_')"

pushd $BASE/../src
./main --mode synth --database db.bin --input "$SENT" --textgrid $BASE/concat.TextGrid > "$TEMP_F"
popd

fix_synth_output "$TEMP_F"
praat "$(os_path $BASE/concat.praat)" "$(os_path $TEMP_F)" concat-output.wav
