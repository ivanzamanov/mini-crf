#!/bin/bash
for METRIC in MFCC LogSpectrum LogSpectrumCritical WSS SegSNR
do

START_DATE=`date`
OPTS="--no-color --test-corpus-size 10 --ranges trans-ctx,trans-mfcc,trans-pitch,state-pitch,state-duration,state-energy --metric $METRIC --max-per-delta 20"
echo "CMD: $OPTS"
./main-opt $OPTS < ../configs/train-gergana.conf 2>&1 | tee training.log

END_DATE=`date`
echo "Exit code: $#" | tee -a training.log
echo "Start date: $START_DATE
End date: $END_DATE" | tee -a training.log

mv training.log training-$METRIC.log
done
