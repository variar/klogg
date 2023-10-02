FROM ubuntu:focal

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update -y && \
    apt-get install --no-install-recommends build-essential \
                    qtbase5-dev qt5-qmake qtbase5-dev-tools qttools5-dev qttranslations5-l10n\
                    libboost-dev libssl-dev libicu-dev libcurl4-openssl-dev \
                    ragel ninja-build zlib1g-dev git \
                    wget ca-certificates fuse -y && \
    apt-get clean && rm -rf /var/lib/apt/lists/*

RUN wget https://github.com/Kitware/CMake/releases/download/v3.20.2/cmake-3.20.2-linux-x86_64.sh && \
    chmod 755 cmake-3.20.2-linux-x86_64.sh && \
    ./cmake-3.20.2-linux-x86_64.sh --prefix=/opt/ --exclude-subdir --skip-license && \
    rm cmake-3.20.2-linux-x86_64.sh

ENV PATH=/opt/bin:$PATH
