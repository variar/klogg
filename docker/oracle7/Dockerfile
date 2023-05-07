FROM oraclelinux:7

COPY epel.repo /etc/yum.repos.d

RUN yum update -y

RUN yum install -y ragel make wget rpm-build git \
    qt5-qtbase-devel openssl-devel boost-devel zlib-devel \
    devtoolset-7-gcc devtoolset-7-gcc-c++ ninja-build

RUN wget https://github.com/Kitware/CMake/releases/download/v3.20.2/cmake-3.20.2-linux-x86_64.sh && \
    chmod 755 cmake-3.20.2-linux-x86_64.sh && \
    ./cmake-3.20.2-linux-x86_64.sh --prefix=/opt/ --exclude-subdir --skip-license

ENV PATH=/opt/rh/devtoolset-7/root/usr/bin:/opt/bin:$PATH

