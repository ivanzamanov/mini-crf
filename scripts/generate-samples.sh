
set -e

if [ $# -lt 1 ]; then
  echo "Usage: $(readlink -f $0) <synth|baseline>"
  exit 1
fi

MODE=$1

DIR=$(mktemp -d)

for ((i=0;i<30;i++))
do
  ./run-$MODE.sh $i 06-07-23-16 ../src/coefs-opt-cep-dist.conf
  mv $MODE.wav $DIR/$i.wav
done

PACKAGE=$MODE-package.zip

pushd $DIR
zip $MODE-package.zip *.wav
popd $DIR

echo Output: $(readlink -f $DIR/$PACKAGE)
