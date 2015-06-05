DATE=$(date +%m-%d-%H-%M)
set -e
./features-extract.sh ~/SpeechSynthesis/corpus-test/ ~/SpeechSynthesis/features-test-$DATE
../src/text2bin ~/SpeechSynthesis/features-test-$DATE/files-list ~/SpeechSynthesis/db-test-$DATE.bin

./features-extract.sh ~/SpeechSynthesis/corpus-synth/ ~/SpeechSynthesis/features-synth-$DATE
../src/text2bin ~/SpeechSynthesis/features-synth-$DATE/files-list ~/SpeechSynthesis/db-synth-$DATE.bin
