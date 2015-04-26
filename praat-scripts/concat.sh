set -x

DIR=$(dirname $0)
praat $(readlink -f $DIR/concat.praat) $(readlink -f $1) $(readlink -f $2)
