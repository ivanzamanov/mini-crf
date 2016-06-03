rm -rf samples
mkdir samples

set -e

echo "Generating samples"
CONF_FILE=../configs/work.conf
COUNT=10
for (( i=0 ; i < $COUNT; i++)); do
  OUTPUT=samples/resynth-$i.wav
  ORIGINAL=samples/original-$i.wav
  echo $i
  ./main-opt --mode resynth --input $i \
    --output $OUTPUT \
    --original $ORIGINAL < $CONF_FILE
done

echo "Generating comparisons"
for (( i=0 ; i < $COUNT; i++)); do
  INPUT=samples/resynth-$i.wav
  ORIGINAL=samples/original-$i.wav
  OUTPUT=samples/comparisons-$i.csv
  ./main-opt --mode compare \
    --output $OUTPUT \
    --i $ORIGINAL --o $INPUT < $CONF_FILE
done

echo "Concatenating comparisons"
CSV=samples/comparisons-all.csv
head -n 1 samples/comparisons-0.csv > $CSV
for (( i=0 ; i < $COUNT; i++)); do
  CMP=samples/comparisons-$i.csv
  tail -n $(expr `wc -l < $CMP` - 1) $CMP >> $CSV
done
