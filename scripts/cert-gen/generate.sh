#/bin/bash

docker build -t iot_cert_gen .  --no-cache
docker run --name iot_cert_gen iot_cert_gen

docker cp iot_cert_gen:/iot_server-key.pem ./iot_server.key
docker cp iot_cert_gen:/iot_server.pem ./iot_server.crt

docker rm iot_cert_gen
docker rmi iot_cert_gen