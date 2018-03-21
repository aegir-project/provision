#!/usr/bin/env bash
docker build -t provision4/base -f Dockerfile.base .
docker build -t provision4/http:php7 -f Dockerfile.http .
docker run --rm -ti -v /home/jon/.config/provision/server_mskcc:/var/provision/config/server_mskcc  -e SERVER_NAME=server_mskcc  provision4/http:php7 bash
