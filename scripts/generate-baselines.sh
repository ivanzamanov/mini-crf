rm -rf baselines
mkdir baselines

for((i=0;i<10;i++)); do
  ./main-opt --mode baseline --input $i --output baselines/baseline-$i.wav < ../configs/home.conf
done
