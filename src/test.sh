INPUT="Diana.wav"

if [ -n $(grep wav <<< "$INPUT") ]; then
        echo Same
fi
