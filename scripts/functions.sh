
SED=/usr/bin/sed
CYGPATH_CMD=/usr/bin/cygpath
GREP=/usr/bin/grep
function os_path {
  $CYGPATH_CMD -w "$1" | $SED 's/\\/\\\\\\/g'
}

function fix_synth_output {
  $SED -i 's/$/\r/g' $1
  for file in $($GREP -o '/.*wav' $1)
  do
    REPL="$(os_path "$file" | $SED 's/\\/\\\\/g')"
    $SED -i s\|"$file"\|"$REPL"\|g $1
  done
}
