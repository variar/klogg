FROM ubuntu:bionic

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update -y && \
    apt-get install --no-install-recommends software-properties-common -y && \
    add-apt-repository ppa:beineri/opt-qt-5.15.2-bionic -y && \
    apt-get update -y && \
    apt-get install --no-install-recommends build-essential \
                    qt515base qt515svg qt515tools qt515imageformats qt515translations \
                    mesa-common-dev libglu1-mesa-dev \
                    libboost-dev libicu-dev libssl-dev libcurl4-openssl-dev \
                    ragel ninja-build zlib1g-dev git \
                    wget fuse -y && \
    apt-get clean && rm -rf /var/lib/apt/lists/*

RUN wget https://github.com/Kitware/CMake/releases/download/v3.20.2/cmake-3.20.2-linux-x86_64.sh && \
    chmod 755 cmake-3.20.2-linux-x86_64.sh && \
    ./cmake-3.20.2-linux-x86_64.sh --prefix=/opt/ --exclude-subdir --skip-license && \
    rm cmake-3.20.2-linux-x86_64.sh

ENV PATH=/opt/bin:/opt/qt515/bin:$PATH
