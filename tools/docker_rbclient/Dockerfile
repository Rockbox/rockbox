FROM debian:9

WORKDIR /home/rb

ENV HOME /home/rb
ENV MAKEFLAGS -j12

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
    latex-xcolor \
    texlive-base \
    libsdl1.2-dev \
    libsdl1.2debian

RUN cd /home/rb && git clone git://git.rockbox.org/rockbox

RUN cd /home/rb/rockbox && ./tools/rockboxdev.sh --target="s"
RUN cd /home/rb/rockbox && ./tools/rockboxdev.sh --target="m"
RUN cd /home/rb/rockbox && ./tools/rockboxdev.sh --target="a"
RUN cd /home/rb/rockbox && ./tools/rockboxdev.sh --target="i"
RUN cd /home/rb/rockbox && ./tools/rockboxdev.sh --target="x"
RUN cd /home/rb/rockbox && ./tools/rockboxdev.sh --target="y"

# compile sometimes fails; place this last to avoid duplicate work
RUN cd /home/rb/rockbox && ./tools/rockboxdev.sh --target="r"

RUN cd /home/rb/rockbox && \
    wget "http://git.rockbox.org/?p=www.git;a=blob_plain;f=buildserver/rbclient.pl;hb=HEAD" -O rbclient.pl && \
    chmod +x rbclient.pl

COPY runclient_modified.sh /home/rb/rockbox/runclient.sh

RUN cd /home/rb/rockbox && chmod +x runclient.sh

CMD cd /home/rb/rockbox && ./runclient.sh $USER $PASS $NAME
