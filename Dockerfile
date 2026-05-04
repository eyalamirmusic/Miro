# Linux/GCC build environment for Miro. Use this with CLion's Docker
# toolchain to reproduce the Linux GCC CI lane locally and step through
# failures in gdb. The image installs only what's needed to configure,
# build, and debug Miro — sources are mounted in by CLion at runtime.
#
# CLion setup:
#   Settings > Build, Execution, Deployment > Toolchains > + > Docker
#     Image: miro-linux:latest  (build it once with the command below)
#   Settings > ... > CMake > + > add a profile bound to the Docker toolchain
#
# Standalone usage (mirrors what CLion does internally):
#   docker build -t miro-linux .
#   docker run --rm -it -v "$PWD":/workspace miro-linux
#   # then inside the container:
#   cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug \
#       -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ \
#       -DMIRO_UNITY_BUILD=OFF
#   cmake --build build
#   ctest --test-dir build --output-on-failure

FROM ubuntu:latest

ARG DEBIAN_FRONTEND=noninteractive

# build-essential pulls gcc/g++/make. ninja-build is the generator we use
# for fast incremental rebuilds. gdb is required for CLion's debugger.
# git + ca-certificates are needed because CMake FetchContent pulls
# NanoTest and nlohmann/json from GitHub at configure time. rsync shows
# up in some CLion sync paths, harmless to include. CMake comes from the
# upstream binary tarball — the apt cmake on the current Ubuntu LTS is
# 3.28, but the project requires 3.29+.
ARG CMAKE_VERSION=3.30.5

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        build-essential \
        gcc \
        g++ \
        gdb \
        ninja-build \
        git \
        ca-certificates \
        curl \
        rsync \
    && rm -rf /var/lib/apt/lists/* \
    && ARCH="$(uname -m)" \
    && curl -fsSL "https://github.com/Kitware/CMake/releases/download/v${CMAKE_VERSION}/cmake-${CMAKE_VERSION}-linux-${ARCH}.tar.gz" \
        | tar -xz --strip-components=1 -C /usr/local

WORKDIR /workspace

CMD ["/bin/bash"]
