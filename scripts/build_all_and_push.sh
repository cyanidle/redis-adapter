#!/usr/bin/env bash
set -eux
./scripts/build_amd64.sh
docker push rsk39.tech/redis-adapter:broker.1.0-amd64 &
./scripts/build_rpi4.sh
docker push rsk39.tech/redis-adapter:broker.1.0-rpi4