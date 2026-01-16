FROM debian:12

WORKDIR /home/rb
ENV HOME=/home/rb

# Install build dependencies
RUN apt update && \
    DEBIAN_FRONTEND=noninteractive apt install -y \
    build-essential \
    git \
    perl \
    curl \
    texinfo \
    flex \
    bison \
    bzip2 \
    gzip \
    zip \
    patch \
    automake \
    libtool \
    libtool-bin \
    autoconf \
    libmpc-dev \
    gawk \
    wget && \
    rm -rf /var/lib/apt/lists/*

# Clone rockbox and build ARM toolchain
RUN git clone git://git.rockbox.org/rockbox

RUN cd /home/rb/rockbox && \
    RBDEV_PREFIX=/opt/toolchain ./tools/rockboxdev.sh --target="a"

# Add toolchain to PATH
ENV PATH="/opt/toolchain/bin:$PATH"

WORKDIR /src
