[![Build](https://github.com/HumbleNet/HumbleNet/actions/workflows/build.yml/badge.svg)](https://github.com/HumbleNet/HumbleNet/actions/workflows/build.yml)

HumbleNet
=========

HumbleNet is a cross platform networking library that utilizes WebRTC and WebSockets to handle network communication.

Using WebRTC and Websockets allows HumbleNet to support not only traditional platforms such as Windows, OS X, and Linux, but also web platforms such as ASM.JS / Emscripten.

Language Support
----------------
HumbleNet is a simple clean C based API that allows for wrappers to be written for any language that offers C binding.

Included Language Bindings
==========================
* C/C++
  - a simple C header is available for both C and C++ development.  There is also a BSD socket wrapper that will redirect the C socket API through humblenet.
* C#
  - wrappers are included that work in Unity for its Desktop and WebGL platform support. They should also work in any other .NET application.

Demo applications
=================
We have several demos to show off integrating HumbleNet.

TestPeer
--------
This is a simple "chat" app in the tests folder for both C and C#.   The C app will run on Windows, MacOS X, Linux, and Emscripten.   The C# will run on .NET or Mono.

Quake 2
-------
We have a port of Quake2 (Based on [R1Q2](http://www.r1ch.net/stuff/r1q2/)) that includes HumbleNet networking and runs on Windows, Mac OS X, Linux and Emscripten.

[Quake 2 HumbleNet port](https://github.com/HumbleNet/quake2)

Quake 3
-------
We have a port of Quake3 (Based on [QuakeJS](http://quakejs.com/)) that includes HumbleNet networking and runs on Windows, Mac OS X, Linux and Emscripten.

[Quake 3 HumbleNet port](https://github.com/HumbleNet/quake3)

Build Instructions
==================

Project Dependencies:
---------------------

1. CMake (https://cmake.org/):
    - Linux: use your preferred package manager, like `apt-get`.
    - Mac: can be installed using `brew`.
    - Win: download the installer via site https://cmake.org/download/.
2. Build system supported by CMake
    - Make (https://www.gnu.org/software/make/manual/make.html)
    - Visual Studio ( https://visualstudio.microsoft.com/ )
    - Ninja ( https://ninja-build.org/ )
    - XCode ( https://developer.apple.com/xcode/ )
3. FlatBuffers (https://google.github.io/flatbuffers/index.html)
    - This is included as a submodule now so ensure you run
      ```sh
      git submodule update --init
      ```
4. Go Language [here](https://golang.org/)
    - Download and install the compiler via [download page](https://golang.org/dl/).
    - Test on your terminal if you can execute `go version`;
    - Tested here with version `go1.9.1 darwin/amd64`.

Compilation:
------------

1. Clone the source code from github
   ```shell
   git clone https://github.com/HumbleNet/HumbleNet
   ```
2. Go into the checkout folder
3. Run the command: `cmake -S . -B build` this will configure into the directory all files needed to build the library;
4. From the `build` folder you can run the `cmake --build .` command alone, and it will try to build all code, or with the desired `target`, example:
    1. `cmake --build .`: will build all executables and libraries;
    2. `cmake --build . --target clean`: will clean the directory, but mantain the config files;
    3. `cmake --build . --target humblenet_test_peer`: build the test peer found in `<HumberNetDir>/tests/test_peer.cpp`;
    4. `cmake --build . --target peer-server`: will build the test server found in `<HumbleNetDir>/src/peer-server`;