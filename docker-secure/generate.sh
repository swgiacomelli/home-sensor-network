#/bin/bash

docker build -t generator .  --no-cache
docker run --name generator generator

docker cp generator:/script/database.crt ./database.crt
docker cp generator:/script/database.key ./database.key

docker cp generator:/script/dashboard.crt ./dashboard.crt
docker cp generator:/script/dashboard.key ./dashboard.key

docker cp generator:/script/telegraf.conf ./telegraf.conf

docker cp generator:/script/influx_token.key ./influx_token.key
docker cp generator:/script/influx_password.key ./influx_password.key
docker cp generator:/script/grafana_password.key ./grafana_password.key

docker rm generator
docker rmi generator