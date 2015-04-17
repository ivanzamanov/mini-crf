set -e
source $(dirname $0)/functions.sh
BASE=$(readlink -f $(dirname $0))

TEMP_F=$(mktemp)
SENT="$(echo "$1" | tr ' ' '_')"
pushd $BASE/../src > /dev/null
./main "$SENT" > "$TEMP_F"
popd > /dev/null
praat "$(os_path $BASE/concat.praat)" "$(os_path $TEMP_F)" "$(os_path /home/ivo/concat-output.wav)"
