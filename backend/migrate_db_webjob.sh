#!/bin/bash
set -e

# find env dynamically
VENV=$(find /tmp -maxdepth 2 -type d -name antenv | head -n 1)
source "$VENV/bin/activate"

cd $VENV
cd ../backend

export FLASK_APP=run:app
flask db upgrade
flask seed-admin
