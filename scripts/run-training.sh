mkdir -p logs
LOG=logs/$(date +%M-%d-%H-%m).log
nodejs trainer.js | tee "$LOG"
