
if [ $# -lt 1 ]; then
    echo "Usage: $0 <wav> <png>"
    exit 1
fi

SCRIPT=$(dirname $0)/draw.praat
WAV=$(readlink -f $1)
OUTPUT=$2

praat $SCRIPT $WAV $OUTPUT
