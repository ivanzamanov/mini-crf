mkdir -p resynthed

for ((i=0;i<33;i++)) ; do
  INPUT_FILE_LINE=$(scripts/run-synth.sh $i resynth src/default.conf 2>&1 | grep "Input file: ")
  INPUT_FILE=${INPUT_FILE_LINE##Input file: }
  mv -v synth.wav resynthed/$(basename $INPUT_FILE)
done
