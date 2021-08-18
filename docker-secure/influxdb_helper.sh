#!/bin/bash
set -eo pipefail

export DOCKER_INFLUXDB_INIT_ADMIN_TOKEN=$(cat /run/secrets/influx_token)
export DOCKER_INFLUXDB_INIT_PASSWORD=$(cat /run/secrets/influx_password)

/entrypoint.sh "${@}"