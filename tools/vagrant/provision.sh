#!/bin/bash -e

# Update the OS and install all required packages
apt-get update
apt-get dist-upgrade -y
apt-get install -y zip build-essential gcc-mingw-w64-i686 gcc-mingw-w64-x86-64 texinfo automake libtool-bin flex bison libncurses-dev
