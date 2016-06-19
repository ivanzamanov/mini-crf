#!/bin/bash

if [ $# -lt 1 ]; then
  echo "Usage: $0 <conf-files>..."
  exit 1
fi

for CONF in $@ ; do
  SUFFIX=-$(basename $CONF .conf)
  echo $SUFFIX
  SUFFIX=$SUFFIX bash $(dirname $0)/generate-samples.sh resynth $CONF
done

bash $(dirname $0)/generate-samples.sh baseline $CONF
