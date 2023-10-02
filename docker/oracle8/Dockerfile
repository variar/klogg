FROM oraclelinux:8

COPY epel.repo /etc/yum.repos.d

RUN yum update -y && \
    yum install -y ragel make wget python38 rpm-build git \
    qt5-qtbase-devel qt5-linguist qt5-qttranslations \
    openssl-devel boost-devel zlib-devel libcurl-devel \
    gcc gcc-c++ elfutils-devel && \
    yum clean all

RUN wget https://github.com/Kitware/CMake/releases/download/v3.20.2/cmake-3.20.2-linux-x86_64.sh && \
    chmod 755 cmake-3.20.2-linux-x86_64.sh && \
    ./cmake-3.20.2-linux-x86_64.sh --prefix=/opt/ --exclude-subdir --skip-license && \
    rm cmake-3.20.2-linux-x86_64.sh

RUN wget https://github.com/ninja-build/ninja/releases/download/v1.10.2/ninja-linux.zip && \
    unzip ninja-linux.zip && chmod 755 ninja && mv ninja /opt/bin && rm ninja-linux.zip

ENV PATH=/opt/bin:$PATH

