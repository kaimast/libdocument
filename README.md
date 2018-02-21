# libdocument

Libdocument is a "super fast" C++17 json library inspired by MongoDB's BSON. All documents are stored in a binary format.
The library comes with support for creating modifing and traversing the documents quickly.

Contributions are very welcome!

## Building
You need to install the following dependencies to build libdocument:
* Meson + Ninja
* GCC
* Google Testing
* [bitstream](https://github.com/kaimast/bitstream)

Then you can just build and install the library using Meson.
```
meson build
cd build
ninja
sudo ninja install
```
