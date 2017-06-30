HumbleNet
---------

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
-----------------
We have several demos to show off integrating HumbleNet.

TestPeer
========
This is a simple "chat" app in the tests folder for both Cand C#.   The C app will run on Windows, MacOS X, Linux, and Emscripten.   The C# will run on .NET or Mono.

Quake 2
=======
We have a port of Quake2 (Based on [R1Q2](http://www.r1ch.net/stuff/r1q2/)) that includes HumbleNet networking and runs on Windows, Mac OS X, Linux and Emscripten.

[Quake 2 HumbleNet port](https://github.com/HumbleNet/quake2)

Quake 3
=======
We have a port of Quake3 (Based on [QuakeJS](http://quakejs.com/)) that includes HumbleNet networking and runs on Windows, Mac OS X, Linux and Emscripten.

[Quake 3 HumbleNet port](https://github.com/HumbleNet/quake3)
