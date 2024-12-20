#!/bin/bash -e

INSTALL_DIR=$HOME/local
BUILDTYPE=release

export LIBRARY_PATH=/usr/lib:/usr/local/lib:$INSTALL_DIR/lib:$INSTALL_DIR/lib/x86_64-linux-gnu
export LD_LIBRARY_PATH=/usr/lib:/usr/local/lib:$INSTALL_DIR/lib:$INSTALL_DIR/lib/x86_64-linux-gnu

pushd deps
echo "Building bitstream"
pushd bitstream
meson setup build -Dbuildtype=$BUILDTYPE --prefix=$INSTALL_DIR
cd build
ninja install -v
