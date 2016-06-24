#!/bin/bash

if [ $# -lt 1 ]; then
  echo "Usage: $0 <conf-files>..."
  exit 1
fi

for CONF in $@ ; do
  DIR=samples/${CONF%%.conf}
  bash $(dirname $0)/generate-samples.sh resynth $DIR $CONF
done

DIR=baseline
bash $(dirname $0)/generate-samples.sh baseline samples/$DIR $CONF
