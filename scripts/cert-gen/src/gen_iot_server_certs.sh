#!/bin/sh

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

cfssl gencert -initca ca-csr.json | cfssljson -bare ca

cfssl gencert \
    -ca=ca.pem \
    -ca-key=ca-key.pem \
    -config=ca-config.json \
    -hostname=iot_server \
    -profile=iot iot_server-csr.json |
    cfssljson -bare iot_server
