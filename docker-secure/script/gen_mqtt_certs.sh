#!/bin/bash

CFSSL_VERSION="1.6.0"
CFFSL_URL="https://github.com/cloudflare/cfssl/releases/download/v${CFSSL_VERSION}/cfssl_${CFSSL_VERSION}_linux_amd64"
CFFSL_BIN="cfssl"

CFFSL_JSON_URL="https://github.com/cloudflare/cfssl/releases/download/v${CFSSL_VERSION}/cfssljson_${CFSSL_VERSION}_linux_amd64"
CFFSL_JSON_BIN="cfssljson"

curl $CFFSL_URL -o $CFFSL_BIN -L
curl $CFFSL_JSON_URL -o $CFFSL_JSON_BIN -L

chmod +x $CFFSL_BIN
chmod +x $CFFSL_JSON_BIN

mv ./$CFFSL_BIN /usr/local/bin/$CFFSL_BIN
mv ./$CFFSL_JSON_BIN /usr/local/bin/$CFFSL_JSON_BIN

cd /script

cfssl gencert -initca ca-csr.json | cfssljson -bare mqtt_ca

cfssl gencert \
    -ca=mqtt_ca.pem \
    -ca-key=mqtt_ca-key.pem \
    -config=ca-config.json \
    -hostname=127.0.0.1,mqtt_server \
    -profile=mqtt mqtt_server-csr.json |
    cfssljson -bare mqtt_server

cfssl gencert \
    -ca=mqtt_ca.pem \
    -ca-key=mqtt_ca-key.pem \
    -config=ca-config.json \
    -hostname=127.0.0.1,mqtt_client \
    -profile=mqtt mqtt_client-csr.json |
    cfssljson -bare mqtt_client
