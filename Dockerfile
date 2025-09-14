FROM ubuntu:latest

RUN apt-get update && apt-get upgrade -y && apt-get install -y python3 python3-venv npm

COPY ./website /website
WORKDIR /website

RUN cd frontend && \
    npm install && \
    npm run build && \
    cd ..

RUN python3 -m venv .venv && \
    . .venv/bin/activate && \
    pip install -r requirements.txt
ENV PATH="/website/.venv/bin:$PATH"
