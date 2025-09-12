FROM ubuntu:latest

RUN apt-get update && apt-get upgrade -y && apt-get install -y python3 python3-venv npm

COPY ./website /website
WORKDIR "/website"

RUN chmod +x initialisation.sh start_webapp.sh
RUN ./initialisation.sh

CMD ["./start_webapp.sh"]