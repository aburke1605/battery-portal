#!/bin/bash

: <<'END'
    ENSURE APP IS RUNNING WITH:
    `$ FLASK_ENV=production gunicorn -k gevent run:app -b 127.0.0.1:8000
END


# install nginx
sudo apt update
sudo apt install nginx -y


# remove default site
sudo rm /etc/nginx/sites-available/default
sudo rm /etc/nginx/sites-enabled/default


# create site for web app
: <<'END'
    CREATE THIS FILE:
    ```/etc/nginx/sites-available/batteryportal
    server {
        listen 80;
        server_name localhost;

        # serve React frontend build directly
        location / {
            root /home/aburke/github/aburke1605/battery-portal/website/frontend/dist;
            index home.html;
            try_files $uri /index.html;
        }

        # serve Flask app where all other endpoints begin with /api
        location /api/ {
            proxy_pass http://127.0.0.1:8000; # because running with `gunicorn -k gevent run:app -b 127.0.0.1:8000`
            proxy_set_header Host $host;
            proxy_set_header X-Real-IP $remote_addr;
            proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;

            # WebSocket headers
            proxy_http_version 1.1;
            proxy_set_header Upgrade $http_upgrade;
            proxy_set_header Connection "upgrade";
        }
    }
    ```
END
sudo ln -s /etc/nginx/sites-available/batteryportal /etc/nginx/sites-enabled/


# set directory and file permissions so nginx (www-data) can read static files
ROOT="$PWD"
while [ "$ROOT" != "/" ]; do # allow traversal (execute) and read for all directories in the path
  sudo chmod o+rx "$ROOT"
  ROOT=$(dirname "$ROOT")
done
ROOT="$PWD"
# then ensure all subdirs are traversable and files are readable
sudo find "$ROOT" -type d -exec chmod o+rx {} +
sudo find "$ROOT" -type f -exec chmod o+r {} +


# reload nginx
sudo nginx -t # test config
sudo systemctl reload nginx