#!/bin/bash
set -e

# find env dynamically
VENV=$(find /tmp -maxdepth 2 -type d -name antenv | head -n 1)
source "$VENV/bin/activate"

cd $VENV/../backend
export PYTHONPATH=$PWD:$PYTHONPATH

cd -
python train_model_webjob.py
