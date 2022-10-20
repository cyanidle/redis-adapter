rm GIT_COMMIT || echo "Generating commit hash"
COMMIT=$(git rev-parse --short HEAD)
echo $COMMIT > GIT_COMMIT
CURRENT_VERSION=$(head -1 VERSION)
echo "COMMIT=$COMMIT" >> build_env_file
echo "CURRENT_VERSION=$CURRENT_VERSION" >> build_env_file