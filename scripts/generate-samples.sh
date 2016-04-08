rm -rf samples
mkdir samples

for((i=0;i<10;i++)); do
  ./main-opt --mode resynth --input $i --output samples/resynth-$i.wav $@ < ../configs/home.conf
done
