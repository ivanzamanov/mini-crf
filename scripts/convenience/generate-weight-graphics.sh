#!/bin/bash

set -e

mkdir -p graphics

for CONF in config-trained-pb*.conf
do        
  METRIC=${CONF%%.conf}
  METRIC=${METRIC##config-trained-pb-}
  echo $CONF $METRIC
  python3 ../mini-crf/python/tabulate-config.py $CONF config-trained-$METRIC.conf -o graphics/e2-e3-${CONF%%.conf}.png --title $METRIC
done

