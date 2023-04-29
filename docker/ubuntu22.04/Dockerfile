FROM ubuntu:jammy

ENV DEBIAN_FRONTEND=noninteractive

RUN apt update -y

RUN apt install build-essential \
                    qtbase5-dev qt5-qmake qtbase5-dev-tools qttools5-dev \
                    libboost-dev libssl-dev \
                    ragel ninja-build zlib1g-dev git \
                    wget fuse file -y && \
    apt clean && rm -rf /var/lib/apt/lists/*

RUN wget https://github.com/Kitware/CMake/releases/download/v3.20.2/cmake-3.20.2-linux-x86_64.sh && \
    chmod 755 cmake-3.20.2-linux-x86_64.sh && \
    ./cmake-3.20.2-linux-x86_64.sh --prefix=/opt/ --exclude-subdir --skip-license

ENV PATH=/opt/bin:$PATH
