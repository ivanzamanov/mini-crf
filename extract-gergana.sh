./scripts/features-extract.sh /Users/ivanzamanov/SpeechSynthesis/corpuses-gergana/test /tmp/features-test /Users/ivanzamanov/SpeechSynthesis/GERGANA/glottograph
./scripts/features-extract.sh /Users/ivanzamanov/SpeechSynthesis/corpuses-gergana/synth /tmp/features-synth /Users/ivanzamanov/SpeechSynthesis/GERGANA/glottograph
#./scripts/features-extract.sh /Users/ivanzamanov/SpeechSynthesis/corpuses-gergana/eval /tmp/features-eval /Users/ivanzamanov/SpeechSynthesis/GERGANA/glottograph

./src/text2bin /tmp/features-test/files-list /Users/ivanzamanov/SpeechSynthesis/db-test-gergana.bin
./src/text2bin /tmp/features-synth/files-list /Users/ivanzamanov/SpeechSynthesis/db-synth-gergana.bin
#./src/text2bin /tmp/features-eval/files-list /Users/ivanzamanov/SpeechSynthesis/db-eval-gergana.bin
