from secrets import token_bytes, token_hex
from base64 import b64encode
from OpenSSL import crypto
from datetime import date, datetime
from subprocess import run

def generate_influxdb_token():
    token = token_bytes(64)
    return b64encode(token).decode('utf-8')


def generate_password():
    return token_hex(32)

def cert_gen(common_name: str,
             validityEndInSeconds: int = 10 * 365 * 24 * 60 * 60,
             ca_key=None,
             ca_cert=None):
    k = crypto.PKey()
    k.generate_key(crypto.TYPE_RSA, 4096)
    cert = crypto.X509()
    cert.get_subject().C = 'CA'
    cert.get_subject().ST = 'QC'
    cert.get_subject().L = 'Montreal'
    cert.get_subject().O = 'IOT'
    cert.get_subject().OU = 'IOT-' + common_name.upper()
    cert.get_subject().CN = common_name
    cert.add_extensions([
        crypto.X509Extension(
            b'subjectAltName', False,
            b'DNS:localhost,DNS:' + common_name.encode('utf-8'))
    ])
    cert.set_serial_number(int(datetime.now().timestamp()))
    cert.gmtime_adj_notBefore(0)
    cert.gmtime_adj_notAfter(validityEndInSeconds)
    cert.set_pubkey(k)

    if ca_key is None:
        cert.set_issuer(cert.get_subject())
        cert.sign(k, 'sha512')
    else:
        cert.set_issuer(ca_cert.get_subject())
        cert.sign(ca_key, 'sha512')

    with open(f"/script/{common_name}.crt", "wt") as f:
        f.write(
            crypto.dump_certificate(crypto.FILETYPE_PEM, cert).decode("utf-8"))
    with open(f"/script/{common_name}.key", "wt") as f:
        f.write(crypto.dump_privatekey(crypto.FILETYPE_PEM, k).decode("utf-8"))

    return k, cert


cert_gen('database')
cert_gen('dashboard')

influx_token = generate_influxdb_token()

with open("/script/influx_token.key", "wt") as f:
    f.write(influx_token)

with open("/script/telegraf.conf.tmp", "rt") as f:
    telegraf_temp = f.read().replace("$$INFLUX_TOKEN$$", influx_token)

with open("/script/telegraf.conf", "wt") as f:
    f.write(telegraf_temp)

with open("/script/influx_password.key", "wt") as f:
    f.write(generate_password())

with open("/script/grafana_password.key", "wt") as f:
    f.write(generate_password())

run(["/bin/bash", "/script/gen_mqtt_certs.sh"])