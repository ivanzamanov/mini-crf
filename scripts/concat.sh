set -x

DIR=$(dirname $0)
praat $DIR/concat.praat $1 $(pwd)/$2
