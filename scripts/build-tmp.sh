rm -rf ~/SpeechSynthesis/features-test-tmp/
./features-extract.sh /home/ivo/SpeechSynthesis/corpus-test/ /home/ivo/SpeechSynthesis/features-test-tmp
../src/text2bin /home/ivo/SpeechSynthesis/features-test-tmp/files-list /home/ivo/SpeechSynthesis/db-test-tmp.bin
