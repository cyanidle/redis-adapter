# syntax=docker/dockerfile:1
ARG APP_DIR=/app
ARG APP_NAME=redis-adapter
ARG TARGET_DEVICE=rpi4
ARG JOBS=4

FROM rsk39.tech/qt6-env:${TARGET_DEVICE} as builder
ARG APP_NAME
ENV APP_NAME=${APP_NAME}
ARG APP_DIR
ARG TARGETPLATFORM
ENV TARGET_PLATFORM=${TARGETPLATFORM}
ARG JOBS
ENV JOBS=${JOBS}
WORKDIR /build
COPY . .
RUN /env/setup-cross
RUN set -eux; \
    qmake; \
    make -j${JOBS}
RUN set -eux; \
    mkdir -p ${APP_DIR}; \
    modules=$(cat .qtmodules); \
# using $QT_BIN_PATH, $SYSROOT_DIR and $TOOLCHAIN_PATH from rsk39/qt5-env image
    app_src_libs=$(ls | grep libradapter.*);\
    qt_libs=$(for module in $modules; do ls -d ${QT_BIN_PATH}/lib/* | grep -i $module*.so*; done); \
    os_specific="libicui18n libicuuc libicudata libmariadb"; \
    os_libs=$(for module in $os_specific; do ls -d ${SYSROOT_DIR}/usr/lib/${TOOLCHAIN_ARCH}/* | grep "$module.*\.so.*"; done); \
    plugins=$(ls plugins | grep .so | xargs printf "plugins/%s "); \
    mkdir ${APP_DIR}/plugins; \
    cp -dr $plugins ${APP_DIR}/plugins; \
    cp -dr \
        VERSION \
        GIT_COMMIT \
        $app_src_libs \
        $qt_libs \
        $os_libs \
        ${APP_NAME} \
    ${APP_DIR}; \
    cd ${APP_DIR}; \
    mkdir sqldrivers; \
    cp ${QT_BIN_PATH}/plugins/sqldrivers/libqsqlmysql.so sqldrivers/ 
WORKDIR ${APP_DIR}
CMD ["bash"]

FROM --platform=${TARGETPLATFORM} debian:bullseye-slim as runner
ARG APP_NAME
ARG APP_DIR
ENV APP_NAME=${APP_NAME}
ENV APP_DIR=${APP_DIR}
ARG TARGET_DEVICE
ENV TARGET_DEVICE=${TARGET_DEVICE}
ARG TARGETPLATFORM
ENV TARGET_PLATFORM=${TARGETPLATFORM}
COPY --from=builder ${APP_DIR}/ ${APP_DIR}/
COPY --from=builder /starter.sh ${APP_DIR}/${APP_NAME}.sh
COPY --from=builder /docker-entrypoint.sh /usr/local/bin/
COPY --from=builder /tini /usr/bin/tini
WORKDIR ${APP_DIR}
ENTRYPOINT ["/usr/bin/tini", "--", "/usr/local/bin/docker-entrypoint.sh"]