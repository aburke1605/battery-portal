# build web app image
FROM python:3.12-slim AS backend-build
WORKDIR /battery-portal/website

RUN apt-get update && apt-get install -y build-essential && rm -rf /var/lib/apt/lists/*

COPY ./website/requirements.txt ./
RUN python -m venv .venv
ENV PATH="/battery-portal/website/.venv/bin:$PATH"
RUN pip install --no-cache-dir -r requirements.txt

COPY ./website/app/ ./app/
COPY ./website/utils.py ./
COPY ./website/run.py ./
COPY ./website/cert/ ./cert/
COPY ./website/migrations/ ./migrations/



# build frontend image
FROM node:lts AS frontend-build
WORKDIR /battery-portal/website/frontend

COPY ./website/frontend/package*.json ./
RUN npm install

# COPY ./website/frontend/ ./
COPY ./website/frontend/node_modules/ ./node_modules/
COPY ./website/frontend/src/ ./src/
COPY ./website/frontend/public/ ./public/
COPY ./website/frontend/index.html ./
COPY ./website/frontend/esp32.html ./
COPY ./website/frontend/eslint.config.js ./
COPY ./website/frontend/postcss.config.js ./
COPY ./website/frontend/tsconfig* ./
COPY ./website/frontend/vite.config.ts ./
RUN npm run build



# build nginx image
FROM nginx:stable AS nginx-build

# copy built frontend; overwrites default site html file
COPY --from=frontend-build /battery-portal/website/frontend/dist /usr/share/nginx/html

# copy nginx config file from host machine as default site
COPY ./nginx/battery-portal.conf /etc/nginx/conf.d/default.conf

# copy ssl certificates from host machine too
COPY ./website/cert /etc/nginx/certs
