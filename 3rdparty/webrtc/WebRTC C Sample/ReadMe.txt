This is a WebRTC data stack built entirely in C code with a working sample. Just run it and open a browser to the URL to get a WebRTC session going between the browser and the C application. All code is released under Apache 2.0 license except for OpenSSL which comes under it's own license.

On Windows, load "WebRTC MicroStackSample.sln" in Visual Studio, compile and run.

On Linux type:

	make linux-32
	make linux-64
	make linux-arm

For "Linux-arm", you need to edit the path of the ARM compiler in the makefile.