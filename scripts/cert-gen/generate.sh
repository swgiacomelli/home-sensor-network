#/bin/bash

CONFIG_SSL_H=../../src/esp8266_sensor/config_ssl.h
MQTT_SSL_H=../../src/esp8266_sensor/mqtt_ssl.h

docker build -t iot_cert_gen .  --no-cache
docker run --name iot_cert_gen iot_cert_gen

docker cp iot_cert_gen:/ca.pem ./ca.crt

docker cp iot_cert_gen:/iot_server-key.pem ./iot_server.key
docker cp iot_cert_gen:/iot_server.pem ./iot_server.crt

docker cp iot_cert_gen:/mqtt_server-key.pem ./mqtt_server.key
docker cp iot_cert_gen:/mqtt_server.pem ./mqtt_server.crt

docker cp iot_cert_gen:/mqtt_iot_client-key.pem ./mqtt_iot_client.key
docker cp iot_cert_gen:/mqtt_iot_client.pem ./mqtt_iot_client.crt

docker cp iot_cert_gen:/mqtt_telegraf_client-key.pem ./mqtt_telegraf_client.key
docker cp iot_cert_gen:/mqtt_telegraf_client.pem ./mqtt_telegraf_client.crt

echo '#pragma once' > $CONFIG_SSL_H
echo '#include <Arduino.h>' >> $CONFIG_SSL_H
echo '// use /scripts/cert-gen/generate.sh to generate unique keys' >> $CONFIG_SSL_H
echo 'namespace config {' >> $CONFIG_SSL_H

echo 'static const char server_cert[] PROGMEM = R"EOF(' >> $CONFIG_SSL_H
cat ./iot_server.crt >> $CONFIG_SSL_H
echo ')EOF";' >> $CONFIG_SSL_H

echo 'static const char server_key[] PROGMEM =  R"EOF(' >> $CONFIG_SSL_H
cat ./iot_server.key >> $CONFIG_SSL_H
echo ')EOF";' >> $CONFIG_SSL_H

echo '}' >> $CONFIG_SSL_H

echo '#pragma once' > $MQTT_SSL_H
echo '#include <Arduino.h>' >> $MQTT_SSL_H
echo '// use /scripts/cert-gen/generate.sh to generate unique keys' >> $MQTT_SSL_H
echo 'namespace mqtt {' >> $MQTT_SSL_H

echo 'static const char server_ca[] PROGMEM = R"EOF(' >> $MQTT_SSL_H
cat ./ca.crt >> $MQTT_SSL_H
echo ')EOF";' >> $MQTT_SSL_H

echo 'static const char client_cert[] PROGMEM = R"EOF(' >> $MQTT_SSL_H
cat ./mqtt_iot_client.crt >> $MQTT_SSL_H
echo ')EOF";' >> $MQTT_SSL_H

echo 'static const char client_key[] PROGMEM =  R"EOF(' >> $MQTT_SSL_H
cat ./mqtt_iot_client.key >> $MQTT_SSL_H
echo ')EOF";' >> $MQTT_SSL_H
echo '}' >> $MQTT_SSL_H

cp ./ca.crt  ../../docker-secure/mqtt_ca.crt
cp ./mqtt_server.key  ../../docker-secure/mqtt_server.key
cp ./mqtt_server.crt  ../../docker-secure/mqtt_server.crt
cp ./mqtt_telegraf_client.key  ../../docker-secure/mqtt_client.key
cp ./mqtt_telegraf_client.crt  ../../docker-secure/mqtt_client.crt

docker rm iot_cert_gen
docker rmi iot_cert_gen
