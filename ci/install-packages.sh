#! /bin/bash

# For g++-9
sudo add-apt-repository ppa:ubuntu-toolchain-r/test -y

# We need a more recent meson too
sudo add-apt-repository ppa:jonathonf/meson -y

sudo apt-get update

sudo apt-get install meson build-essential git g++-9 libgtest-dev libgflags-dev pkg-config make clang-7.0 clang-tidy-7.0 -y
