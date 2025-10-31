#!/bin/bash

: <<'END'
    SCRIPT TO SETUP AND RUN NGINX LOCALLY, WITHOUT DOCKER
    ENSURE FRONTEND IS BUILT:
        $ cd frontend && npm run build
    AND BACKEND APP IS RUNNING:
        $ cd backend && FLASK_ENV=production gunicorn -k gevent run:app -b 127.0.0.1:8000

    THEN:
        $ sudo chmod +x nginx/local_setup.sh && sudo ./nginx/local_setup.sh
END


# install nginx
apt update
apt install nginx -y


# remove default site
rm /etc/nginx/sites-available/default
rm /etc/nginx/sites-enabled/default


# create site for web app
cat <<EOF > /etc/nginx/sites-available/battery-portal
# HTTP: redirect to HTTPS
server {
    listen 80;
    server_name localhost;
    return 301 https://\$host\$request_uri;
}

# HTTPS
server {
    listen 443 ssl;
    server_name localhost;

    ssl_certificate $PWD/backend/cert/local_cert.pem;
    ssl_certificate_key $PWD/backend/cert/local_key.pem;

    # serve React frontend build directly
    root $PWD/frontend/dist;
    index index.html;
    location = / { # built html entry file
        try_files /index.html =404;
    }
    location / { # all other built files (static)
        try_files \$uri =404;
    }

    # serve Flask app where all other endpoints begin with /api
    location /api/ {
        proxy_pass http://127.0.0.1:8000; # because running with 'gunicorn -k gevent run:app -b 127.0.0.1:8000'
        proxy_set_header Host \$host;
        proxy_set_header X-Real-IP \$remote_addr;
        proxy_set_header X-Forwarded-For \$proxy_add_x_forwarded_for;

        # WebSocket headers
        proxy_http_version 1.1;
        proxy_set_header Upgrade \$http_upgrade;
        proxy_set_header Connection "upgrade";
    }
}
EOF
ln -sf /etc/nginx/sites-available/battery-portal /etc/nginx/sites-enabled/


# set directory and file permissions so nginx (www-data) can read static files
ROOT="$PWD"
while [ "$ROOT" != "/" ]; do # allow traversal (execute) and read for all directories in the path
  chmod o+rx "$ROOT"
  ROOT=$(dirname "$ROOT")
done
ROOT="$PWD"
# then ensure all subdirs are traversable and files are readable
find "$ROOT" -type d -exec chmod o+rx {} +
find "$ROOT" -type f -exec chmod o+r {} +


# reload nginx
nginx -t # test config
systemctl reload nginx
