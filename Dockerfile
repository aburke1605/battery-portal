# build web app image
FROM python:3.12-slim AS backend-build
WORKDIR /battery-portal/backend

RUN apt-get update && apt-get install -y build-essential && rm -rf /var/lib/apt/lists/*

COPY ./backend/requirements.txt ./
RUN python -m venv .venv
ENV PATH="/battery-portal/backend/.venv/bin:$PATH"
RUN pip install --no-cache-dir -r requirements.txt

COPY ./backend/app/ ./app/
COPY ./backend/utils.py ./
COPY ./backend/run.py ./
COPY ./backend/cert/ ./cert/
COPY ./backend/migrations/ ./migrations/



# build frontend image
FROM node:lts AS frontend-build
WORKDIR /battery-portal/frontend

COPY ./frontend/package*.json ./
RUN npm install

COPY ./frontend/src/ ./src/
COPY ./frontend/public/ ./public/
COPY ./frontend/index.html ./
COPY ./frontend/esp32.html ./
COPY ./frontend/eslint.config.js ./
COPY ./frontend/postcss.config.js ./
COPY ./frontend/tsconfig* ./
COPY ./frontend/vite.config.ts ./
RUN npm run build



# build nginx image
FROM nginx:stable AS nginx-build

# copy built frontend; overwrites default site html file
COPY --from=frontend-build /battery-portal/frontend/dist /usr/share/nginx/html

# copy nginx config file from host machine as default site
COPY ./nginx/battery-portal.conf /etc/nginx/conf.d/default.conf

# copy ssl certificates from host machine too
COPY ./backend/cert /etc/nginx/certs
