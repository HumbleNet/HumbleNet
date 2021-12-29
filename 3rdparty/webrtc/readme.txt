Simple C# WebRTC sample, part of the Meshcentral open source project v0.10
--------------------------------------------------------------------------

All code is released under Apache 2.0 license except for OpenSSL which comes under it's own license. Open license.txt for more details.
For more documentation, information or updates, goto http://meshcommander.com

--- For Windows, WebRTC C# sample ---

If you are just getting started go in "webrtc/C# Sample" folder and compile and run "WebRTC Sample.exe". You then hit the "Open Browser" button. This will open a browser that will connect back to the C# sample. We included a tiny HTTP server to serve the sample web page and exchange the WebRTC offer and answer. Once open, the page will automatically start the WebRTC session and you get a chat window on both sides you can use to send and receive data.

The WebRTC stack itself is built in C and compiled into WebRTC.dll. The file is large because we statically link OpenSSL which provides the WebRTC security and dTLS support. We then have "WebRTC.cs", the code that does the C-to-C# bindings.

To use WebRTC in your own applications, all you need to do is include the WebRTC.cs file in your project, add the line "using OpenSource.WebRTC;" to your code and start using WebRTC C# classes.

-- For Linux and Windows, WebRTC C sample ---

The WebRTC data stack built entirely in C code with a working C sample that will compile on both Windows and Linux. Compile it, run it and open a browser to the specified URL to get a WebRTC session going between the browser and the C application.

On Windows, load "WebRTC MicroStackSample.sln" in Visual Studio, compile and run.

On Linux type the following to compile:

	make linux-32
	make linux-64
	make linux-arm

For "linux-arm", you need to edit the path of the ARM compiler in the makefile.


