FROM debian:9

WORKDIR /home/rb
ENV HOME /home/rb

# Install tools needed
RUN apt-get update && \
    DEBIAN_FRONTEND=noninteractive apt-get install -y \
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
    python \
    python-lzo \
    python-setuptools \ 
    mtd-utils \
    xorriso && \
    rm -rf /var/lib/apt/lists/*

# Clone rockbox repository
RUN cd /home/rb && \
    git clone http://gerrit.rockbox.org/p/rockbox

# Build cross toolchain (It takes quite long)
RUN cd /home/rb/rockbox && \
    ./tools/rockboxdev.sh --target=y

# Install tools for unpacking ubifs
RUN cd /home/rb && \
    git clone https://github.com/jrspruitt/ubi_reader.git && \
    cd /home/rb/ubi_reader && \
    python setup.py install

# Copy build script
RUN cp /home/rb/rockbox/tools/agptek_rocker/bootloader_install.sh /usr/local/bin && \
    chmod 755 /usr/local/bin/bootloader_install.sh
