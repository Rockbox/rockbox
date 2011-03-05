#!/bin/sh

CONFIG_DIR=.config/rockbox.org
mkdir --parents $CONFIG_DIR

# Copy over a default config file or set default language
# cp --no-clobber config.cfg.default $CONFIG_DIR/config.cfg

export HOME=$(pwd)
./rockbox/bin/rockbox
