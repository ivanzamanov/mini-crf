DATE=$(date +%m-%d-%H-%M)
./features-extract.sh ~/SpeechSynthesis/corpus-synth/ ~/SpeechSynthesis/features-synth-$DATE
./features-extract.sh ~/SpeechSynthesis/corpus-test/ ~/SpeechSynthesis/features-test-$DATE
../src/text2bin ~/SpeechSynthesis/features-synth/files-list ~/SpeechSynthesis/db-synth-$DATE.bin
../src/text2bin ~/SpeechSynthesis/features-test/files-list ~/SpeechSynthesis/db-test-$DATE.bin
