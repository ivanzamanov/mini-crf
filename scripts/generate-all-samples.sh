#!/bin/bash

for CONF in /home/ivo/SpeechSynthesis/training-logs/05-29/*.conf ; do
  SUFFIX=-$(basename $CONF .conf)
  echo $SUFFIX
  SUFFIX=$SUFFIX bash $(dirname $0)/generate-samples.sh resynth $CONF
done

bash $(dirname $0)/generate-samples.sh baseline $CONF
