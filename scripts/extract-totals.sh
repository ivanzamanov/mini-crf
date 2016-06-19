#!/bin/bash

if [ $# -lt 1 ]; then
  echo "Usage: <cmd> <dir> <output>"
  exit 1
fi

DIR=$1
OUTPUT=$2

cat $DIR/comparisons-0.csv | head -n 1 > "$OUTPUT"

for FILE in $DIR/comparisons-*.csv ; do
  #echo $FILE
  if [ $(basename $FILE) != "comparisons-all.csv" ]; then
    cat $FILE | head -n 2 | tail -n 1 >> "$OUTPUT"
  fi
done

