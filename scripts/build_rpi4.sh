#!/usr/bin/env bash
set -eux
./scripts/generate_version.sh
full_ver=$(cat VERSION)
version=${full_ver:0:3}
docker buildx build --load --progress=plain \
    --platform linux/arm/v7 \
    --build-arg TARGET_DEVICE=rpi4 \
    --build-arg JOBS=16 \
    -t rsk39.tech/redis-adapter:broker.$version-rpi4 .