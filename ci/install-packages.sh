#! /bin/bash

# For g++-9 #TODO remove once we update to bionic
sudo add-apt-repository ppa:ubuntu-toolchain-r/test -y

# We need a more recent meson too
sudo add-apt-repository ppa:jonathonf/meson -y

sudo apt-get update

# TODO move to newer clang once we update to bionic
sudo apt-get install meson build-essential git g++-9 libgtest-dev libgflags-dev pkg-config make clang-tidy-6.0 -y
