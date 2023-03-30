# syntax=docker/dockerfile:1
ARG APP_DIR=/app
ARG APP_NAME=redis-adapter
ARG TARGET_DEVICE=rpi4
ARG TEST_RUNNER=runtests.sh
ARG TEST_SRC_DIR=tests
ARG TEST_BIN_DIR=tests-bin
ARG JOBS=4

FROM rsk39/qt5-env:${TARGET_DEVICE} as builder
ARG APP_NAME
ENV APP_NAME=${APP_NAME}
#
ARG APP_DIR
ARG TEST_SRC_DIR
ARG TEST_RUNNER
#
ARG TARGETPLATFORM
ENV TARGET_PLATFORM=${TARGETPLATFORM}
#
ARG JOBS
ENV JOBS=${JOBS}
WORKDIR /build
COPY . .
RUN set -eux; \
    qmake; \
    make -j${JOBS}; \
    mkdir -p ${APP_DIR}/${APP_NAME}; \
    modules=$(cat .qtmodules); \
# using $QT_BIN_PATH, $SYSROOT_DIR and $TOOLCHAIN_PATH from rsk39/qt5-env image
    app_src_libs=$(ls | grep libradapter.*so);\
    qt_libs=$(for module in $modules; do ls -d ${QT_BIN_PATH}/lib/* | grep -i $module*.so*; done); \
    os_specific="libicui18n libicuuc libicudata libmariadb"; \
    os_libs=$(for module in $os_specific; do ls -d ${SYSROOT_DIR}/usr/lib/${TOOLCHAIN_ARCH}/* | grep "$module.*\.so.*"; done); \
    cp -dr \
        conf/ \
        VERSION \
        GIT_COMMIT \
        $app_src_libs \
        $qt_libs \
        $os_libs \
        ${APP_NAME} \
    ${APP_DIR}/${APP_NAME}; \
    cd ${APP_DIR}/${APP_NAME}; \
    mkdir sqldrivers; \
    cp ${QT_BIN_PATH}/plugins/sqldrivers/libqsqlmysql.so sqldrivers/ ; \
    ###  Qt5Testlib.so copy
    mkdir -p /${TEST_SRC_DIR}; \
    cp -dr $(ls -d ${QT_BIN_PATH}/lib/* | grep -i test*.so*) /${TEST_SRC_DIR}; \
    cd /build; \
    chmod +x ${TEST_RUNNER};
WORKDIR ${APP_DIR}
CMD ["bash"]

FROM --platform=${TARGETPLATFORM} debian:bullseye-slim as tester
ARG APP_NAME
ARG APP_DIR
ENV APP_DIR=${APP_DIR}
#
ARG TEST_SRC_DIR
ARG TEST_BIN_DIR
#
ARG TEST_RUNNER
ENV TEST_RUNNER=${TEST_RUNNER}
#
ARG TARGET_DEVICE
ENV TARGET_DEVICE=${TARGET_DEVICE}
#
ARG TARGETPLATFORM
ENV TARGET_PLATFORM=${TARGETPLATFORM}
#
COPY --from=builder ${APP_DIR}/ ${APP_DIR}/ 
COPY --from=builder /build/${TEST_SRC_DIR}/${TEST_BIN_DIR} ${APP_DIR}/${APP_NAME}/${TEST_BIN_DIR}
COPY --from=builder /build/${TEST_RUNNER} ${APP_DIR}/${APP_NAME}
COPY --from=builder /${TEST_SRC_DIR}/*Test.so* ${APP_DIR}/${APP_NAME}
#
WORKDIR ${APP_DIR}/${APP_NAME}
ENV SHARED_LIBS_DIR=${APP_DIR}/${APP_NAME}
CMD ./$TEST_RUNNER

FROM --platform=${TARGETPLATFORM} debian:bullseye-slim as runner
ARG APP_NAME
ARG APP_DIR
ENV APP_NAME=${APP_NAME}
ENV APP_DIR=${APP_DIR}/${APP_NAME}
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
