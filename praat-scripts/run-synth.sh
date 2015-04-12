set -e
BASE=$(readlink -f $(dirname $0))

TEMP_F=$(mktemp)
SENT="$(echo "$1" | tr ' ' '_')"
pushd $BASE/../src > /dev/null
./main "$SENT" > "$TEMP_F"
popd > /dev/null
praat $BASE/concat.praat $TEMP_F /home/ivo/concat-output.wav
