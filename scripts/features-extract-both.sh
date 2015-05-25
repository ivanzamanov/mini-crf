./features-extract.sh ~/SpeechSynthesis/corpus-synth/ ~/SpeechSynthesis/features-synth
./features-extract.sh ~/SpeechSynthesis/corpus-test/ ~/SpeechSynthesis/features-test
../src/text2bin ~/SpeechSynthesis/features-synth/files-list ~/SpeechSynthesis/db-synth.bin
../src/text2bin ~/SpeechSynthesis/features-test/files-list ~/SpeechSynthesis/db-test.bin
