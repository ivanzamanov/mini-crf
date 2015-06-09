
set -e

if [ $# -lt 1 ]; then
  echo "Usage: $(readlink -f $0) <synth|baseline>"
  exit 1
fi

MODE=$1

DIR=$(mktemp -d)
LOG_FILE=$DIR/log.txt

for ((i=0;i<30;i++))
do
  ./run-$MODE.sh $i 06-07-23-16 ../src/coefs-opt-cep-dist.conf &>> $LOG_FILE
  mv $MODE.wav $DIR/$i.wav
done

PACKAGE=$MODE-package.zip

pushd $DIR
zip $MODE-package.zip log.txt *.wav
popd

echo Output: $(readlink -f $DIR/$PACKAGE)
