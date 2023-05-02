#!/usr/bin/env bash
set -eux
./scripts/generate_version.sh
full_ver=$(cat VERSION)
version=${full_ver:1:3}
docker buildx build --load --progress=plain \
    --build-arg TARGET_DEVICE=amd64 \
    --build-arg JOBS=16 \
    -t rsk39.tech/redis-adapter:broker.$version-amd64 .