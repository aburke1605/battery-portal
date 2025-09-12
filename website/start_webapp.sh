#!/bin/bash

. .venv/bin/activate
which gunicorn

flask db upgrade

gunicorn -k gevent run:app
