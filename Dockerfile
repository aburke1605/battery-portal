# build web app pip environment
FROM python:3.12-slim AS backend-build
WORKDIR /battery-portal/website

RUN apt-get update && apt-get install -y build-essential && rm -rf /var/lib/apt/lists/*

COPY ./website/requirements.txt ./
RUN python -m venv .venv
ENV PATH="/battery-portal/website/.venv/bin:$PATH"
RUN pip install --no-cache-dir -r requirements.txt

COPY ./website/ ./


# default command, overridden in compose.yml
EXPOSE 8000
CMD ["flask", "db", "upgrade", "&&", "FLASK_ENV=production", "gunicorn", "-k", "gevent", "run:app", "-b", "0.0.0.0:8000"]



# build web app frontend
FROM node:lts AS frontend-build
WORKDIR /battery-portal/website/frontend

COPY ./website/frontend/package*.json ./
RUN npm install

COPY ./website/frontend/ ./
RUN npm run build



# nginx
FROM nginx:stable AS nginx-build

# copy built frontend; overwrites default site html file
COPY --from=frontend-build /battery-portal/website/frontend/dist /usr/share/nginx/html

# copy nginx config file from host machine as default site
COPY ./nginx/battery-portal.conf /etc/nginx/conf.d/default.conf
