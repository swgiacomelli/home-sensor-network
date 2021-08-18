#!/bin/bash
set -eo pipefail

export GF_SECURITY_ADMIN_PASSWORD="$(cat /run/secrets/grafana_password)"

/run.sh "${@}"