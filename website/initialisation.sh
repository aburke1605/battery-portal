#!/bin/bash

cd frontend
npm install
npm run build
cd ..

python3 -m venv .venv
. .venv/bin/activate

pip install -r requirements.txt
