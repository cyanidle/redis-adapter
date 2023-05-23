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
RUN set -eux; \
    test -f ${APP_NAME}.pro || (echo "Project is not called ${APP_NAME}.pro" && exit 1); \
    test -f VERSION || (echo "VERSION file not found" && exit 1); \
    test -f GIT_COMMIT || (echo "GIT_COMMIT file not found" && exit 1); \
    echo "VERSION: $(cat VERSION)"; \
    echo "GIT_COMMIT: $(cat GIT_COMMIT)"; \
    ls -ll; \
    qmake; \
    ls -ll; \
    make -j${JOBS}; \
    mkdir -p ${APP_DIR}; \
# using $QT_DIR, $SYSROOT_DIR and $TOOLCHAIN_PATH from rsk39/qt6-env image
    app_src_libs=$(ls | grep -i radapter-sdk*.so*) || app_src_libs=;\
    modules="core serialbus serialport sql websockets network httpserver concurrent test"; \
    qt_libs=$(for module in $modules; do ls -d ${QT_DIR}/lib/* | grep -i $module*.so*; done); \
    os_specific="libicui18n libicuuc libicudata libmariadb libbrotlidec libb2 libbrotlicommon libgomp ssl crypto"; \
    os_libs=$(for module in $os_specific; do ls -d ${SYSROOT_DIR}/usr/lib/${TOOLCHAIN_ARCH}/* | grep "$module.*\.so.*"; done); \
    mkdir ${APP_DIR}/plugins; \
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
    cp ${QT_DIR}/plugins/sqldrivers/libqsqlmysql.so sqldrivers/ ; \
    cp -dr ${QT_DIR}/plugins/tls tls/
WORKDIR ${APP_DIR}
CMD ["bash"]

FROM --platform=${TARGETPLATFORM} debian:bullseye-slim as py_install
RUN apt update && apt install -y --no-install-recommends python3.9-dev python3-pip build-essential libffi-dev
COPY requirements.txt /requirements.txt
RUN pip3 install -r /requirements.txt
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
COPY --from=py_install /usr/lib/python3 /usr/lib/python3.9 /usr/lib/
COPY --from=py_install /usr/local/lib/python3.9 /usr/local/lib/python3.9
COPY --from=py_install /lib/*/libexpat* /lib/
COPY --from=py_install /usr/bin/python* /usr/bin/
COPY --from=builder ${APP_DIR}/ ${APP_DIR}/
COPY --from=builder /starter.sh ${APP_DIR}/${APP_NAME}.sh
COPY --from=builder /docker-entrypoint.sh /usr/local/bin/
COPY --from=builder /tini /usr/bin/tini
WORKDIR ${APP_DIR}
ENTRYPOINT ["/usr/bin/tini", "--", "/usr/local/bin/docker-entrypoint.sh"]