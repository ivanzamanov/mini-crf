
function os_path {
  cygpath -w "$1"
}

function fix_synth_output {
  sed -i 's/$/\r/g' $1
  for file in $(grep -o '/.*wav' $1)
  do
    REPL="$(os_path "$file" | sed 's/\\/\\\\/g')"
    sed -i s\|"$file"\|"$REPL"\|g $1
  done
}