FROM ubuntu:focal

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update -y && \
    apt-get install --no-install-recommends build-essential python3 python-is-python3 \
                    qtbase5-dev qt5-qmake qtbase5-dev-tools qttools5-dev qttranslations5-l10n\
                    libboost-dev libssl-dev libicu-dev libcurl4-openssl-dev \
                    ragel ninja-build zlib1g-dev git pkg-config \
                    wget ca-certificates fuse file -y && \
    apt-get clean && rm -rf /var/lib/apt/lists/*

# focal ships gcc 9, dependencies (vectorscan, sentry) need gcc >= 11
RUN apt-get update -y && \
    apt-get install --no-install-recommends software-properties-common gpg-agent -y && \
    add-apt-repository -y ppa:ubuntu-toolchain-r/test && \
    apt-get update -y && \
    apt-get install --no-install-recommends gcc-13 g++-13 -y && \
    update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-13 130 \
                        --slave /usr/bin/g++ g++ /usr/bin/g++-13 \
                        --slave /usr/bin/gcc-ar gcc-ar /usr/bin/gcc-ar-13 \
                        --slave /usr/bin/gcc-nm gcc-nm /usr/bin/gcc-nm-13 \
                        --slave /usr/bin/gcc-ranlib gcc-ranlib /usr/bin/gcc-ranlib-13 && \
    update-alternatives --install /usr/bin/cc cc /usr/bin/gcc 130 && \
    update-alternatives --install /usr/bin/c++ c++ /usr/bin/g++ 130 && \
    apt-get clean && rm -rf /var/lib/apt/lists/*

RUN wget https://github.com/Kitware/CMake/releases/download/v3.31.12/cmake-3.31.12-linux-x86_64.sh && \
    chmod 755 cmake-3.31.12-linux-x86_64.sh && \
    ./cmake-3.31.12-linux-x86_64.sh --prefix=/opt/ --exclude-subdir --skip-license && \
    rm cmake-3.31.12-linux-x86_64.sh

ENV PATH=/opt/bin:$PATH
