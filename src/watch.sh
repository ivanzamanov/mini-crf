#!/bin/bash
TARGET=$1
COMMAND="$2"

echo "Watching with target $TARGET"
while inotifywait -e modify include main.cpp test.cpp ; do
  make -j 4 -B $TARGET
  if [ $? == 0 -a -n "$COMMAND" ]; then
    bash -c "$COMMAND"
  fi
done
