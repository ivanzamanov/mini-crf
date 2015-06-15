#!/bin/bash
while [ -n "$(pgrep -f trainer.js)" ]; do
  echo Pid: "$(pgrep -f trainer.js)"
  sleep 60
done
poweroff
