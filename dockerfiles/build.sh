#!/usr/bin/env bash
docker build -t provision4/base -f Dockerfile.base .
docker build -t provision4/http:php7 -f Dockerfile.http.php7 .
