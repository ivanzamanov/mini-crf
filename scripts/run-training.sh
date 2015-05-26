mkdir -p logs
LOG=logs/$(date +%m-%d-%H-%M).log
nodejs trainer.js | tee "$LOG"
