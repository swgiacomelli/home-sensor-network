#/bin/bash

docker build -t config_server_test .  --no-cache
docker run --name config_server_test -p 8000:8000 -v $PWD/static:/static config_server_test

docker rm config_server_test
docker rmi config_server_test