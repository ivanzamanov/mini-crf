
BASE=$(dirname $0)
DATE=$(date +%m-%d-%H-%M)
set -e
set -x

mkdir -p logs
LOG_FILE=logs/features-extraction-${DATE}.log

if [ ! "$1" == "test" ]; then
$BASE/features-extract.sh ~/SpeechSynthesis/corpus-test/ ~/SpeechSynthesis/features-test-$DATE 2>&1 | tee -a $LOG_FILE
$BASE/../src/text2bin ~/SpeechSynthesis/features-test-$DATE/files-list ~/SpeechSynthesis/db-test-$DATE.bin 2>&1 | tee -a $LOG_FILE
fi

if [ ! "$2" == "synth" ]; then
$BASE/features-extract.sh ~/SpeechSynthesis/corpus-synth/ ~/SpeechSynthesis/features-synth-$DATE 2>&1 | tee -a $LOG_FILE
$BASE/../src/text2bin ~/SpeechSynthesis/features-synth-$DATE/files-list ~/SpeechSynthesis/db-synth-$DATE.bin 2>&1 | tee -a $LOG_FILE
fi

echo "Generated dbs $DATE"
