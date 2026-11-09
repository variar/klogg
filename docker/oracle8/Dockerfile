FROM oraclelinux:8

COPY epel.repo /etc/yum.repos.d

RUN yum update -y && \
    yum install -y ragel make wget python38 rpm-build git \
    qt5-qtbase-devel qt5-linguist qt5-qttranslations \
    openssl-devel boost-devel zlib-devel libcurl-devel \
    gcc gcc-c++ elfutils-devel \
    gcc-toolset-13-gcc gcc-toolset-13-gcc-c++ gcc-toolset-13-libatomic-devel unzip && \
    yum clean all

RUN wget https://github.com/Kitware/CMake/releases/download/v3.31.12/cmake-3.31.12-linux-x86_64.sh && \
    chmod 755 cmake-3.31.12-linux-x86_64.sh && \
    ./cmake-3.31.12-linux-x86_64.sh --prefix=/opt/ --exclude-subdir --skip-license && \
    rm cmake-3.31.12-linux-x86_64.sh

RUN wget https://github.com/ninja-build/ninja/releases/download/v1.10.2/ninja-linux.zip && \
    unzip ninja-linux.zip && chmod 755 ninja && mv ninja /opt/bin && rm ninja-linux.zip

# system gcc 8 is too old for current dependencies (sentry/crashpad), use gcc-toolset-13
ENV PATH=/opt/bin:/opt/rh/gcc-toolset-13/root/usr/bin:$PATH
ENV LD_LIBRARY_PATH=/opt/rh/gcc-toolset-13/root/usr/lib64:/opt/rh/gcc-toolset-13/root/usr/lib
ENV CC=/opt/rh/gcc-toolset-13/root/usr/bin/gcc
ENV CXX=/opt/rh/gcc-toolset-13/root/usr/bin/g++
