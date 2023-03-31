#!/usr/bin/env sh
echo "Generating version and commit hash"
COMMIT=$(git rev-parse --short HEAD)
echo "GIT_COMMIT: $COMMIT" && echo $COMMIT > GIT_COMMIT
BUILD_NUMBER=$(git rev-list --count HEAD)
VERSION_TAG=$(git describe --tags --abbrev=0 --match v[0-9]* | cut -c 2-)
VERSION="$VERSION_TAG.$BUILD_NUMBER"
echo "VERSION: $VERSION" && echo $VERSION > VERSION
