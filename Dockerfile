FROM mhart/alpine-node:7.9.0

RUN \
  apk --update add --no-cache build-base python htop && \
  npm i -g yarn

WORKDIR /src

CMD sh
