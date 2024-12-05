FROM debian:12

WORKDIR /home/rb

ENV HOME /home/rb
ENV MAKEFLAGS -j$(nproc)

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
    gettext \
    gtk-doc-tools \
    gzip \
    zip \
    patch \
    automake \
    libtool \
    libtool-bin \
    autoconf \
    libmpc-dev \
    gawk \
    python3 \
    python3-lzo \
    python3-setuptools \
    mtd-utils \
    xorriso \
    wget \
    subversion \
    libncurses5-dev \
    texlive-latex-base \
    texlive-binaries \
    texlive-latex-extra \
    tex4ht \
    texlive-fonts-recommended \
    lmodern \
    texlive-base \
    libsdl1.2-dev \
    libsdl1.2debian \
    libsdl2-dev

RUN cd /home/rb && git clone git://git.rockbox.org/rockbox

RUN cd /home/rb/rockbox && ./tools/rockboxdev.sh --target="m a i x y"

ENV PATH="/tmp/local/bin:$PATH"

RUN cd /home/rb/rockbox && \
    wget "https://git.rockbox.org/cgit/www.git/plain/buildserver/rbclient.pl" -O rbclient.pl && \
    chmod +x rbclient.pl

RUN cd /home/rb/rockbox && \
    wget "https://git.rockbox.org/cgit/www.git/plain/buildserver/runclient.sh" -O runclient.sh && \
    chmod +x runclient.sh

CMD cd /home/rb/rockbox && ./runclient.sh $USER $PASS $NAME
