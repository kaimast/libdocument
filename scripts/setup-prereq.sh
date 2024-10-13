#!/bin/bash -e

INSTALL_DIR=$HOME/local
BUILDTYPE=release

export LIBRARY_PATH=/usr/lib:/usr/local/lib:$INSTALL_DIR/lib:$INSTALL_DIR/lib/x86_64-linux-gnu
export LD_LIBRARY_PATH=/usr/lib:/usr/local/lib:$INSTALL_DIR/lib:$INSTALL_DIR/lib/x86_64-linux-gnu

echo "Building bitstream"
cd deps/bitstream
meson build
cd build
ninja install  -Dbuildtype=$BUILDTYPE --prefix=$INSTALL_DIR
