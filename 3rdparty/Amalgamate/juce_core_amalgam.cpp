/*
  ==============================================================================

   This file is part of the JUCE library - "Jules' Utility Class Extensions"
   Copyright 2004-11 by Raw Material Software Ltd.

  ------------------------------------------------------------------------------

   JUCE can be redistributed and/or modified under the terms of the GNU General
   Public License (Version 2), as published by the Free Software Foundation.
   A copy of the license is included in the JUCE distribution, or can be found
   online at www.gnu.org/licenses.

   JUCE is distributed in the hope that it will be useful, but WITHOUT ANY
   WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
   A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

  ------------------------------------------------------------------------------

   To release a closed-source product which uses JUCE, commercial licenses are
   available: visit www.rawmaterialsoftware.com/juce for more information.

  ==============================================================================
*/

#define JUCE_AMALGAMATED_INCLUDE 1

/*** Start of inlined file: juce_core.cpp ***/
#if defined (__JUCE_CORE_JUCEHEADER__) && ! JUCE_AMALGAMATED_INCLUDE
 /* When you add this cpp file to your project, you mustn't include it in a file where you've
	already included any other headers - just put it inside a file on its own, possibly with your config
	flags preceding it, but don't include anything else. That also includes avoiding any automatic prefix
	header files that the compiler may be using.
 */
 #error "Incorrect use of JUCE cpp file"
#endif

// Your project must contain an AppConfig.h file with your project-specific settings in it,
// and your header search path must make it accessible to the module's files.
#include "AppConfig.h"


/*** Start of inlined file: juce_BasicNativeHeaders.h ***/
#ifndef __JUCE_BASICNATIVEHEADERS_JUCEHEADER__
#define __JUCE_BASICNATIVEHEADERS_JUCEHEADER__


/*** Start of inlined file: juce_TargetPlatform.h ***/
#ifndef __JUCE_TARGETPLATFORM_JUCEHEADER__
#define __JUCE_TARGETPLATFORM_JUCEHEADER__

/*  This file figures out which platform is being built, and defines some macros
	that the rest of the code can use for OS-specific compilation.

	Macros that will be set here are:

	- One of JUCE_WINDOWS, JUCE_MAC JUCE_LINUX, JUCE_IOS, JUCE_ANDROID, etc.
	- Either JUCE_32BIT or JUCE_64BIT, depending on the architecture.
	- Either JUCE_LITTLE_ENDIAN or JUCE_BIG_ENDIAN.
	- Either JUCE_INTEL or JUCE_PPC
	- Either JUCE_GCC or JUCE_MSVC
*/

#if (defined (_WIN32) || defined (_WIN64))
  #define       JUCE_WIN32 1
  #define       JUCE_WINDOWS 1
#elif defined (JUCE_ANDROID)
  #undef        JUCE_ANDROID
  #define       JUCE_ANDROID 1
#elif defined (LINUX) || defined (__linux__)
  #define     JUCE_LINUX 1
#elif defined (__APPLE_CPP__) || defined(__APPLE_CC__)
  #define Point CarbonDummyPointName // (workaround to avoid definition of "Point" by old Carbon headers)
  #define Component CarbonDummyCompName
  #include <CoreFoundation/CoreFoundation.h> // (needed to find out what platform we're using)
  #undef Point
  #undef Component

  #if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
	#define     JUCE_IPHONE 1
	#define     JUCE_IOS 1
  #else
	#define     JUCE_MAC 1
  #endif
#else
  #error "Unknown platform!"
#endif

#if JUCE_WINDOWS
  #ifdef _MSC_VER
	#ifdef _WIN64
	  #define JUCE_64BIT 1
	#else
	  #define JUCE_32BIT 1
	#endif
  #endif

  #ifdef _DEBUG
	#define JUCE_DEBUG 1
  #endif

  #ifdef __MINGW32__
	#define JUCE_MINGW 1
  #endif

  /** If defined, this indicates that the processor is little-endian. */
  #define JUCE_LITTLE_ENDIAN 1

  #define JUCE_INTEL 1
#endif

#if JUCE_MAC || JUCE_IOS

  #if defined (DEBUG) || defined (_DEBUG) || ! (defined (NDEBUG) || defined (_NDEBUG))
	#define JUCE_DEBUG 1
  #endif

  #if ! (defined (DEBUG) || defined (_DEBUG) || defined (NDEBUG) || defined (_NDEBUG))
	#warning "Neither NDEBUG or DEBUG has been defined - you should set one of these to make it clear whether this is a release build,"
  #endif

  #ifdef __LITTLE_ENDIAN__
	#define JUCE_LITTLE_ENDIAN 1
  #else
	#define JUCE_BIG_ENDIAN 1
  #endif
#endif

#if JUCE_MAC

  #if defined (__ppc__) || defined (__ppc64__)
	#define JUCE_PPC 1
  #else
	#define JUCE_INTEL 1
  #endif

  #ifdef __LP64__
	#define JUCE_64BIT 1
  #else
	#define JUCE_32BIT 1
  #endif

  #if MAC_OS_X_VERSION_MIN_REQUIRED < MAC_OS_X_VERSION_10_4
	#error "Building for OSX 10.3 is no longer supported!"
  #endif

  #ifndef MAC_OS_X_VERSION_10_5
	#error "To build with 10.4 compatibility, use a 10.5 or 10.6 SDK and set the deployment target to 10.4"
  #endif

#endif

#if JUCE_LINUX || JUCE_ANDROID

  #ifdef _DEBUG
	#define JUCE_DEBUG 1
  #endif

  // Allow override for big-endian Linux platforms
  #if defined (__LITTLE_ENDIAN__) || ! defined (JUCE_BIG_ENDIAN)
	#define JUCE_LITTLE_ENDIAN 1
	#undef JUCE_BIG_ENDIAN
  #else
	#undef JUCE_LITTLE_ENDIAN
	#define JUCE_BIG_ENDIAN 1
  #endif

  #if defined (__LP64__) || defined (_LP64)
	#define JUCE_64BIT 1
  #else
	#define JUCE_32BIT 1
  #endif

  #if __MMX__ || __SSE__ || __amd64__
	#define JUCE_INTEL 1
  #endif
#endif

// Compiler type macros.

#ifdef __GNUC__
  #define JUCE_GCC 1
#elif defined (_MSC_VER)
  #define JUCE_MSVC 1

  #if _MSC_VER < 1500
	#define JUCE_VC8_OR_EARLIER 1

	#if _MSC_VER < 1400
	  #define JUCE_VC7_OR_EARLIER 1

	  #if _MSC_VER < 1300
		#warning "MSVC 6.0 is no longer supported!"
	  #endif
	#endif
  #endif

  #if JUCE_64BIT || ! JUCE_VC7_OR_EARLIER
	#define JUCE_USE_INTRINSICS 1
  #endif
#else
  #error unknown compiler
#endif

#endif   // __JUCE_TARGETPLATFORM_JUCEHEADER__

/*** End of inlined file: juce_TargetPlatform.h ***/

#undef T

#if JUCE_MAC || JUCE_IOS

 #if JUCE_IOS
  #import <Foundation/Foundation.h>
  #import <UIKit/UIKit.h>
  #import <CoreData/CoreData.h>
  #import <MobileCoreServices/MobileCoreServices.h>
  #include <sys/fcntl.h>
 #else
  #define Point CarbonDummyPointName
  #define Component CarbonDummyCompName
  #import <Cocoa/Cocoa.h>
  #import <CoreAudio/HostTime.h>
  #undef Point
  #undef Component
  #include <sys/dir.h>
 #endif

 #include <sys/socket.h>
 #include <sys/sysctl.h>
 #include <sys/stat.h>
 #include <sys/param.h>
 #include <sys/mount.h>
 #include <sys/utsname.h>
 #include <sys/mman.h>
 #include <fnmatch.h>
 #include <utime.h>
 #include <dlfcn.h>
 #include <ifaddrs.h>
 #include <net/if_dl.h>
 #include <mach/mach_time.h>
 #include <mach-o/dyld.h>

#elif JUCE_WINDOWS
 #if JUCE_MSVC
  #ifndef _CPPRTTI
   #error "You're compiling without RTTI enabled! This is needed for a lot of JUCE classes, please update your compiler settings!"
  #endif

  #ifndef _CPPUNWIND
   #error "You're compiling without exceptions enabled! This is needed for a lot of JUCE classes, please update your compiler settings!"
  #endif

  #pragma warning (push)
  #pragma warning (disable : 4100 4201 4514 4312 4995)
 #endif

 #define STRICT 1
 #define WIN32_LEAN_AND_MEAN 1
 #define _WIN32_WINNT 0x0600
 #define _UNICODE 1
 #define UNICODE 1
 #ifndef _WIN32_IE
  #define _WIN32_IE 0x0400
 #endif

 #include <windows.h>
 #include <shellapi.h>
 #include <tchar.h>
 #include <stddef.h>
 #include <ctime>
 #include <wininet.h>
 #include <nb30.h>
 #include <iphlpapi.h>
 #include <mapi.h>
 #include <float.h>
 #include <process.h>
 #include <shlobj.h>
 #include <shlwapi.h>
 #include <mmsystem.h>

 #if JUCE_MINGW
  #include <basetyps.h>
 #else
  #include <crtdbg.h>
  #include <comutil.h>
 #endif

 #undef PACKED

 #if JUCE_MSVC
  #pragma warning (pop)
  #pragma warning (4: 4511 4512 4100 /*4365*/)  // (enable some warnings that are turned off in VC8)
 #endif

 #if JUCE_MSVC && ! JUCE_DONT_AUTOLINK_TO_WIN32_LIBRARIES
  #pragma comment (lib, "kernel32.lib")
  #pragma comment (lib, "user32.lib")
  #pragma comment (lib, "shell32.lib")
  #pragma comment (lib, "wininet.lib")
  #pragma comment (lib, "advapi32.lib")
  #pragma comment (lib, "ws2_32.lib")
  #pragma comment (lib, "version.lib")
  #pragma comment (lib, "shlwapi.lib")
  #pragma comment (lib, "winmm.lib")

  #ifdef _NATIVE_WCHAR_T_DEFINED
   #ifdef _DEBUG
	#pragma comment (lib, "comsuppwd.lib")
   #else
	#pragma comment (lib, "comsuppw.lib")
   #endif
  #else
   #ifdef _DEBUG
	#pragma comment (lib, "comsuppd.lib")
   #else
	#pragma comment (lib, "comsupp.lib")
   #endif
  #endif
 #endif

 /* Used with DynamicLibrary to simplify importing functions

	functionName: function to import
	localFunctionName: name you want to use to actually call it (must be different)
	returnType: the return type
	object: the DynamicLibrary to use
	params: list of params (bracketed)
 */
 #define JUCE_DLL_FUNCTION(functionName, localFunctionName, returnType, object, params) \
	typedef returnType (WINAPI *type##localFunctionName) params; \
	type##localFunctionName localFunctionName = (type##localFunctionName)object.getFunction (#functionName);

#elif JUCE_LINUX
 #include <sched.h>
 #include <pthread.h>
 #include <sys/time.h>
 #include <errno.h>
 #include <sys/stat.h>
 #include <sys/dir.h>
 #include <sys/ptrace.h>
 #include <sys/vfs.h>
 #include <sys/wait.h>
 #include <sys/mman.h>
 #include <fnmatch.h>
 #include <utime.h>
 #include <pwd.h>
 #include <fcntl.h>
 #include <dlfcn.h>
 #include <netdb.h>
 #include <arpa/inet.h>
 #include <netinet/in.h>
 #include <sys/types.h>
 #include <sys/ioctl.h>
 #include <sys/socket.h>
 #include <net/if.h>
 #include <sys/sysinfo.h>
 #include <sys/file.h>
 #include <sys/prctl.h>
 #include <signal.h>

#elif JUCE_ANDROID
 #include <jni.h>
 #include <pthread.h>
 #include <sched.h>
 #include <sys/time.h>
 #include <utime.h>
 #include <errno.h>
 #include <fcntl.h>
 #include <dlfcn.h>
 #include <sys/stat.h>
 #include <sys/statfs.h>
 #include <sys/ptrace.h>
 #include <sys/sysinfo.h>
 #include <sys/mman.h>
 #include <pwd.h>
 #include <dirent.h>
 #include <fnmatch.h>
 #include <sys/wait.h>
#endif

// Need to clear various moronic redefinitions made by system headers..
#undef max
#undef min
#undef direct
#undef check

#endif   // __JUCE_BASICNATIVEHEADERS_JUCEHEADER__

/*** End of inlined file: juce_BasicNativeHeaders.h ***/

#include "juce_core_amalgam.h"

#include <locale>
#include <cctype>
#include <sys/timeb.h>

#if ! JUCE_ANDROID
 #include <cwctype>
#endif

#if JUCE_WINDOWS
 #include <ctime>
 #include <winsock2.h>
 #include <ws2tcpip.h>

 #if JUCE_MINGW
  #include <ws2spi.h>
 #endif

#else
 #if JUCE_LINUX || JUCE_ANDROID
  #include <sys/types.h>
  #include <sys/socket.h>
  #include <sys/errno.h>
  #include <unistd.h>
  #include <netinet/in.h>
 #endif

 #include <pwd.h>
 #include <fcntl.h>
 #include <netdb.h>
 #include <arpa/inet.h>
 #include <netinet/tcp.h>
 #include <sys/time.h>
#endif

#if JUCE_MAC || JUCE_IOS
 #include <xlocale.h>
#endif

#if JUCE_ANDROID
 #include <android/log.h>
#endif

namespace juce
{

// START_AUTOINCLUDE containers/*.cpp, files/*.cpp, json/*.cpp, logging/*.cpp, maths/*.cpp,
// memory/*.cpp, misc/*.cpp, network/*.cpp, streams/*.cpp, system/*.cpp, text/*.cpp, threads/*.cpp,
// time/*.cpp, unit_tests/*.cpp, xml/*.cpp, zip/juce_GZIPD*.cpp, zip/juce_GZIPC*.cpp, zip/juce_Zip*.cpp

/*** Start of inlined file: juce_AbstractFifo.cpp ***/
AbstractFifo::AbstractFifo (const int capacity) noexcept
	: bufferSize (capacity)
{
	jassert (bufferSize > 0);
}

AbstractFifo::~AbstractFifo() {}

int AbstractFifo::getTotalSize() const noexcept           { return bufferSize; }
int AbstractFifo::getFreeSpace() const noexcept           { return bufferSize - getNumReady(); }

int AbstractFifo::getNumReady() const noexcept
{
	const int vs = validStart.get();
	const int ve = validEnd.get();
	return ve >= vs ? (ve - vs) : (bufferSize - (vs - ve));
}

void AbstractFifo::reset() noexcept
{
	validEnd = 0;
	validStart = 0;
}

void AbstractFifo::setTotalSize (int newSize) noexcept
{
	jassert (newSize > 0);
	reset();
	bufferSize = newSize;
}

void AbstractFifo::prepareToWrite (int numToWrite, int& startIndex1, int& blockSize1, int& startIndex2, int& blockSize2) const noexcept
{
	const int vs = validStart.get();
	const int ve = validEnd.value;

	const int freeSpace = ve >= vs ? (bufferSize - (ve - vs)) : (vs - ve);
	numToWrite = jmin (numToWrite, freeSpace - 1);

	if (numToWrite <= 0)
	{
		startIndex1 = 0;
		startIndex2 = 0;
		blockSize1 = 0;
		blockSize2 = 0;
	}
	else
	{
		startIndex1 = ve;
		startIndex2 = 0;
		blockSize1 = jmin (bufferSize - ve, numToWrite);
		numToWrite -= blockSize1;
		blockSize2 = numToWrite <= 0 ? 0 : jmin (numToWrite, vs);
	}
}

void AbstractFifo::finishedWrite (int numWritten) noexcept
{
	jassert (numWritten >= 0 && numWritten < bufferSize);
	int newEnd = validEnd.value + numWritten;
	if (newEnd >= bufferSize)
		newEnd -= bufferSize;

	validEnd = newEnd;
}

void AbstractFifo::prepareToRead (int numWanted, int& startIndex1, int& blockSize1, int& startIndex2, int& blockSize2) const noexcept
{
	const int vs = validStart.value;
	const int ve = validEnd.get();

	const int numReady = ve >= vs ? (ve - vs) : (bufferSize - (vs - ve));
	numWanted = jmin (numWanted, numReady);

	if (numWanted <= 0)
	{
		startIndex1 = 0;
		startIndex2 = 0;
		blockSize1 = 0;
		blockSize2 = 0;
	}
	else
	{
		startIndex1 = vs;
		startIndex2 = 0;
		blockSize1 = jmin (bufferSize - vs, numWanted);
		numWanted -= blockSize1;
		blockSize2 = numWanted <= 0 ? 0 : jmin (numWanted, ve);
	}
}

void AbstractFifo::finishedRead (int numRead) noexcept
{
	jassert (numRead >= 0 && numRead <= bufferSize);

	int newStart = validStart.value + numRead;
	if (newStart >= bufferSize)
		newStart -= bufferSize;

	validStart = newStart;
}

#if JUCE_UNIT_TESTS

class AbstractFifoTests  : public UnitTest
{
public:
	AbstractFifoTests() : UnitTest ("Abstract Fifo") {}

	class WriteThread  : public Thread
	{
	public:
		WriteThread (AbstractFifo& fifo_, int* buffer_)
			: Thread ("fifo writer"), fifo (fifo_), buffer (buffer_)
		{
			startThread();
		}

		~WriteThread()
		{
			stopThread (5000);
		}

		void run()
		{
			int n = 0;
			Random r;

			while (! threadShouldExit())
			{
				int num = r.nextInt (2000) + 1;

				int start1, size1, start2, size2;
				fifo.prepareToWrite (num, start1, size1, start2, size2);

				jassert (size1 >= 0 && size2 >= 0);
				jassert (size1 == 0 || (start1 >= 0 && start1 < fifo.getTotalSize()));
				jassert (size2 == 0 || (start2 >= 0 && start2 < fifo.getTotalSize()));

				int i;
				for (i = 0; i < size1; ++i)
					buffer [start1 + i] = n++;

				for (i = 0; i < size2; ++i)
					buffer [start2 + i] = n++;

				fifo.finishedWrite (size1 + size2);
			}
		}

	private:
		AbstractFifo& fifo;
		int* buffer;
	};

	void runTest()
	{
		beginTest ("AbstractFifo");

		int buffer [5000];
		AbstractFifo fifo (numElementsInArray (buffer));

		WriteThread writer (fifo, buffer);

		int n = 0;
		Random r;

		for (int count = 1000000; --count >= 0;)
		{
			int num = r.nextInt (6000) + 1;

			int start1, size1, start2, size2;
			fifo.prepareToRead (num, start1, size1, start2, size2);

			if (! (size1 >= 0 && size2 >= 0)
					&& (size1 == 0 || (start1 >= 0 && start1 < fifo.getTotalSize()))
					&& (size2 == 0 || (start2 >= 0 && start2 < fifo.getTotalSize())))
			{
				expect (false, "prepareToRead returned -ve values");
				break;
			}

			bool failed = false;

			int i;
			for (i = 0; i < size1; ++i)
				failed = (buffer [start1 + i] != n++) || failed;

			for (i = 0; i < size2; ++i)
				failed = (buffer [start2 + i] != n++) || failed;

			if (failed)
			{
				expect (false, "read values were incorrect");
				break;
			}

			fifo.finishedRead (size1 + size2);
		}
	}
};

static AbstractFifoTests fifoUnitTests;

#endif

/*** End of inlined file: juce_AbstractFifo.cpp ***/



/*** Start of inlined file: juce_DynamicObject.cpp ***/
DynamicObject::DynamicObject()
{
}

DynamicObject::~DynamicObject()
{
}

bool DynamicObject::hasProperty (const Identifier& propertyName) const
{
	const var* const v = properties.getVarPointer (propertyName);
	return v != nullptr && ! v->isMethod();
}

var DynamicObject::getProperty (const Identifier& propertyName) const
{
	return properties [propertyName];
}

void DynamicObject::setProperty (const Identifier& propertyName, const var& newValue)
{
	properties.set (propertyName, newValue);
}

void DynamicObject::removeProperty (const Identifier& propertyName)
{
	properties.remove (propertyName);
}

bool DynamicObject::hasMethod (const Identifier& methodName) const
{
	return getProperty (methodName).isMethod();
}

var DynamicObject::invokeMethod (const Identifier& methodName,
								 const var* parameters,
								 int numParameters)
{
	return properties [methodName].invokeMethod (this, parameters, numParameters);
}

void DynamicObject::setMethod (const Identifier& name,
							   var::MethodFunction methodFunction)
{
	properties.set (name, var (methodFunction));
}

void DynamicObject::clear()
{
	properties.clear();
}

/*** End of inlined file: juce_DynamicObject.cpp ***/


/*** Start of inlined file: juce_NamedValueSet.cpp ***/
NamedValueSet::NamedValue::NamedValue() noexcept
{
}

inline NamedValueSet::NamedValue::NamedValue (const Identifier& name_, const var& value_)
	: name (name_), value (value_)
{
}

NamedValueSet::NamedValue::NamedValue (const NamedValue& other)
	: name (other.name), value (other.value)
{
}

NamedValueSet::NamedValue& NamedValueSet::NamedValue::operator= (const NamedValueSet::NamedValue& other)
{
	name = other.name;
	value = other.value;
	return *this;
}

#if JUCE_COMPILER_SUPPORTS_MOVE_SEMANTICS
NamedValueSet::NamedValue::NamedValue (NamedValue&& other) noexcept
	: nextListItem (static_cast <LinkedListPointer<NamedValue>&&> (other.nextListItem)),
	  name (static_cast <Identifier&&> (other.name)),
	  value (static_cast <var&&> (other.value))
{
}

inline NamedValueSet::NamedValue::NamedValue (const Identifier& name_, var&& value_)
	: name (name_), value (static_cast <var&&> (value_))
{
}

NamedValueSet::NamedValue& NamedValueSet::NamedValue::operator= (NamedValue&& other) noexcept
{
	nextListItem = static_cast <LinkedListPointer<NamedValue>&&> (other.nextListItem);
	name = static_cast <Identifier&&> (other.name);
	value = static_cast <var&&> (other.value);
	return *this;
}
#endif

bool NamedValueSet::NamedValue::operator== (const NamedValueSet::NamedValue& other) const noexcept
{
	return name == other.name && value == other.value;
}

NamedValueSet::NamedValueSet() noexcept
{
}

NamedValueSet::NamedValueSet (const NamedValueSet& other)
{
	values.addCopyOfList (other.values);
}

NamedValueSet& NamedValueSet::operator= (const NamedValueSet& other)
{
	clear();
	values.addCopyOfList (other.values);
	return *this;
}

#if JUCE_COMPILER_SUPPORTS_MOVE_SEMANTICS
NamedValueSet::NamedValueSet (NamedValueSet&& other) noexcept
	: values (static_cast <LinkedListPointer<NamedValue>&&> (other.values))
{
}

NamedValueSet& NamedValueSet::operator= (NamedValueSet&& other) noexcept
{
	other.values.swapWith (values);
	return *this;
}
#endif

NamedValueSet::~NamedValueSet()
{
	clear();
}

void NamedValueSet::clear()
{
	values.deleteAll();
}

bool NamedValueSet::operator== (const NamedValueSet& other) const
{
	const NamedValue* i1 = values;
	const NamedValue* i2 = other.values;

	while (i1 != nullptr && i2 != nullptr)
	{
		if (! (*i1 == *i2))
			return false;

		i1 = i1->nextListItem;
		i2 = i2->nextListItem;
	}

	return true;
}

bool NamedValueSet::operator!= (const NamedValueSet& other) const
{
	return ! operator== (other);
}

int NamedValueSet::size() const noexcept
{
	return values.size();
}

const var& NamedValueSet::operator[] (const Identifier& name) const
{
	for (NamedValue* i = values; i != nullptr; i = i->nextListItem)
		if (i->name == name)
			return i->value;

	return var::null;
}

var NamedValueSet::getWithDefault (const Identifier& name, const var& defaultReturnValue) const
{
	const var* const v = getVarPointer (name);
	return v != nullptr ? *v : defaultReturnValue;
}

var* NamedValueSet::getVarPointer (const Identifier& name) const noexcept
{
	for (NamedValue* i = values; i != nullptr; i = i->nextListItem)
		if (i->name == name)
			return &(i->value);

	return nullptr;
}

#if JUCE_COMPILER_SUPPORTS_MOVE_SEMANTICS
bool NamedValueSet::set (const Identifier& name, var&& newValue)
{
	LinkedListPointer<NamedValue>* i = &values;

	while (i->get() != nullptr)
	{
		NamedValue* const v = i->get();

		if (v->name == name)
		{
			if (v->value.equalsWithSameType (newValue))
				return false;

			v->value = static_cast <var&&> (newValue);
			return true;
		}

		i = &(v->nextListItem);
	}

	i->insertNext (new NamedValue (name, static_cast <var&&> (newValue)));
	return true;
}
#endif

bool NamedValueSet::set (const Identifier& name, const var& newValue)
{
	LinkedListPointer<NamedValue>* i = &values;

	while (i->get() != nullptr)
	{
		NamedValue* const v = i->get();

		if (v->name == name)
		{
			if (v->value.equalsWithSameType (newValue))
				return false;

			v->value = newValue;
			return true;
		}

		i = &(v->nextListItem);
	}

	i->insertNext (new NamedValue (name, newValue));
	return true;
}

bool NamedValueSet::contains (const Identifier& name) const
{
	return getVarPointer (name) != nullptr;
}

bool NamedValueSet::remove (const Identifier& name)
{
	LinkedListPointer<NamedValue>* i = &values;

	for (;;)
	{
		NamedValue* const v = i->get();

		if (v == nullptr)
			break;

		if (v->name == name)
		{
			delete i->removeNext();
			return true;
		}

		i = &(v->nextListItem);
	}

	return false;
}

const Identifier NamedValueSet::getName (const int index) const
{
	const NamedValue* const v = values[index];
	jassert (v != nullptr);
	return v->name;
}

const var& NamedValueSet::getValueAt (const int index) const
{
	const NamedValue* const v = values[index];
	jassert (v != nullptr);
	return v->value;
}

void NamedValueSet::setFromXmlAttributes (const XmlElement& xml)
{
	clear();
	LinkedListPointer<NamedValue>::Appender appender (values);

	const int numAtts = xml.getNumAttributes(); // xxx inefficient - should write an att iterator..

	for (int i = 0; i < numAtts; ++i)
		appender.append (new NamedValue (xml.getAttributeName (i), var (xml.getAttributeValue (i))));
}

void NamedValueSet::copyToXmlAttributes (XmlElement& xml) const
{
	for (NamedValue* i = values; i != nullptr; i = i->nextListItem)
	{
		jassert (! i->value.isObject()); // DynamicObjects can't be stored as XML!

		xml.setAttribute (i->name.toString(),
						  i->value.toString());
	}
}

/*** End of inlined file: juce_NamedValueSet.cpp ***/


/*** Start of inlined file: juce_PropertySet.cpp ***/
PropertySet::PropertySet (const bool ignoreCaseOfKeyNames)
	: properties (ignoreCaseOfKeyNames),
	  fallbackProperties (nullptr),
	  ignoreCaseOfKeys (ignoreCaseOfKeyNames)
{
}

PropertySet::PropertySet (const PropertySet& other)
	: properties (other.properties),
	  fallbackProperties (other.fallbackProperties),
	  ignoreCaseOfKeys (other.ignoreCaseOfKeys)
{
}

PropertySet& PropertySet::operator= (const PropertySet& other)
{
	properties = other.properties;
	fallbackProperties = other.fallbackProperties;
	ignoreCaseOfKeys = other.ignoreCaseOfKeys;

	propertyChanged();
	return *this;
}

PropertySet::~PropertySet()
{
}

void PropertySet::clear()
{
	const ScopedLock sl (lock);

	if (properties.size() > 0)
	{
		properties.clear();
		propertyChanged();
	}
}

String PropertySet::getValue (const String& keyName,
							  const String& defaultValue) const noexcept
{
	const ScopedLock sl (lock);

	const int index = properties.getAllKeys().indexOf (keyName, ignoreCaseOfKeys);

	if (index >= 0)
		return properties.getAllValues() [index];

	return fallbackProperties != nullptr ? fallbackProperties->getValue (keyName, defaultValue)
										 : defaultValue;
}

int PropertySet::getIntValue (const String& keyName,
							  const int defaultValue) const noexcept
{
	const ScopedLock sl (lock);
	const int index = properties.getAllKeys().indexOf (keyName, ignoreCaseOfKeys);

	if (index >= 0)
		return properties.getAllValues() [index].getIntValue();

	return fallbackProperties != nullptr ? fallbackProperties->getIntValue (keyName, defaultValue)
										 : defaultValue;
}

double PropertySet::getDoubleValue (const String& keyName,
									const double defaultValue) const noexcept
{
	const ScopedLock sl (lock);
	const int index = properties.getAllKeys().indexOf (keyName, ignoreCaseOfKeys);

	if (index >= 0)
		return properties.getAllValues()[index].getDoubleValue();

	return fallbackProperties != nullptr ? fallbackProperties->getDoubleValue (keyName, defaultValue)
										 : defaultValue;
}

bool PropertySet::getBoolValue (const String& keyName,
								const bool defaultValue) const noexcept
{
	const ScopedLock sl (lock);
	const int index = properties.getAllKeys().indexOf (keyName, ignoreCaseOfKeys);

	if (index >= 0)
		return properties.getAllValues() [index].getIntValue() != 0;

	return fallbackProperties != nullptr ? fallbackProperties->getBoolValue (keyName, defaultValue)
										 : defaultValue;
}

XmlElement* PropertySet::getXmlValue (const String& keyName) const
{
	return XmlDocument::parse (getValue (keyName));
}

void PropertySet::setValue (const String& keyName, const var& v)
{
	jassert (keyName.isNotEmpty()); // shouldn't use an empty key name!

	if (keyName.isNotEmpty())
	{
		const String value (v.toString());
		const ScopedLock sl (lock);

		const int index = properties.getAllKeys().indexOf (keyName, ignoreCaseOfKeys);

		if (index < 0 || properties.getAllValues() [index] != value)
		{
			properties.set (keyName, value);
			propertyChanged();
		}
	}
}

void PropertySet::removeValue (const String& keyName)
{
	if (keyName.isNotEmpty())
	{
		const ScopedLock sl (lock);
		const int index = properties.getAllKeys().indexOf (keyName, ignoreCaseOfKeys);

		if (index >= 0)
		{
			properties.remove (keyName);
			propertyChanged();
		}
	}
}

void PropertySet::setValue (const String& keyName, const XmlElement* const xml)
{
	setValue (keyName, xml == nullptr ? var::null
									  : var (xml->createDocument (String::empty, true)));
}

bool PropertySet::containsKey (const String& keyName) const noexcept
{
	const ScopedLock sl (lock);
	return properties.getAllKeys().contains (keyName, ignoreCaseOfKeys);
}

void PropertySet::addAllPropertiesFrom (const PropertySet& source)
{
	const ScopedLock sl (source.getLock());

	for (int i = 0; i < source.properties.size(); ++i)
		setValue (source.properties.getAllKeys() [i],
				  source.properties.getAllValues() [i]);
}

void PropertySet::setFallbackPropertySet (PropertySet* fallbackProperties_) noexcept
{
	const ScopedLock sl (lock);
	fallbackProperties = fallbackProperties_;
}

XmlElement* PropertySet::createXml (const String& nodeName) const
{
	const ScopedLock sl (lock);
	XmlElement* const xml = new XmlElement (nodeName);

	for (int i = 0; i < properties.getAllKeys().size(); ++i)
	{
		XmlElement* const e = xml->createNewChildElement ("VALUE");
		e->setAttribute ("name", properties.getAllKeys()[i]);
		e->setAttribute ("val", properties.getAllValues()[i]);
	}

	return xml;
}

void PropertySet::restoreFromXml (const XmlElement& xml)
{
	const ScopedLock sl (lock);
	clear();

	forEachXmlChildElementWithTagName (xml, e, "VALUE")
	{
		if (e->hasAttribute ("name")
			 && e->hasAttribute ("val"))
		{
			properties.set (e->getStringAttribute ("name"),
							e->getStringAttribute ("val"));
		}
	}

	if (properties.size() > 0)
		propertyChanged();
}

void PropertySet::propertyChanged()
{
}

/*** End of inlined file: juce_PropertySet.cpp ***/


/*** Start of inlined file: juce_Variant.cpp ***/
enum VariantStreamMarkers
{
	varMarker_Int       = 1,
	varMarker_BoolTrue  = 2,
	varMarker_BoolFalse = 3,
	varMarker_Double    = 4,
	varMarker_String    = 5,
	varMarker_Int64     = 6,
	varMarker_Array     = 7
};

class var::VariantType
{
public:
	VariantType() noexcept {}
	virtual ~VariantType() noexcept {}

	virtual int toInt (const ValueUnion&) const noexcept                        { return 0; }
	virtual int64 toInt64 (const ValueUnion&) const noexcept                    { return 0; }
	virtual double toDouble (const ValueUnion&) const noexcept                  { return 0; }
	virtual String toString (const ValueUnion&) const                           { return String::empty; }
	virtual bool toBool (const ValueUnion&) const noexcept                      { return false; }
	virtual ReferenceCountedObject* toObject (const ValueUnion&) const noexcept { return nullptr; }
	virtual Array<var>* toArray (const ValueUnion&) const noexcept              { return 0; }

	virtual bool isVoid() const noexcept      { return false; }
	virtual bool isInt() const noexcept       { return false; }
	virtual bool isInt64() const noexcept     { return false; }
	virtual bool isBool() const noexcept      { return false; }
	virtual bool isDouble() const noexcept    { return false; }
	virtual bool isString() const noexcept    { return false; }
	virtual bool isObject() const noexcept    { return false; }
	virtual bool isArray() const noexcept     { return false; }
	virtual bool isMethod() const noexcept    { return false; }

	virtual void cleanUp (ValueUnion&) const noexcept {}
	virtual void createCopy (ValueUnion& dest, const ValueUnion& source) const      { dest = source; }
	virtual bool equals (const ValueUnion& data, const ValueUnion& otherData, const VariantType& otherType) const noexcept = 0;
	virtual void writeToStream (const ValueUnion& data, OutputStream& output) const = 0;
};

class var::VariantType_Void  : public var::VariantType
{
public:
	VariantType_Void() noexcept {}
	static const VariantType_Void instance;

	bool isVoid() const noexcept    { return true; }
	bool equals (const ValueUnion&, const ValueUnion&, const VariantType& otherType) const noexcept { return otherType.isVoid(); }
	void writeToStream (const ValueUnion&, OutputStream& output) const   { output.writeCompressedInt (0); }
};

class var::VariantType_Int  : public var::VariantType
{
public:
	VariantType_Int() noexcept {}
	static const VariantType_Int instance;

	int toInt (const ValueUnion& data) const noexcept       { return data.intValue; };
	int64 toInt64 (const ValueUnion& data) const noexcept   { return (int64) data.intValue; };
	double toDouble (const ValueUnion& data) const noexcept { return (double) data.intValue; }
	String toString (const ValueUnion& data) const          { return String (data.intValue); }
	bool toBool (const ValueUnion& data) const noexcept     { return data.intValue != 0; }
	bool isInt() const noexcept                             { return true; }

	bool equals (const ValueUnion& data, const ValueUnion& otherData, const VariantType& otherType) const noexcept
	{
		return otherType.toInt (otherData) == data.intValue;
	}

	void writeToStream (const ValueUnion& data, OutputStream& output) const
	{
		output.writeCompressedInt (5);
		output.writeByte (varMarker_Int);
		output.writeInt (data.intValue);
	}
};

class var::VariantType_Int64  : public var::VariantType
{
public:
	VariantType_Int64() noexcept {}
	static const VariantType_Int64 instance;

	int toInt (const ValueUnion& data) const noexcept       { return (int) data.int64Value; };
	int64 toInt64 (const ValueUnion& data) const noexcept   { return data.int64Value; };
	double toDouble (const ValueUnion& data) const noexcept { return (double) data.int64Value; }
	String toString (const ValueUnion& data) const          { return String (data.int64Value); }
	bool toBool (const ValueUnion& data) const noexcept     { return data.int64Value != 0; }
	bool isInt64() const noexcept                           { return true; }

	bool equals (const ValueUnion& data, const ValueUnion& otherData, const VariantType& otherType) const noexcept
	{
		return otherType.toInt64 (otherData) == data.int64Value;
	}

	void writeToStream (const ValueUnion& data, OutputStream& output) const
	{
		output.writeCompressedInt (9);
		output.writeByte (varMarker_Int64);
		output.writeInt64 (data.int64Value);
	}
};

class var::VariantType_Double   : public var::VariantType
{
public:
	VariantType_Double() noexcept {}
	static const VariantType_Double instance;

	int toInt (const ValueUnion& data) const noexcept       { return (int) data.doubleValue; };
	int64 toInt64 (const ValueUnion& data) const noexcept   { return (int64) data.doubleValue; };
	double toDouble (const ValueUnion& data) const noexcept { return data.doubleValue; }
	String toString (const ValueUnion& data) const          { return String (data.doubleValue); }
	bool toBool (const ValueUnion& data) const noexcept     { return data.doubleValue != 0; }
	bool isDouble() const noexcept                          { return true; }

	bool equals (const ValueUnion& data, const ValueUnion& otherData, const VariantType& otherType) const noexcept
	{
		return std::abs (otherType.toDouble (otherData) - data.doubleValue) < std::numeric_limits<double>::epsilon();
	}

	void writeToStream (const ValueUnion& data, OutputStream& output) const
	{
		output.writeCompressedInt (9);
		output.writeByte (varMarker_Double);
		output.writeDouble (data.doubleValue);
	}
};

class var::VariantType_Bool   : public var::VariantType
{
public:
	VariantType_Bool() noexcept {}
	static const VariantType_Bool instance;

	int toInt (const ValueUnion& data) const noexcept       { return data.boolValue ? 1 : 0; };
	int64 toInt64 (const ValueUnion& data) const noexcept   { return data.boolValue ? 1 : 0; };
	double toDouble (const ValueUnion& data) const noexcept { return data.boolValue ? 1.0 : 0.0; }
	String toString (const ValueUnion& data) const          { return String::charToString (data.boolValue ? (juce_wchar) '1' : (juce_wchar) '0'); }
	bool toBool (const ValueUnion& data) const noexcept     { return data.boolValue; }
	bool isBool() const noexcept                            { return true; }

	bool equals (const ValueUnion& data, const ValueUnion& otherData, const VariantType& otherType) const noexcept
	{
		return otherType.toBool (otherData) == data.boolValue;
	}

	void writeToStream (const ValueUnion& data, OutputStream& output) const
	{
		output.writeCompressedInt (1);
		output.writeByte (data.boolValue ? (char) varMarker_BoolTrue : (char) varMarker_BoolFalse);
	}
};

class var::VariantType_String   : public var::VariantType
{
public:
	VariantType_String() noexcept {}
	static const VariantType_String instance;

	void cleanUp (ValueUnion& data) const noexcept                       { getString (data)-> ~String(); }
	void createCopy (ValueUnion& dest, const ValueUnion& source) const   { new (dest.stringValue) String (*getString (source)); }

	bool isString() const noexcept                          { return true; }
	int toInt (const ValueUnion& data) const noexcept       { return getString (data)->getIntValue(); };
	int64 toInt64 (const ValueUnion& data) const noexcept   { return getString (data)->getLargeIntValue(); };
	double toDouble (const ValueUnion& data) const noexcept { return getString (data)->getDoubleValue(); }
	String toString (const ValueUnion& data) const          { return *getString (data); }
	bool toBool (const ValueUnion& data) const noexcept     { return getString (data)->getIntValue() != 0
																	  || getString (data)->trim().equalsIgnoreCase ("true")
																	  || getString (data)->trim().equalsIgnoreCase ("yes"); }

	bool equals (const ValueUnion& data, const ValueUnion& otherData, const VariantType& otherType) const noexcept
	{
		return otherType.toString (otherData) == *getString (data);
	}

	void writeToStream (const ValueUnion& data, OutputStream& output) const
	{
		const String* const s = getString (data);
		const int len = s->getNumBytesAsUTF8() + 1;
		HeapBlock<char> temp ((size_t) len);
		s->copyToUTF8 (temp, len);
		output.writeCompressedInt (len + 1);
		output.writeByte (varMarker_String);
		output.write (temp, len);
	}

private:
	static inline const String* getString (const ValueUnion& data) noexcept { return reinterpret_cast <const String*> (data.stringValue); }
	static inline String* getString (ValueUnion& data) noexcept             { return reinterpret_cast <String*> (data.stringValue); }
};

class var::VariantType_Object   : public var::VariantType
{
public:
	VariantType_Object() noexcept {}
	static const VariantType_Object instance;

	void cleanUp (ValueUnion& data) const noexcept                      { if (data.objectValue != nullptr) data.objectValue->decReferenceCount(); }

	void createCopy (ValueUnion& dest, const ValueUnion& source) const
	{
		dest.objectValue = source.objectValue;
		if (dest.objectValue != nullptr)
			dest.objectValue->incReferenceCount();
	}

	String toString (const ValueUnion& data) const                            { return "Object 0x" + String::toHexString ((int) (pointer_sized_int) data.objectValue); }
	bool toBool (const ValueUnion& data) const noexcept                       { return data.objectValue != 0; }
	ReferenceCountedObject* toObject (const ValueUnion& data) const noexcept  { return data.objectValue; }
	bool isObject() const noexcept                                            { return true; }

	bool equals (const ValueUnion& data, const ValueUnion& otherData, const VariantType& otherType) const noexcept
	{
		return otherType.toObject (otherData) == data.objectValue;
	}

	void writeToStream (const ValueUnion&, OutputStream& output) const
	{
		jassertfalse; // Can't write an object to a stream!
		output.writeCompressedInt (0);
	}
};

class var::VariantType_Array   : public var::VariantType
{
public:
	VariantType_Array() noexcept {}
	static const VariantType_Array instance;

	void cleanUp (ValueUnion& data) const noexcept                      { delete data.arrayValue; }
	void createCopy (ValueUnion& dest, const ValueUnion& source) const  { dest.arrayValue = new Array<var> (*(source.arrayValue)); }

	String toString (const ValueUnion&) const                           { return "[Array]"; }
	bool isArray() const noexcept                                       { return true; }
	Array<var>* toArray (const ValueUnion& data) const noexcept         { return data.arrayValue; }

	bool equals (const ValueUnion& data, const ValueUnion& otherData, const VariantType& otherType) const noexcept
	{
		const Array<var>* const otherArray = otherType.toArray (otherData);
		return otherArray != nullptr && *otherArray == *(data.arrayValue);
	}

	void writeToStream (const ValueUnion& data, OutputStream& output) const
	{
		MemoryOutputStream buffer (512);
		const int numItems = data.arrayValue->size();
		buffer.writeCompressedInt (numItems);

		for (int i = 0; i < numItems; ++i)
			data.arrayValue->getReference(i).writeToStream (buffer);

		output.writeCompressedInt (1 + (int) buffer.getDataSize());
		output.writeByte (varMarker_Array);
		output << buffer;
	}
};

class var::VariantType_Method   : public var::VariantType
{
public:
	VariantType_Method() noexcept {}
	static const VariantType_Method instance;

	String toString (const ValueUnion&) const               { return "Method"; }
	bool toBool (const ValueUnion& data) const noexcept     { return data.methodValue != 0; }
	bool isMethod() const noexcept                          { return true; }

	bool equals (const ValueUnion& data, const ValueUnion& otherData, const VariantType& otherType) const noexcept
	{
		return otherType.isMethod() && otherData.methodValue == data.methodValue;
	}

	void writeToStream (const ValueUnion&, OutputStream& output) const
	{
		jassertfalse; // Can't write a method to a stream!
		output.writeCompressedInt (0);
	}
};

const var::VariantType_Void    var::VariantType_Void::instance;
const var::VariantType_Int     var::VariantType_Int::instance;
const var::VariantType_Int64   var::VariantType_Int64::instance;
const var::VariantType_Bool    var::VariantType_Bool::instance;
const var::VariantType_Double  var::VariantType_Double::instance;
const var::VariantType_String  var::VariantType_String::instance;
const var::VariantType_Object  var::VariantType_Object::instance;
const var::VariantType_Array   var::VariantType_Array::instance;
const var::VariantType_Method  var::VariantType_Method::instance;

var::var() noexcept : type (&VariantType_Void::instance)
{
}

var::~var() noexcept
{
	type->cleanUp (value);
}

const var var::null;

var::var (const var& valueToCopy)  : type (valueToCopy.type)
{
	type->createCopy (value, valueToCopy.value);
}

var::var (const int value_) noexcept       : type (&VariantType_Int::instance)    { value.intValue = value_; }
var::var (const int64 value_) noexcept     : type (&VariantType_Int64::instance)  { value.int64Value = value_; }
var::var (const bool value_) noexcept      : type (&VariantType_Bool::instance)   { value.boolValue = value_; }
var::var (const double value_) noexcept    : type (&VariantType_Double::instance) { value.doubleValue = value_; }
var::var (MethodFunction method_) noexcept : type (&VariantType_Method::instance) { value.methodValue = method_; }

var::var (const String& value_)        : type (&VariantType_String::instance) { new (value.stringValue) String (value_); }
var::var (const char* const value_)    : type (&VariantType_String::instance) { new (value.stringValue) String (value_); }
var::var (const wchar_t* const value_) : type (&VariantType_String::instance) { new (value.stringValue) String (value_); }

var::var (const Array<var>& value_)  : type (&VariantType_Array::instance)
{
	value.arrayValue = new Array<var> (value_);
}

var::var (ReferenceCountedObject* const object)  : type (&VariantType_Object::instance)
{
	value.objectValue = object;

	if (object != nullptr)
		object->incReferenceCount();
}

bool var::isVoid() const noexcept     { return type->isVoid(); }
bool var::isInt() const noexcept      { return type->isInt(); }
bool var::isInt64() const noexcept    { return type->isInt64(); }
bool var::isBool() const noexcept     { return type->isBool(); }
bool var::isDouble() const noexcept   { return type->isDouble(); }
bool var::isString() const noexcept   { return type->isString(); }
bool var::isObject() const noexcept   { return type->isObject(); }
bool var::isArray() const noexcept    { return type->isArray(); }
bool var::isMethod() const noexcept   { return type->isMethod(); }

var::operator int() const noexcept                      { return type->toInt (value); }
var::operator int64() const noexcept                    { return type->toInt64 (value); }
var::operator bool() const noexcept                     { return type->toBool (value); }
var::operator float() const noexcept                    { return (float) type->toDouble (value); }
var::operator double() const noexcept                   { return type->toDouble (value); }
String var::toString() const                            { return type->toString (value); }
var::operator String() const                            { return type->toString (value); }
ReferenceCountedObject* var::getObject() const noexcept { return type->toObject (value); }
Array<var>* var::getArray() const noexcept              { return type->toArray (value); }
DynamicObject* var::getDynamicObject() const noexcept   { return dynamic_cast <DynamicObject*> (getObject()); }

void var::swapWith (var& other) noexcept
{
	std::swap (type, other.type);
	std::swap (value, other.value);
}

var& var::operator= (const var& v)               { type->cleanUp (value); type = v.type; type->createCopy (value, v.value); return *this; }
var& var::operator= (const int v)                { type->cleanUp (value); type = &VariantType_Int::instance; value.intValue = v; return *this; }
var& var::operator= (const int64 v)              { type->cleanUp (value); type = &VariantType_Int64::instance; value.int64Value = v; return *this; }
var& var::operator= (const bool v)               { type->cleanUp (value); type = &VariantType_Bool::instance; value.boolValue = v; return *this; }
var& var::operator= (const double v)             { type->cleanUp (value); type = &VariantType_Double::instance; value.doubleValue = v; return *this; }
var& var::operator= (const char* const v)        { type->cleanUp (value); type = &VariantType_String::instance; new (value.stringValue) String (v); return *this; }
var& var::operator= (const wchar_t* const v)     { type->cleanUp (value); type = &VariantType_String::instance; new (value.stringValue) String (v); return *this; }
var& var::operator= (const String& v)            { type->cleanUp (value); type = &VariantType_String::instance; new (value.stringValue) String (v); return *this; }
var& var::operator= (const Array<var>& v)        { var v2 (v); swapWith (v2); return *this; }
var& var::operator= (ReferenceCountedObject* v)  { var v2 (v); swapWith (v2); return *this; }
var& var::operator= (MethodFunction v)           { var v2 (v); swapWith (v2); return *this; }

#if JUCE_COMPILER_SUPPORTS_MOVE_SEMANTICS
var::var (var&& other) noexcept
	: type (other.type),
	  value (other.value)
{
	other.type = &VariantType_Void::instance;
}

var& var::operator= (var&& other) noexcept
{
	swapWith (other);
	return *this;
}

var::var (String&& value_)  : type (&VariantType_String::instance)
{
	new (value.stringValue) String (static_cast<String&&> (value_));
}

var& var::operator= (String&& v)
{
	type->cleanUp (value);
	type = &VariantType_String::instance;
	new (value.stringValue) String (static_cast<String&&> (v));
	return *this;
}
#endif

bool var::equals (const var& other) const noexcept
{
	return type->equals (value, other.value, *other.type);
}

bool var::equalsWithSameType (const var& other) const noexcept
{
	return type == other.type && equals (other);
}

bool operator== (const var& v1, const var& v2) noexcept     { return v1.equals (v2); }
bool operator!= (const var& v1, const var& v2) noexcept     { return ! v1.equals (v2); }
bool operator== (const var& v1, const String& v2)           { return v1.toString() == v2; }
bool operator!= (const var& v1, const String& v2)           { return v1.toString() != v2; }
bool operator== (const var& v1, const char* const v2)       { return v1.toString() == v2; }
bool operator!= (const var& v1, const char* const v2)       { return v1.toString() != v2; }

var var::operator[] (const Identifier& propertyName) const
{
	DynamicObject* const o = getDynamicObject();
	return o != nullptr ? o->getProperty (propertyName) : var::null;
}

var var::operator[] (const char* const propertyName) const
{
	return operator[] (Identifier (propertyName));
}

var var::getProperty (const Identifier& propertyName, const var& defaultReturnValue) const
{
	DynamicObject* const o = getDynamicObject();
	return o != nullptr ? o->getProperties().getWithDefault (propertyName, defaultReturnValue) : defaultReturnValue;
}

var var::invoke (const Identifier& method, const var* arguments, int numArguments) const
{
	DynamicObject* const o = getDynamicObject();
	return o != nullptr ? o->invokeMethod (method, arguments, numArguments) : var::null;
}

var var::invokeMethod (DynamicObject* const target, const var* const arguments, const int numArguments) const
{
	jassert (target != nullptr);

	if (isMethod())
		return (target->*(value.methodValue)) (arguments, numArguments);

	return var::null;
}

var var::call (const Identifier& method) const
{
	return invoke (method, nullptr, 0);
}

var var::call (const Identifier& method, const var& arg1) const
{
	return invoke (method, &arg1, 1);
}

var var::call (const Identifier& method, const var& arg1, const var& arg2) const
{
	var args[] = { arg1, arg2 };
	return invoke (method, args, 2);
}

var var::call (const Identifier& method, const var& arg1, const var& arg2, const var& arg3)
{
	var args[] = { arg1, arg2, arg3 };
	return invoke (method, args, 3);
}

var var::call (const Identifier& method, const var& arg1, const var& arg2, const var& arg3, const var& arg4) const
{
	var args[] = { arg1, arg2, arg3, arg4 };
	return invoke (method, args, 4);
}

var var::call (const Identifier& method, const var& arg1, const var& arg2, const var& arg3, const var& arg4, const var& arg5) const
{
	var args[] = { arg1, arg2, arg3, arg4, arg5 };
	return invoke (method, args, 5);
}

int var::size() const
{
	const Array<var>* const array = getArray();
	return array != nullptr ? array->size() : 0;
}

const var& var::operator[] (int arrayIndex) const
{
	const Array<var>* const array = getArray();

	// When using this method, the var must actually be an array, and the index
	// must be in-range!
	jassert (array != nullptr && isPositiveAndBelow (arrayIndex, array->size()));

	return array->getReference (arrayIndex);
}

var& var::operator[] (int arrayIndex)
{
	const Array<var>* const array = getArray();

	// When using this method, the var must actually be an array, and the index
	// must be in-range!
	jassert (array != nullptr && isPositiveAndBelow (arrayIndex, array->size()));

	return array->getReference (arrayIndex);
}

Array<var>* var::convertToArray()
{
	Array<var>* array = getArray();

	if (array == nullptr)
	{
		const Array<var> tempVar;
		var v (tempVar);
		array = v.value.arrayValue;

		if (! isVoid())
			array->add (*this);

		swapWith (v);
	}

	return array;
}

void var::append (const var& n)
{
	convertToArray()->add (n);
}

void var::remove (const int index)
{
	Array<var>* const array = getArray();

	if (array != nullptr)
		array->remove (index);
}

void var::insert (const int index, const var& n)
{
	convertToArray()->insert (index, n);
}

void var::resize (const int numArrayElementsWanted)
{
	convertToArray()->resize (numArrayElementsWanted);
}

int var::indexOf (const var& n) const
{
	const Array<var>* const array = getArray();
	return array != nullptr ? array->indexOf (n) : -1;
}

void var::writeToStream (OutputStream& output) const
{
	type->writeToStream (value, output);
}

var var::readFromStream (InputStream& input)
{
	const int numBytes = input.readCompressedInt();

	if (numBytes > 0)
	{
		switch (input.readByte())
		{
			case varMarker_Int:         return var (input.readInt());
			case varMarker_Int64:       return var (input.readInt64());
			case varMarker_BoolTrue:    return var (true);
			case varMarker_BoolFalse:   return var (false);
			case varMarker_Double:      return var (input.readDouble());
			case varMarker_String:
			{
				MemoryOutputStream mo;
				mo.writeFromInputStream (input, numBytes - 1);
				return var (mo.toUTF8());
			}

			case varMarker_Array:
			{
				var v;
				Array<var>* const destArray = v.convertToArray();

				for (int i = input.readCompressedInt(); --i >= 0;)
					destArray->add (readFromStream (input));

				return v;
			}

			default:
				input.skipNextBytes (numBytes - 1); break;
		}
	}

	return var::null;
}

/*** End of inlined file: juce_Variant.cpp ***/


/*** Start of inlined file: juce_DirectoryIterator.cpp ***/
DirectoryIterator::DirectoryIterator (const File& directory,
									  bool isRecursive_,
									  const String& wildCard_,
									  const int whatToLookFor_)
  : fileFinder (directory, isRecursive_ ? "*" : wildCard_),
	wildCard (wildCard_),
	path (File::addTrailingSeparator (directory.getFullPathName())),
	index (-1),
	totalNumFiles (-1),
	whatToLookFor (whatToLookFor_),
	isRecursive (isRecursive_),
	hasBeenAdvanced (false)
{
	// you have to specify the type of files you're looking for!
	jassert ((whatToLookFor_ & (File::findFiles | File::findDirectories)) != 0);
	jassert (whatToLookFor_ > 0 && whatToLookFor_ <= 7);
}

DirectoryIterator::~DirectoryIterator()
{
}

bool DirectoryIterator::next()
{
	return next (nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
}

bool DirectoryIterator::next (bool* const isDirResult, bool* const isHiddenResult, int64* const fileSize,
							  Time* const modTime, Time* const creationTime, bool* const isReadOnly)
{
	hasBeenAdvanced = true;

	if (subIterator != nullptr)
	{
		if (subIterator->next (isDirResult, isHiddenResult, fileSize, modTime, creationTime, isReadOnly))
			return true;

		subIterator = nullptr;
	}

	String filename;
	bool isDirectory, isHidden;
	while (fileFinder.next (filename, &isDirectory, &isHidden, fileSize, modTime, creationTime, isReadOnly))
	{
		++index;

		if (! filename.containsOnly ("."))
		{
			bool matches = false;

			if (isDirectory)
			{
				if (isRecursive && ((whatToLookFor & File::ignoreHiddenFiles) == 0 || ! isHidden))
					subIterator = new DirectoryIterator (File::createFileWithoutCheckingPath (path + filename),
														 true, wildCard, whatToLookFor);

				matches = (whatToLookFor & File::findDirectories) != 0;
			}
			else
			{
				matches = (whatToLookFor & File::findFiles) != 0;
			}

			// if recursive, we're not relying on the OS iterator to do the wildcard match, so do it now..
			if (matches && isRecursive)
				matches = filename.matchesWildcard (wildCard, ! File::areFileNamesCaseSensitive());

			if (matches && (whatToLookFor & File::ignoreHiddenFiles) != 0)
				matches = ! isHidden;

			if (matches)
			{
				currentFile = File::createFileWithoutCheckingPath (path + filename);
				if (isHiddenResult != nullptr)     *isHiddenResult = isHidden;
				if (isDirResult != nullptr)        *isDirResult = isDirectory;

				return true;
			}
			else if (subIterator != nullptr)
			{
				return next();
			}
		}
	}

	return false;
}

const File& DirectoryIterator::getFile() const
{
	if (subIterator != nullptr && subIterator->hasBeenAdvanced)
		return subIterator->getFile();

	// You need to call DirectoryIterator::next() before asking it for the file that it found!
	jassert (hasBeenAdvanced);

	return currentFile;
}

float DirectoryIterator::getEstimatedProgress() const
{
	if (totalNumFiles < 0)
		totalNumFiles = File (path).getNumberOfChildFiles (File::findFilesAndDirectories);

	if (totalNumFiles <= 0)
		return 0.0f;

	const float detailedIndex = (subIterator != nullptr) ? index + subIterator->getEstimatedProgress()
														 : (float) index;

	return detailedIndex / totalNumFiles;
}

/*** End of inlined file: juce_DirectoryIterator.cpp ***/


/*** Start of inlined file: juce_File.cpp ***/
File::File (const String& fullPathName)
	: fullPath (parseAbsolutePath (fullPathName))
{
}

File File::createFileWithoutCheckingPath (const String& path)
{
	File f;
	f.fullPath = path;
	return f;
}

File::File (const File& other)
	: fullPath (other.fullPath)
{
}

File& File::operator= (const String& newPath)
{
	fullPath = parseAbsolutePath (newPath);
	return *this;
}

File& File::operator= (const File& other)
{
	fullPath = other.fullPath;
	return *this;
}

#if JUCE_COMPILER_SUPPORTS_MOVE_SEMANTICS
File::File (File&& other) noexcept
	: fullPath (static_cast <String&&> (other.fullPath))
{
}

File& File::operator= (File&& other) noexcept
{
	fullPath = static_cast <String&&> (other.fullPath);
	return *this;
}
#endif

const File File::nonexistent;

String File::parseAbsolutePath (const String& p)
{
	if (p.isEmpty())
		return String::empty;

#if JUCE_WINDOWS
	// Windows..
	String path (p.replaceCharacter ('/', '\\'));

	if (path.startsWithChar (separator))
	{
		if (path[1] != separator)
		{
			/*  When you supply a raw string to the File object constructor, it must be an absolute path.
				If you're trying to parse a string that may be either a relative path or an absolute path,
				you MUST provide a context against which the partial path can be evaluated - you can do
				this by simply using File::getChildFile() instead of the File constructor. E.g. saying
				"File::getCurrentWorkingDirectory().getChildFile (myUnknownPath)" would return an absolute
				path if that's what was supplied, or would evaluate a partial path relative to the CWD.
			*/
			jassertfalse;

			path = File::getCurrentWorkingDirectory().getFullPathName().substring (0, 2) + path;
		}
	}
	else if (! path.containsChar (':'))
	{
		/*  When you supply a raw string to the File object constructor, it must be an absolute path.
			If you're trying to parse a string that may be either a relative path or an absolute path,
			you MUST provide a context against which the partial path can be evaluated - you can do
			this by simply using File::getChildFile() instead of the File constructor. E.g. saying
			"File::getCurrentWorkingDirectory().getChildFile (myUnknownPath)" would return an absolute
			path if that's what was supplied, or would evaluate a partial path relative to the CWD.
		*/
		jassertfalse;

		return File::getCurrentWorkingDirectory().getChildFile (path).getFullPathName();
	}
#else
	// Mac or Linux..

	// Yes, I know it's legal for a unix pathname to contain a backslash, but this assertion is here
	// to catch anyone who's trying to run code that was written on Windows with hard-coded path names.
	// If that's why you've ended up here, use File::getChildFile() to build your paths instead.
	jassert ((! p.containsChar ('\\')) || (p.indexOfChar ('/') >= 0 && p.indexOfChar ('/') < p.indexOfChar ('\\')));

	String path (p);

	if (path.startsWithChar ('~'))
	{
		if (path[1] == separator || path[1] == 0)
		{
			// expand a name of the form "~/abc"
			path = File::getSpecialLocation (File::userHomeDirectory).getFullPathName()
					+ path.substring (1);
		}
		else
		{
			// expand a name of type "~dave/abc"
			const String userName (path.substring (1).upToFirstOccurrenceOf ("/", false, false));

			struct passwd* const pw = getpwnam (userName.toUTF8());
			if (pw != nullptr)
				path = addTrailingSeparator (pw->pw_dir) + path.fromFirstOccurrenceOf ("/", false, false);
		}
	}
	else if (! path.startsWithChar (separator))
	{
		/*  When you supply a raw string to the File object constructor, it must be an absolute path.
			If you're trying to parse a string that may be either a relative path or an absolute path,
			you MUST provide a context against which the partial path can be evaluated - you can do
			this by simply using File::getChildFile() instead of the File constructor. E.g. saying
			"File::getCurrentWorkingDirectory().getChildFile (myUnknownPath)" would return an absolute
			path if that's what was supplied, or would evaluate a partial path relative to the CWD.
		*/
		jassert (path.startsWith ("./") || path.startsWith ("../")); // (assume that a path "./xyz" is deliberately intended to be relative to the CWD)

		return File::getCurrentWorkingDirectory().getChildFile (path).getFullPathName();
	}
#endif

	while (path.endsWithChar (separator) && path != separatorString) // careful not to turn a single "/" into an empty string.
		path = path.dropLastCharacters (1);

	return path;
}

String File::addTrailingSeparator (const String& path)
{
	return path.endsWithChar (separator) ? path
										 : path + separator;
}

#if JUCE_LINUX
 #define NAMES_ARE_CASE_SENSITIVE 1
#endif

bool File::areFileNamesCaseSensitive()
{
   #if NAMES_ARE_CASE_SENSITIVE
	return true;
   #else
	return false;
   #endif
}

bool File::operator== (const File& other) const
{
   #if NAMES_ARE_CASE_SENSITIVE
	return fullPath == other.fullPath;
   #else
	return fullPath.equalsIgnoreCase (other.fullPath);
   #endif
}

bool File::operator!= (const File& other) const
{
	return ! operator== (other);
}

bool File::operator< (const File& other) const
{
   #if NAMES_ARE_CASE_SENSITIVE
	return fullPath < other.fullPath;
   #else
	return fullPath.compareIgnoreCase (other.fullPath) < 0;
   #endif
}

bool File::operator> (const File& other) const
{
   #if NAMES_ARE_CASE_SENSITIVE
	return fullPath > other.fullPath;
   #else
	return fullPath.compareIgnoreCase (other.fullPath) > 0;
   #endif
}

bool File::setReadOnly (const bool shouldBeReadOnly,
						const bool applyRecursively) const
{
	bool worked = true;

	if (applyRecursively && isDirectory())
	{
		Array <File> subFiles;
		findChildFiles (subFiles, File::findFilesAndDirectories, false);

		for (int i = subFiles.size(); --i >= 0;)
			worked = subFiles.getReference(i).setReadOnly (shouldBeReadOnly, true) && worked;
	}

	return setFileReadOnlyInternal (shouldBeReadOnly) && worked;
}

bool File::deleteRecursively() const
{
	bool worked = true;

	if (isDirectory())
	{
		Array<File> subFiles;
		findChildFiles (subFiles, File::findFilesAndDirectories, false);

		for (int i = subFiles.size(); --i >= 0;)
			worked = subFiles.getReference(i).deleteRecursively() && worked;
	}

	return deleteFile() && worked;
}

bool File::moveFileTo (const File& newFile) const
{
	if (newFile.fullPath == fullPath)
		return true;

   #if ! NAMES_ARE_CASE_SENSITIVE
	if (*this != newFile)
   #endif
		if (! newFile.deleteFile())
			return false;

	return moveInternal (newFile);
}

bool File::copyFileTo (const File& newFile) const
{
	return (*this == newFile)
			|| (exists() && newFile.deleteFile() && copyInternal (newFile));
}

bool File::copyDirectoryTo (const File& newDirectory) const
{
	if (isDirectory() && newDirectory.createDirectory())
	{
		Array<File> subFiles;
		findChildFiles (subFiles, File::findFiles, false);

		int i;
		for (i = 0; i < subFiles.size(); ++i)
			if (! subFiles.getReference(i).copyFileTo (newDirectory.getChildFile (subFiles.getReference(i).getFileName())))
				return false;

		subFiles.clear();
		findChildFiles (subFiles, File::findDirectories, false);

		for (i = 0; i < subFiles.size(); ++i)
			if (! subFiles.getReference(i).copyDirectoryTo (newDirectory.getChildFile (subFiles.getReference(i).getFileName())))
				return false;

		return true;
	}

	return false;
}

String File::getPathUpToLastSlash() const
{
	const int lastSlash = fullPath.lastIndexOfChar (separator);

	if (lastSlash > 0)
		return fullPath.substring (0, lastSlash);
	else if (lastSlash == 0)
		return separatorString;
	else
		return fullPath;
}

File File::getParentDirectory() const
{
	File f;
	f.fullPath = getPathUpToLastSlash();
	return f;
}

String File::getFileName() const
{
	return fullPath.substring (fullPath.lastIndexOfChar (separator) + 1);
}

int File::hashCode() const
{
	return fullPath.hashCode();
}

int64 File::hashCode64() const
{
	return fullPath.hashCode64();
}

String File::getFileNameWithoutExtension() const
{
	const int lastSlash = fullPath.lastIndexOfChar (separator) + 1;
	const int lastDot = fullPath.lastIndexOfChar ('.');

	if (lastDot > lastSlash)
		return fullPath.substring (lastSlash, lastDot);
	else
		return fullPath.substring (lastSlash);
}

bool File::isAChildOf (const File& potentialParent) const
{
	if (potentialParent == File::nonexistent)
		return false;

	const String ourPath (getPathUpToLastSlash());

   #if NAMES_ARE_CASE_SENSITIVE
	if (potentialParent.fullPath == ourPath)
   #else
	if (potentialParent.fullPath.equalsIgnoreCase (ourPath))
   #endif
	{
		return true;
	}
	else if (potentialParent.fullPath.length() >= ourPath.length())
	{
		return false;
	}
	else
	{
		return getParentDirectory().isAChildOf (potentialParent);
	}
}

bool File::isAbsolutePath (const String& path)
{
	return path.startsWithChar (separator)
		   #if JUCE_WINDOWS
			|| (path.isNotEmpty() && path[1] == ':');
		   #else
			|| path.startsWithChar ('~');
		   #endif
}

File File::getChildFile (String relativePath) const
{
	if (isAbsolutePath (relativePath))
		return File (relativePath);

	String path (fullPath);

	// It's relative, so remove any ../ or ./ bits at the start..
	if (relativePath[0] == '.')
	{
	   #if JUCE_WINDOWS
		relativePath = relativePath.replaceCharacter ('/', '\\').trimStart();
	   #else
		relativePath = relativePath.trimStart();
	   #endif

		while (relativePath[0] == '.')
		{
			const juce_wchar secondChar = relativePath[1];

			if (secondChar == '.')
			{
				const juce_wchar thirdChar = relativePath[2];

				if (thirdChar == 0 || thirdChar == separator)
				{
					const int lastSlash = path.lastIndexOfChar (separator);
					if (lastSlash >= 0)
						path = path.substring (0, lastSlash);

					relativePath = relativePath.substring (3);
				}
				else
				{
					break;
				}
			}
			else if (secondChar == separator)
			{
				relativePath = relativePath.substring (2);
			}
			else
			{
				break;
			}
		}
	}

	return File (addTrailingSeparator (path) + relativePath);
}

File File::getSiblingFile (const String& fileName) const
{
	return getParentDirectory().getChildFile (fileName);
}

String File::descriptionOfSizeInBytes (const int64 bytes)
{
	if (bytes == 1)                       return "1 byte";
	else if (bytes < 1024)                return String (bytes) + " bytes";
	else if (bytes < 1024 * 1024)         return String (bytes / 1024.0, 1) + " KB";
	else if (bytes < 1024 * 1024 * 1024)  return String (bytes / (1024.0 * 1024.0), 1) + " MB";
	else                                  return String (bytes / (1024.0 * 1024.0 * 1024.0), 1) + " GB";
}

Result File::create() const
{
	if (exists())
		return Result::ok();

	const File parentDir (getParentDirectory());

	if (parentDir == *this)
		return Result::fail ("Cannot create parent directory");

	Result r (parentDir.createDirectory());

	if (r.wasOk())
	{
		FileOutputStream fo (*this, 8);

		r = fo.getStatus();
	}

	return r;
}

Result File::createDirectory() const
{
	if (isDirectory())
		return Result::ok();

	const File parentDir (getParentDirectory());

	if (parentDir == *this)
		return Result::fail ("Cannot create parent directory");

	Result r (parentDir.createDirectory());

	if (r.wasOk())
		r = createDirectoryInternal (fullPath.trimCharactersAtEnd (separatorString));

	return r;
}

Time File::getLastModificationTime() const                  { int64 m, a, c; getFileTimesInternal (m, a, c); return Time (m); }
Time File::getLastAccessTime() const                        { int64 m, a, c; getFileTimesInternal (m, a, c); return Time (a); }
Time File::getCreationTime() const                          { int64 m, a, c; getFileTimesInternal (m, a, c); return Time (c); }

bool File::setLastModificationTime (const Time& t) const    { return setFileTimesInternal (t.toMilliseconds(), 0, 0); }
bool File::setLastAccessTime (const Time& t) const          { return setFileTimesInternal (0, t.toMilliseconds(), 0); }
bool File::setCreationTime (const Time& t) const            { return setFileTimesInternal (0, 0, t.toMilliseconds()); }

bool File::loadFileAsData (MemoryBlock& destBlock) const
{
	if (! existsAsFile())
		return false;

	FileInputStream in (*this);
	return in.openedOk() && getSize() == in.readIntoMemoryBlock (destBlock);
}

String File::loadFileAsString() const
{
	if (! existsAsFile())
		return String::empty;

	FileInputStream in (*this);
	return in.openedOk() ? in.readEntireStreamAsString()
						 : String::empty;
}

void File::readLines (StringArray& destLines) const
{
	destLines.addLines (loadFileAsString());
}

int File::findChildFiles (Array<File>& results,
						  const int whatToLookFor,
						  const bool searchRecursively,
						  const String& wildCardPattern) const
{
	DirectoryIterator di (*this, searchRecursively, wildCardPattern, whatToLookFor);

	int total = 0;
	while (di.next())
	{
		results.add (di.getFile());
		++total;
	}

	return total;
}

int File::getNumberOfChildFiles (const int whatToLookFor, const String& wildCardPattern) const
{
	DirectoryIterator di (*this, false, wildCardPattern, whatToLookFor);

	int total = 0;
	while (di.next())
		++total;

	return total;
}

bool File::containsSubDirectories() const
{
	if (isDirectory())
	{
		DirectoryIterator di (*this, false, "*", findDirectories);
		return di.next();
	}

	return false;
}

File File::getNonexistentChildFile (const String& prefix_,
									const String& suffix,
									bool putNumbersInBrackets) const
{
	File f (getChildFile (prefix_ + suffix));

	if (f.exists())
	{
		int num = 2;
		String prefix (prefix_);

		// remove any bracketed numbers that may already be on the end..
		if (prefix.trim().endsWithChar (')'))
		{
			putNumbersInBrackets = true;

			const int openBracks  = prefix.lastIndexOfChar ('(');
			const int closeBracks = prefix.lastIndexOfChar (')');

			if (openBracks > 0
				 && closeBracks > openBracks
				 && prefix.substring (openBracks + 1, closeBracks).containsOnly ("0123456789"))
			{
				num = prefix.substring (openBracks + 1, closeBracks).getIntValue() + 1;
				prefix = prefix.substring (0, openBracks);
			}
		}

		// also use brackets if it ends in a digit.
		putNumbersInBrackets = putNumbersInBrackets
								|| CharacterFunctions::isDigit (prefix.getLastCharacter());

		do
		{
			String newName (prefix);

			if (putNumbersInBrackets)
				newName << '(' << num++ << ')';
			else
				newName << num++;

			f = getChildFile (newName + suffix);

		} while (f.exists());
	}

	return f;
}

File File::getNonexistentSibling (const bool putNumbersInBrackets) const
{
	if (exists())
		return getParentDirectory()
				.getNonexistentChildFile (getFileNameWithoutExtension(),
										  getFileExtension(),
										  putNumbersInBrackets);
	return *this;
}

String File::getFileExtension() const
{
	const int indexOfDot = fullPath.lastIndexOfChar ('.');

	if (indexOfDot > fullPath.lastIndexOfChar (separator))
		return fullPath.substring (indexOfDot);

	return String::empty;
}

bool File::hasFileExtension (const String& possibleSuffix) const
{
	if (possibleSuffix.isEmpty())
		return fullPath.lastIndexOfChar ('.') <= fullPath.lastIndexOfChar (separator);

	const int semicolon = possibleSuffix.indexOfChar (0, ';');

	if (semicolon >= 0)
	{
		return hasFileExtension (possibleSuffix.substring (0, semicolon).trimEnd())
				|| hasFileExtension (possibleSuffix.substring (semicolon + 1).trimStart());
	}
	else
	{
		if (fullPath.endsWithIgnoreCase (possibleSuffix))
		{
			if (possibleSuffix.startsWithChar ('.'))
				return true;

			const int dotPos = fullPath.length() - possibleSuffix.length() - 1;

			if (dotPos >= 0)
				return fullPath [dotPos] == '.';
		}
	}

	return false;
}

File File::withFileExtension (const String& newExtension) const
{
	if (fullPath.isEmpty())
		return File::nonexistent;

	String filePart (getFileName());

	int i = filePart.lastIndexOfChar ('.');
	if (i >= 0)
		filePart = filePart.substring (0, i);

	if (newExtension.isNotEmpty() && ! newExtension.startsWithChar ('.'))
		filePart << '.';

	return getSiblingFile (filePart + newExtension);
}

bool File::startAsProcess (const String& parameters) const
{
	return exists() && Process::openDocument (fullPath, parameters);
}

FileInputStream* File::createInputStream() const
{
	if (existsAsFile())
		return new FileInputStream (*this);

	return nullptr;
}

FileOutputStream* File::createOutputStream (const int bufferSize) const
{
	ScopedPointer<FileOutputStream> out (new FileOutputStream (*this, bufferSize));

	return out->failedToOpen() ? nullptr
							   : out.release();
}

bool File::appendData (const void* const dataToAppend,
					   const int numberOfBytes) const
{
	if (numberOfBytes <= 0)
		return true;

	FileOutputStream out (*this, 8192);
	return out.openedOk() && out.write (dataToAppend, numberOfBytes);
}

bool File::replaceWithData (const void* const dataToWrite,
							const int numberOfBytes) const
{
	jassert (numberOfBytes >= 0); // a negative number of bytes??

	if (numberOfBytes <= 0)
		return deleteFile();

	TemporaryFile tempFile (*this, TemporaryFile::useHiddenFile);
	tempFile.getFile().appendData (dataToWrite, numberOfBytes);
	return tempFile.overwriteTargetFileWithTemporary();
}

bool File::appendText (const String& text,
					   const bool asUnicode,
					   const bool writeUnicodeHeaderBytes) const
{
	FileOutputStream out (*this);

	if (out.failedToOpen())
		return false;

	out.writeText (text, asUnicode, writeUnicodeHeaderBytes);
	return true;
}

bool File::replaceWithText (const String& textToWrite,
							const bool asUnicode,
							const bool writeUnicodeHeaderBytes) const
{
	TemporaryFile tempFile (*this, TemporaryFile::useHiddenFile);
	tempFile.getFile().appendText (textToWrite, asUnicode, writeUnicodeHeaderBytes);
	return tempFile.overwriteTargetFileWithTemporary();
}

bool File::hasIdenticalContentTo (const File& other) const
{
	if (other == *this)
		return true;

	if (getSize() == other.getSize() && existsAsFile() && other.existsAsFile())
	{
		FileInputStream in1 (*this), in2 (other);

		if (in1.openedOk() && in2.openedOk())
		{
			const int bufferSize = 4096;
			HeapBlock <char> buffer1 (bufferSize), buffer2 (bufferSize);

			for (;;)
			{
				const int num1 = in1.read (buffer1, bufferSize);
				const int num2 = in2.read (buffer2, bufferSize);

				if (num1 != num2)
					break;

				if (num1 <= 0)
					return true;

				if (memcmp (buffer1, buffer2, (size_t) num1) != 0)
					break;
			}
		}
	}

	return false;
}

String File::createLegalPathName (const String& original)
{
	String s (original);
	String start;

	if (s[1] == ':')
	{
		start = s.substring (0, 2);
		s = s.substring (2);
	}

	return start + s.removeCharacters ("\"#@,;:<>*^|?")
					.substring (0, 1024);
}

String File::createLegalFileName (const String& original)
{
	String s (original.removeCharacters ("\"#@,;:<>*^|?\\/"));

	const int maxLength = 128; // only the length of the filename, not the whole path
	const int len = s.length();

	if (len > maxLength)
	{
		const int lastDot = s.lastIndexOfChar ('.');

		if (lastDot > jmax (0, len - 12))
		{
			s = s.substring (0, maxLength - (len - lastDot))
				 + s.substring (lastDot);
		}
		else
		{
			s = s.substring (0, maxLength);
		}
	}

	return s;
}

static int countNumberOfSeparators (String::CharPointerType s)
{
	int num = 0;

	for (;;)
	{
		const juce_wchar c = s.getAndAdvance();

		if (c == 0)
			break;

		if (c == File::separator)
			++num;
	}

	return num;
}

String File::getRelativePathFrom (const File& dir)  const
{
	String thisPath (fullPath);

	while (thisPath.endsWithChar (separator))
		thisPath = thisPath.dropLastCharacters (1);

	String dirPath (addTrailingSeparator (dir.existsAsFile() ? dir.getParentDirectory().getFullPathName()
															 : dir.fullPath));

	int commonBitLength = 0;
	String::CharPointerType thisPathAfterCommon (thisPath.getCharPointer());
	String::CharPointerType dirPathAfterCommon  (dirPath.getCharPointer());

	{
		String::CharPointerType thisPathIter (thisPath.getCharPointer());
		String::CharPointerType dirPathIter  (dirPath.getCharPointer());

		for (int i = 0;;)
		{
			const juce_wchar c1 = thisPathIter.getAndAdvance();
			const juce_wchar c2 = dirPathIter.getAndAdvance();

		   #if NAMES_ARE_CASE_SENSITIVE
			if (c1 != c2
		   #else
			if ((c1 != c2 && CharacterFunctions::toLowerCase (c1) != CharacterFunctions::toLowerCase (c2))
		   #endif
				 || c1 == 0)
				break;

			++i;

			if (c1 == separator)
			{
				thisPathAfterCommon = thisPathIter;
				dirPathAfterCommon  = dirPathIter;
				commonBitLength = i;
			}
		}
	}

	// if the only common bit is the root, then just return the full path..
	if (commonBitLength == 0 || (commonBitLength == 1 && thisPath[1] == separator))
		return fullPath;

	const int numUpDirectoriesNeeded = countNumberOfSeparators (dirPathAfterCommon);

	if (numUpDirectoriesNeeded == 0)
		return thisPathAfterCommon;

   #if JUCE_WINDOWS
	String s (String::repeatedString ("..\\", numUpDirectoriesNeeded));
   #else
	String s (String::repeatedString ("../",  numUpDirectoriesNeeded));
   #endif
	s.appendCharPointer (thisPathAfterCommon);
	return s;
}

File File::createTempFile (const String& fileNameEnding)
{
	const File tempFile (getSpecialLocation (tempDirectory)
							.getChildFile ("temp_" + String::toHexString (Random::getSystemRandom().nextInt()))
							.withFileExtension (fileNameEnding));

	if (tempFile.exists())
		return createTempFile (fileNameEnding);

	return tempFile;
}

#if JUCE_UNIT_TESTS

class FileTests  : public UnitTest
{
public:
	FileTests() : UnitTest ("Files") {}

	void runTest()
	{
		beginTest ("Reading");

		const File home (File::getSpecialLocation (File::userHomeDirectory));
		const File temp (File::getSpecialLocation (File::tempDirectory));

		expect (! File::nonexistent.exists());
		expect (home.isDirectory());
		expect (home.exists());
		expect (! home.existsAsFile());
		expect (File::getSpecialLocation (File::userDocumentsDirectory).isDirectory());
		expect (File::getSpecialLocation (File::userApplicationDataDirectory).isDirectory());
		expect (File::getSpecialLocation (File::currentExecutableFile).exists());
		expect (File::getSpecialLocation (File::currentApplicationFile).exists());
		expect (File::getSpecialLocation (File::invokedExecutableFile).exists());
		expect (home.getVolumeTotalSize() > 1024 * 1024);
		expect (home.getBytesFreeOnVolume() > 0);
		expect (! home.isHidden());
		expect (home.isOnHardDisk());
		expect (! home.isOnCDRomDrive());
		expect (File::getCurrentWorkingDirectory().exists());
		expect (home.setAsCurrentWorkingDirectory());
		expect (File::getCurrentWorkingDirectory() == home);

		{
			Array<File> roots;
			File::findFileSystemRoots (roots);
			expect (roots.size() > 0);

			int numRootsExisting = 0;
			for (int i = 0; i < roots.size(); ++i)
				if (roots[i].exists())
					++numRootsExisting;

			// (on windows, some of the drives may not contain media, so as long as at least one is ok..)
			expect (numRootsExisting > 0);
		}

		beginTest ("Writing");

		File demoFolder (temp.getChildFile ("Juce UnitTests Temp Folder.folder"));
		expect (demoFolder.deleteRecursively());
		expect (demoFolder.createDirectory());
		expect (demoFolder.isDirectory());
		expect (demoFolder.getParentDirectory() == temp);
		expect (temp.isDirectory());

		{
			Array<File> files;
			temp.findChildFiles (files, File::findFilesAndDirectories, false, "*");
			expect (files.contains (demoFolder));
		}

		{
			Array<File> files;
			temp.findChildFiles (files, File::findDirectories, true, "*.folder");
			expect (files.contains (demoFolder));
		}

		File tempFile (demoFolder.getNonexistentChildFile ("test", ".txt", false));

		expect (tempFile.getFileExtension() == ".txt");
		expect (tempFile.hasFileExtension (".txt"));
		expect (tempFile.hasFileExtension ("txt"));
		expect (tempFile.withFileExtension ("xyz").hasFileExtension (".xyz"));
		expect (tempFile.withFileExtension ("xyz").hasFileExtension ("abc;xyz;foo"));
		expect (tempFile.withFileExtension ("xyz").hasFileExtension ("xyz;foo"));
		expect (! tempFile.withFileExtension ("h").hasFileExtension ("bar;foo;xx"));
		expect (tempFile.getSiblingFile ("foo").isAChildOf (temp));
		expect (tempFile.hasWriteAccess());

		{
			FileOutputStream fo (tempFile);
			fo.write ("0123456789", 10);
		}

		expect (tempFile.exists());
		expect (tempFile.getSize() == 10);
		expect (std::abs ((int) (tempFile.getLastModificationTime().toMilliseconds() - Time::getCurrentTime().toMilliseconds())) < 3000);
		expectEquals (tempFile.loadFileAsString(), String ("0123456789"));
		expect (! demoFolder.containsSubDirectories());

		expectEquals (tempFile.getRelativePathFrom (demoFolder.getParentDirectory()), demoFolder.getFileName() + File::separatorString + tempFile.getFileName());
		expectEquals (demoFolder.getParentDirectory().getRelativePathFrom (tempFile), ".." + File::separatorString + ".." + File::separatorString + demoFolder.getParentDirectory().getFileName());

		expect (demoFolder.getNumberOfChildFiles (File::findFiles) == 1);
		expect (demoFolder.getNumberOfChildFiles (File::findFilesAndDirectories) == 1);
		expect (demoFolder.getNumberOfChildFiles (File::findDirectories) == 0);
		demoFolder.getNonexistentChildFile ("tempFolder", "", false).createDirectory();
		expect (demoFolder.getNumberOfChildFiles (File::findDirectories) == 1);
		expect (demoFolder.getNumberOfChildFiles (File::findFilesAndDirectories) == 2);
		expect (demoFolder.containsSubDirectories());

		expect (tempFile.hasWriteAccess());
		tempFile.setReadOnly (true);
		expect (! tempFile.hasWriteAccess());
		tempFile.setReadOnly (false);
		expect (tempFile.hasWriteAccess());

		Time t (Time::getCurrentTime());
		tempFile.setLastModificationTime (t);
		Time t2 = tempFile.getLastModificationTime();
		expect (std::abs ((int) (t2.toMilliseconds() - t.toMilliseconds())) <= 1000);

		{
			MemoryBlock mb;
			tempFile.loadFileAsData (mb);
			expect (mb.getSize() == 10);
			expect (mb[0] == '0');
		}

		{
			expect (tempFile.getSize() == 10);
			FileOutputStream fo (tempFile);
			expect (fo.openedOk());

			expect (fo.setPosition  (7));
			expect (fo.truncate().wasOk());
			expect (tempFile.getSize() == 7);
			fo.write ("789", 3);
			fo.flush();
			expect (tempFile.getSize() == 10);
		}

		beginTest ("Memory-mapped files");

		{
			MemoryMappedFile mmf (tempFile, MemoryMappedFile::readOnly);
			expect (mmf.getSize() == 10);
			expect (mmf.getData() != nullptr);
			expect (memcmp (mmf.getData(), "0123456789", 10) == 0);
		}

		{
			const File tempFile2 (tempFile.getNonexistentSibling (false));
			expect (tempFile2.create());
			expect (tempFile2.appendData ("xxxxxxxxxx", 10));

			{
				MemoryMappedFile mmf (tempFile2, MemoryMappedFile::readWrite);
				expect (mmf.getSize() == 10);
				expect (mmf.getData() != nullptr);
				memcpy (mmf.getData(), "abcdefghij", 10);
			}

			{
				MemoryMappedFile mmf (tempFile2, MemoryMappedFile::readWrite);
				expect (mmf.getSize() == 10);
				expect (mmf.getData() != nullptr);
				expect (memcmp (mmf.getData(), "abcdefghij", 10) == 0);
			}

			expect (tempFile2.deleteFile());
		}

		beginTest ("More writing");

		expect (tempFile.appendData ("abcdefghij", 10));
		expect (tempFile.getSize() == 20);
		expect (tempFile.replaceWithData ("abcdefghij", 10));
		expect (tempFile.getSize() == 10);

		File tempFile2 (tempFile.getNonexistentSibling (false));
		expect (tempFile.copyFileTo (tempFile2));
		expect (tempFile2.exists());
		expect (tempFile2.hasIdenticalContentTo (tempFile));
		expect (tempFile.deleteFile());
		expect (! tempFile.exists());
		expect (tempFile2.moveFileTo (tempFile));
		expect (tempFile.exists());
		expect (! tempFile2.exists());

		expect (demoFolder.deleteRecursively());
		expect (! demoFolder.exists());
	}
};

static FileTests fileUnitTests;

#endif

/*** End of inlined file: juce_File.cpp ***/


/*** Start of inlined file: juce_FileInputStream.cpp ***/
int64 juce_fileSetPosition (void* handle, int64 pos);

FileInputStream::FileInputStream (const File& f)
	: file (f),
	  fileHandle (nullptr),
	  currentPosition (0),
	  totalSize (0),
	  status (Result::ok()),
	  needToSeek (true)
{
	openHandle();
}

FileInputStream::~FileInputStream()
{
	closeHandle();
}

int64 FileInputStream::getTotalLength()
{
	return totalSize;
}

int FileInputStream::read (void* buffer, int bytesToRead)
{
	jassert (openedOk());
	jassert (buffer != nullptr && bytesToRead >= 0);

	if (needToSeek)
	{
		if (juce_fileSetPosition (fileHandle, currentPosition) < 0)
			return 0;

		needToSeek = false;
	}

	const size_t num = readInternal (buffer, (size_t) bytesToRead);
	currentPosition += num;

	return (int) num;
}

bool FileInputStream::isExhausted()
{
	return currentPosition >= totalSize;
}

int64 FileInputStream::getPosition()
{
	return currentPosition;
}

bool FileInputStream::setPosition (int64 pos)
{
	jassert (openedOk());
	pos = jlimit ((int64) 0, totalSize, pos);

	needToSeek |= (currentPosition != pos);
	currentPosition = pos;

	return true;
}

/*** End of inlined file: juce_FileInputStream.cpp ***/


/*** Start of inlined file: juce_FileOutputStream.cpp ***/
int64 juce_fileSetPosition (void* handle, int64 pos);

FileOutputStream::FileOutputStream (const File& f, const int bufferSize_)
	: file (f),
	  fileHandle (nullptr),
	  status (Result::ok()),
	  currentPosition (0),
	  bufferSize (bufferSize_),
	  bytesInBuffer (0),
	  buffer ((size_t) jmax (bufferSize_, 16))
{
	openHandle();
}

FileOutputStream::~FileOutputStream()
{
	flushBuffer();
	flushInternal();
	closeHandle();
}

int64 FileOutputStream::getPosition()
{
	return currentPosition;
}

bool FileOutputStream::setPosition (int64 newPosition)
{
	if (newPosition != currentPosition)
	{
		flushBuffer();
		currentPosition = juce_fileSetPosition (fileHandle, newPosition);
	}

	return newPosition == currentPosition;
}

bool FileOutputStream::flushBuffer()
{
	bool ok = true;

	if (bytesInBuffer > 0)
	{
		ok = (writeInternal (buffer, bytesInBuffer) == bytesInBuffer);
		bytesInBuffer = 0;
	}

	return ok;
}

void FileOutputStream::flush()
{
	flushBuffer();
	flushInternal();
}

bool FileOutputStream::write (const void* const src, const int numBytes)
{
	jassert (src != nullptr && numBytes >= 0);

	if (bytesInBuffer + numBytes < bufferSize)
	{
		memcpy (buffer + bytesInBuffer, src, (size_t) numBytes);
		bytesInBuffer += numBytes;
		currentPosition += numBytes;
	}
	else
	{
		if (! flushBuffer())
			return false;

		if (numBytes < bufferSize)
		{
			memcpy (buffer + bytesInBuffer, src, (size_t) numBytes);
			bytesInBuffer += numBytes;
			currentPosition += numBytes;
		}
		else
		{
			const int bytesWritten = writeInternal (src, numBytes);

			if (bytesWritten < 0)
				return false;

			currentPosition += bytesWritten;
			return bytesWritten == numBytes;
		}
	}

	return true;
}

void FileOutputStream::writeRepeatedByte (uint8 byte, int numBytes)
{
	jassert (numBytes >= 0);

	if (bytesInBuffer + numBytes < bufferSize)
	{
		memset (buffer + bytesInBuffer, byte, (size_t) numBytes);
		bytesInBuffer += numBytes;
		currentPosition += numBytes;
	}
	else
	{
		OutputStream::writeRepeatedByte (byte, numBytes);
	}
}

/*** End of inlined file: juce_FileOutputStream.cpp ***/


/*** Start of inlined file: juce_FileSearchPath.cpp ***/
FileSearchPath::FileSearchPath()
{
}

FileSearchPath::FileSearchPath (const String& path)
{
	init (path);
}

FileSearchPath::FileSearchPath (const FileSearchPath& other)
  : directories (other.directories)
{
}

FileSearchPath::~FileSearchPath()
{
}

FileSearchPath& FileSearchPath::operator= (const String& path)
{
	init (path);
	return *this;
}

void FileSearchPath::init (const String& path)
{
	directories.clear();
	directories.addTokens (path, ";", "\"");
	directories.trim();
	directories.removeEmptyStrings();

	for (int i = directories.size(); --i >= 0;)
		directories.set (i, directories[i].unquoted());
}

int FileSearchPath::getNumPaths() const
{
	return directories.size();
}

File FileSearchPath::operator[] (const int index) const
{
	return File (directories [index]);
}

String FileSearchPath::toString() const
{
	StringArray directories2 (directories);
	for (int i = directories2.size(); --i >= 0;)
		if (directories2[i].containsChar (';'))
			directories2.set (i, directories2[i].quoted());

	return directories2.joinIntoString (";");
}

void FileSearchPath::add (const File& dir, const int insertIndex)
{
	directories.insert (insertIndex, dir.getFullPathName());
}

void FileSearchPath::addIfNotAlreadyThere (const File& dir)
{
	for (int i = 0; i < directories.size(); ++i)
		if (File (directories[i]) == dir)
			return;

	add (dir);
}

void FileSearchPath::remove (const int index)
{
	directories.remove (index);
}

void FileSearchPath::addPath (const FileSearchPath& other)
{
	for (int i = 0; i < other.getNumPaths(); ++i)
		addIfNotAlreadyThere (other[i]);
}

void FileSearchPath::removeRedundantPaths()
{
	for (int i = directories.size(); --i >= 0;)
	{
		const File d1 (directories[i]);

		for (int j = directories.size(); --j >= 0;)
		{
			const File d2 (directories[j]);

			if ((i != j) && (d1.isAChildOf (d2) || d1 == d2))
			{
				directories.remove (i);
				break;
			}
		}
	}
}

void FileSearchPath::removeNonExistentPaths()
{
	for (int i = directories.size(); --i >= 0;)
		if (! File (directories[i]).isDirectory())
			directories.remove (i);
}

int FileSearchPath::findChildFiles (Array<File>& results,
									const int whatToLookFor,
									const bool searchRecursively,
									const String& wildCardPattern) const
{
	int total = 0;

	for (int i = 0; i < directories.size(); ++i)
		total += operator[] (i).findChildFiles (results,
												whatToLookFor,
												searchRecursively,
												wildCardPattern);

	return total;
}

bool FileSearchPath::isFileInPath (const File& fileToCheck,
								   const bool checkRecursively) const
{
	for (int i = directories.size(); --i >= 0;)
	{
		const File d (directories[i]);

		if (checkRecursively)
		{
			if (fileToCheck.isAChildOf (d))
				return true;
		}
		else
		{
			if (fileToCheck.getParentDirectory() == d)
				return true;
		}
	}

	return false;
}

/*** End of inlined file: juce_FileSearchPath.cpp ***/


/*** Start of inlined file: juce_TemporaryFile.cpp ***/
TemporaryFile::TemporaryFile (const String& suffix, const int optionFlags)
{
	createTempFile (File::getSpecialLocation (File::tempDirectory),
					"temp_" + String::toHexString (Random::getSystemRandom().nextInt()),
					suffix,
					optionFlags);
}

TemporaryFile::TemporaryFile (const File& targetFile_, const int optionFlags)
	: targetFile (targetFile_)
{
	// If you use this constructor, you need to give it a valid target file!
	jassert (targetFile != File::nonexistent);

	createTempFile (targetFile.getParentDirectory(),
					targetFile.getFileNameWithoutExtension()
						+ "_temp" + String::toHexString (Random::getSystemRandom().nextInt()),
					targetFile.getFileExtension(),
					optionFlags);
}

void TemporaryFile::createTempFile (const File& parentDirectory, String name,
									const String& suffix, const int optionFlags)
{
	if ((optionFlags & useHiddenFile) != 0)
		name = "." + name;

	temporaryFile = parentDirectory.getNonexistentChildFile (name, suffix, (optionFlags & putNumbersInBrackets) != 0);
}

TemporaryFile::~TemporaryFile()
{
	if (! deleteTemporaryFile())
	{
		/* Failed to delete our temporary file! The most likely reason for this would be
		   that you've not closed an output stream that was being used to write to file.

		   If you find that something beyond your control is changing permissions on
		   your temporary files and preventing them from being deleted, you may want to
		   call TemporaryFile::deleteTemporaryFile() to detect those error cases and
		   handle them appropriately.
		*/
		jassertfalse;
	}
}

bool TemporaryFile::overwriteTargetFileWithTemporary() const
{
	// This method only works if you created this object with the constructor
	// that takes a target file!
	jassert (targetFile != File::nonexistent);

	if (temporaryFile.exists())
	{
		// Have a few attempts at overwriting the file before giving up..
		for (int i = 5; --i >= 0;)
		{
			if (temporaryFile.moveFileTo (targetFile))
				return true;

			Thread::sleep (100);
		}
	}
	else
	{
		// There's no temporary file to use. If your write failed, you should
		// probably check, and not bother calling this method.
		jassertfalse;
	}

	return false;
}

bool TemporaryFile::deleteTemporaryFile() const
{
	// Have a few attempts at deleting the file before giving up..
	for (int i = 5; --i >= 0;)
	{
		if (temporaryFile.deleteFile())
			return true;

		Thread::sleep (50);
	}

	return false;
}

/*** End of inlined file: juce_TemporaryFile.cpp ***/


/*** Start of inlined file: juce_JSON.cpp ***/
class JSONParser
{
public:
	static Result parseAny (String::CharPointerType& t, var& result)
	{
		t = t.findEndOfWhitespace();
		String::CharPointerType t2 (t);

		switch (t2.getAndAdvance())
		{
			case '{':    t = t2; return parseObject (t, result);
			case '[':    t = t2; return parseArray (t, result);
			case '"':    t = t2; return parseString (t, result);

			case '-':
				t2 = t2.findEndOfWhitespace();
				if (! CharacterFunctions::isDigit (*t2))
					break;

				t = t2;
				return parseNumber (t, result, true);

			case '0': case '1': case '2': case '3': case '4':
			case '5': case '6': case '7': case '8': case '9':
				return parseNumber (t, result, false);

			case 't':   // "true"
				if (t2.getAndAdvance() == 'r' && t2.getAndAdvance() == 'u' && t2.getAndAdvance() == 'e')
				{
					t = t2;
					result = var (true);
					return Result::ok();
				}
				break;

			case 'f':   // "false"
				if (t2.getAndAdvance() == 'a' && t2.getAndAdvance() == 'l'
					  && t2.getAndAdvance() == 's' && t2.getAndAdvance() == 'e')
				{
					t = t2;
					result = var (false);
					return Result::ok();
				}
				break;

			case 'n':   // "null"
				if (t2.getAndAdvance() == 'u' && t2.getAndAdvance() == 'l' && t2.getAndAdvance() == 'l')
				{
					t = t2;
					result = var::null;
					return Result::ok();
				}
				break;

			default:
				break;
		}

		return createFail ("Syntax error", &t);
	}

private:
	static Result createFail (const char* const message, const String::CharPointerType* location = nullptr)
	{
		String m (message);
		if (location != nullptr)
			m << ": \"" << String (*location, 20) << '"';

		return Result::fail (m);
	}

	static Result parseNumber (String::CharPointerType& t, var& result, const bool isNegative)
	{
		String::CharPointerType oldT (t);

		int64 intValue = t.getAndAdvance() - '0';
		jassert (intValue >= 0 && intValue < 10);

		for (;;)
		{
			String::CharPointerType previousChar (t);
			const juce_wchar c = t.getAndAdvance();
			const int digit = ((int) c) - '0';

			if (isPositiveAndBelow (digit, 10))
			{
				intValue = intValue * 10 + digit;
				continue;
			}

			if (c == 'e' || c == 'E' || c == '.')
			{
				t = oldT;
				const double asDouble = CharacterFunctions::readDoubleValue (t);
				result = isNegative ? -asDouble : asDouble;
				return Result::ok();
			}

			if (CharacterFunctions::isWhitespace (c)
				 || c == ',' || c == '}' || c == ']' || c == 0)
			{
				t = previousChar;
				break;
			}

			return createFail ("Syntax error in number", &oldT);
		}

		const int64 correctedValue = isNegative ? -intValue : intValue;

		if ((intValue >> 31) != 0)
			result = correctedValue;
		else
			result = (int) correctedValue;

		return Result::ok();
	}

	static Result parseObject (String::CharPointerType& t, var& result)
	{
		DynamicObject* const resultObject = new DynamicObject();
		result = resultObject;
		NamedValueSet& resultProperties = resultObject->getProperties();

		for (;;)
		{
			t = t.findEndOfWhitespace();

			String::CharPointerType oldT (t);
			const juce_wchar c = t.getAndAdvance();

			if (c == '}')
				break;

			if (c == 0)
				return createFail ("Unexpected end-of-input in object declaration");

			if (c == '"')
			{
				var propertyNameVar;
				Result r (parseString (t, propertyNameVar));

				if (r.failed())
					return r;

				const String propertyName (propertyNameVar.toString());

				if (propertyName.isNotEmpty())
				{
					t = t.findEndOfWhitespace();
					oldT = t;

					const juce_wchar c2 = t.getAndAdvance();
					if (c2 != ':')
						return createFail ("Expected ':', but found", &oldT);

					resultProperties.set (propertyName, var::null);
					var* propertyValue = resultProperties.getVarPointer (propertyName);

					Result r2 (parseAny (t, *propertyValue));

					if (r2.failed())
						return r2;

					t = t.findEndOfWhitespace();
					oldT = t;

					const juce_wchar nextChar = t.getAndAdvance();

					if (nextChar == ',')
						continue;
					else if (nextChar == '}')
						break;
				}
			}

			return createFail ("Expected object member declaration, but found", &oldT);
		}

		return Result::ok();
	}

	static Result parseArray (String::CharPointerType& t, var& result)
	{
		result = var (Array<var>());
		Array<var>* const destArray = result.getArray();

		for (;;)
		{
			t = t.findEndOfWhitespace();

			String::CharPointerType oldT (t);
			const juce_wchar c = t.getAndAdvance();

			if (c == ']')
				break;

			if (c == 0)
				return createFail ("Unexpected end-of-input in array declaration");

			t = oldT;
			destArray->add (var::null);
			Result r (parseAny (t, destArray->getReference (destArray->size() - 1)));

			if (r.failed())
				return r;

			t = t.findEndOfWhitespace();
			oldT = t;

			const juce_wchar nextChar = t.getAndAdvance();

			if (nextChar == ',')
				continue;
			else if (nextChar == ']')
				break;

			return createFail ("Expected object array item, but found", &oldT);
		}

		return Result::ok();
	}

	static Result parseString (String::CharPointerType& t, var& result)
	{
		Array<juce_wchar> buffer;
		buffer.ensureStorageAllocated (256);

		for (;;)
		{
			juce_wchar c = t.getAndAdvance();

			if (c == '"')
				break;

			if (c == '\\')
			{
				c = t.getAndAdvance();

				switch (c)
				{
					case '"':
					case '\\':
					case '/':  break;

					case 'b':  c = '\b'; break;
					case 'f':  c = '\f'; break;
					case 'n':  c = '\n'; break;
					case 'r':  c = '\r'; break;
					case 't':  c = '\t'; break;

					case 'u':
					{
						c = 0;

						for (int i = 4; --i >= 0;)
						{
							const int digitValue = CharacterFunctions::getHexDigitValue (t.getAndAdvance());
							if (digitValue < 0)
								return createFail ("Syntax error in unicode escape sequence");

							c = (juce_wchar) ((c << 4) + digitValue);
						}

						break;
					}
				}
			}

			if (c == 0)
				return createFail ("Unexpected end-of-input in string constant");

			buffer.add (c);
		}

		buffer.add (0);
		result = String (CharPointer_UTF32 (buffer.getRawDataPointer()));
		return Result::ok();
	}
};

class JSONFormatter
{
public:
	static void write (OutputStream& out, const var& v,
					   const int indentLevel, const bool allOnOneLine)
	{
		if (v.isString())
		{
			writeString (out, v.toString().getCharPointer());
		}
		else if (v.isVoid())
		{
			out << "null";
		}
		else if (v.isBool())
		{
			out << (static_cast<bool> (v) ? "true" : "false");
		}
		else if (v.isArray())
		{
			writeArray (out, *v.getArray(), indentLevel, allOnOneLine);
		}
		else if (v.isObject())
		{
			DynamicObject* const object = v.getDynamicObject();

			jassert (object != nullptr); // Only DynamicObjects can be converted to JSON!

			writeObject (out, *object, indentLevel, allOnOneLine);
		}
		else
		{
			jassert (! v.isMethod()); // Can't convert an object with methods to JSON!

			out << v.toString();
		}
	}

private:
	enum { indentSize = 2 };

	static void writeEscapedChar (OutputStream& out, const unsigned short value)
	{
		out << "\\u" << String::toHexString ((int) value).paddedLeft ('0', 4);
	}

	static void writeString (OutputStream& out, String::CharPointerType t)
	{
		out << '"';

		for (;;)
		{
			const juce_wchar c (t.getAndAdvance());

			switch (c)
			{
				case 0:  out << '"'; return;

				case '\"':  out << "\\\""; break;
				case '\\':  out << "\\\\"; break;
				case '\b':  out << "\\b";  break;
				case '\f':  out << "\\f";  break;
				case '\t':  out << "\\t";  break;
				case '\r':  out << "\\r";  break;
				case '\n':  out << "\\n";  break;

				default:
					if (c >= 32 && c < 127)
					{
						out << (char) c;
					}
					else
					{
						if (CharPointer_UTF16::getBytesRequiredFor (c) > 2)
						{
							CharPointer_UTF16::CharType chars[2];
							CharPointer_UTF16 utf16 (chars);
							utf16.write (c);

							for (int i = 0; i < 2; ++i)
								writeEscapedChar (out, (unsigned short) chars[i]);
						}
						else
						{
							writeEscapedChar (out, (unsigned short) c);
						}
					}

					break;
			}
		}
	}

	static void writeSpaces (OutputStream& out, int numSpaces)
	{
		out.writeRepeatedByte (' ', numSpaces);
	}

	static void writeArray (OutputStream& out, const Array<var>& array,
							const int indentLevel, const bool allOnOneLine)
	{
		out << '[';
		if (! allOnOneLine)
			out << newLine;

		for (int i = 0; i < array.size(); ++i)
		{
			if (! allOnOneLine)
				writeSpaces (out, indentLevel + indentSize);

			write (out, array.getReference(i), indentLevel + indentSize, allOnOneLine);

			if (i < array.size() - 1)
			{
				if (allOnOneLine)
					out << ", ";
				else
					out << ',' << newLine;
			}
			else if (! allOnOneLine)
				out << newLine;
		}

		if (! allOnOneLine)
			writeSpaces (out, indentLevel);

		out << ']';
	}

	static void writeObject (OutputStream& out, DynamicObject& object,
							 const int indentLevel, const bool allOnOneLine)
	{
		NamedValueSet& props = object.getProperties();

		out << '{';
		if (! allOnOneLine)
			out << newLine;

		LinkedListPointer<NamedValueSet::NamedValue>* i = &(props.values);

		for (;;)
		{
			NamedValueSet::NamedValue* const v = i->get();

			if (v == nullptr)
				break;

			if (! allOnOneLine)
				writeSpaces (out, indentLevel + indentSize);

			writeString (out, v->name);
			out << ": ";
			write (out, v->value, indentLevel + indentSize, allOnOneLine);

			if (v->nextListItem.get() != nullptr)
			{
				if (allOnOneLine)
					out << ", ";
				else
					out << ',' << newLine;
			}
			else if (! allOnOneLine)
				out << newLine;

			i = &(v->nextListItem);
		}

		if (! allOnOneLine)
			writeSpaces (out, indentLevel);

		out << '}';
	}
};

var JSON::parse (const String& text)
{
	var result;
	String::CharPointerType t (text.getCharPointer());
	if (! JSONParser::parseAny (t, result))
		result = var::null;

	return result;
}

var JSON::parse (InputStream& input)
{
	return parse (input.readEntireStreamAsString());
}

var JSON::parse (const File& file)
{
	return parse (file.loadFileAsString());
}

Result JSON::parse (const String& text, var& result)
{
	String::CharPointerType t (text.getCharPointer());
	return JSONParser::parseAny (t, result);
}

String JSON::toString (const var& data, const bool allOnOneLine)
{
	MemoryOutputStream mo (1024);
	JSONFormatter::write (mo, data, 0, allOnOneLine);
	return mo.toString();
}

void JSON::writeToStream (OutputStream& output, const var& data, const bool allOnOneLine)
{
	JSONFormatter::write (output, data, 0, allOnOneLine);
}

#if JUCE_UNIT_TESTS

class JSONTests  : public UnitTest
{
public:
	JSONTests() : UnitTest ("JSON") {}

	static String createRandomWideCharString (Random& r)
	{
		juce_wchar buffer[40] = { 0 };

		for (int i = 0; i < numElementsInArray (buffer) - 1; ++i)
		{
			if (r.nextBool())
			{
				do
				{
					buffer[i] = (juce_wchar) (1 + r.nextInt (0x10ffff - 1));
				}
				while (! CharPointer_UTF16::canRepresent (buffer[i]));
			}
			else
				buffer[i] = (juce_wchar) (1 + r.nextInt (0xff));
		}

		return CharPointer_UTF32 (buffer);
	}

	static String createRandomIdentifier (Random& r)
	{
		char buffer[30] = { 0 };

		for (int i = 0; i < numElementsInArray (buffer) - 1; ++i)
		{
			static const char chars[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-:";
			buffer[i] = chars [r.nextInt (sizeof (chars) - 1)];
		}

		return CharPointer_ASCII (buffer);
	}

	static var createRandomVar (Random& r, int depth)
	{
		switch (r.nextInt (depth > 3 ? 6 : 8))
		{
			case 0:     return var::null;
			case 1:     return r.nextInt();
			case 2:     return r.nextInt64();
			case 3:     return r.nextBool();
			case 4:     return r.nextDouble();
			case 5:     return createRandomWideCharString (r);

			case 6:
			{
				var v (createRandomVar (r, depth + 1));

				for (int i = 1 + r.nextInt (30); --i >= 0;)
					v.append (createRandomVar (r, depth + 1));

				return v;
			}

			case 7:
			{
				DynamicObject* o = new DynamicObject();

				for (int i = r.nextInt (30); --i >= 0;)
					o->setProperty (createRandomIdentifier (r), createRandomVar (r, depth + 1));

				return o;
			}

			default:
				return var::null;
		}
	}

	void runTest()
	{
		beginTest ("JSON");
		Random r;
		r.setSeedRandomly();

		expect (JSON::parse (String::empty) == var::null);
		expect (JSON::parse ("{}").isObject());
		expect (JSON::parse ("[]").isArray());
		expect (JSON::parse ("1234").isInt());
		expect (JSON::parse ("12345678901234").isInt64());
		expect (JSON::parse ("1.123e3").isDouble());
		expect (JSON::parse ("-1234").isInt());
		expect (JSON::parse ("-12345678901234").isInt64());
		expect (JSON::parse ("-1.123e3").isDouble());

		for (int i = 100; --i >= 0;)
		{
			var v;

			if (i > 0)
				v = createRandomVar (r, 0);

			const bool oneLine = r.nextBool();
			String asString (JSON::toString (v, oneLine));
			var parsed = JSON::parse (asString);
			String parsedString (JSON::toString (parsed, oneLine));
			expect (asString.isNotEmpty() && parsedString == asString);
		}
	}
};

static JSONTests JSONUnitTests;

#endif

/*** End of inlined file: juce_JSON.cpp ***/


/*** Start of inlined file: juce_FileLogger.cpp ***/
FileLogger::FileLogger (const File& logFile_,
						const String& welcomeMessage,
						const int maxInitialFileSizeBytes)
	: logFile (logFile_)
{
	if (maxInitialFileSizeBytes >= 0)
		trimFileSize (maxInitialFileSizeBytes);

	if (! logFile_.exists())
	{
		// do this so that the parent directories get created..
		logFile_.create();
	}

	String welcome;
	welcome << newLine
			<< "**********************************************************" << newLine
			<< welcomeMessage << newLine
			<< "Log started: " << Time::getCurrentTime().toString (true, true) << newLine;

	FileLogger::logMessage (welcome);
}

FileLogger::~FileLogger()
{
}

void FileLogger::logMessage (const String& message)
{
	DBG (message);

	const ScopedLock sl (logLock);

	FileOutputStream out (logFile, 256);
	out << message << newLine;
}

void FileLogger::trimFileSize (int maxFileSizeBytes) const
{
	if (maxFileSizeBytes <= 0)
	{
		logFile.deleteFile();
	}
	else
	{
		const int64 fileSize = logFile.getSize();

		if (fileSize > maxFileSizeBytes)
		{
			ScopedPointer <FileInputStream> in (logFile.createInputStream());
			jassert (in != nullptr);

			if (in != nullptr)
			{
				in->setPosition (fileSize - maxFileSizeBytes);
				String content;

				{
					MemoryBlock contentToSave;
					contentToSave.setSize ((size_t) maxFileSizeBytes + 4);
					contentToSave.fillWith (0);

					in->read (contentToSave.getData(), maxFileSizeBytes);
					in = nullptr;

					content = contentToSave.toString();
				}

				int newStart = 0;

				while (newStart < fileSize
						&& content[newStart] != '\n'
						&& content[newStart] != '\r')
					++newStart;

				logFile.deleteFile();
				logFile.appendText (content.substring (newStart), false, false);
			}
		}
	}
}

FileLogger* FileLogger::createDefaultAppLogger (const String& logFileSubDirectoryName,
												const String& logFileName,
												const String& welcomeMessage,
												const int maxInitialFileSizeBytes)
{
   #if JUCE_MAC
	File logFile ("~/Library/Logs");
	logFile = logFile.getChildFile (logFileSubDirectoryName)
					 .getChildFile (logFileName);

   #else
	File logFile (File::getSpecialLocation (File::userApplicationDataDirectory));

	if (logFile.isDirectory())
	{
		logFile = logFile.getChildFile (logFileSubDirectoryName)
						 .getChildFile (logFileName);
	}
   #endif

	return new FileLogger (logFile, welcomeMessage, maxInitialFileSizeBytes);
}

/*** End of inlined file: juce_FileLogger.cpp ***/


/*** Start of inlined file: juce_Logger.cpp ***/
Logger::Logger()
{
}

Logger::~Logger()
{
}

Logger* Logger::currentLogger = nullptr;

void Logger::setCurrentLogger (Logger* const newLogger,
							   const bool deleteOldLogger)
{
	Logger* const oldLogger = currentLogger;
	currentLogger = newLogger;

	if (deleteOldLogger)
		delete oldLogger;
}

void Logger::writeToLog (const String& message)
{
	if (currentLogger != nullptr)
		currentLogger->logMessage (message);
	else
		outputDebugString (message);
}

#if JUCE_LOG_ASSERTIONS
void JUCE_API logAssertion (const char* filename, const int lineNum) noexcept
{
	String m ("JUCE Assertion failure in ");
	m << filename << ", line " << lineNum;

	Logger::writeToLog (m);
}
#endif

/*** End of inlined file: juce_Logger.cpp ***/


/*** Start of inlined file: juce_BigInteger.cpp ***/
namespace
{
	inline size_t bitToIndex (const int bit) noexcept   { return (size_t) (bit >> 5); }
	inline uint32 bitToMask  (const int bit) noexcept   { return (uint32) 1 << (bit & 31); }
}

BigInteger::BigInteger()
	: numValues (4),
	  highestBit (-1),
	  negative (false)
{
	values.calloc (numValues + 1);
}

BigInteger::BigInteger (const int32 value)
	: numValues (4),
	  highestBit (31),
	  negative (value < 0)
{
	values.calloc (numValues + 1);
	values[0] = (uint32) abs (value);
	highestBit = getHighestBit();
}

BigInteger::BigInteger (const uint32 value)
	: numValues (4),
	  highestBit (31),
	  negative (false)
{
	values.calloc (numValues + 1);
	values[0] = value;
	highestBit = getHighestBit();
}

BigInteger::BigInteger (int64 value)
	: numValues (4),
	  highestBit (63),
	  negative (value < 0)
{
	values.calloc (numValues + 1);

	if (value < 0)
		value = -value;

	values[0] = (uint32) value;
	values[1] = (uint32) (value >> 32);
	highestBit = getHighestBit();
}

BigInteger::BigInteger (const BigInteger& other)
	: numValues ((size_t) jmax ((size_t) 4, bitToIndex (other.highestBit) + 1)),
	  highestBit (other.getHighestBit()),
	  negative (other.negative)
{
	values.malloc (numValues + 1);
	memcpy (values, other.values, sizeof (uint32) * (numValues + 1));
}

#if JUCE_COMPILER_SUPPORTS_MOVE_SEMANTICS
BigInteger::BigInteger (BigInteger&& other) noexcept
	: values (static_cast <HeapBlock <uint32>&&> (other.values)),
	  numValues (other.numValues),
	  highestBit (other.highestBit),
	  negative (other.negative)
{
}

BigInteger& BigInteger::operator= (BigInteger&& other) noexcept
{
	values = static_cast <HeapBlock <uint32>&&> (other.values);
	numValues = other.numValues;
	highestBit = other.highestBit;
	negative = other.negative;
	return *this;
}
#endif

BigInteger::~BigInteger()
{
}

void BigInteger::swapWith (BigInteger& other) noexcept
{
	values.swapWith (other.values);
	std::swap (numValues, other.numValues);
	std::swap (highestBit, other.highestBit);
	std::swap (negative, other.negative);
}

BigInteger& BigInteger::operator= (const BigInteger& other)
{
	if (this != &other)
	{
		highestBit = other.getHighestBit();
		jassert (other.numValues >= 4);
		numValues = (size_t) jmax ((size_t) 4, bitToIndex (highestBit) + 1);
		negative = other.negative;
		values.malloc (numValues + 1);
		memcpy (values, other.values, sizeof (uint32) * (numValues + 1));
	}

	return *this;
}

void BigInteger::ensureSize (const size_t numVals)
{
	if (numVals + 2 >= numValues)
	{
		size_t oldSize = numValues;
		numValues = ((numVals + 2) * 3) / 2;
		values.realloc (numValues + 1);

		while (oldSize < numValues)
			values [oldSize++] = 0;
	}
}

bool BigInteger::operator[] (const int bit) const noexcept
{
	return bit <= highestBit && bit >= 0
			 && ((values [bitToIndex (bit)] & bitToMask (bit)) != 0);
}

int BigInteger::toInteger() const noexcept
{
	const int n = (int) (values[0] & 0x7fffffff);
	return negative ? -n : n;
}

BigInteger BigInteger::getBitRange (int startBit, int numBits) const
{
	BigInteger r;
	numBits = jmin (numBits, getHighestBit() + 1 - startBit);
	r.ensureSize ((size_t) bitToIndex (numBits));
	r.highestBit = numBits;

	int i = 0;
	while (numBits > 0)
	{
		r.values[i++] = getBitRangeAsInt (startBit, (int) jmin (32, numBits));
		numBits -= 32;
		startBit += 32;
	}

	r.highestBit = r.getHighestBit();
	return r;
}

uint32 BigInteger::getBitRangeAsInt (const int startBit, int numBits) const noexcept
{
	if (numBits > 32)
	{
		jassertfalse;  // use getBitRange() if you need more than 32 bits..
		numBits = 32;
	}

	numBits = jmin (numBits, highestBit + 1 - startBit);

	if (numBits <= 0)
		return 0;

	const size_t pos = bitToIndex (startBit);
	const int offset = startBit & 31;
	const int endSpace = 32 - numBits;

	uint32 n = ((uint32) values [pos]) >> offset;

	if (offset > endSpace)
		n |= ((uint32) values [pos + 1]) << (32 - offset);

	return n & (((uint32) 0xffffffff) >> endSpace);
}

void BigInteger::setBitRangeAsInt (const int startBit, int numBits, uint32 valueToSet)
{
	if (numBits > 32)
	{
		jassertfalse;
		numBits = 32;
	}

	for (int i = 0; i < numBits; ++i)
	{
		setBit (startBit + i, (valueToSet & 1) != 0);
		valueToSet >>= 1;
	}
}

void BigInteger::clear()
{
	if (numValues > 16)
	{
		numValues = 4;
		values.calloc (numValues + 1);
	}
	else
	{
		values.clear (numValues + 1);
	}

	highestBit = -1;
	negative = false;
}

void BigInteger::setBit (const int bit)
{
	if (bit >= 0)
	{
		if (bit > highestBit)
		{
			ensureSize (bitToIndex (bit));
			highestBit = bit;
		}

		values [bitToIndex (bit)] |= bitToMask (bit);
	}
}

void BigInteger::setBit (const int bit, const bool shouldBeSet)
{
	if (shouldBeSet)
		setBit (bit);
	else
		clearBit (bit);
}

void BigInteger::clearBit (const int bit) noexcept
{
	if (bit >= 0 && bit <= highestBit)
		values [bitToIndex (bit)] &= ~bitToMask (bit);
}

void BigInteger::setRange (int startBit, int numBits, const bool shouldBeSet)
{
	while (--numBits >= 0)
		setBit (startBit++, shouldBeSet);
}

void BigInteger::insertBit (const int bit, const bool shouldBeSet)
{
	if (bit >= 0)
		shiftBits (1, bit);

	setBit (bit, shouldBeSet);
}

bool BigInteger::isZero() const noexcept
{
	return getHighestBit() < 0;
}

bool BigInteger::isOne() const noexcept
{
	return getHighestBit() == 0 && ! negative;
}

bool BigInteger::isNegative() const noexcept
{
	return negative && ! isZero();
}

void BigInteger::setNegative (const bool neg) noexcept
{
	negative = neg;
}

void BigInteger::negate() noexcept
{
	negative = (! negative) && ! isZero();
}

#if JUCE_USE_INTRINSICS && ! defined (__INTEL_COMPILER)
 #pragma intrinsic (_BitScanReverse)
#endif

namespace BitFunctions
{
	inline int countBitsInInt32 (uint32 n) noexcept
	{
		n -= ((n >> 1) & 0x55555555);
		n =  (((n >> 2) & 0x33333333) + (n & 0x33333333));
		n =  (((n >> 4) + n) & 0x0f0f0f0f);
		n += (n >> 8);
		n += (n >> 16);
		return (int) (n & 0x3f);
	}

	inline int highestBitInInt (uint32 n) noexcept
	{
		jassert (n != 0); // (the built-in functions may not work for n = 0)

	  #if JUCE_GCC
		return 31 - __builtin_clz (n);
	  #elif JUCE_USE_INTRINSICS
		unsigned long highest;
		_BitScanReverse (&highest, n);
		return (int) highest;
	  #else
		n |= (n >> 1);
		n |= (n >> 2);
		n |= (n >> 4);
		n |= (n >> 8);
		n |= (n >> 16);
		return countBitsInInt32 (n >> 1);
	  #endif
	}
}

int BigInteger::countNumberOfSetBits() const noexcept
{
	int total = 0;

	for (int i = (int) bitToIndex (highestBit) + 1; --i >= 0;)
		total += BitFunctions::countBitsInInt32 (values[i]);

	return total;
}

int BigInteger::getHighestBit() const noexcept
{
	for (int i = (int) bitToIndex (highestBit + 1); i >= 0; --i)
	{
		const uint32 n = values[i];

		if (n != 0)
			return BitFunctions::highestBitInInt (n) + (i << 5);
	}

	return -1;
}

int BigInteger::findNextSetBit (int i) const noexcept
{
	for (; i <= highestBit; ++i)
		if ((values [bitToIndex (i)] & bitToMask (i)) != 0)
			return i;

	return -1;
}

int BigInteger::findNextClearBit (int i) const noexcept
{
	for (; i <= highestBit; ++i)
		if ((values [bitToIndex (i)] & bitToMask (i)) == 0)
			break;

	return i;
}

BigInteger& BigInteger::operator+= (const BigInteger& other)
{
	if (other.isNegative())
		return operator-= (-other);

	if (isNegative())
	{
		if (compareAbsolute (other) < 0)
		{
			BigInteger temp (*this);
			temp.negate();
			*this = other;
			operator-= (temp);
		}
		else
		{
			negate();
			operator-= (other);
			negate();
		}
	}
	else
	{
		if (other.highestBit > highestBit)
			highestBit = other.highestBit;

		++highestBit;

		const size_t numInts = bitToIndex (highestBit) + 1;
		ensureSize (numInts);

		int64 remainder = 0;

		for (size_t i = 0; i <= numInts; ++i)
		{
			if (i < numValues)
				remainder += values[i];

			if (i < other.numValues)
				remainder += other.values[i];

			values[i] = (uint32) remainder;
			remainder >>= 32;
		}

		jassert (remainder == 0);
		highestBit = getHighestBit();
	}

	return *this;
}

BigInteger& BigInteger::operator-= (const BigInteger& other)
{
	if (other.isNegative())
		return operator+= (-other);

	if (! isNegative())
	{
		if (compareAbsolute (other) < 0)
		{
			BigInteger temp (other);
			swapWith (temp);
			operator-= (temp);
			negate();
			return *this;
		}
	}
	else
	{
		negate();
		operator+= (other);
		negate();
		return *this;
	}

	const size_t numInts = bitToIndex (highestBit) + 1;
	const size_t maxOtherInts = bitToIndex (other.highestBit) + 1;
	int64 amountToSubtract = 0;

	for (size_t i = 0; i <= numInts; ++i)
	{
		if (i <= maxOtherInts)
			amountToSubtract += (int64) other.values[i];

		if (values[i] >= amountToSubtract)
		{
			values[i] = (uint32) (values[i] - amountToSubtract);
			amountToSubtract = 0;
		}
		else
		{
			const int64 n = ((int64) values[i] + (((int64) 1) << 32)) - amountToSubtract;
			values[i] = (uint32) n;
			amountToSubtract = 1;
		}
	}

	return *this;
}

BigInteger& BigInteger::operator*= (const BigInteger& other)
{
	BigInteger total;
	highestBit = getHighestBit();
	const bool wasNegative = isNegative();
	setNegative (false);

	for (int i = 0; i <= highestBit; ++i)
	{
		if (operator[](i))
		{
			BigInteger n (other);
			n.setNegative (false);
			n <<= i;
			total += n;
		}
	}

	total.setNegative (wasNegative ^ other.isNegative());
	swapWith (total);
	return *this;
}

void BigInteger::divideBy (const BigInteger& divisor, BigInteger& remainder)
{
	jassert (this != &remainder); // (can't handle passing itself in to get the remainder)

	const int divHB = divisor.getHighestBit();
	const int ourHB = getHighestBit();

	if (divHB < 0 || ourHB < 0)
	{
		// division by zero
		remainder.clear();
		clear();
	}
	else
	{
		const bool wasNegative = isNegative();

		swapWith (remainder);
		remainder.setNegative (false);
		clear();

		BigInteger temp (divisor);
		temp.setNegative (false);

		int leftShift = ourHB - divHB;
		temp <<= leftShift;

		while (leftShift >= 0)
		{
			if (remainder.compareAbsolute (temp) >= 0)
			{
				remainder -= temp;
				setBit (leftShift);
			}

			if (--leftShift >= 0)
				temp >>= 1;
		}

		negative = wasNegative ^ divisor.isNegative();
		remainder.setNegative (wasNegative);
	}
}

BigInteger& BigInteger::operator/= (const BigInteger& other)
{
	BigInteger remainder;
	divideBy (other, remainder);
	return *this;
}

BigInteger& BigInteger::operator|= (const BigInteger& other)
{
	// this operation doesn't take into account negative values..
	jassert (isNegative() == other.isNegative());

	if (other.highestBit >= 0)
	{
		ensureSize (bitToIndex (other.highestBit));

		int n = (int) bitToIndex (other.highestBit) + 1;

		while (--n >= 0)
			values[n] |= other.values[n];

		if (other.highestBit > highestBit)
			highestBit = other.highestBit;

		highestBit = getHighestBit();
	}

	return *this;
}

BigInteger& BigInteger::operator&= (const BigInteger& other)
{
	// this operation doesn't take into account negative values..
	jassert (isNegative() == other.isNegative());

	int n = (int) numValues;

	while (n > (int) other.numValues)
		values[--n] = 0;

	while (--n >= 0)
		values[n] &= other.values[n];

	if (other.highestBit < highestBit)
		highestBit = other.highestBit;

	highestBit = getHighestBit();
	return *this;
}

BigInteger& BigInteger::operator^= (const BigInteger& other)
{
	// this operation will only work with the absolute values
	jassert (isNegative() == other.isNegative());

	if (other.highestBit >= 0)
	{
		ensureSize (bitToIndex (other.highestBit));

		int n = (int) bitToIndex (other.highestBit) + 1;

		while (--n >= 0)
			values[n] ^= other.values[n];

		if (other.highestBit > highestBit)
			highestBit = other.highestBit;

		highestBit = getHighestBit();
	}

	return *this;
}

BigInteger& BigInteger::operator%= (const BigInteger& divisor)
{
	BigInteger remainder;
	divideBy (divisor, remainder);
	swapWith (remainder);
	return *this;
}

BigInteger& BigInteger::operator++()      { return operator+= (1); }
BigInteger& BigInteger::operator--()      { return operator-= (1); }
BigInteger  BigInteger::operator++ (int)  { const BigInteger old (*this); operator+= (1); return old; }
BigInteger  BigInteger::operator-- (int)  { const BigInteger old (*this); operator-= (1); return old; }

BigInteger  BigInteger::operator-() const                            { BigInteger b (*this); b.negate(); return b; }
BigInteger  BigInteger::operator+   (const BigInteger& other) const  { BigInteger b (*this); return b += other; }
BigInteger  BigInteger::operator-   (const BigInteger& other) const  { BigInteger b (*this); return b -= other; }
BigInteger  BigInteger::operator*   (const BigInteger& other) const  { BigInteger b (*this); return b *= other; }
BigInteger  BigInteger::operator/   (const BigInteger& other) const  { BigInteger b (*this); return b /= other; }
BigInteger  BigInteger::operator|   (const BigInteger& other) const  { BigInteger b (*this); return b |= other; }
BigInteger  BigInteger::operator&   (const BigInteger& other) const  { BigInteger b (*this); return b &= other; }
BigInteger  BigInteger::operator^   (const BigInteger& other) const  { BigInteger b (*this); return b ^= other; }
BigInteger  BigInteger::operator%   (const BigInteger& other) const  { BigInteger b (*this); return b %= other; }
BigInteger  BigInteger::operator<<  (const int numBits) const        { BigInteger b (*this); return b <<= numBits; }
BigInteger  BigInteger::operator>>  (const int numBits) const        { BigInteger b (*this); return b >>= numBits; }
BigInteger& BigInteger::operator<<= (const int numBits)              { shiftBits (numBits, 0);  return *this; }
BigInteger& BigInteger::operator>>= (const int numBits)              { shiftBits (-numBits, 0); return *this; }

int BigInteger::compare (const BigInteger& other) const noexcept
{
	if (isNegative() == other.isNegative())
	{
		const int absComp = compareAbsolute (other);
		return isNegative() ? -absComp : absComp;
	}
	else
	{
		return isNegative() ? -1 : 1;
	}
}

int BigInteger::compareAbsolute (const BigInteger& other) const noexcept
{
	const int h1 = getHighestBit();
	const int h2 = other.getHighestBit();

	if (h1 > h2)
		return 1;
	else if (h1 < h2)
		return -1;

	for (int i = (int) bitToIndex (h1) + 1; --i >= 0;)
		if (values[i] != other.values[i])
			return (values[i] > other.values[i]) ? 1 : -1;

	return 0;
}

bool BigInteger::operator== (const BigInteger& other) const noexcept    { return compare (other) == 0; }
bool BigInteger::operator!= (const BigInteger& other) const noexcept    { return compare (other) != 0; }
bool BigInteger::operator<  (const BigInteger& other) const noexcept    { return compare (other) < 0; }
bool BigInteger::operator<= (const BigInteger& other) const noexcept    { return compare (other) <= 0; }
bool BigInteger::operator>  (const BigInteger& other) const noexcept    { return compare (other) > 0; }
bool BigInteger::operator>= (const BigInteger& other) const noexcept    { return compare (other) >= 0; }

void BigInteger::shiftLeft (int bits, const int startBit)
{
	if (startBit > 0)
	{
		for (int i = highestBit + 1; --i >= startBit;)
			setBit (i + bits, operator[] (i));

		while (--bits >= 0)
			clearBit (bits + startBit);
	}
	else
	{
		ensureSize (bitToIndex (highestBit + bits) + 1);

		const size_t wordsToMove = bitToIndex (bits);
		size_t top = 1 + bitToIndex (highestBit);
		highestBit += bits;

		if (wordsToMove > 0)
		{
			for (int i = (int) top; --i >= 0;)
				values [(size_t) i + wordsToMove] = values [i];

			for (size_t j = 0; j < wordsToMove; ++j)
				values [j] = 0;

			bits &= 31;
		}

		if (bits != 0)
		{
			const int invBits = 32 - bits;

			for (size_t i = top + 1 + wordsToMove; --i > wordsToMove;)
				values[i] = (values[i] << bits) | (values [i - 1] >> invBits);

			values [wordsToMove] = values [wordsToMove] << bits;
		}

		highestBit = getHighestBit();
	}
}

void BigInteger::shiftRight (int bits, const int startBit)
{
	if (startBit > 0)
	{
		for (int i = startBit; i <= highestBit; ++i)
			setBit (i, operator[] (i + bits));

		highestBit = getHighestBit();
	}
	else
	{
		if (bits > highestBit)
		{
			clear();
		}
		else
		{
			const size_t wordsToMove = bitToIndex (bits);
			size_t top = 1 + bitToIndex (highestBit) - wordsToMove;
			highestBit -= bits;

			if (wordsToMove > 0)
			{
				size_t i;
				for (i = 0; i < top; ++i)
					values [i] = values [i + wordsToMove];

				for (i = 0; i < wordsToMove; ++i)
					values [top + i] = 0;

				bits &= 31;
			}

			if (bits != 0)
			{
				const int invBits = 32 - bits;

				--top;
				for (size_t i = 0; i < top; ++i)
					values[i] = (values[i] >> bits) | (values [i + 1] << invBits);

				values[top] = (values[top] >> bits);
			}

			highestBit = getHighestBit();
		}
	}
}

void BigInteger::shiftBits (int bits, const int startBit)
{
	if (highestBit >= 0)
	{
		if (bits < 0)
			shiftRight (-bits, startBit);
		else if (bits > 0)
			shiftLeft (bits, startBit);
	}
}

static BigInteger simpleGCD (BigInteger* m, BigInteger* n)
{
	while (! m->isZero())
	{
		if (n->compareAbsolute (*m) > 0)
			std::swap (m, n);

		*m -= *n;
	}

	return *n;
}

BigInteger BigInteger::findGreatestCommonDivisor (BigInteger n) const
{
	BigInteger m (*this);

	while (! n.isZero())
	{
		if (abs (m.getHighestBit() - n.getHighestBit()) <= 16)
			return simpleGCD (&m, &n);

		BigInteger temp2;
		m.divideBy (n, temp2);

		m.swapWith (n);
		n.swapWith (temp2);
	}

	return m;
}

void BigInteger::exponentModulo (const BigInteger& exponent, const BigInteger& modulus)
{
	BigInteger exp (exponent);
	exp %= modulus;

	BigInteger value (1);
	swapWith (value);
	value %= modulus;

	while (! exp.isZero())
	{
		if (exp [0])
		{
			operator*= (value);
			operator%= (modulus);
		}

		value *= value;
		value %= modulus;
		exp >>= 1;
	}
}

void BigInteger::inverseModulo (const BigInteger& modulus)
{
	if (modulus.isOne() || modulus.isNegative())
	{
		clear();
		return;
	}

	if (isNegative() || compareAbsolute (modulus) >= 0)
		operator%= (modulus);

	if (isOne())
		return;

	if (! (*this)[0])
	{
		// not invertible
		clear();
		return;
	}

	BigInteger a1 (modulus);
	BigInteger a2 (*this);
	BigInteger b1 (modulus);
	BigInteger b2 (1);

	while (! a2.isOne())
	{
		BigInteger temp1, multiplier (a1);
		multiplier.divideBy (a2, temp1);

		temp1 = a2;
		temp1 *= multiplier;
		BigInteger temp2 (a1);
		temp2 -= temp1;
		a1 = a2;
		a2 = temp2;

		temp1 = b2;
		temp1 *= multiplier;
		temp2 = b1;
		temp2 -= temp1;
		b1 = b2;
		b2 = temp2;
	}

	while (b2.isNegative())
		b2 += modulus;

	b2 %= modulus;
	swapWith (b2);
}

OutputStream& JUCE_CALLTYPE operator<< (OutputStream& stream, const BigInteger& value)
{
	return stream << value.toString (10);
}

String BigInteger::toString (const int base, const int minimumNumCharacters) const
{
	String s;
	BigInteger v (*this);

	if (base == 2 || base == 8 || base == 16)
	{
		const int bits = (base == 2) ? 1 : (base == 8 ? 3 : 4);
		static const char hexDigits[] = "0123456789abcdef";

		for (;;)
		{
			const uint32 remainder = v.getBitRangeAsInt (0, bits);

			v >>= bits;

			if (remainder == 0 && v.isZero())
				break;

			s = String::charToString ((juce_wchar) (uint8) hexDigits [remainder]) + s;
		}
	}
	else if (base == 10)
	{
		const BigInteger ten (10);
		BigInteger remainder;

		for (;;)
		{
			v.divideBy (ten, remainder);

			if (remainder.isZero() && v.isZero())
				break;

			s = String (remainder.getBitRangeAsInt (0, 8)) + s;
		}
	}
	else
	{
		jassertfalse; // can't do the specified base!
		return String::empty;
	}

	s = s.paddedLeft ('0', minimumNumCharacters);

	return isNegative() ? "-" + s : s;
}

void BigInteger::parseString (const String& text, const int base)
{
	clear();
	String::CharPointerType t (text.getCharPointer());

	if (base == 2 || base == 8 || base == 16)
	{
		const int bits = (base == 2) ? 1 : (base == 8 ? 3 : 4);

		for (;;)
		{
			const juce_wchar c = t.getAndAdvance();
			const int digit = CharacterFunctions::getHexDigitValue (c);

			if (((uint32) digit) < (uint32) base)
			{
				operator<<= (bits);
				operator+= (digit);
			}
			else if (c == 0)
			{
				break;
			}
		}
	}
	else if (base == 10)
	{
		const BigInteger ten ((uint32) 10);

		for (;;)
		{
			const juce_wchar c = t.getAndAdvance();

			if (c >= '0' && c <= '9')
			{
				operator*= (ten);
				operator+= ((int) (c - '0'));
			}
			else if (c == 0)
			{
				break;
			}
		}
	}

	setNegative (text.trimStart().startsWithChar ('-'));
}

MemoryBlock BigInteger::toMemoryBlock() const
{
	const int numBytes = (getHighestBit() + 8) >> 3;
	MemoryBlock mb ((size_t) numBytes);

	for (int i = 0; i < numBytes; ++i)
		mb[i] = (char) getBitRangeAsInt (i << 3, 8);

	return mb;
}

void BigInteger::loadFromMemoryBlock (const MemoryBlock& data)
{
	clear();

	for (int i = (int) data.getSize(); --i >= 0;)
		this->setBitRangeAsInt (i << 3, 8, (uint32) data [i]);
}

/*** End of inlined file: juce_BigInteger.cpp ***/


/*** Start of inlined file: juce_Expression.cpp ***/
class Expression::Term  : public SingleThreadedReferenceCountedObject
{
public:
	Term() {}
	virtual ~Term() {}

	virtual Type getType() const noexcept = 0;
	virtual Term* clone() const = 0;
	virtual ReferenceCountedObjectPtr<Term> resolve (const Scope&, int recursionDepth) = 0;
	virtual String toString() const = 0;
	virtual double toDouble() const                                          { return 0; }
	virtual int getInputIndexFor (const Term*) const                         { return -1; }
	virtual int getOperatorPrecedence() const                                { return 0; }
	virtual int getNumInputs() const                                         { return 0; }
	virtual Term* getInput (int) const                                       { return nullptr; }
	virtual ReferenceCountedObjectPtr<Term> negated();

	virtual ReferenceCountedObjectPtr<Term> createTermToEvaluateInput (const Scope&, const Term* /*inputTerm*/,
																	   double /*overallTarget*/, Term* /*topLevelTerm*/) const
	{
		jassertfalse;
		return nullptr;
	}

	virtual String getName() const
	{
		jassertfalse; // You shouldn't call this for an expression that's not actually a function!
		return String::empty;
	}

	virtual void renameSymbol (const Symbol& oldSymbol, const String& newName, const Scope& scope, int recursionDepth)
	{
		for (int i = getNumInputs(); --i >= 0;)
			getInput (i)->renameSymbol (oldSymbol, newName, scope, recursionDepth);
	}

	class SymbolVisitor
	{
	public:
		virtual ~SymbolVisitor() {}
		virtual void useSymbol (const Symbol&) = 0;
	};

	virtual void visitAllSymbols (SymbolVisitor& visitor, const Scope& scope, int recursionDepth)
	{
		for (int i = getNumInputs(); --i >= 0;)
			getInput(i)->visitAllSymbols (visitor, scope, recursionDepth);
	}

private:
	JUCE_DECLARE_NON_COPYABLE (Term);
};

struct Expression::Helpers
{
	typedef ReferenceCountedObjectPtr<Term> TermPtr;

	static void checkRecursionDepth (const int depth)
	{
		if (depth > 256)
			throw EvaluationError ("Recursive symbol references");
	}

	friend class Expression::Term;

	/** An exception that can be thrown by Expression::evaluate(). */
	class EvaluationError  : public std::exception
	{
	public:
		EvaluationError (const String& description_)
			: description (description_)
		{
			DBG ("Expression::EvaluationError: " + description);
		}

		String description;
	};

	class Constant  : public Term
	{
	public:
		Constant (const double value_, const bool isResolutionTarget_)
			: value (value_), isResolutionTarget (isResolutionTarget_) {}

		Type getType() const noexcept                { return constantType; }
		Term* clone() const                          { return new Constant (value, isResolutionTarget); }
		TermPtr resolve (const Scope&, int)          { return this; }
		double toDouble() const                      { return value; }
		TermPtr negated()                            { return new Constant (-value, isResolutionTarget); }

		String toString() const
		{
			String s (value);
			if (isResolutionTarget)
				s = "@" + s;

			return s;
		}

		double value;
		bool isResolutionTarget;
	};

	class BinaryTerm  : public Term
	{
	public:
		BinaryTerm (Term* const l, Term* const r) : left (l), right (r)
		{
			jassert (l != nullptr && r != nullptr);
		}

		int getInputIndexFor (const Term* possibleInput) const
		{
			return possibleInput == left ? 0 : (possibleInput == right ? 1 : -1);
		}

		Type getType() const noexcept       { return operatorType; }
		int getNumInputs() const            { return 2; }
		Term* getInput (int index) const    { return index == 0 ? left.getObject() : (index == 1 ? right.getObject() : 0); }

		virtual double performFunction (double left, double right) const = 0;
		virtual void writeOperator (String& dest) const = 0;

		TermPtr resolve (const Scope& scope, int recursionDepth)
		{
			return new Constant (performFunction (left ->resolve (scope, recursionDepth)->toDouble(),
												  right->resolve (scope, recursionDepth)->toDouble()), false);
		}

		String toString() const
		{
			String s;

			const int ourPrecendence = getOperatorPrecedence();
			if (left->getOperatorPrecedence() > ourPrecendence)
				s << '(' << left->toString() << ')';
			else
				s = left->toString();

			writeOperator (s);

			if (right->getOperatorPrecedence() >= ourPrecendence)
				s << '(' << right->toString() << ')';
			else
				s << right->toString();

			return s;
		}

	protected:
		const TermPtr left, right;

		TermPtr createDestinationTerm (const Scope& scope, const Term* input, double overallTarget, Term* topLevelTerm) const
		{
			jassert (input == left || input == right);
			if (input != left && input != right)
				return nullptr;

			const Term* const dest = findDestinationFor (topLevelTerm, this);

			if (dest == nullptr)
				return new Constant (overallTarget, false);

			return dest->createTermToEvaluateInput (scope, this, overallTarget, topLevelTerm);
		}
	};

	class SymbolTerm  : public Term
	{
	public:
		explicit SymbolTerm (const String& symbol_) : symbol (symbol_) {}

		TermPtr resolve (const Scope& scope, int recursionDepth)
		{
			checkRecursionDepth (recursionDepth);
			return scope.getSymbolValue (symbol).term->resolve (scope, recursionDepth + 1);
		}

		Type getType() const noexcept   { return symbolType; }
		Term* clone() const             { return new SymbolTerm (symbol); }
		String toString() const         { return symbol; }
		String getName() const          { return symbol; }

		void visitAllSymbols (SymbolVisitor& visitor, const Scope& scope, int recursionDepth)
		{
			checkRecursionDepth (recursionDepth);
			visitor.useSymbol (Symbol (scope.getScopeUID(), symbol));
			scope.getSymbolValue (symbol).term->visitAllSymbols (visitor, scope, recursionDepth + 1);
		}

		void renameSymbol (const Symbol& oldSymbol, const String& newName, const Scope& scope, int /*recursionDepth*/)
		{
			if (oldSymbol.symbolName == symbol && scope.getScopeUID() == oldSymbol.scopeUID)
				symbol = newName;
		}

		String symbol;
	};

	class Function  : public Term
	{
	public:
		explicit Function (const String& functionName_)  : functionName (functionName_) {}

		Function (const String& functionName_, const Array<Expression>& parameters_)
			: functionName (functionName_), parameters (parameters_)
		{}

		Type getType() const noexcept   { return functionType; }
		Term* clone() const             { return new Function (functionName, parameters); }
		int getNumInputs() const        { return parameters.size(); }
		Term* getInput (int i) const    { return parameters.getReference(i).term; }
		String getName() const          { return functionName; }

		TermPtr resolve (const Scope& scope, int recursionDepth)
		{
			checkRecursionDepth (recursionDepth);
			double result = 0;
			const int numParams = parameters.size();
			if (numParams > 0)
			{
				HeapBlock<double> params ((size_t) numParams);
				for (int i = 0; i < numParams; ++i)
					params[i] = parameters.getReference(i).term->resolve (scope, recursionDepth + 1)->toDouble();

				result = scope.evaluateFunction (functionName, params, numParams);
			}
			else
			{
				result = scope.evaluateFunction (functionName, nullptr, 0);
			}

			return new Constant (result, false);
		}

		int getInputIndexFor (const Term* possibleInput) const
		{
			for (int i = 0; i < parameters.size(); ++i)
				if (parameters.getReference(i).term == possibleInput)
					return i;

			return -1;
		}

		String toString() const
		{
			if (parameters.size() == 0)
				return functionName + "()";

			String s (functionName + " (");

			for (int i = 0; i < parameters.size(); ++i)
			{
				s << parameters.getReference(i).term->toString();

				if (i < parameters.size() - 1)
					s << ", ";
			}

			s << ')';
			return s;
		}

		const String functionName;
		Array<Expression> parameters;
	};

	class DotOperator  : public BinaryTerm
	{
	public:
		DotOperator (SymbolTerm* const l, Term* const r)  : BinaryTerm (l, r) {}

		TermPtr resolve (const Scope& scope, int recursionDepth)
		{
			checkRecursionDepth (recursionDepth);

			EvaluationVisitor visitor (right, recursionDepth + 1);
			scope.visitRelativeScope (getSymbol()->symbol, visitor);
			return visitor.output;
		}

		Term* clone() const                             { return new DotOperator (getSymbol(), right); }
		String getName() const                          { return "."; }
		int getOperatorPrecedence() const               { return 1; }
		void writeOperator (String& dest) const         { dest << '.'; }
		double performFunction (double, double) const   { return 0.0; }

		void visitAllSymbols (SymbolVisitor& visitor, const Scope& scope, int recursionDepth)
		{
			checkRecursionDepth (recursionDepth);
			visitor.useSymbol (Symbol (scope.getScopeUID(), getSymbol()->symbol));

			SymbolVisitingVisitor v (right, visitor, recursionDepth + 1);

			try
			{
				scope.visitRelativeScope (getSymbol()->symbol, v);
			}
			catch (...) {}
		}

		void renameSymbol (const Symbol& oldSymbol, const String& newName, const Scope& scope, int recursionDepth)
		{
			checkRecursionDepth (recursionDepth);
			getSymbol()->renameSymbol (oldSymbol, newName, scope, recursionDepth);

			SymbolRenamingVisitor visitor (right, oldSymbol, newName, recursionDepth + 1);

			try
			{
				scope.visitRelativeScope (getSymbol()->symbol, visitor);
			}
			catch (...) {}
		}

	private:

		class EvaluationVisitor  : public Scope::Visitor
		{
		public:
			EvaluationVisitor (const TermPtr& input_, const int recursionCount_)
				: input (input_), output (input_), recursionCount (recursionCount_) {}

			void visit (const Scope& scope)   { output = input->resolve (scope, recursionCount); }

			const TermPtr input;
			TermPtr output;
			const int recursionCount;

		private:
			JUCE_DECLARE_NON_COPYABLE (EvaluationVisitor);
		};

		class SymbolVisitingVisitor  : public Scope::Visitor
		{
		public:
			SymbolVisitingVisitor (const TermPtr& input_, SymbolVisitor& visitor_, const int recursionCount_)
				: input (input_), visitor (visitor_), recursionCount (recursionCount_) {}

			void visit (const Scope& scope)   { input->visitAllSymbols (visitor, scope, recursionCount); }

		private:
			const TermPtr input;
			SymbolVisitor& visitor;
			const int recursionCount;

			JUCE_DECLARE_NON_COPYABLE (SymbolVisitingVisitor);
		};

		class SymbolRenamingVisitor   : public Scope::Visitor
		{
		public:
			SymbolRenamingVisitor (const TermPtr& input_, const Expression::Symbol& symbol_, const String& newName_, const int recursionCount_)
				: input (input_), symbol (symbol_), newName (newName_), recursionCount (recursionCount_)  {}

			void visit (const Scope& scope)   { input->renameSymbol (symbol, newName, scope, recursionCount); }

		private:
			const TermPtr input;
			const Symbol& symbol;
			const String newName;
			const int recursionCount;

			JUCE_DECLARE_NON_COPYABLE (SymbolRenamingVisitor);
		};

		SymbolTerm* getSymbol() const  { return static_cast <SymbolTerm*> (left.getObject()); }

		JUCE_DECLARE_NON_COPYABLE (DotOperator);
	};

	class Negate  : public Term
	{
	public:
		explicit Negate (const TermPtr& input_) : input (input_)
		{
			jassert (input_ != nullptr);
		}

		Type getType() const noexcept                           { return operatorType; }
		int getInputIndexFor (const Term* possibleInput) const  { return possibleInput == input ? 0 : -1; }
		int getNumInputs() const                                { return 1; }
		Term* getInput (int index) const                        { return index == 0 ? input.getObject() : nullptr; }
		Term* clone() const                                     { return new Negate (input->clone()); }

		TermPtr resolve (const Scope& scope, int recursionDepth)
		{
			return new Constant (-input->resolve (scope, recursionDepth)->toDouble(), false);
		}

		String getName() const          { return "-"; }
		TermPtr negated()               { return input; }

		TermPtr createTermToEvaluateInput (const Scope& scope, const Term* input_, double overallTarget, Term* topLevelTerm) const
		{
			(void) input_;
			jassert (input_ == input);

			const Term* const dest = findDestinationFor (topLevelTerm, this);

			return new Negate (dest == nullptr ? new Constant (overallTarget, false)
											   : dest->createTermToEvaluateInput (scope, this, overallTarget, topLevelTerm));
		}

		String toString() const
		{
			if (input->getOperatorPrecedence() > 0)
				return "-(" + input->toString() + ")";
			else
				return "-" + input->toString();
		}

	private:
		const TermPtr input;
	};

	class Add  : public BinaryTerm
	{
	public:
		Add (Term* const l, Term* const r) : BinaryTerm (l, r) {}

		Term* clone() const                     { return new Add (left->clone(), right->clone()); }
		double performFunction (double lhs, double rhs) const    { return lhs + rhs; }
		int getOperatorPrecedence() const       { return 3; }
		String getName() const                  { return "+"; }
		void writeOperator (String& dest) const { dest << " + "; }

		TermPtr createTermToEvaluateInput (const Scope& scope, const Term* input, double overallTarget, Term* topLevelTerm) const
		{
			const TermPtr newDest (createDestinationTerm (scope, input, overallTarget, topLevelTerm));
			if (newDest == nullptr)
				return nullptr;

			return new Subtract (newDest, (input == left ? right : left)->clone());
		}

	private:
		JUCE_DECLARE_NON_COPYABLE (Add);
	};

	class Subtract  : public BinaryTerm
	{
	public:
		Subtract (Term* const l, Term* const r) : BinaryTerm (l, r) {}

		Term* clone() const                     { return new Subtract (left->clone(), right->clone()); }
		double performFunction (double lhs, double rhs) const    { return lhs - rhs; }
		int getOperatorPrecedence() const       { return 3; }
		String getName() const                  { return "-"; }
		void writeOperator (String& dest) const { dest << " - "; }

		TermPtr createTermToEvaluateInput (const Scope& scope, const Term* input, double overallTarget, Term* topLevelTerm) const
		{
			const TermPtr newDest (createDestinationTerm (scope, input, overallTarget, topLevelTerm));
			if (newDest == nullptr)
				return nullptr;

			if (input == left)
				return new Add (newDest, right->clone());
			else
				return new Subtract (left->clone(), newDest);
		}

	private:
		JUCE_DECLARE_NON_COPYABLE (Subtract);
	};

	class Multiply  : public BinaryTerm
	{
	public:
		Multiply (Term* const l, Term* const r) : BinaryTerm (l, r) {}

		Term* clone() const                     { return new Multiply (left->clone(), right->clone()); }
		double performFunction (double lhs, double rhs) const    { return lhs * rhs; }
		String getName() const                  { return "*"; }
		void writeOperator (String& dest) const { dest << " * "; }
		int getOperatorPrecedence() const       { return 2; }

		TermPtr createTermToEvaluateInput (const Scope& scope, const Term* input, double overallTarget, Term* topLevelTerm) const
		{
			const TermPtr newDest (createDestinationTerm (scope, input, overallTarget, topLevelTerm));
			if (newDest == nullptr)
				return nullptr;

			return new Divide (newDest, (input == left ? right : left)->clone());
		}

	private:
		JUCE_DECLARE_NON_COPYABLE (Multiply);
	};

	class Divide  : public BinaryTerm
	{
	public:
		Divide (Term* const l, Term* const r) : BinaryTerm (l, r) {}

		Term* clone() const                     { return new Divide (left->clone(), right->clone()); }
		double performFunction (double lhs, double rhs) const    { return lhs / rhs; }
		String getName() const                  { return "/"; }
		void writeOperator (String& dest) const { dest << " / "; }
		int getOperatorPrecedence() const       { return 2; }

		TermPtr createTermToEvaluateInput (const Scope& scope, const Term* input, double overallTarget, Term* topLevelTerm) const
		{
			const TermPtr newDest (createDestinationTerm (scope, input, overallTarget, topLevelTerm));
			if (newDest == nullptr)
				return nullptr;

			if (input == left)
				return new Multiply (newDest, right->clone());
			else
				return new Divide (left->clone(), newDest);
		}

	private:
		JUCE_DECLARE_NON_COPYABLE (Divide);
	};

	static Term* findDestinationFor (Term* const topLevel, const Term* const inputTerm)
	{
		const int inputIndex = topLevel->getInputIndexFor (inputTerm);
		if (inputIndex >= 0)
			return topLevel;

		for (int i = topLevel->getNumInputs(); --i >= 0;)
		{
			Term* const t = findDestinationFor (topLevel->getInput (i), inputTerm);

			if (t != nullptr)
				return t;
		}

		return nullptr;
	}

	static Constant* findTermToAdjust (Term* const term, const bool mustBeFlagged)
	{
		jassert (term != nullptr);

		if (term->getType() == constantType)
		{
			Constant* const c = static_cast<Constant*> (term);
			if (c->isResolutionTarget || ! mustBeFlagged)
				return c;
		}

		if (term->getType() == functionType)
			return nullptr;

		const int numIns = term->getNumInputs();

		for (int i = 0; i < numIns; ++i)
		{
			Term* const input = term->getInput (i);

			if (input->getType() == constantType)
			{
				Constant* const c = static_cast<Constant*> (input);

				if (c->isResolutionTarget || ! mustBeFlagged)
					return c;
			}
		}

		for (int i = 0; i < numIns; ++i)
		{
			Constant* const c = findTermToAdjust (term->getInput (i), mustBeFlagged);
			if (c != nullptr)
				return c;
		}

		return nullptr;
	}

	static bool containsAnySymbols (const Term* const t)
	{
		if (t->getType() == Expression::symbolType)
			return true;

		for (int i = t->getNumInputs(); --i >= 0;)
			if (containsAnySymbols (t->getInput (i)))
				return true;

		return false;
	}

	class SymbolCheckVisitor  : public Term::SymbolVisitor
	{
	public:
		SymbolCheckVisitor (const Symbol& symbol_) : wasFound (false), symbol (symbol_) {}
		void useSymbol (const Symbol& s)    { wasFound = wasFound || s == symbol; }

		bool wasFound;

	private:
		const Symbol& symbol;

		JUCE_DECLARE_NON_COPYABLE (SymbolCheckVisitor);
	};

	class SymbolListVisitor  : public Term::SymbolVisitor
	{
	public:
		SymbolListVisitor (Array<Symbol>& list_) : list (list_) {}
		void useSymbol (const Symbol& s)    { list.addIfNotAlreadyThere (s); }

	private:
		Array<Symbol>& list;

		JUCE_DECLARE_NON_COPYABLE (SymbolListVisitor);
	};

	class Parser
	{
	public:

		Parser (String::CharPointerType& stringToParse)
			: text (stringToParse)
		{
		}

		TermPtr readUpToComma()
		{
			if (text.isEmpty())
				return new Constant (0.0, false);

			const TermPtr e (readExpression());

			if (e == nullptr || ((! readOperator (",")) && ! text.isEmpty()))
				throw ParseError ("Syntax error: \"" + String (text) + "\"");

			return e;
		}

	private:
		String::CharPointerType& text;

		static inline bool isDecimalDigit (const juce_wchar c) noexcept
		{
			return c >= '0' && c <= '9';
		}

		bool readChar (const juce_wchar required) noexcept
		{
			if (*text == required)
			{
				++text;
				return true;
			}

			return false;
		}

		bool readOperator (const char* ops, char* const opType = nullptr) noexcept
		{
			text = text.findEndOfWhitespace();

			while (*ops != 0)
			{
				if (readChar ((juce_wchar) (uint8) *ops))
				{
					if (opType != nullptr)
						*opType = *ops;

					return true;
				}

				++ops;
			}

			return false;
		}

		bool readIdentifier (String& identifier) noexcept
		{
			text = text.findEndOfWhitespace();
			String::CharPointerType t (text);
			int numChars = 0;

			if (t.isLetter() || *t == '_')
			{
				++t;
				++numChars;

				while (t.isLetterOrDigit() || *t == '_')
				{
					++t;
					++numChars;
				}
			}

			if (numChars > 0)
			{
				identifier = String (text, (size_t) numChars);
				text = t;
				return true;
			}

			return false;
		}

		Term* readNumber() noexcept
		{
			text = text.findEndOfWhitespace();
			String::CharPointerType t (text);

			const bool isResolutionTarget = (*t == '@');
			if (isResolutionTarget)
			{
				++t;
				t = t.findEndOfWhitespace();
				text = t;
			}

			if (*t == '-')
			{
				++t;
				t = t.findEndOfWhitespace();
			}

			if (isDecimalDigit (*t) || (*t == '.' && isDecimalDigit (t[1])))
				return new Constant (CharacterFunctions::readDoubleValue (text), isResolutionTarget);

			return nullptr;
		}

		TermPtr readExpression()
		{
			TermPtr lhs (readMultiplyOrDivideExpression());

			char opType;
			while (lhs != nullptr && readOperator ("+-", &opType))
			{
				TermPtr rhs (readMultiplyOrDivideExpression());

				if (rhs == nullptr)
					throw ParseError ("Expected expression after \"" + String::charToString ((juce_wchar) (uint8) opType) + "\"");

				if (opType == '+')
					lhs = new Add (lhs, rhs);
				else
					lhs = new Subtract (lhs, rhs);
			}

			return lhs;
		}

		TermPtr readMultiplyOrDivideExpression()
		{
			TermPtr lhs (readUnaryExpression());

			char opType;
			while (lhs != nullptr && readOperator ("*/", &opType))
			{
				TermPtr rhs (readUnaryExpression());

				if (rhs == nullptr)
					throw ParseError ("Expected expression after \"" + String::charToString ((juce_wchar) (uint8) opType) + "\"");

				if (opType == '*')
					lhs = new Multiply (lhs, rhs);
				else
					lhs = new Divide (lhs, rhs);
			}

			return lhs;
		}

		TermPtr readUnaryExpression()
		{
			char opType;
			if (readOperator ("+-", &opType))
			{
				TermPtr term (readUnaryExpression());

				if (term == nullptr)
					throw ParseError ("Expected expression after \"" + String::charToString ((juce_wchar) (uint8) opType) + "\"");

				if (opType == '-')
					term = term->negated();

				return term;
			}

			return readPrimaryExpression();
		}

		TermPtr readPrimaryExpression()
		{
			TermPtr e (readParenthesisedExpression());
			if (e != nullptr)
				return e;

			e = readNumber();
			if (e != nullptr)
				return e;

			return readSymbolOrFunction();
		}

		TermPtr readSymbolOrFunction()
		{
			String identifier;
			if (readIdentifier (identifier))
			{
				if (readOperator ("(")) // method call...
				{
					Function* const f = new Function (identifier);
					ScopedPointer<Term> func (f);  // (can't use ScopedPointer<Function> in MSVC)

					TermPtr param (readExpression());

					if (param == nullptr)
					{
						if (readOperator (")"))
							return func.release();

						throw ParseError ("Expected parameters after \"" + identifier + " (\"");
					}

					f->parameters.add (Expression (param));

					while (readOperator (","))
					{
						param = readExpression();

						if (param == nullptr)
							throw ParseError ("Expected expression after \",\"");

						f->parameters.add (Expression (param));
					}

					if (readOperator (")"))
						return func.release();

					throw ParseError ("Expected \")\"");
				}
				else if (readOperator ("."))
				{
					TermPtr rhs (readSymbolOrFunction());

					if (rhs == nullptr)
						throw ParseError ("Expected symbol or function after \".\"");

					if (identifier == "this")
						return rhs;

					return new DotOperator (new SymbolTerm (identifier), rhs);
				}
				else // just a symbol..
				{
					jassert (identifier.trim() == identifier);
					return new SymbolTerm (identifier);
				}
			}

			return nullptr;
		}

		TermPtr readParenthesisedExpression()
		{
			if (! readOperator ("("))
				return nullptr;

			const TermPtr e (readExpression());
			if (e == nullptr || ! readOperator (")"))
				return nullptr;

			return e;
		}

		JUCE_DECLARE_NON_COPYABLE (Parser);
	};
};

Expression::Expression()
	: term (new Expression::Helpers::Constant (0, false))
{
}

Expression::~Expression()
{
}

Expression::Expression (Term* const term_)
	: term (term_)
{
	jassert (term != nullptr);
}

Expression::Expression (const double constant)
	: term (new Expression::Helpers::Constant (constant, false))
{
}

Expression::Expression (const Expression& other)
	: term (other.term)
{
}

Expression& Expression::operator= (const Expression& other)
{
	term = other.term;
	return *this;
}

#if JUCE_COMPILER_SUPPORTS_MOVE_SEMANTICS
Expression::Expression (Expression&& other) noexcept
	: term (static_cast <ReferenceCountedObjectPtr<Term>&&> (other.term))
{
}

Expression& Expression::operator= (Expression&& other) noexcept
{
	term = static_cast <ReferenceCountedObjectPtr<Term>&&> (other.term);
	return *this;
}
#endif

Expression::Expression (const String& stringToParse)
{
	String::CharPointerType text (stringToParse.getCharPointer());
	Helpers::Parser parser (text);
	term = parser.readUpToComma();
}

Expression Expression::parse (String::CharPointerType& stringToParse)
{
	Helpers::Parser parser (stringToParse);
	return Expression (parser.readUpToComma());
}

double Expression::evaluate() const
{
	return evaluate (Expression::Scope());
}

double Expression::evaluate (const Expression::Scope& scope) const
{
	try
	{
		return term->resolve (scope, 0)->toDouble();
	}
	catch (Helpers::EvaluationError&)
	{}

	return 0;
}

double Expression::evaluate (const Scope& scope, String& evaluationError) const
{
	try
	{
		return term->resolve (scope, 0)->toDouble();
	}
	catch (Helpers::EvaluationError& e)
	{
		evaluationError = e.description;
	}

	return 0;
}

Expression Expression::operator+ (const Expression& other) const  { return Expression (new Helpers::Add (term, other.term)); }
Expression Expression::operator- (const Expression& other) const  { return Expression (new Helpers::Subtract (term, other.term)); }
Expression Expression::operator* (const Expression& other) const  { return Expression (new Helpers::Multiply (term, other.term)); }
Expression Expression::operator/ (const Expression& other) const  { return Expression (new Helpers::Divide (term, other.term)); }
Expression Expression::operator-() const                          { return Expression (term->negated()); }
Expression Expression::symbol (const String& symbol)              { return Expression (new Helpers::SymbolTerm (symbol)); }

Expression Expression::function (const String& functionName, const Array<Expression>& parameters)
{
	return Expression (new Helpers::Function (functionName, parameters));
}

Expression Expression::adjustedToGiveNewResult (const double targetValue, const Expression::Scope& scope) const
{
	ScopedPointer<Term> newTerm (term->clone());

	Helpers::Constant* termToAdjust = Helpers::findTermToAdjust (newTerm, true);

	if (termToAdjust == nullptr)
		termToAdjust = Helpers::findTermToAdjust (newTerm, false);

	if (termToAdjust == nullptr)
	{
		newTerm = new Helpers::Add (newTerm.release(), new Helpers::Constant (0, false));
		termToAdjust = Helpers::findTermToAdjust (newTerm, false);
	}

	jassert (termToAdjust != nullptr);

	const Term* const parent = Helpers::findDestinationFor (newTerm, termToAdjust);

	if (parent == nullptr)
	{
		termToAdjust->value = targetValue;
	}
	else
	{
		const Helpers::TermPtr reverseTerm (parent->createTermToEvaluateInput (scope, termToAdjust, targetValue, newTerm));

		if (reverseTerm == nullptr)
			return Expression (targetValue);

		termToAdjust->value = reverseTerm->resolve (scope, 0)->toDouble();
	}

	return Expression (newTerm.release());
}

Expression Expression::withRenamedSymbol (const Expression::Symbol& oldSymbol, const String& newName, const Scope& scope) const
{
	jassert (newName.toLowerCase().containsOnly ("abcdefghijklmnopqrstuvwxyz0123456789_"));

	if (oldSymbol.symbolName == newName)
		return *this;

	Expression e (term->clone());
	e.term->renameSymbol (oldSymbol, newName, scope, 0);
	return e;
}

bool Expression::referencesSymbol (const Expression::Symbol& symbolToCheck, const Scope& scope) const
{
	Helpers::SymbolCheckVisitor visitor (symbolToCheck);

	try
	{
		term->visitAllSymbols (visitor, scope, 0);
	}
	catch (Helpers::EvaluationError&)
	{}

	return visitor.wasFound;
}

void Expression::findReferencedSymbols (Array<Symbol>& results, const Scope& scope) const
{
	try
	{
		Helpers::SymbolListVisitor visitor (results);
		term->visitAllSymbols (visitor, scope, 0);
	}
	catch (Helpers::EvaluationError&)
	{}
}

String Expression::toString() const                     { return term->toString(); }
bool Expression::usesAnySymbols() const                 { return Helpers::containsAnySymbols (term); }
Expression::Type Expression::getType() const noexcept   { return term->getType(); }
String Expression::getSymbolOrFunction() const          { return term->getName(); }
int Expression::getNumInputs() const                    { return term->getNumInputs(); }
Expression Expression::getInput (int index) const       { return Expression (term->getInput (index)); }

ReferenceCountedObjectPtr<Expression::Term> Expression::Term::negated()
{
	return new Helpers::Negate (this);
}

Expression::ParseError::ParseError (const String& message)
	: description (message)
{
	DBG ("Expression::ParseError: " + message);
}

Expression::Symbol::Symbol (const String& scopeUID_, const String& symbolName_)
	: scopeUID (scopeUID_), symbolName (symbolName_)
{
}

bool Expression::Symbol::operator== (const Symbol& other) const noexcept
{
	return symbolName == other.symbolName && scopeUID == other.scopeUID;
}

bool Expression::Symbol::operator!= (const Symbol& other) const noexcept
{
	return ! operator== (other);
}

Expression::Scope::Scope()  {}
Expression::Scope::~Scope() {}

Expression Expression::Scope::getSymbolValue (const String& symbol) const
{
	throw Helpers::EvaluationError ("Unknown symbol: " + symbol);
}

double Expression::Scope::evaluateFunction (const String& functionName, const double* parameters, int numParams) const
{
	if (numParams > 0)
	{
		if (functionName == "min")
		{
			double v = parameters[0];
			for (int i = 1; i < numParams; ++i)
				v = jmin (v, parameters[i]);

			return v;
		}
		else if (functionName == "max")
		{
			double v = parameters[0];
			for (int i = 1; i < numParams; ++i)
				v = jmax (v, parameters[i]);

			return v;
		}
		else if (numParams == 1)
		{
			if      (functionName == "sin")     return sin (parameters[0]);
			else if (functionName == "cos")     return cos (parameters[0]);
			else if (functionName == "tan")     return tan (parameters[0]);
			else if (functionName == "abs")     return std::abs (parameters[0]);
		}
	}

	throw Helpers::EvaluationError ("Unknown function: \"" + functionName + "\"");
}

void Expression::Scope::visitRelativeScope (const String& scopeName, Visitor&) const
{
	throw Helpers::EvaluationError ("Unknown symbol: " + scopeName);
}

String Expression::Scope::getScopeUID() const
{
	return String::empty;
}

/*** End of inlined file: juce_Expression.cpp ***/


/*** Start of inlined file: juce_Random.cpp ***/
Random::Random (const int64 seedValue) noexcept
	: seed (seedValue)
{
}

Random::Random()
	: seed (1)
{
	setSeedRandomly();
}

Random::~Random() noexcept
{
}

void Random::setSeed (const int64 newSeed) noexcept
{
	seed = newSeed;
}

void Random::combineSeed (const int64 seedValue) noexcept
{
	seed ^= nextInt64() ^ seedValue;
}

void Random::setSeedRandomly()
{
	static int64 globalSeed = 0;

	combineSeed (globalSeed ^ (int64) (pointer_sized_int) this);
	combineSeed (Time::getMillisecondCounter());
	combineSeed (Time::getHighResolutionTicks());
	combineSeed (Time::getHighResolutionTicksPerSecond());
	combineSeed (Time::currentTimeMillis());
	globalSeed ^= seed;
}

Random& Random::getSystemRandom() noexcept
{
	static Random sysRand;
	return sysRand;
}

int Random::nextInt() noexcept
{
	seed = (seed * literal64bit (0x5deece66d) + 11) & literal64bit (0xffffffffffff);

	return (int) (seed >> 16);
}

int Random::nextInt (const int maxValue) noexcept
{
	jassert (maxValue > 0);
	return (int) ((((unsigned int) nextInt()) * (uint64) maxValue) >> 32);
}

int64 Random::nextInt64() noexcept
{
	return (((int64) nextInt()) << 32) | (int64) (uint64) (uint32) nextInt();
}

bool Random::nextBool() noexcept
{
	return (nextInt() & 0x40000000) != 0;
}

float Random::nextFloat() noexcept
{
	return static_cast <uint32> (nextInt()) / (float) 0xffffffff;
}

double Random::nextDouble() noexcept
{
	return static_cast <uint32> (nextInt()) / (double) 0xffffffff;
}

BigInteger Random::nextLargeNumber (const BigInteger& maximumValue)
{
	BigInteger n;

	do
	{
		fillBitsRandomly (n, 0, maximumValue.getHighestBit() + 1);
	}
	while (n >= maximumValue);

	return n;
}

void Random::fillBitsRandomly (BigInteger& arrayToChange, int startBit, int numBits)
{
	arrayToChange.setBit (startBit + numBits - 1, true);  // to force the array to pre-allocate space

	while ((startBit & 31) != 0 && numBits > 0)
	{
		arrayToChange.setBit (startBit++, nextBool());
		--numBits;
	}

	while (numBits >= 32)
	{
		arrayToChange.setBitRangeAsInt (startBit, 32, (unsigned int) nextInt());
		startBit += 32;
		numBits -= 32;
	}

	while (--numBits >= 0)
		arrayToChange.setBit (startBit + numBits, nextBool());
}

#if JUCE_UNIT_TESTS

class RandomTests  : public UnitTest
{
public:
	RandomTests() : UnitTest ("Random") {}

	void runTest()
	{
		beginTest ("Random");

		for (int j = 10; --j >= 0;)
		{
			Random r;
			r.setSeedRandomly();

			for (int i = 20; --i >= 0;)
			{
				expect (r.nextDouble() >= 0.0 && r.nextDouble() < 1.0);
				expect (r.nextFloat() >= 0.0f && r.nextFloat() < 1.0f);
				expect (r.nextInt (5) >= 0 && r.nextInt (5) < 5);
				expect (r.nextInt (1) == 0);

				int n = r.nextInt (50) + 1;
				expect (r.nextInt (n) >= 0 && r.nextInt (n) < n);

				n = r.nextInt (0x7ffffffe) + 1;
				expect (r.nextInt (n) >= 0 && r.nextInt (n) < n);
			}
		}
	}
};

static RandomTests randomTests;

#endif

/*** End of inlined file: juce_Random.cpp ***/


/*** Start of inlined file: juce_MemoryBlock.cpp ***/
MemoryBlock::MemoryBlock() noexcept
	: size (0)
{
}

MemoryBlock::MemoryBlock (const size_t initialSize, const bool initialiseToZero)
{
	if (initialSize > 0)
	{
		size = initialSize;
		data.allocate (initialSize, initialiseToZero);
	}
	else
	{
		size = 0;
	}
}

MemoryBlock::MemoryBlock (const MemoryBlock& other)
	: size (other.size)
{
	if (size > 0)
	{
		jassert (other.data != nullptr);
		data.malloc (size);
		memcpy (data, other.data, size);
	}
}

MemoryBlock::MemoryBlock (const void* const dataToInitialiseFrom, const size_t sizeInBytes)
	: size (jmax ((size_t) 0, sizeInBytes))
{
	jassert (sizeInBytes >= 0);

	if (size > 0)
	{
		jassert (dataToInitialiseFrom != nullptr); // non-zero size, but a zero pointer passed-in?

		data.malloc (size);

		if (dataToInitialiseFrom != nullptr)
			memcpy (data, dataToInitialiseFrom, size);
	}
}

MemoryBlock::~MemoryBlock() noexcept
{
}

MemoryBlock& MemoryBlock::operator= (const MemoryBlock& other)
{
	if (this != &other)
	{
		setSize (other.size, false);
		memcpy (data, other.data, size);
	}

	return *this;
}

#if JUCE_COMPILER_SUPPORTS_MOVE_SEMANTICS
MemoryBlock::MemoryBlock (MemoryBlock&& other) noexcept
	: data (static_cast <HeapBlock <char>&&> (other.data)),
	  size (other.size)
{
}

MemoryBlock& MemoryBlock::operator= (MemoryBlock&& other) noexcept
{
	data = static_cast <HeapBlock <char>&&> (other.data);
	size = other.size;
	return *this;
}
#endif

bool MemoryBlock::operator== (const MemoryBlock& other) const noexcept
{
	return matches (other.data, other.size);
}

bool MemoryBlock::operator!= (const MemoryBlock& other) const noexcept
{
	return ! operator== (other);
}

bool MemoryBlock::matches (const void* dataToCompare, size_t dataSize) const noexcept
{
	return size == dataSize
			&& memcmp (data, dataToCompare, size) == 0;
}

// this will resize the block to this size
void MemoryBlock::setSize (const size_t newSize, const bool initialiseToZero)
{
	if (size != newSize)
	{
		if (newSize <= 0)
		{
			data.free();
			size = 0;
		}
		else
		{
			if (data != nullptr)
			{
				data.realloc (newSize);

				if (initialiseToZero && (newSize > size))
					zeromem (data + size, newSize - size);
			}
			else
			{
				data.allocate (newSize, initialiseToZero);
			}

			size = newSize;
		}
	}
}

void MemoryBlock::ensureSize (const size_t minimumSize, const bool initialiseToZero)
{
	if (size < minimumSize)
		setSize (minimumSize, initialiseToZero);
}

void MemoryBlock::swapWith (MemoryBlock& other) noexcept
{
	std::swap (size, other.size);
	data.swapWith (other.data);
}

void MemoryBlock::fillWith (const uint8 value) noexcept
{
	memset (data, (int) value, size);
}

void MemoryBlock::append (const void* const srcData, const size_t numBytes)
{
	if (numBytes > 0)
	{
		jassert (srcData != nullptr); // this must not be null!
		const size_t oldSize = size;
		setSize (size + numBytes);
		memcpy (data + oldSize, srcData, numBytes);
	}
}

void MemoryBlock::insert (const void* const srcData, const size_t numBytes, size_t insertPosition)
{
	if (numBytes > 0)
	{
		jassert (srcData != nullptr); // this must not be null!
		insertPosition = jmin (size, insertPosition);
		const size_t trailingDataSize = size - insertPosition;
		setSize (size + numBytes, false);

		if (trailingDataSize > 0)
			memmove (data + insertPosition + numBytes,
					 data + insertPosition,
					 trailingDataSize);

		memcpy (data + insertPosition, srcData, numBytes);
	}
}

void MemoryBlock::removeSection (const size_t startByte, const size_t numBytesToRemove)
{
	if (startByte + numBytesToRemove >= size)
	{
		setSize (startByte);
	}
	else if (numBytesToRemove > 0)
	{
		memmove (data + startByte,
				 data + startByte + numBytesToRemove,
				 size - (startByte + numBytesToRemove));

		setSize (size - numBytesToRemove);
	}
}

void MemoryBlock::copyFrom (const void* const src, int offset, size_t num) noexcept
{
	const char* d = static_cast<const char*> (src);

	if (offset < 0)
	{
		d -= offset;
		num -= offset;
		offset = 0;
	}

	if (offset + num > size)
		num = size - offset;

	if (num > 0)
		memcpy (data + offset, d, num);
}

void MemoryBlock::copyTo (void* const dst, int offset, size_t num) const noexcept
{
	char* d = static_cast<char*> (dst);

	if (offset < 0)
	{
		zeromem (d, (size_t) -offset);
		d -= offset;

		num += offset;
		offset = 0;
	}

	if (offset + num > size)
	{
		const size_t newNum = size - offset;
		zeromem (d + newNum, num - newNum);
		num = newNum;
	}

	if (num > 0)
		memcpy (d, data + offset, num);
}

String MemoryBlock::toString() const
{
	return String (CharPointer_UTF8 (data), size);
}

int MemoryBlock::getBitRange (const size_t bitRangeStart, size_t numBits) const noexcept
{
	int res = 0;

	size_t byte = bitRangeStart >> 3;
	int offsetInByte = (int) bitRangeStart & 7;
	size_t bitsSoFar = 0;

	while (numBits > 0 && (size_t) byte < size)
	{
		const int bitsThisTime = jmin ((int) numBits, 8 - offsetInByte);
		const int mask = (0xff >> (8 - bitsThisTime)) << offsetInByte;

		res |= (((data[byte] & mask) >> offsetInByte) << bitsSoFar);

		bitsSoFar += bitsThisTime;
		numBits -= bitsThisTime;
		++byte;
		offsetInByte = 0;
	}

	return res;
}

void MemoryBlock::setBitRange (const size_t bitRangeStart, size_t numBits, int bitsToSet) noexcept
{
	size_t byte = bitRangeStart >> 3;
	int offsetInByte = (int) bitRangeStart & 7;
	unsigned int mask = ~((((unsigned int) 0xffffffff) << (32 - numBits)) >> (32 - numBits));

	while (numBits > 0 && (size_t) byte < size)
	{
		const int bitsThisTime = jmin ((int) numBits, 8 - offsetInByte);

		const unsigned int tempMask = (mask << offsetInByte) | ~((((unsigned int) 0xffffffff) >> offsetInByte) << offsetInByte);
		const unsigned int tempBits = (unsigned int) bitsToSet << offsetInByte;

		data[byte] = (char) ((data[byte] & tempMask) | tempBits);

		++byte;
		numBits -= bitsThisTime;
		bitsToSet >>= bitsThisTime;
		mask >>= bitsThisTime;
		offsetInByte = 0;
	}
}

void MemoryBlock::loadFromHexString (const String& hex)
{
	ensureSize ((size_t) hex.length() >> 1);
	char* dest = data;
	String::CharPointerType t (hex.getCharPointer());

	for (;;)
	{
		int byte = 0;

		for (int loop = 2; --loop >= 0;)
		{
			byte <<= 4;

			for (;;)
			{
				const juce_wchar c = t.getAndAdvance();

				if (c >= '0' && c <= '9')
				{
					byte |= c - '0';
					break;
				}
				else if (c >= 'a' && c <= 'z')
				{
					byte |= c - ('a' - 10);
					break;
				}
				else if (c >= 'A' && c <= 'Z')
				{
					byte |= c - ('A' - 10);
					break;
				}
				else if (c == 0)
				{
					setSize (static_cast <size_t> (dest - data));
					return;
				}
			}
		}

		*dest++ = (char) byte;
	}
}

const char* const MemoryBlock::encodingTable = ".ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+";

String MemoryBlock::toBase64Encoding() const
{
	const size_t numChars = ((size << 3) + 5) / 6;

	String destString ((unsigned int) size); // store the length, followed by a '.', and then the data.
	const int initialLen = destString.length();
	destString.preallocateBytes (sizeof (String::CharPointerType::CharType) * (initialLen + 2 + numChars));

	String::CharPointerType d (destString.getCharPointer());
	d += initialLen;
	d.write ('.');

	for (size_t i = 0; i < numChars; ++i)
		d.write ((juce_wchar) (uint8) encodingTable [getBitRange (i * 6, 6)]);

	d.writeNull();
	return destString;
}

bool MemoryBlock::fromBase64Encoding (const String& s)
{
	const int startPos = s.indexOfChar ('.') + 1;

	if (startPos <= 0)
		return false;

	const int numBytesNeeded = s.substring (0, startPos - 1).getIntValue();

	setSize ((size_t) numBytesNeeded, true);

	const int numChars = s.length() - startPos;

	String::CharPointerType srcChars (s.getCharPointer());
	srcChars += startPos;
	int pos = 0;

	for (int i = 0; i < numChars; ++i)
	{
		const char c = (char) srcChars.getAndAdvance();

		for (int j = 0; j < 64; ++j)
		{
			if (encodingTable[j] == c)
			{
				setBitRange ((size_t) pos, 6, j);
				pos += 6;
				break;
			}
		}
	}

	return true;
}

/*** End of inlined file: juce_MemoryBlock.cpp ***/


/*** Start of inlined file: juce_Result.cpp ***/
Result::Result (const String& message) noexcept
	: errorMessage (message)
{
}

Result::Result (const Result& other)
	: errorMessage (other.errorMessage)
{
}

Result& Result::operator= (const Result& other)
{
	errorMessage = other.errorMessage;
	return *this;
}

#if JUCE_COMPILER_SUPPORTS_MOVE_SEMANTICS
Result::Result (Result&& other) noexcept
	: errorMessage (static_cast <String&&> (other.errorMessage))
{
}

Result& Result::operator= (Result&& other) noexcept
{
	errorMessage = static_cast <String&&> (other.errorMessage);
	return *this;
}
#endif

bool Result::operator== (const Result& other) const noexcept
{
	return errorMessage == other.errorMessage;
}

bool Result::operator!= (const Result& other) const noexcept
{
	return errorMessage != other.errorMessage;
}

Result Result::ok() noexcept
{
	return Result (String::empty);
}

Result Result::fail (const String& errorMessage) noexcept
{
	return Result (errorMessage.isEmpty() ? "Unknown Error" : errorMessage);
}

const String& Result::getErrorMessage() const noexcept
{
	return errorMessage;
}

bool Result::wasOk() const noexcept         { return errorMessage.isEmpty(); }
Result::operator bool() const noexcept      { return errorMessage.isEmpty(); }
bool Result::failed() const noexcept        { return errorMessage.isNotEmpty(); }
bool Result::operator!() const noexcept     { return errorMessage.isNotEmpty(); }

/*** End of inlined file: juce_Result.cpp ***/


/*** Start of inlined file: juce_Uuid.cpp ***/
namespace
{
	int64 getRandomSeedFromMACAddresses()
	{
		Array<MACAddress> result;
		MACAddress::findAllAddresses (result);

		Random r;
		for (int i = 0; i < result.size(); ++i)
			r.combineSeed (result[i].toInt64());

		return r.nextInt64();
	}
}

Uuid::Uuid()
{
	// The normal random seeding is pretty good, but we'll throw some MAC addresses
	// into the mix too, to make it very very unlikely that two UUIDs will ever be the same..

	static Random r1 (getRandomSeedFromMACAddresses());
	Random r2;

	for (size_t i = 0; i < sizeof (uuid); ++i)
		uuid[i] = (uint8) (r1.nextInt() ^ r2.nextInt());
}

Uuid::~Uuid() noexcept {}

Uuid::Uuid (const Uuid& other) noexcept
{
	memcpy (uuid, other.uuid, sizeof (uuid));
}

Uuid& Uuid::operator= (const Uuid& other) noexcept
{
	memcpy (uuid, other.uuid, sizeof (uuid));
	return *this;
}

bool Uuid::operator== (const Uuid& other) const noexcept    { return memcmp (uuid, other.uuid, sizeof (uuid)) == 0; }
bool Uuid::operator!= (const Uuid& other) const noexcept    { return ! operator== (other); }

bool Uuid::isNull() const noexcept
{
	for (size_t i = 0; i < sizeof (uuid); ++i)
		if (uuid[i] != 0)
			return false;

	return true;
}

String Uuid::toString() const
{
	return String::toHexString (uuid, sizeof (uuid), 0);
}

Uuid::Uuid (const String& uuidString)
{
	operator= (uuidString);
}

Uuid& Uuid::operator= (const String& uuidString)
{
	MemoryBlock mb;
	mb.loadFromHexString (uuidString);
	mb.ensureSize (sizeof (uuid), true);
	mb.copyTo (uuid, 0, sizeof (uuid));
	return *this;
}

Uuid::Uuid (const uint8* const rawData)
{
	operator= (rawData);
}

Uuid& Uuid::operator= (const uint8* const rawData) noexcept
{
	if (rawData != nullptr)
		memcpy (uuid, rawData, sizeof (uuid));
	else
		zeromem (uuid, sizeof (uuid));

	return *this;
}

/*** End of inlined file: juce_Uuid.cpp ***/


/*** Start of inlined file: juce_MACAddress.cpp ***/
MACAddress::MACAddress()
{
	zeromem (address, sizeof (address));
}

MACAddress::MACAddress (const MACAddress& other)
{
	memcpy (address, other.address, sizeof (address));
}

MACAddress& MACAddress::operator= (const MACAddress& other)
{
	memcpy (address, other.address, sizeof (address));
	return *this;
}

MACAddress::MACAddress (const uint8 bytes[6])
{
	memcpy (address, bytes, sizeof (address));
}

String MACAddress::toString() const
{
	String s;

	for (size_t i = 0; i < sizeof (address); ++i)
	{
		s << String::toHexString ((int) address[i]).paddedLeft ('0', 2);

		if (i < sizeof (address) - 1)
			s << '-';
	}

	return s;
}

int64 MACAddress::toInt64() const noexcept
{
	int64 n = 0;

	for (int i = (int) sizeof (address); --i >= 0;)
		n = (n << 8) | address[i];

	return n;
}

bool MACAddress::isNull() const noexcept                                { return toInt64() == 0; }

bool MACAddress::operator== (const MACAddress& other) const noexcept    { return memcmp (address, other.address, sizeof (address)) == 0; }
bool MACAddress::operator!= (const MACAddress& other) const noexcept    { return ! operator== (other); }

/*** End of inlined file: juce_MACAddress.cpp ***/


/*** Start of inlined file: juce_NamedPipe.cpp ***/
bool NamedPipe::openExisting (const String& pipeName)
{
	currentPipeName = pipeName;
	return openInternal (pipeName, false);
}

bool NamedPipe::createNewPipe (const String& pipeName)
{
	currentPipeName = pipeName;
	return openInternal (pipeName, true);
}

String NamedPipe::getName() const
{
	return currentPipeName;
}

// other methods for this class are implemented in the platform-specific files

/*** End of inlined file: juce_NamedPipe.cpp ***/


/*** Start of inlined file: juce_Socket.cpp ***/
#if JUCE_MSVC
 #pragma warning (push)
 #pragma warning (disable : 4127 4389 4018)
#endif

#ifndef AI_NUMERICSERV  // (missing in older Mac SDKs)
 #define AI_NUMERICSERV 0x1000
#endif

#if JUCE_WINDOWS
 typedef int       juce_socklen_t;
#else
 typedef socklen_t juce_socklen_t;
#endif

namespace SocketHelpers
{
	static void initSockets()
	{
	   #if JUCE_WINDOWS
		static bool socketsStarted = false;

		if (! socketsStarted)
		{
			socketsStarted = true;

			WSADATA wsaData;
			const WORD wVersionRequested = MAKEWORD (1, 1);
			WSAStartup (wVersionRequested, &wsaData);
		}
	   #endif
	}

	static bool resetSocketOptions (const int handle, const bool isDatagram, const bool allowBroadcast) noexcept
	{
		const int sndBufSize = 65536;
		const int rcvBufSize = 65536;
		const int one = 1;

		return handle > 0
				&& setsockopt (handle, SOL_SOCKET, SO_RCVBUF, (const char*) &rcvBufSize, sizeof (rcvBufSize)) == 0
				&& setsockopt (handle, SOL_SOCKET, SO_SNDBUF, (const char*) &sndBufSize, sizeof (sndBufSize)) == 0
				&& (isDatagram ? ((! allowBroadcast) || setsockopt (handle, SOL_SOCKET, SO_BROADCAST, (const char*) &one, sizeof (one)) == 0)
							   : (setsockopt (handle, IPPROTO_TCP, TCP_NODELAY, (const char*) &one, sizeof (one)) == 0));
	}

	static bool bindSocketToPort (const int handle, const int port) noexcept
	{
		if (handle <= 0 || port <= 0)
			return false;

		struct sockaddr_in servTmpAddr = { 0 };
		servTmpAddr.sin_family = PF_INET;
		servTmpAddr.sin_addr.s_addr = htonl (INADDR_ANY);
		servTmpAddr.sin_port = htons ((uint16) port);

		return bind (handle, (struct sockaddr*) &servTmpAddr, sizeof (struct sockaddr_in)) >= 0;
	}

	static int readSocket (const int handle,
						   void* const destBuffer, const int maxBytesToRead,
						   bool volatile& connected,
						   const bool blockUntilSpecifiedAmountHasArrived) noexcept
	{
		int bytesRead = 0;

		while (bytesRead < maxBytesToRead)
		{
			int bytesThisTime;

		   #if JUCE_WINDOWS
			bytesThisTime = recv (handle, static_cast<char*> (destBuffer) + bytesRead, maxBytesToRead - bytesRead, 0);
		   #else
			while ((bytesThisTime = (int) ::read (handle, addBytesToPointer (destBuffer, bytesRead), maxBytesToRead - bytesRead)) < 0
					 && errno == EINTR
					 && connected)
			{
			}
		   #endif

			if (bytesThisTime <= 0 || ! connected)
			{
				if (bytesRead == 0)
					bytesRead = -1;

				break;
			}

			bytesRead += bytesThisTime;

			if (! blockUntilSpecifiedAmountHasArrived)
				break;
		}

		return bytesRead;
	}

	static int waitForReadiness (const int handle, const bool forReading, const int timeoutMsecs) noexcept
	{
		struct timeval timeout;
		struct timeval* timeoutp;

		if (timeoutMsecs >= 0)
		{
			timeout.tv_sec = timeoutMsecs / 1000;
			timeout.tv_usec = (timeoutMsecs % 1000) * 1000;
			timeoutp = &timeout;
		}
		else
		{
			timeoutp = 0;
		}

		fd_set rset, wset;
		FD_ZERO (&rset);
		FD_SET (handle, &rset);
		FD_ZERO (&wset);
		FD_SET (handle, &wset);

		fd_set* const prset = forReading ? &rset : nullptr;
		fd_set* const pwset = forReading ? nullptr : &wset;

	   #if JUCE_WINDOWS
		if (select (handle + 1, prset, pwset, 0, timeoutp) < 0)
			return -1;
	   #else
		{
			int result;
			while ((result = select (handle + 1, prset, pwset, 0, timeoutp)) < 0
					&& errno == EINTR)
			{
			}

			if (result < 0)
				return -1;
		}
	   #endif

		{
			int opt;
			juce_socklen_t len = sizeof (opt);

			if (getsockopt (handle, SOL_SOCKET, SO_ERROR, (char*) &opt, &len) < 0
				 || opt != 0)
				return -1;
		}

		return FD_ISSET (handle, forReading ? &rset : &wset) ? 1 : 0;
	}

	static bool setSocketBlockingState (const int handle, const bool shouldBlock) noexcept
	{
	   #if JUCE_WINDOWS
		u_long nonBlocking = shouldBlock ? 0 : (u_long) 1;
		return ioctlsocket (handle, FIONBIO, &nonBlocking) == 0;
	   #else
		int socketFlags = fcntl (handle, F_GETFL, 0);

		if (socketFlags == -1)
			return false;

		if (shouldBlock)
			socketFlags &= ~O_NONBLOCK;
		else
			socketFlags |= O_NONBLOCK;

		return fcntl (handle, F_SETFL, socketFlags) == 0;
	   #endif
	}

	static bool connectSocket (int volatile& handle,
							   const bool isDatagram,
							   struct addrinfo** const serverAddress,
							   const String& hostName,
							   const int portNumber,
							   const int timeOutMillisecs) noexcept
	{
		struct addrinfo hints = { 0 };
		hints.ai_family = AF_UNSPEC;
		hints.ai_socktype = isDatagram ? SOCK_DGRAM : SOCK_STREAM;
		hints.ai_flags = AI_NUMERICSERV;

		struct addrinfo* info = nullptr;
		if (getaddrinfo (hostName.toUTF8(), String (portNumber).toUTF8(), &hints, &info) != 0
			 || info == nullptr)
			return false;

		if (handle < 0)
			handle = (int) socket (info->ai_family, info->ai_socktype, 0);

		if (handle < 0)
		{
			freeaddrinfo (info);
			return false;
		}

		if (isDatagram)
		{
			if (*serverAddress != nullptr)
				freeaddrinfo (*serverAddress);

			*serverAddress = info;
			return true;
		}

		setSocketBlockingState (handle, false);
		const int result = ::connect (handle, info->ai_addr, (int) info->ai_addrlen);
		freeaddrinfo (info);

		if (result < 0)
		{
		   #if JUCE_WINDOWS
			if (result == SOCKET_ERROR && WSAGetLastError() == WSAEWOULDBLOCK)
		   #else
			if (errno == EINPROGRESS)
		   #endif
			{
				if (waitForReadiness (handle, false, timeOutMillisecs) != 1)
				{
					setSocketBlockingState (handle, true);
					return false;
				}
			}
		}

		setSocketBlockingState (handle, true);
		resetSocketOptions (handle, false, false);

		return true;
	}
}

StreamingSocket::StreamingSocket()
	: portNumber (0),
	  handle (-1),
	  connected (false),
	  isListener (false)
{
	SocketHelpers::initSockets();
}

StreamingSocket::StreamingSocket (const String& hostName_,
								  const int portNumber_,
								  const int handle_)
	: hostName (hostName_),
	  portNumber (portNumber_),
	  handle (handle_),
	  connected (true),
	  isListener (false)
{
	SocketHelpers::initSockets();
	SocketHelpers::resetSocketOptions (handle_, false, false);
}

StreamingSocket::~StreamingSocket()
{
	close();
}

int StreamingSocket::read (void* destBuffer, const int maxBytesToRead, const bool blockUntilSpecifiedAmountHasArrived)
{
	return (connected && ! isListener) ? SocketHelpers::readSocket (handle, destBuffer, maxBytesToRead, connected, blockUntilSpecifiedAmountHasArrived)
									   : -1;
}

int StreamingSocket::write (const void* sourceBuffer, const int numBytesToWrite)
{
	if (isListener || ! connected)
		return -1;

   #if JUCE_WINDOWS
	return send (handle, (const char*) sourceBuffer, numBytesToWrite, 0);
   #else
	int result;

	while ((result = (int) ::write (handle, sourceBuffer, numBytesToWrite)) < 0
			&& errno == EINTR)
	{
	}

	return result;
   #endif
}

int StreamingSocket::waitUntilReady (const bool readyForReading,
									 const int timeoutMsecs) const
{
	return connected ? SocketHelpers::waitForReadiness (handle, readyForReading, timeoutMsecs)
					 : -1;
}

bool StreamingSocket::bindToPort (const int port)
{
	return SocketHelpers::bindSocketToPort (handle, port);
}

bool StreamingSocket::connect (const String& remoteHostName,
							   const int remotePortNumber,
							   const int timeOutMillisecs)
{
	if (isListener)
	{
		jassertfalse;    // a listener socket can't connect to another one!
		return false;
	}

	if (connected)
		close();

	hostName = remoteHostName;
	portNumber = remotePortNumber;
	isListener = false;

	connected = SocketHelpers::connectSocket (handle, false, nullptr, remoteHostName,
											  remotePortNumber, timeOutMillisecs);

	if (! (connected && SocketHelpers::resetSocketOptions (handle, false, false)))
	{
		close();
		return false;
	}

	return true;
}

void StreamingSocket::close()
{
   #if JUCE_WINDOWS
	if (handle != SOCKET_ERROR || connected)
		closesocket (handle);

	connected = false;
   #else
	if (connected)
	{
		connected = false;

		if (isListener)
		{
			// need to do this to interrupt the accept() function..
			StreamingSocket temp;
			temp.connect ("localhost", portNumber, 1000);
		}
	}

	if (handle != -1)
		::close (handle);
   #endif

	hostName = String::empty;
	portNumber = 0;
	handle = -1;
	isListener = false;
}

bool StreamingSocket::createListener (const int newPortNumber, const String& localHostName)
{
	if (connected)
		close();

	hostName = "listener";
	portNumber = newPortNumber;
	isListener = true;

	struct sockaddr_in servTmpAddr = { 0 };
	servTmpAddr.sin_family = PF_INET;
	servTmpAddr.sin_addr.s_addr = htonl (INADDR_ANY);

	if (localHostName.isNotEmpty())
		servTmpAddr.sin_addr.s_addr = ::inet_addr (localHostName.toUTF8());

	servTmpAddr.sin_port = htons ((uint16) portNumber);

	handle = (int) socket (AF_INET, SOCK_STREAM, 0);

	if (handle < 0)
		return false;

	const int reuse = 1;
	setsockopt (handle, SOL_SOCKET, SO_REUSEADDR, (const char*) &reuse, sizeof (reuse));

	if (bind (handle, (struct sockaddr*) &servTmpAddr, sizeof (struct sockaddr_in)) < 0
		 || listen (handle, SOMAXCONN) < 0)
	{
		close();
		return false;
	}

	connected = true;
	return true;
}

StreamingSocket* StreamingSocket::waitForNextConnection() const
{
	jassert (isListener || ! connected); // to call this method, you first have to use createListener() to
										 // prepare this socket as a listener.

	if (connected && isListener)
	{
		struct sockaddr_storage address;
		juce_socklen_t len = sizeof (address);
		const int newSocket = (int) accept (handle, (struct sockaddr*) &address, &len);

		if (newSocket >= 0 && connected)
			return new StreamingSocket (inet_ntoa (((struct sockaddr_in*) &address)->sin_addr),
										portNumber, newSocket);
	}

	return nullptr;
}

bool StreamingSocket::isLocal() const noexcept
{
	return hostName == "127.0.0.1";
}

DatagramSocket::DatagramSocket (const int localPortNumber, const bool allowBroadcast_)
	: portNumber (0),
	  handle (-1),
	  connected (true),
	  allowBroadcast (allowBroadcast_),
	  serverAddress (nullptr)
{
	SocketHelpers::initSockets();

	handle = (int) socket (AF_INET, SOCK_DGRAM, 0);
	bindToPort (localPortNumber);
}

DatagramSocket::DatagramSocket (const String& hostName_, const int portNumber_,
								const int handle_, const int localPortNumber)
	: hostName (hostName_),
	  portNumber (portNumber_),
	  handle (handle_),
	  connected (true),
	  allowBroadcast (false),
	  serverAddress (nullptr)
{
	SocketHelpers::initSockets();

	SocketHelpers::resetSocketOptions (handle_, true, allowBroadcast);
	bindToPort (localPortNumber);
}

DatagramSocket::~DatagramSocket()
{
	close();

	if (serverAddress != nullptr)
		freeaddrinfo (static_cast <struct addrinfo*> (serverAddress));
}

void DatagramSocket::close()
{
   #if JUCE_WINDOWS
	closesocket (handle);
	connected = false;
   #else
	connected = false;
	::close (handle);
   #endif

	hostName = String::empty;
	portNumber = 0;
	handle = -1;
}

bool DatagramSocket::bindToPort (const int port)
{
	return SocketHelpers::bindSocketToPort (handle, port);
}

bool DatagramSocket::connect (const String& remoteHostName,
							  const int remotePortNumber,
							  const int timeOutMillisecs)
{
	if (connected)
		close();

	hostName = remoteHostName;
	portNumber = remotePortNumber;

	connected = SocketHelpers::connectSocket (handle, true, (struct addrinfo**) &serverAddress,
											  remoteHostName, remotePortNumber,
											  timeOutMillisecs);

	if (! (connected && SocketHelpers::resetSocketOptions (handle, true, allowBroadcast)))
	{
		close();
		return false;
	}

	return true;
}

DatagramSocket* DatagramSocket::waitForNextConnection() const
{
	while (waitUntilReady (true, -1) == 1)
	{
		struct sockaddr_storage address;
		juce_socklen_t len = sizeof (address);
		char buf[1];

		if (recvfrom (handle, buf, 0, 0, (struct sockaddr*) &address, &len) > 0)
			return new DatagramSocket (inet_ntoa (((struct sockaddr_in*) &address)->sin_addr),
									   ntohs (((struct sockaddr_in*) &address)->sin_port),
									   -1, -1);
	}

	return nullptr;
}

int DatagramSocket::waitUntilReady (const bool readyForReading,
									const int timeoutMsecs) const
{
	return connected ? SocketHelpers::waitForReadiness (handle, readyForReading, timeoutMsecs)
					 : -1;
}

int DatagramSocket::read (void* destBuffer, const int maxBytesToRead, const bool blockUntilSpecifiedAmountHasArrived)
{
	return connected ? SocketHelpers::readSocket (handle, destBuffer, maxBytesToRead, connected, blockUntilSpecifiedAmountHasArrived)
					 : -1;
}

int DatagramSocket::write (const void* sourceBuffer, const int numBytesToWrite)
{
	// You need to call connect() first to set the server address..
	jassert (serverAddress != nullptr && connected);

	return connected ? (int) sendto (handle, (const char*) sourceBuffer,
									 numBytesToWrite, 0,
									 static_cast <const struct addrinfo*> (serverAddress)->ai_addr,
									 static_cast <const struct addrinfo*> (serverAddress)->ai_addrlen)
					 : -1;
}

bool DatagramSocket::isLocal() const noexcept
{
	return hostName == "127.0.0.1";
}

#if JUCE_MSVC
 #pragma warning (pop)
#endif

/*** End of inlined file: juce_Socket.cpp ***/


/*** Start of inlined file: juce_URL.cpp ***/
URL::URL()
{
}

URL::URL (const String& url_)
	: url (url_)
{
	int i = url.indexOfChar ('?');

	if (i >= 0)
	{
		do
		{
			const int nextAmp   = url.indexOfChar (i + 1, '&');
			const int equalsPos = url.indexOfChar (i + 1, '=');

			if (equalsPos > i + 1)
			{
				if (nextAmp < 0)
				{
					addParameter (removeEscapeChars (url.substring (i + 1, equalsPos)),
								  removeEscapeChars (url.substring (equalsPos + 1)));
				}
				else if (nextAmp > 0 && equalsPos < nextAmp)
				{
					addParameter (removeEscapeChars (url.substring (i + 1, equalsPos)),
								  removeEscapeChars (url.substring (equalsPos + 1, nextAmp)));
				}
			}

			i = nextAmp;
		}
		while (i >= 0);

		url = url.upToFirstOccurrenceOf ("?", false, false);
	}
}

URL::URL (const URL& other)
	: url (other.url),
	  postData (other.postData),
	  parameterNames (other.parameterNames),
	  parameterValues (other.parameterValues),
	  filesToUpload (other.filesToUpload),
	  mimeTypes (other.mimeTypes)
{
}

URL& URL::operator= (const URL& other)
{
	url = other.url;
	postData = other.postData;
	parameterNames = other.parameterNames;
	parameterValues = other.parameterValues;
	filesToUpload = other.filesToUpload;
	mimeTypes = other.mimeTypes;

	return *this;
}

bool URL::operator== (const URL& other) const
{
	return url == other.url
		&& postData == other.postData
		&& parameterNames == other.parameterNames
		&& parameterValues == other.parameterValues
		&& filesToUpload == other.filesToUpload
		&& mimeTypes == other.mimeTypes;
}

bool URL::operator!= (const URL& other) const
{
	return ! operator== (other);
}

URL::~URL()
{
}

namespace URLHelpers
{
	static String getMangledParameters (const URL& url)
	{
		jassert (url.getParameterNames().size() == url.getParameterValues().size());
		String p;

		for (int i = 0; i < url.getParameterNames().size(); ++i)
		{
			if (i > 0)
				p << '&';

			p << URL::addEscapeChars (url.getParameterNames()[i], true)
			  << '='
			  << URL::addEscapeChars (url.getParameterValues()[i], true);
		}

		return p;
	}

	static int findEndOfScheme (const String& url)
	{
		int i = 0;

		while (CharacterFunctions::isLetterOrDigit (url[i])
				|| url[i] == '+' || url[i] == '-' || url[i] == '.')
			++i;

		return url[i] == ':' ? i + 1 : 0;
	}

	static int findStartOfNetLocation (const String& url)
	{
		int start = findEndOfScheme (url);
		while (url[start] == '/')
			++start;

		return start;
	}

	static int findStartOfPath (const String& url)
	{
		return url.indexOfChar (findStartOfNetLocation (url), '/') + 1;
	}

	static void createHeadersAndPostData (const URL& url, String& headers, MemoryBlock& postData)
	{
		MemoryOutputStream data (postData, false);

		if (url.getFilesToUpload().size() > 0)
		{
			// need to upload some files, so do it as multi-part...
			const String boundary (String::toHexString (Random::getSystemRandom().nextInt64()));

			headers << "Content-Type: multipart/form-data; boundary=" << boundary << "\r\n";

			data << "--" << boundary;

			int i;
			for (i = 0; i < url.getParameterNames().size(); ++i)
			{
				data << "\r\nContent-Disposition: form-data; name=\""
					 << url.getParameterNames() [i]
					 << "\"\r\n\r\n"
					 << url.getParameterValues() [i]
					 << "\r\n--"
					 << boundary;
			}

			for (i = 0; i < url.getFilesToUpload().size(); ++i)
			{
				const File file (url.getFilesToUpload().getAllValues() [i]);
				const String paramName (url.getFilesToUpload().getAllKeys() [i]);

				data << "\r\nContent-Disposition: form-data; name=\"" << paramName
					 << "\"; filename=\"" << file.getFileName() << "\"\r\n";

				const String mimeType (url.getMimeTypesOfUploadFiles()
										  .getValue (paramName, String::empty));

				if (mimeType.isNotEmpty())
					data << "Content-Type: " << mimeType << "\r\n";

				data << "Content-Transfer-Encoding: binary\r\n\r\n"
					 << file << "\r\n--" << boundary;
			}

			data << "--\r\n";
		}
		else
		{
			data << getMangledParameters (url)
				 << url.getPostData();

			// just a short text attachment, so use simple url encoding..
			headers << "Content-Type: application/x-www-form-urlencoded\r\nContent-length: "
					<< (int) data.getDataSize() << "\r\n";
		}
	}

	static void concatenatePaths (String& path, const String& suffix)
	{
		if (! path.endsWithChar ('/'))
			path << '/';

		if (suffix.startsWithChar ('/'))
			path += suffix.substring (1);
		else
			path += suffix;
	}
}

void URL::addParameter (const String& name, const String& value)
{
	parameterNames.add (name);
	parameterValues.add (value);
}

String URL::toString (const bool includeGetParameters) const
{
	if (includeGetParameters && parameterNames.size() > 0)
		return url + "?" + URLHelpers::getMangledParameters (*this);
	else
		return url;
}

bool URL::isWellFormed() const
{
	//xxx TODO
	return url.isNotEmpty();
}

String URL::getDomain() const
{
	const int start = URLHelpers::findStartOfNetLocation (url);
	const int end1 = url.indexOfChar (start, '/');
	const int end2 = url.indexOfChar (start, ':');

	const int end = (end1 < 0 && end2 < 0) ? std::numeric_limits<int>::max()
										   : ((end1 < 0 || end2 < 0) ? jmax (end1, end2)
																	 : jmin (end1, end2));
	return url.substring (start, end);
}

String URL::getSubPath() const
{
	const int startOfPath = URLHelpers::findStartOfPath (url);

	return startOfPath <= 0 ? String::empty
							: url.substring (startOfPath);
}

String URL::getScheme() const
{
	return url.substring (0, URLHelpers::findEndOfScheme (url) - 1);
}

int URL::getPort() const
{
	const int colonPos = url.indexOfChar (URLHelpers::findStartOfNetLocation (url), ':');

	return colonPos > 0 ? url.substring (colonPos + 1).getIntValue() : 0;
}

URL URL::withNewSubPath (const String& newPath) const
{
	const int startOfPath = URLHelpers::findStartOfPath (url);

	URL u (*this);

	if (startOfPath > 0)
		u.url = url.substring (0, startOfPath);

	URLHelpers::concatenatePaths (u.url, newPath);
	return u;
}

URL URL::getChildURL (const String& subPath) const
{
	URL u (*this);
	URLHelpers::concatenatePaths (u.url, subPath);
	return u;
}

bool URL::isProbablyAWebsiteURL (const String& possibleURL)
{
	const char* validProtocols[] = { "http:", "ftp:", "https:" };

	for (int i = 0; i < numElementsInArray (validProtocols); ++i)
		if (possibleURL.startsWithIgnoreCase (validProtocols[i]))
			return true;

	if (possibleURL.containsChar ('@')
		 || possibleURL.containsChar (' '))
		return false;

	const String topLevelDomain (possibleURL.upToFirstOccurrenceOf ("/", false, false)
											.fromLastOccurrenceOf (".", false, false));

	return topLevelDomain.isNotEmpty() && topLevelDomain.length() <= 3;
}

bool URL::isProbablyAnEmailAddress (const String& possibleEmailAddress)
{
	const int atSign = possibleEmailAddress.indexOfChar ('@');

	return atSign > 0
			&& possibleEmailAddress.lastIndexOfChar ('.') > (atSign + 1)
			&& (! possibleEmailAddress.endsWithChar ('.'));
}

InputStream* URL::createInputStream (const bool usePostCommand,
									 OpenStreamProgressCallback* const progressCallback,
									 void* const progressCallbackContext,
									 const String& extraHeaders,
									 const int timeOutMs,
									 StringPairArray* const responseHeaders) const
{
	String headers;
	MemoryBlock headersAndPostData;

	if (usePostCommand)
		URLHelpers::createHeadersAndPostData (*this, headers, headersAndPostData);

	headers += extraHeaders;

	if (! headers.endsWithChar ('\n'))
		headers << "\r\n";

	return createNativeStream (toString (! usePostCommand), usePostCommand, headersAndPostData,
							   progressCallback, progressCallbackContext,
							   headers, timeOutMs, responseHeaders);
}

bool URL::readEntireBinaryStream (MemoryBlock& destData,
								  const bool usePostCommand) const
{
	const ScopedPointer <InputStream> in (createInputStream (usePostCommand));

	if (in != nullptr)
	{
		in->readIntoMemoryBlock (destData);
		return true;
	}

	return false;
}

String URL::readEntireTextStream (const bool usePostCommand) const
{
	const ScopedPointer <InputStream> in (createInputStream (usePostCommand));

	if (in != nullptr)
		return in->readEntireStreamAsString();

	return String::empty;
}

XmlElement* URL::readEntireXmlStream (const bool usePostCommand) const
{
	return XmlDocument::parse (readEntireTextStream (usePostCommand));
}

URL URL::withParameter (const String& parameterName,
						const String& parameterValue) const
{
	URL u (*this);
	u.addParameter (parameterName, parameterValue);
	return u;
}

URL URL::withFileToUpload (const String& parameterName,
						   const File& fileToUpload,
						   const String& mimeType) const
{
	jassert (mimeType.isNotEmpty()); // You need to supply a mime type!

	URL u (*this);
	u.filesToUpload.set (parameterName, fileToUpload.getFullPathName());
	u.mimeTypes.set (parameterName, mimeType);
	return u;
}

URL URL::withPOSTData (const String& postData_) const
{
	URL u (*this);
	u.postData = postData_;
	return u;
}

const StringPairArray& URL::getFilesToUpload() const
{
	return filesToUpload;
}

const StringPairArray& URL::getMimeTypesOfUploadFiles() const
{
	return mimeTypes;
}

String URL::removeEscapeChars (const String& s)
{
	String result (s.replaceCharacter ('+', ' '));

	if (! result.containsChar ('%'))
		return result;

	// We need to operate on the string as raw UTF8 chars, and then recombine them into unicode
	// after all the replacements have been made, so that multi-byte chars are handled.
	Array<char> utf8 (result.toUTF8().getAddress(), result.getNumBytesAsUTF8());

	for (int i = 0; i < utf8.size(); ++i)
	{
		if (utf8.getUnchecked(i) == '%')
		{
			const int hexDigit1 = CharacterFunctions::getHexDigitValue ((juce_wchar) (uint8) utf8 [i + 1]);
			const int hexDigit2 = CharacterFunctions::getHexDigitValue ((juce_wchar) (uint8) utf8 [i + 2]);

			if (hexDigit1 >= 0 && hexDigit2 >= 0)
			{
				utf8.set (i, (char) ((hexDigit1 << 4) + hexDigit2));
				utf8.removeRange (i + 1, 2);
			}
		}
	}

	return String::fromUTF8 (utf8.getRawDataPointer(), utf8.size());
}

String URL::addEscapeChars (const String& s, const bool isParameter)
{
	const CharPointer_UTF8 legalChars (isParameter ? "_-.*!'()"
												   : ",$_-.*!'()");

	Array<char> utf8 (s.toUTF8().getAddress(), s.getNumBytesAsUTF8());

	for (int i = 0; i < utf8.size(); ++i)
	{
		const char c = utf8.getUnchecked(i);

		if (! (CharacterFunctions::isLetterOrDigit (c)
				 || legalChars.indexOf ((juce_wchar) c) >= 0))
		{
			utf8.set (i, '%');
			utf8.insert (++i, "0123456789abcdef" [((uint8) c) >> 4]);
			utf8.insert (++i, "0123456789abcdef" [c & 15]);
		}
	}

	return String::fromUTF8 (utf8.getRawDataPointer(), utf8.size());
}

bool URL::launchInDefaultBrowser() const
{
	String u (toString (true));

	if (u.containsChar ('@') && ! u.containsChar (':'))
		u = "mailto:" + u;

	return Process::openDocument (u, String::empty);
}

/*** End of inlined file: juce_URL.cpp ***/


/*** Start of inlined file: juce_BufferedInputStream.cpp ***/
namespace
{
	int calcBufferStreamBufferSize (int requestedSize, InputStream* const source) noexcept
	{
		// You need to supply a real stream when creating a BufferedInputStream
		jassert (source != nullptr);

		requestedSize = jmax (256, requestedSize);

		const int64 sourceSize = source->getTotalLength();
		if (sourceSize >= 0 && sourceSize < requestedSize)
			requestedSize = jmax (32, (int) sourceSize);

		return requestedSize;
	}
}

BufferedInputStream::BufferedInputStream (InputStream* const sourceStream, const int bufferSize_,
										  const bool deleteSourceWhenDestroyed)
   : source (sourceStream, deleteSourceWhenDestroyed),
	 bufferSize (calcBufferStreamBufferSize (bufferSize_, sourceStream)),
	 position (sourceStream->getPosition()),
	 lastReadPos (0),
	 bufferStart (position),
	 bufferOverlap (128)
{
	buffer.malloc ((size_t) bufferSize);
}

BufferedInputStream::BufferedInputStream (InputStream& sourceStream, const int bufferSize_)
   : source (&sourceStream, false),
	 bufferSize (calcBufferStreamBufferSize (bufferSize_, &sourceStream)),
	 position (sourceStream.getPosition()),
	 lastReadPos (0),
	 bufferStart (position),
	 bufferOverlap (128)
{
	buffer.malloc ((size_t) bufferSize);
}

BufferedInputStream::~BufferedInputStream()
{
}

int64 BufferedInputStream::getTotalLength()
{
	return source->getTotalLength();
}

int64 BufferedInputStream::getPosition()
{
	return position;
}

bool BufferedInputStream::setPosition (int64 newPosition)
{
	position = jmax ((int64) 0, newPosition);
	return true;
}

bool BufferedInputStream::isExhausted()
{
	return (position >= lastReadPos)
			 && source->isExhausted();
}

void BufferedInputStream::ensureBuffered()
{
	const int64 bufferEndOverlap = lastReadPos - bufferOverlap;

	if (position < bufferStart || position >= bufferEndOverlap)
	{
		int bytesRead;

		if (position < lastReadPos
			 && position >= bufferEndOverlap
			 && position >= bufferStart)
		{
			const int bytesToKeep = (int) (lastReadPos - position);
			memmove (buffer, buffer + (int) (position - bufferStart), (size_t) bytesToKeep);

			bufferStart = position;

			bytesRead = source->read (buffer + bytesToKeep,
									  (int) (bufferSize - bytesToKeep));

			lastReadPos += bytesRead;
			bytesRead += bytesToKeep;
		}
		else
		{
			bufferStart = position;
			source->setPosition (bufferStart);
			bytesRead = source->read (buffer, bufferSize);
			lastReadPos = bufferStart + bytesRead;
		}

		while (bytesRead < bufferSize)
			buffer [bytesRead++] = 0;
	}
}

int BufferedInputStream::read (void* destBuffer, int maxBytesToRead)
{
	jassert (destBuffer != nullptr && maxBytesToRead >= 0);

	if (position >= bufferStart
		 && position + maxBytesToRead <= lastReadPos)
	{
		memcpy (destBuffer, buffer + (int) (position - bufferStart), (size_t) maxBytesToRead);
		position += maxBytesToRead;

		return maxBytesToRead;
	}
	else
	{
		if (position < bufferStart || position >= lastReadPos)
			ensureBuffered();

		int bytesRead = 0;

		while (maxBytesToRead > 0)
		{
			const int bytesAvailable = jmin (maxBytesToRead, (int) (lastReadPos - position));

			if (bytesAvailable > 0)
			{
				memcpy (destBuffer, buffer + (int) (position - bufferStart), (size_t) bytesAvailable);
				maxBytesToRead -= bytesAvailable;
				bytesRead += bytesAvailable;
				position += bytesAvailable;
				destBuffer = static_cast <char*> (destBuffer) + bytesAvailable;
			}

			const int64 oldLastReadPos = lastReadPos;
			ensureBuffered();

			if (oldLastReadPos == lastReadPos)
				break; // if ensureBuffered() failed to read any more data, bail out

			if (isExhausted())
				break;
		}

		return bytesRead;
	}
}

String BufferedInputStream::readString()
{
	if (position >= bufferStart
		 && position < lastReadPos)
	{
		const int maxChars = (int) (lastReadPos - position);

		const char* const src = buffer + (int) (position - bufferStart);

		for (int i = 0; i < maxChars; ++i)
		{
			if (src[i] == 0)
			{
				position += i + 1;
				return String::fromUTF8 (src, i);
			}
		}
	}

	return InputStream::readString();
}

/*** End of inlined file: juce_BufferedInputStream.cpp ***/


/*** Start of inlined file: juce_FileInputSource.cpp ***/
FileInputSource::FileInputSource (const File& file_, bool useFileTimeInHashGeneration_)
	: file (file_), useFileTimeInHashGeneration (useFileTimeInHashGeneration_)
{
}

FileInputSource::~FileInputSource()
{
}

InputStream* FileInputSource::createInputStream()
{
	return file.createInputStream();
}

InputStream* FileInputSource::createInputStreamFor (const String& relatedItemPath)
{
	return file.getSiblingFile (relatedItemPath).createInputStream();
}

int64 FileInputSource::hashCode() const
{
	int64 h = file.hashCode();

	if (useFileTimeInHashGeneration)
		h ^= file.getLastModificationTime().toMilliseconds();

	return h;
}

/*** End of inlined file: juce_FileInputSource.cpp ***/


/*** Start of inlined file: juce_InputStream.cpp ***/
int64 InputStream::getNumBytesRemaining()
{
	int64 len = getTotalLength();

	if (len >= 0)
		len -= getPosition();

	return len;
}

char InputStream::readByte()
{
	char temp = 0;
	read (&temp, 1);
	return temp;
}

bool InputStream::readBool()
{
	return readByte() != 0;
}

short InputStream::readShort()
{
	char temp[2];

	if (read (temp, 2) == 2)
		return (short) ByteOrder::littleEndianShort (temp);

	return 0;
}

short InputStream::readShortBigEndian()
{
	char temp[2];

	if (read (temp, 2) == 2)
		return (short) ByteOrder::bigEndianShort (temp);

	return 0;
}

int InputStream::readInt()
{
	char temp[4];

	if (read (temp, 4) == 4)
		return (int) ByteOrder::littleEndianInt (temp);

	return 0;
}

int InputStream::readIntBigEndian()
{
	char temp[4];

	if (read (temp, 4) == 4)
		return (int) ByteOrder::bigEndianInt (temp);

	return 0;
}

int InputStream::readCompressedInt()
{
	const uint8 sizeByte = (uint8) readByte();
	if (sizeByte == 0)
		return 0;

	const int numBytes = (sizeByte & 0x7f);
	if (numBytes > 4)
	{
		jassertfalse;    // trying to read corrupt data - this method must only be used
					   // to read data that was written by OutputStream::writeCompressedInt()
		return 0;
	}

	char bytes[4] = { 0, 0, 0, 0 };
	if (read (bytes, numBytes) != numBytes)
		return 0;

	const int num = (int) ByteOrder::littleEndianInt (bytes);
	return (sizeByte >> 7) ? -num : num;
}

int64 InputStream::readInt64()
{
	union { uint8 asBytes[8]; uint64 asInt64; } n;

	if (read (n.asBytes, 8) == 8)
		return (int64) ByteOrder::swapIfBigEndian (n.asInt64);

	return 0;
}

int64 InputStream::readInt64BigEndian()
{
	union { uint8 asBytes[8]; uint64 asInt64; } n;

	if (read (n.asBytes, 8) == 8)
		return (int64) ByteOrder::swapIfLittleEndian (n.asInt64);

	return 0;
}

float InputStream::readFloat()
{
	// the union below relies on these types being the same size...
	static_jassert (sizeof (int32) == sizeof (float));
	union { int32 asInt; float asFloat; } n;
	n.asInt = (int32) readInt();
	return n.asFloat;
}

float InputStream::readFloatBigEndian()
{
	union { int32 asInt; float asFloat; } n;
	n.asInt = (int32) readIntBigEndian();
	return n.asFloat;
}

double InputStream::readDouble()
{
	union { int64 asInt; double asDouble; } n;
	n.asInt = readInt64();
	return n.asDouble;
}

double InputStream::readDoubleBigEndian()
{
	union { int64 asInt; double asDouble; } n;
	n.asInt = readInt64BigEndian();
	return n.asDouble;
}

String InputStream::readString()
{
	MemoryBlock buffer (256);
	char* data = static_cast<char*> (buffer.getData());
	size_t i = 0;

	while ((data[i] = readByte()) != 0)
	{
		if (++i >= buffer.getSize())
		{
			buffer.setSize (buffer.getSize() + 512);
			data = static_cast<char*> (buffer.getData());
		}
	}

	return String::fromUTF8 (data, (int) i);
}

String InputStream::readNextLine()
{
	MemoryBlock buffer (256);
	char* data = static_cast<char*> (buffer.getData());
	size_t i = 0;

	while ((data[i] = readByte()) != 0)
	{
		if (data[i] == '\n')
			break;

		if (data[i] == '\r')
		{
			const int64 lastPos = getPosition();

			if (readByte() != '\n')
				setPosition (lastPos);

			break;
		}

		if (++i >= buffer.getSize())
		{
			buffer.setSize (buffer.getSize() + 512);
			data = static_cast<char*> (buffer.getData());
		}
	}

	return String::fromUTF8 (data, (int) i);
}

int InputStream::readIntoMemoryBlock (MemoryBlock& block, ssize_t numBytes)
{
	MemoryOutputStream mo (block, true);
	return mo.writeFromInputStream (*this, numBytes);
}

String InputStream::readEntireStreamAsString()
{
	MemoryOutputStream mo;
	mo << *this;
	return mo.toString();
}

void InputStream::skipNextBytes (int64 numBytesToSkip)
{
	if (numBytesToSkip > 0)
	{
		const int skipBufferSize = (int) jmin (numBytesToSkip, (int64) 16384);
		HeapBlock<char> temp ((size_t) skipBufferSize);

		while (numBytesToSkip > 0 && ! isExhausted())
			numBytesToSkip -= read (temp, (int) jmin (numBytesToSkip, (int64) skipBufferSize));
	}
}

/*** End of inlined file: juce_InputStream.cpp ***/


/*** Start of inlined file: juce_MemoryInputStream.cpp ***/
MemoryInputStream::MemoryInputStream (const void* const sourceData,
									  const size_t sourceDataSize,
									  const bool keepInternalCopy)
	: data (static_cast <const char*> (sourceData)),
	  dataSize (sourceDataSize),
	  position (0)
{
	if (keepInternalCopy)
		createInternalCopy();
}

MemoryInputStream::MemoryInputStream (const MemoryBlock& sourceData,
									  const bool keepInternalCopy)
	: data (static_cast <const char*> (sourceData.getData())),
	  dataSize (sourceData.getSize()),
	  position (0)
{
	if (keepInternalCopy)
		createInternalCopy();
}

void MemoryInputStream::createInternalCopy()
{
	internalCopy.malloc (dataSize);
	memcpy (internalCopy, data, dataSize);
	data = internalCopy;
}

MemoryInputStream::~MemoryInputStream()
{
}

int64 MemoryInputStream::getTotalLength()
{
	return dataSize;
}

int MemoryInputStream::read (void* const buffer, const int howMany)
{
	jassert (buffer != nullptr && howMany >= 0);

	const int num = jmin (howMany, (int) (dataSize - position));
	if (num <= 0)
		return 0;

	memcpy (buffer, data + position, (size_t) num);
	position += num;
	return (int) num;
}

bool MemoryInputStream::isExhausted()
{
	return position >= dataSize;
}

bool MemoryInputStream::setPosition (const int64 pos)
{
	position = (size_t) jlimit ((int64) 0, (int64) dataSize, pos);
	return true;
}

int64 MemoryInputStream::getPosition()
{
	return position;
}

#if JUCE_UNIT_TESTS

class MemoryStreamTests  : public UnitTest
{
public:
	MemoryStreamTests() : UnitTest ("MemoryInputStream & MemoryOutputStream") {}

	void runTest()
	{
		beginTest ("Basics");
		Random r;

		int randomInt = r.nextInt();
		int64 randomInt64 = r.nextInt64();
		double randomDouble = r.nextDouble();
		String randomString (createRandomWideCharString());

		MemoryOutputStream mo;
		mo.writeInt (randomInt);
		mo.writeIntBigEndian (randomInt);
		mo.writeCompressedInt (randomInt);
		mo.writeString (randomString);
		mo.writeInt64 (randomInt64);
		mo.writeInt64BigEndian (randomInt64);
		mo.writeDouble (randomDouble);
		mo.writeDoubleBigEndian (randomDouble);

		MemoryInputStream mi (mo.getData(), mo.getDataSize(), false);
		expect (mi.readInt() == randomInt);
		expect (mi.readIntBigEndian() == randomInt);
		expect (mi.readCompressedInt() == randomInt);
		expectEquals (mi.readString(), randomString);
		expect (mi.readInt64() == randomInt64);
		expect (mi.readInt64BigEndian() == randomInt64);
		expect (mi.readDouble() == randomDouble);
		expect (mi.readDoubleBigEndian() == randomDouble);
	}

	static String createRandomWideCharString()
	{
		juce_wchar buffer [50] = { 0 };
		Random r;

		for (int i = 0; i < numElementsInArray (buffer) - 1; ++i)
		{
			if (r.nextBool())
			{
				do
				{
					buffer[i] = (juce_wchar) (1 + r.nextInt (0x10ffff - 1));
				}
				while (! CharPointer_UTF16::canRepresent (buffer[i]));
			}
			else
				buffer[i] = (juce_wchar) (1 + r.nextInt (0xff));
		}

		return CharPointer_UTF32 (buffer);
	}
};

static MemoryStreamTests memoryInputStreamUnitTests;

#endif

/*** End of inlined file: juce_MemoryInputStream.cpp ***/


/*** Start of inlined file: juce_MemoryOutputStream.cpp ***/
MemoryOutputStream::MemoryOutputStream (const size_t initialSize)
  : data (internalBlock),
	position (0),
	size (0)
{
	internalBlock.setSize (initialSize, false);
}

MemoryOutputStream::MemoryOutputStream (MemoryBlock& memoryBlockToWriteTo,
										const bool appendToExistingBlockContent)
  : data (memoryBlockToWriteTo),
	position (0),
	size (0)
{
	if (appendToExistingBlockContent)
		position = size = memoryBlockToWriteTo.getSize();
}

MemoryOutputStream::~MemoryOutputStream()
{
	trimExternalBlockSize();
}

void MemoryOutputStream::flush()
{
	trimExternalBlockSize();
}

void MemoryOutputStream::trimExternalBlockSize()
{
	if (&data != &internalBlock)
		data.setSize (size, false);
}

void MemoryOutputStream::preallocate (const size_t bytesToPreallocate)
{
	data.ensureSize (bytesToPreallocate + 1);
}

void MemoryOutputStream::reset() noexcept
{
	position = 0;
	size = 0;
}

void MemoryOutputStream::prepareToWrite (int numBytes)
{
	const size_t storageNeeded = position + numBytes;

	if (storageNeeded >= data.getSize())
		data.ensureSize ((storageNeeded + jmin ((int) (storageNeeded / 2), 1024 * 1024) + 32) & ~31);
}

bool MemoryOutputStream::write (const void* const buffer, int howMany)
{
	jassert (buffer != nullptr && howMany >= 0);

	if (howMany > 0)
	{
		prepareToWrite (howMany);
		memcpy (static_cast<char*> (data.getData()) + position, buffer, (size_t) howMany);
		position += howMany;
		size = jmax (size, position);
	}

	return true;
}

void MemoryOutputStream::writeRepeatedByte (uint8 byte, int howMany)
{
	if (howMany > 0)
	{
		prepareToWrite (howMany);
		memset (static_cast<char*> (data.getData()) + position, byte, (size_t) howMany);
		position += howMany;
		size = jmax (size, position);
	}
}

MemoryBlock MemoryOutputStream::getMemoryBlock() const
{
	return MemoryBlock (getData(), getDataSize());
}

const void* MemoryOutputStream::getData() const noexcept
{
	if (data.getSize() > size)
		static_cast <char*> (data.getData()) [size] = 0;

	return data.getData();
}

bool MemoryOutputStream::setPosition (int64 newPosition)
{
	if (newPosition <= (int64) size)
	{
		// ok to seek backwards
		position = jlimit ((size_t) 0, size, (size_t) newPosition);
		return true;
	}
	else
	{
		// trying to make it bigger isn't a good thing to do..
		return false;
	}
}

int MemoryOutputStream::writeFromInputStream (InputStream& source, int64 maxNumBytesToWrite)
{
	// before writing from an input, see if we can preallocate to make it more efficient..
	int64 availableData = source.getTotalLength() - source.getPosition();

	if (availableData > 0)
	{
		if (maxNumBytesToWrite > 0 && maxNumBytesToWrite < availableData)
			availableData = maxNumBytesToWrite;

		preallocate (data.getSize() + (size_t) maxNumBytesToWrite);
	}

	return OutputStream::writeFromInputStream (source, maxNumBytesToWrite);
}

String MemoryOutputStream::toUTF8() const
{
	const char* const d = static_cast <const char*> (getData());
	return String (CharPointer_UTF8 (d), CharPointer_UTF8 (d + getDataSize()));
}

String MemoryOutputStream::toString() const
{
	return String::createStringFromData (getData(), (int) getDataSize());
}

OutputStream& JUCE_CALLTYPE operator<< (OutputStream& stream, const MemoryOutputStream& streamToRead)
{
	const int dataSize = (int) streamToRead.getDataSize();
	if (dataSize > 0)
		stream.write (streamToRead.getData(), dataSize);

	return stream;
}

/*** End of inlined file: juce_MemoryOutputStream.cpp ***/


/*** Start of inlined file: juce_OutputStream.cpp ***/
#if JUCE_DEBUG

struct DanglingStreamChecker
{
	DanglingStreamChecker() {}

	~DanglingStreamChecker()
	{
		/*
			It's always a bad idea to leak any object, but if you're leaking output
			streams, then there's a good chance that you're failing to flush a file
			to disk properly, which could result in corrupted data and other similar
			nastiness..
		*/
		jassert (activeStreams.size() == 0);
	}

	Array<void*, CriticalSection> activeStreams;
};

static DanglingStreamChecker danglingStreamChecker;
#endif

OutputStream::OutputStream()
	: newLineString (NewLine::getDefault())
{
   #if JUCE_DEBUG
	danglingStreamChecker.activeStreams.add (this);
   #endif
}

OutputStream::~OutputStream()
{
   #if JUCE_DEBUG
	danglingStreamChecker.activeStreams.removeValue (this);
   #endif
}

void OutputStream::writeBool (const bool b)
{
	writeByte (b ? (char) 1
				 : (char) 0);
}

void OutputStream::writeByte (char byte)
{
	write (&byte, 1);
}

void OutputStream::writeRepeatedByte (uint8 byte, int numTimesToRepeat)
{
	while (--numTimesToRepeat >= 0)
		writeByte ((char) byte);
}

void OutputStream::writeShort (short value)
{
	const unsigned short v = ByteOrder::swapIfBigEndian ((unsigned short) value);
	write (&v, 2);
}

void OutputStream::writeShortBigEndian (short value)
{
	const unsigned short v = ByteOrder::swapIfLittleEndian ((unsigned short) value);
	write (&v, 2);
}

void OutputStream::writeInt (int value)
{
	const unsigned int v = ByteOrder::swapIfBigEndian ((unsigned int) value);
	write (&v, 4);
}

void OutputStream::writeIntBigEndian (int value)
{
	const unsigned int v = ByteOrder::swapIfLittleEndian ((unsigned int) value);
	write (&v, 4);
}

void OutputStream::writeCompressedInt (int value)
{
	unsigned int un = (value < 0) ? (unsigned int) -value
								  : (unsigned int) value;

	uint8 data[5];
	int num = 0;

	while (un > 0)
	{
		data[++num] = (uint8) un;
		un >>= 8;
	}

	data[0] = (uint8) num;

	if (value < 0)
		data[0] |= 0x80;

	write (data, num + 1);
}

void OutputStream::writeInt64 (int64 value)
{
	const uint64 v = ByteOrder::swapIfBigEndian ((uint64) value);
	write (&v, 8);
}

void OutputStream::writeInt64BigEndian (int64 value)
{
	const uint64 v = ByteOrder::swapIfLittleEndian ((uint64) value);
	write (&v, 8);
}

void OutputStream::writeFloat (float value)
{
	union { int asInt; float asFloat; } n;
	n.asFloat = value;
	writeInt (n.asInt);
}

void OutputStream::writeFloatBigEndian (float value)
{
	union { int asInt; float asFloat; } n;
	n.asFloat = value;
	writeIntBigEndian (n.asInt);
}

void OutputStream::writeDouble (double value)
{
	union { int64 asInt; double asDouble; } n;
	n.asDouble = value;
	writeInt64 (n.asInt);
}

void OutputStream::writeDoubleBigEndian (double value)
{
	union { int64 asInt; double asDouble; } n;
	n.asDouble = value;
	writeInt64BigEndian (n.asInt);
}

void OutputStream::writeString (const String& text)
{
	// (This avoids using toUTF8() to prevent the memory bloat that it would leave behind
	// if lots of large, persistent strings were to be written to streams).
	const int numBytes = text.getNumBytesAsUTF8() + 1;
	HeapBlock<char> temp ((size_t) numBytes);
	text.copyToUTF8 (temp, numBytes);
	write (temp, numBytes);
}

void OutputStream::writeText (const String& text, const bool asUTF16,
							  const bool writeUTF16ByteOrderMark)
{
	if (asUTF16)
	{
		if (writeUTF16ByteOrderMark)
			write ("\x0ff\x0fe", 2);

		String::CharPointerType src (text.getCharPointer());
		bool lastCharWasReturn = false;

		for (;;)
		{
			const juce_wchar c = src.getAndAdvance();

			if (c == 0)
				break;

			if (c == '\n' && ! lastCharWasReturn)
				writeShort ((short) '\r');

			lastCharWasReturn = (c == L'\r');
			writeShort ((short) c);
		}
	}
	else
	{
		const char* src = text.toUTF8();
		const char* t = src;

		for (;;)
		{
			if (*t == '\n')
			{
				if (t > src)
					write (src, (int) (t - src));

				write ("\r\n", 2);
				src = t + 1;
			}
			else if (*t == '\r')
			{
				if (t[1] == '\n')
					++t;
			}
			else if (*t == 0)
			{
				if (t > src)
					write (src, (int) (t - src));

				break;
			}

			++t;
		}
	}
}

int OutputStream::writeFromInputStream (InputStream& source, int64 numBytesToWrite)
{
	if (numBytesToWrite < 0)
		numBytesToWrite = std::numeric_limits<int64>::max();

	int numWritten = 0;

	while (numBytesToWrite > 0 && ! source.isExhausted())
	{
		char buffer [8192];
		const int num = source.read (buffer, (int) jmin (numBytesToWrite, (int64) sizeof (buffer)));

		if (num <= 0)
			break;

		write (buffer, num);

		numBytesToWrite -= num;
		numWritten += num;
	}

	return numWritten;
}

void OutputStream::setNewLineString (const String& newLineString_)
{
	newLineString = newLineString_;
}

OutputStream& JUCE_CALLTYPE operator<< (OutputStream& stream, const int number)
{
	return stream << String (number);
}

OutputStream& JUCE_CALLTYPE operator<< (OutputStream& stream, const double number)
{
	return stream << String (number);
}

OutputStream& JUCE_CALLTYPE operator<< (OutputStream& stream, const char character)
{
	stream.writeByte (character);
	return stream;
}

OutputStream& JUCE_CALLTYPE operator<< (OutputStream& stream, const char* const text)
{
	stream.write (text, (int) strlen (text));
	return stream;
}

OutputStream& JUCE_CALLTYPE operator<< (OutputStream& stream, const MemoryBlock& data)
{
	if (data.getSize() > 0)
		stream.write (data.getData(), (int) data.getSize());

	return stream;
}

OutputStream& JUCE_CALLTYPE operator<< (OutputStream& stream, const File& fileToRead)
{
	FileInputStream in (fileToRead);

	if (in.openedOk())
		return stream << in;

	return stream;
}

OutputStream& JUCE_CALLTYPE operator<< (OutputStream& stream, InputStream& streamToRead)
{
	stream.writeFromInputStream (streamToRead, -1);
	return stream;
}

OutputStream& JUCE_CALLTYPE operator<< (OutputStream& stream, const NewLine&)
{
	return stream << stream.getNewLineString();
}

/*** End of inlined file: juce_OutputStream.cpp ***/


/*** Start of inlined file: juce_SubregionStream.cpp ***/
SubregionStream::SubregionStream (InputStream* const sourceStream,
								  const int64 startPositionInSourceStream_,
								  const int64 lengthOfSourceStream_,
								  const bool deleteSourceWhenDestroyed)
  : source (sourceStream, deleteSourceWhenDestroyed),
	startPositionInSourceStream (startPositionInSourceStream_),
	lengthOfSourceStream (lengthOfSourceStream_)
{
	SubregionStream::setPosition (0);
}

SubregionStream::~SubregionStream()
{
}

int64 SubregionStream::getTotalLength()
{
	const int64 srcLen = source->getTotalLength() - startPositionInSourceStream;

	return (lengthOfSourceStream >= 0) ? jmin (lengthOfSourceStream, srcLen)
									   : srcLen;
}

int64 SubregionStream::getPosition()
{
	return source->getPosition() - startPositionInSourceStream;
}

bool SubregionStream::setPosition (int64 newPosition)
{
	return source->setPosition (jmax ((int64) 0, newPosition + startPositionInSourceStream));
}

int SubregionStream::read (void* destBuffer, int maxBytesToRead)
{
	jassert (destBuffer != nullptr && maxBytesToRead >= 0);

	if (lengthOfSourceStream < 0)
	{
		return source->read (destBuffer, maxBytesToRead);
	}
	else
	{
		maxBytesToRead = (int) jmin ((int64) maxBytesToRead, lengthOfSourceStream - getPosition());

		if (maxBytesToRead <= 0)
			return 0;

		return source->read (destBuffer, maxBytesToRead);
	}
}

bool SubregionStream::isExhausted()
{
	if (lengthOfSourceStream >= 0)
		return (getPosition() >= lengthOfSourceStream) || source->isExhausted();
	else
		return source->isExhausted();
}

/*** End of inlined file: juce_SubregionStream.cpp ***/


/*** Start of inlined file: juce_SystemStats.cpp ***/
const SystemStats::CPUFlags& SystemStats::getCPUFlags()
{
	static CPUFlags cpuFlags;
	return cpuFlags;
}

String SystemStats::getJUCEVersion()
{
	// Some basic tests, to keep an eye on things and make sure these types work ok
	// on all platforms. Let me know if any of these assertions fail on your system!
	static_jassert (sizeof (pointer_sized_int) == sizeof (void*));
	static_jassert (sizeof (int8) == 1);
	static_jassert (sizeof (uint8) == 1);
	static_jassert (sizeof (int16) == 2);
	static_jassert (sizeof (uint16) == 2);
	static_jassert (sizeof (int32) == 4);
	static_jassert (sizeof (uint32) == 4);
	static_jassert (sizeof (int64) == 8);
	static_jassert (sizeof (uint64) == 8);

	return "JUCE v" JUCE_STRINGIFY(JUCE_MAJOR_VERSION)
				"." JUCE_STRINGIFY(JUCE_MINOR_VERSION)
				"." JUCE_STRINGIFY(JUCE_BUILDNUMBER);
}

#if JUCE_DEBUG && ! JUCE_ANDROID
 struct JuceVersionPrinter
 {
	 JuceVersionPrinter()
	 {
		 DBG (SystemStats::getJUCEVersion());
	 }
 };

 static JuceVersionPrinter juceVersionPrinter;
#endif

/*** End of inlined file: juce_SystemStats.cpp ***/


/*** Start of inlined file: juce_CharacterFunctions.cpp ***/
#if JUCE_MSVC
 #pragma warning (push)
 #pragma warning (disable: 4514 4996)
#endif

juce_wchar CharacterFunctions::toUpperCase (const juce_wchar character) noexcept
{
	return towupper ((wchar_t) character);
}

juce_wchar CharacterFunctions::toLowerCase (const juce_wchar character) noexcept
{
	return towlower ((wchar_t) character);
}

bool CharacterFunctions::isUpperCase (const juce_wchar character) noexcept
{
   #if JUCE_WINDOWS
	return iswupper ((wchar_t) character) != 0;
   #else
	return toLowerCase (character) != character;
   #endif
}

bool CharacterFunctions::isLowerCase (const juce_wchar character) noexcept
{
   #if JUCE_WINDOWS
	return iswlower ((wchar_t) character) != 0;
   #else
	return toUpperCase (character) != character;
   #endif
}

#if JUCE_MSVC
 #pragma warning (pop)
#endif

bool CharacterFunctions::isWhitespace (const char character) noexcept
{
	return character == ' ' || (character <= 13 && character >= 9);
}

bool CharacterFunctions::isWhitespace (const juce_wchar character) noexcept
{
	return iswspace ((wchar_t) character) != 0;
}

bool CharacterFunctions::isDigit (const char character) noexcept
{
	return (character >= '0' && character <= '9');
}

bool CharacterFunctions::isDigit (const juce_wchar character) noexcept
{
	return iswdigit ((wchar_t) character) != 0;
}

bool CharacterFunctions::isLetter (const char character) noexcept
{
	return (character >= 'a' && character <= 'z')
		|| (character >= 'A' && character <= 'Z');
}

bool CharacterFunctions::isLetter (const juce_wchar character) noexcept
{
	return iswalpha ((wchar_t) character) != 0;
}

bool CharacterFunctions::isLetterOrDigit (const char character) noexcept
{
	return (character >= 'a' && character <= 'z')
		|| (character >= 'A' && character <= 'Z')
		|| (character >= '0' && character <= '9');
}

bool CharacterFunctions::isLetterOrDigit (const juce_wchar character) noexcept
{
	return iswalnum ((wchar_t) character) != 0;
}

int CharacterFunctions::getHexDigitValue (const juce_wchar digit) noexcept
{
	unsigned int d = digit - '0';
	if (d < (unsigned int) 10)
		return (int) d;

	d += (unsigned int) ('0' - 'a');
	if (d < (unsigned int) 6)
		return (int) d + 10;

	d += (unsigned int) ('a' - 'A');
	if (d < (unsigned int) 6)
		return (int) d + 10;

	return -1;
}

double CharacterFunctions::mulexp10 (const double value, int exponent) noexcept
{
	if (exponent == 0)
		return value;

	if (value == 0)
		return 0;

	const bool negative = (exponent < 0);
	if (negative)
		exponent = -exponent;

	double result = 1.0, power = 10.0;
	for (int bit = 1; exponent != 0; bit <<= 1)
	{
		if ((exponent & bit) != 0)
		{
			exponent ^= bit;
			result *= power;
			if (exponent == 0)
				break;
		}
		power *= power;
	}

	return negative ? (value / result) : (value * result);
}

/*** End of inlined file: juce_CharacterFunctions.cpp ***/


/*** Start of inlined file: juce_Identifier.cpp ***/
StringPool& Identifier::getPool()
{
	static StringPool pool;
	return pool;
}

Identifier::Identifier() noexcept
	: name (nullptr)
{
}

Identifier::Identifier (const Identifier& other) noexcept
	: name (other.name)
{
}

Identifier& Identifier::operator= (const Identifier& other) noexcept
{
	name = other.name;
	return *this;
}

Identifier::Identifier (const String& name_)
	: name (Identifier::getPool().getPooledString (name_))
{
	/* An Identifier string must be suitable for use as a script variable or XML
	   attribute, so it can only contain this limited set of characters.. */
	jassert (isValidIdentifier (name_));
}

Identifier::Identifier (const char* const name_)
	: name (Identifier::getPool().getPooledString (name_))
{
	/* An Identifier string must be suitable for use as a script variable or XML
	   attribute, so it can only contain this limited set of characters.. */
	jassert (isValidIdentifier (toString()));
}

Identifier::~Identifier()
{
}

bool Identifier::isValidIdentifier (const String& possibleIdentifier) noexcept
{
	return possibleIdentifier.isNotEmpty()
			&& possibleIdentifier.containsOnly ("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-:#@$%");
}

/*** End of inlined file: juce_Identifier.cpp ***/


/*** Start of inlined file: juce_LocalisedStrings.cpp ***/
LocalisedStrings::LocalisedStrings (const String& fileContents)
{
	loadFromText (fileContents);
}

LocalisedStrings::LocalisedStrings (const File& fileToLoad)
{
	loadFromText (fileToLoad.loadFileAsString());
}

LocalisedStrings::~LocalisedStrings()
{
}

String LocalisedStrings::translate (const String& text) const
{
	return translations.getValue (text, text);
}

String LocalisedStrings::translate (const String& text, const String& resultIfNotFound) const
{
	return translations.getValue (text, resultIfNotFound);
}

namespace
{
   #if JUCE_CHECK_MEMORY_LEAKS
	// By using this object to force a LocalisedStrings object to be created
	// before the currentMappings object, we can force the static order-of-destruction to
	// delete the currentMappings object first, which avoids a bogus leak warning.
	// (Oddly, just creating a LocalisedStrings on the stack doesn't work in gcc, it
	// has to be created with 'new' for this to work..)
	struct LeakAvoidanceTrick
	{
		LeakAvoidanceTrick()
		{
			const ScopedPointer<LocalisedStrings> dummy (new LocalisedStrings (String()));
		}
	};

	LeakAvoidanceTrick leakAvoidanceTrick;
   #endif

	SpinLock currentMappingsLock;
	ScopedPointer<LocalisedStrings> currentMappings;

	int findCloseQuote (const String& text, int startPos)
	{
		juce_wchar lastChar = 0;
		String::CharPointerType t (text.getCharPointer() + startPos);

		for (;;)
		{
			const juce_wchar c = t.getAndAdvance();

			if (c == 0 || (c == '"' && lastChar != '\\'))
				break;

			lastChar = c;
			++startPos;
		}

		return startPos;
	}

	String unescapeString (const String& s)
	{
		return s.replace ("\\\"", "\"")
				.replace ("\\\'", "\'")
				.replace ("\\t", "\t")
				.replace ("\\r", "\r")
				.replace ("\\n", "\n");
	}
}

void LocalisedStrings::loadFromText (const String& fileContents)
{
	StringArray lines;
	lines.addLines (fileContents);

	for (int i = 0; i < lines.size(); ++i)
	{
		String line (lines[i].trim());

		if (line.startsWithChar ('"'))
		{
			int closeQuote = findCloseQuote (line, 1);

			const String originalText (unescapeString (line.substring (1, closeQuote)));

			if (originalText.isNotEmpty())
			{
				const int openingQuote = findCloseQuote (line, closeQuote + 1);
				closeQuote = findCloseQuote (line, openingQuote + 1);

				const String newText (unescapeString (line.substring (openingQuote + 1, closeQuote)));

				if (newText.isNotEmpty())
					translations.set (originalText, newText);
			}
		}
		else if (line.startsWithIgnoreCase ("language:"))
		{
			languageName = line.substring (9).trim();
		}
		else if (line.startsWithIgnoreCase ("countries:"))
		{
			countryCodes.addTokens (line.substring (10).trim(), true);
			countryCodes.trim();
			countryCodes.removeEmptyStrings();
		}
	}
}

void LocalisedStrings::setIgnoresCase (const bool shouldIgnoreCase)
{
	translations.setIgnoresCase (shouldIgnoreCase);
}

void LocalisedStrings::setCurrentMappings (LocalisedStrings* newTranslations)
{
	const SpinLock::ScopedLockType sl (currentMappingsLock);
	currentMappings = newTranslations;
}

LocalisedStrings* LocalisedStrings::getCurrentMappings()
{
	return currentMappings;
}

String LocalisedStrings::translateWithCurrentMappings (const String& text)
{
	return juce::translate (text);
}

String LocalisedStrings::translateWithCurrentMappings (const char* text)
{
	return juce::translate (String (text));
}

String translate (const String& text)
{
	return translate (text, text);
}

String translate (const char* const literal)
{
	const String text (literal);
	return translate (text, text);
}

String translate (const String& text, const String& resultIfNotFound)
{
	const SpinLock::ScopedLockType sl (currentMappingsLock);

	const LocalisedStrings* const currentMappings = LocalisedStrings::getCurrentMappings();
	if (currentMappings != nullptr)
		return currentMappings->translate (text, resultIfNotFound);

	return resultIfNotFound;
}

/*** End of inlined file: juce_LocalisedStrings.cpp ***/


/*** Start of inlined file: juce_String.cpp ***/
#if JUCE_MSVC
 #pragma warning (push)
 #pragma warning (disable: 4514 4996)
#endif

NewLine newLine;

#if defined (JUCE_STRINGS_ARE_UNICODE) && ! JUCE_STRINGS_ARE_UNICODE
 #error "JUCE_STRINGS_ARE_UNICODE is deprecated! All strings are now unicode by default."
#endif

#if JUCE_NATIVE_WCHAR_IS_UTF8
 typedef CharPointer_UTF8          CharPointer_wchar_t;
#elif JUCE_NATIVE_WCHAR_IS_UTF16
 typedef CharPointer_UTF16         CharPointer_wchar_t;
#else
 typedef CharPointer_UTF32         CharPointer_wchar_t;
#endif

static inline CharPointer_wchar_t castToCharPointer_wchar_t (const void* t) noexcept
{
	return CharPointer_wchar_t (static_cast <const CharPointer_wchar_t::CharType*> (t));
}

class StringHolder
{
public:
	StringHolder()
		: refCount (0x3fffffff), allocatedNumBytes (sizeof (*text))
	{
		text[0] = 0;
	}

	typedef String::CharPointerType CharPointerType;
	typedef String::CharPointerType::CharType CharType;

	static const CharPointerType createUninitialisedBytes (const size_t numBytes)
	{
		StringHolder* const s = reinterpret_cast <StringHolder*> (new char [sizeof (StringHolder) - sizeof (CharType) + numBytes]);
		s->refCount.value = 0;
		s->allocatedNumBytes = numBytes;
		return CharPointerType (s->text);
	}

	template <class CharPointer>
	static const CharPointerType createFromCharPointer (const CharPointer& text)
	{
		if (text.getAddress() == nullptr || text.isEmpty())
			return getEmpty();

		CharPointer t (text);
		size_t bytesNeeded = sizeof (CharType);

		while (! t.isEmpty())
			bytesNeeded += CharPointerType::getBytesRequiredFor (t.getAndAdvance());

		const CharPointerType dest (createUninitialisedBytes (bytesNeeded));
		CharPointerType (dest).writeAll (text);
		return dest;
	}

	template <class CharPointer>
	static const CharPointerType createFromCharPointer (const CharPointer& text, size_t maxChars)
	{
		if (text.getAddress() == nullptr || text.isEmpty() || maxChars == 0)
			return getEmpty();

		CharPointer end (text);
		size_t numChars = 0;
		size_t bytesNeeded = sizeof (CharType);

		while (numChars < maxChars && ! end.isEmpty())
		{
			bytesNeeded += CharPointerType::getBytesRequiredFor (end.getAndAdvance());
			++numChars;
		}

		const CharPointerType dest (createUninitialisedBytes (bytesNeeded));
		CharPointerType (dest).writeWithCharLimit (text, (int) numChars + 1);
		return dest;
	}

	template <class CharPointer>
	static const CharPointerType createFromCharPointer (const CharPointer& start, const CharPointer& end)
	{
		if (start.getAddress() == nullptr || start.isEmpty())
			return getEmpty();

		CharPointer e (start);
		int numChars = 0;
		size_t bytesNeeded = sizeof (CharType);

		while (e < end && ! e.isEmpty())
		{
			bytesNeeded += CharPointerType::getBytesRequiredFor (e.getAndAdvance());
			++numChars;
		}

		const CharPointerType dest (createUninitialisedBytes (bytesNeeded));
		CharPointerType (dest).writeWithCharLimit (start, numChars + 1);
		return dest;
	}

	static const CharPointerType createFromFixedLength (const char* const src, const size_t numChars)
	{
		const CharPointerType dest (createUninitialisedBytes (numChars * sizeof (CharType) + sizeof (CharType)));
		CharPointerType (dest).writeWithCharLimit (CharPointer_UTF8 (src), (int) (numChars + 1));
		return dest;
	}

	static inline const CharPointerType getEmpty() noexcept
	{
		return CharPointerType (empty.text);
	}

	static void retain (const CharPointerType& text) noexcept
	{
		++(bufferFromText (text)->refCount);
	}

	static inline void release (StringHolder* const b) noexcept
	{
		if (--(b->refCount) == -1 && b != &empty)
			delete[] reinterpret_cast <char*> (b);
	}

	static void release (const CharPointerType& text) noexcept
	{
		release (bufferFromText (text));
	}

	static CharPointerType makeUnique (const CharPointerType& text)
	{
		StringHolder* const b = bufferFromText (text);

		if (b->refCount.get() <= 0)
			return text;

		CharPointerType newText (createUninitialisedBytes (b->allocatedNumBytes));
		memcpy (newText.getAddress(), text.getAddress(), b->allocatedNumBytes);
		release (b);

		return newText;
	}

	static CharPointerType makeUniqueWithByteSize (const CharPointerType& text, size_t numBytes)
	{
		StringHolder* const b = bufferFromText (text);

		if (b->refCount.get() <= 0 && b->allocatedNumBytes >= numBytes)
			return text;

		CharPointerType newText (createUninitialisedBytes (jmax (b->allocatedNumBytes, numBytes)));
		memcpy (newText.getAddress(), text.getAddress(), b->allocatedNumBytes);
		release (b);

		return newText;
	}

	static size_t getAllocatedNumBytes (const CharPointerType& text) noexcept
	{
		return bufferFromText (text)->allocatedNumBytes;
	}

	Atomic<int> refCount;
	size_t allocatedNumBytes;
	CharType text[1];

	static StringHolder empty;

private:
	static inline StringHolder* bufferFromText (const CharPointerType& text) noexcept
	{
		// (Can't use offsetof() here because of warnings about this not being a POD)
		return reinterpret_cast <StringHolder*> (reinterpret_cast <char*> (text.getAddress())
					- (reinterpret_cast <size_t> (reinterpret_cast <StringHolder*> (1)->text) - 1));
	}

	void compileTimeChecks()
	{
		// Let me know if any of these assertions fail on your system!
	   #if JUCE_NATIVE_WCHAR_IS_UTF8
		static_jassert (sizeof (wchar_t) == 1);
	   #elif JUCE_NATIVE_WCHAR_IS_UTF16
		static_jassert (sizeof (wchar_t) == 2);
	   #elif JUCE_NATIVE_WCHAR_IS_UTF32
		static_jassert (sizeof (wchar_t) == 4);
	   #else
		#error "native wchar_t size is unknown"
	   #endif
	}
};

StringHolder StringHolder::empty;
const String String::empty;

void String::preallocateBytes (const size_t numBytesNeeded)
{
	text = StringHolder::makeUniqueWithByteSize (text, numBytesNeeded + sizeof (CharPointerType::CharType));
}

String::String() noexcept
	: text (StringHolder::getEmpty())
{
}

String::~String() noexcept
{
	StringHolder::release (text);
}

String::String (const String& other) noexcept
	: text (other.text)
{
	StringHolder::retain (text);
}

void String::swapWith (String& other) noexcept
{
	std::swap (text, other.text);
}

String& String::operator= (const String& other) noexcept
{
	StringHolder::retain (other.text);
	StringHolder::release (text.atomicSwap (other.text));
	return *this;
}

#if JUCE_COMPILER_SUPPORTS_MOVE_SEMANTICS
String::String (String&& other) noexcept
	: text (other.text)
{
	other.text = StringHolder::getEmpty();
}

String& String::operator= (String&& other) noexcept
{
	std::swap (text, other.text);
	return *this;
}
#endif

inline String::PreallocationBytes::PreallocationBytes (const size_t numBytes_) : numBytes (numBytes_) {}

String::String (const PreallocationBytes& preallocationSize)
	: text (StringHolder::createUninitialisedBytes (preallocationSize.numBytes + sizeof (CharPointerType::CharType)))
{
}

String::String (const char* const t)
	: text (StringHolder::createFromCharPointer (CharPointer_ASCII (t)))
{
	/*  If you get an assertion here, then you're trying to create a string from 8-bit data
		that contains values greater than 127. These can NOT be correctly converted to unicode
		because there's no way for the String class to know what encoding was used to
		create them. The source data could be UTF-8, ASCII or one of many local code-pages.

		To get around this problem, you must be more explicit when you pass an ambiguous 8-bit
		string to the String class - so for example if your source data is actually UTF-8,
		you'd call String (CharPointer_UTF8 ("my utf8 string..")), and it would be able to
		correctly convert the multi-byte characters to unicode. It's *highly* recommended that
		you use UTF-8 with escape characters in your source code to represent extended characters,
		because there's no other way to represent these strings in a way that isn't dependent on
		the compiler, source code editor and platform.
	*/
	jassert (t == nullptr || CharPointer_ASCII::isValidString (t, std::numeric_limits<int>::max()));
}

String::String (const char* const t, const size_t maxChars)
	: text (StringHolder::createFromCharPointer (CharPointer_ASCII (t), maxChars))
{
	/*  If you get an assertion here, then you're trying to create a string from 8-bit data
		that contains values greater than 127. These can NOT be correctly converted to unicode
		because there's no way for the String class to know what encoding was used to
		create them. The source data could be UTF-8, ASCII or one of many local code-pages.

		To get around this problem, you must be more explicit when you pass an ambiguous 8-bit
		string to the String class - so for example if your source data is actually UTF-8,
		you'd call String (CharPointer_UTF8 ("my utf8 string..")), and it would be able to
		correctly convert the multi-byte characters to unicode. It's *highly* recommended that
		you use UTF-8 with escape characters in your source code to represent extended characters,
		because there's no other way to represent these strings in a way that isn't dependent on
		the compiler, source code editor and platform.
	*/
	jassert (t == nullptr || CharPointer_ASCII::isValidString (t, (int) maxChars));
}

String::String (const wchar_t* const t)      : text (StringHolder::createFromCharPointer (castToCharPointer_wchar_t (t))) {}
String::String (const CharPointer_UTF8&  t)  : text (StringHolder::createFromCharPointer (t)) {}
String::String (const CharPointer_UTF16& t)  : text (StringHolder::createFromCharPointer (t)) {}
String::String (const CharPointer_UTF32& t)  : text (StringHolder::createFromCharPointer (t)) {}
String::String (const CharPointer_ASCII& t)  : text (StringHolder::createFromCharPointer (t)) {}

String::String (const CharPointer_UTF8&  t, const size_t maxChars)  : text (StringHolder::createFromCharPointer (t, maxChars)) {}
String::String (const CharPointer_UTF16& t, const size_t maxChars)  : text (StringHolder::createFromCharPointer (t, maxChars)) {}
String::String (const CharPointer_UTF32& t, const size_t maxChars)  : text (StringHolder::createFromCharPointer (t, maxChars)) {}
String::String (const wchar_t* const t, size_t maxChars)            : text (StringHolder::createFromCharPointer (castToCharPointer_wchar_t (t), maxChars)) {}

String::String (const CharPointer_UTF8&  start, const CharPointer_UTF8&  end) : text (StringHolder::createFromCharPointer (start, end)) {}
String::String (const CharPointer_UTF16& start, const CharPointer_UTF16& end) : text (StringHolder::createFromCharPointer (start, end)) {}
String::String (const CharPointer_UTF32& start, const CharPointer_UTF32& end) : text (StringHolder::createFromCharPointer (start, end)) {}

String String::charToString (const juce_wchar character)
{
	String result (PreallocationBytes (CharPointerType::getBytesRequiredFor (character)));
	CharPointerType t (result.text);
	t.write (character);
	t.writeNull();
	return result;
}

namespace NumberToStringConverters
{
	// pass in a pointer to the END of a buffer..
	static char* numberToString (char* t, const int64 n) noexcept
	{
		*--t = 0;
		int64 v = (n >= 0) ? n : -n;

		do
		{
			*--t = (char) ('0' + (int) (v % 10));
			v /= 10;

		} while (v > 0);

		if (n < 0)
			*--t = '-';

		return t;
	}

	static char* numberToString (char* t, uint64 v) noexcept
	{
		*--t = 0;

		do
		{
			*--t = (char) ('0' + (int) (v % 10));
			v /= 10;

		} while (v > 0);

		return t;
	}

	static char* numberToString (char* t, const int n) noexcept
	{
		if (n == (int) 0x80000000) // (would cause an overflow)
			return numberToString (t, (int64) n);

		*--t = 0;
		int v = abs (n);

		do
		{
			*--t = (char) ('0' + (v % 10));
			v /= 10;

		} while (v > 0);

		if (n < 0)
			*--t = '-';

		return t;
	}

	static char* numberToString (char* t, unsigned int v) noexcept
	{
		*--t = 0;

		do
		{
			*--t = (char) ('0' + (v % 10));
			v /= 10;

		} while (v > 0);

		return t;
	}

	static char* doubleToString (char* buffer, const int numChars, double n, int numDecPlaces, size_t& len) noexcept
	{
		if (numDecPlaces > 0 && n > -1.0e20 && n < 1.0e20)
		{
			char* const end = buffer + numChars;
			char* t = end;
			int64 v = (int64) (pow (10.0, numDecPlaces) * std::abs (n) + 0.5);
			*--t = (char) 0;

			while (numDecPlaces >= 0 || v > 0)
			{
				if (numDecPlaces == 0)
					*--t = '.';

				*--t = (char) ('0' + (v % 10));

				v /= 10;
				--numDecPlaces;
			}

			if (n < 0)
				*--t = '-';

			len = (size_t) (end - t - 1);
			return t;
		}
		else
		{
			// Use a locale-free sprintf where possible (not available on linux AFAICT)
		   #if JUCE_WINDOWS
			len = (size_t) _sprintf_l (buffer, "%.9g", _create_locale (LC_NUMERIC, "C"), n);
		   #elif JUCE_MAC || JUCE_IOS
			len = (size_t)  sprintf_l (buffer, nullptr, "%.9g", n);
		   #else
			len = (size_t)  sprintf (buffer, "%.9g", n);
		   #endif

			return buffer;
		}
	}

	template <typename IntegerType>
	String::CharPointerType createFromInteger (const IntegerType number)
	{
		char buffer [32];
		char* const end = buffer + numElementsInArray (buffer);
		char* const start = numberToString (end, number);

		return StringHolder::createFromFixedLength (start, (size_t) (end - start - 1));
	}

	static String::CharPointerType createFromDouble (const double number, const int numberOfDecimalPlaces)
	{
		char buffer [48];
		size_t len;
		char* const start = doubleToString (buffer, numElementsInArray (buffer), (double) number, numberOfDecimalPlaces, len);
		return StringHolder::createFromFixedLength (start, len);
	}
}

String::String (const int number)            : text (NumberToStringConverters::createFromInteger (number)) {}
String::String (const unsigned int number)   : text (NumberToStringConverters::createFromInteger (number)) {}
String::String (const short number)          : text (NumberToStringConverters::createFromInteger ((int) number)) {}
String::String (const unsigned short number) : text (NumberToStringConverters::createFromInteger ((unsigned int) number)) {}
String::String (const int64 number)          : text (NumberToStringConverters::createFromInteger (number)) {}
String::String (const uint64 number)         : text (NumberToStringConverters::createFromInteger (number)) {}

String::String (const float number)          : text (NumberToStringConverters::createFromDouble ((double) number, 0)) {}
String::String (const double number)         : text (NumberToStringConverters::createFromDouble (number, 0)) {}
String::String (const float number, const int numberOfDecimalPlaces)   : text (NumberToStringConverters::createFromDouble ((double) number, numberOfDecimalPlaces)) {}
String::String (const double number, const int numberOfDecimalPlaces)  : text (NumberToStringConverters::createFromDouble (number, numberOfDecimalPlaces)) {}

int String::length() const noexcept
{
	return (int) text.length();
}

size_t String::getByteOffsetOfEnd() const noexcept
{
	return (size_t) (((char*) text.findTerminatingNull().getAddress()) - (char*) text.getAddress());
}

const juce_wchar String::operator[] (int index) const noexcept
{
	jassert (index == 0 || (index > 0 && index <= (int) text.lengthUpTo ((size_t) index + 1)));
	return text [index];
}

int String::hashCode() const noexcept
{
	CharPointerType t (text);
	int result = 0;

	while (! t.isEmpty())
		result = 31 * result + (int) t.getAndAdvance();

	return result;
}

int64 String::hashCode64() const noexcept
{
	CharPointerType t (text);
	int64 result = 0;

	while (! t.isEmpty())
		result = 101 * result + t.getAndAdvance();

	return result;
}

JUCE_API bool JUCE_CALLTYPE operator== (const String& s1, const String& s2) noexcept            { return s1.compare (s2) == 0; }
JUCE_API bool JUCE_CALLTYPE operator== (const String& s1, const char* const s2) noexcept        { return s1.compare (s2) == 0; }
JUCE_API bool JUCE_CALLTYPE operator== (const String& s1, const wchar_t* const s2) noexcept     { return s1.compare (s2) == 0; }
JUCE_API bool JUCE_CALLTYPE operator== (const String& s1, const CharPointer_UTF8& s2) noexcept  { return s1.getCharPointer().compare (s2) == 0; }
JUCE_API bool JUCE_CALLTYPE operator== (const String& s1, const CharPointer_UTF16& s2) noexcept { return s1.getCharPointer().compare (s2) == 0; }
JUCE_API bool JUCE_CALLTYPE operator== (const String& s1, const CharPointer_UTF32& s2) noexcept { return s1.getCharPointer().compare (s2) == 0; }
JUCE_API bool JUCE_CALLTYPE operator!= (const String& s1, const String& s2) noexcept            { return s1.compare (s2) != 0; }
JUCE_API bool JUCE_CALLTYPE operator!= (const String& s1, const char* const s2) noexcept        { return s1.compare (s2) != 0; }
JUCE_API bool JUCE_CALLTYPE operator!= (const String& s1, const wchar_t* const s2) noexcept     { return s1.compare (s2) != 0; }
JUCE_API bool JUCE_CALLTYPE operator!= (const String& s1, const CharPointer_UTF8& s2) noexcept  { return s1.getCharPointer().compare (s2) != 0; }
JUCE_API bool JUCE_CALLTYPE operator!= (const String& s1, const CharPointer_UTF16& s2) noexcept { return s1.getCharPointer().compare (s2) != 0; }
JUCE_API bool JUCE_CALLTYPE operator!= (const String& s1, const CharPointer_UTF32& s2) noexcept { return s1.getCharPointer().compare (s2) != 0; }
JUCE_API bool JUCE_CALLTYPE operator>  (const String& s1, const String& s2) noexcept            { return s1.compare (s2) > 0; }
JUCE_API bool JUCE_CALLTYPE operator<  (const String& s1, const String& s2) noexcept            { return s1.compare (s2) < 0; }
JUCE_API bool JUCE_CALLTYPE operator>= (const String& s1, const String& s2) noexcept            { return s1.compare (s2) >= 0; }
JUCE_API bool JUCE_CALLTYPE operator<= (const String& s1, const String& s2) noexcept            { return s1.compare (s2) <= 0; }

bool String::equalsIgnoreCase (const wchar_t* const t) const noexcept
{
	return t != nullptr ? text.compareIgnoreCase (castToCharPointer_wchar_t (t)) == 0
						: isEmpty();
}

bool String::equalsIgnoreCase (const char* const t) const noexcept
{
	return t != nullptr ? text.compareIgnoreCase (CharPointer_UTF8 (t)) == 0
						: isEmpty();
}

bool String::equalsIgnoreCase (const String& other) const noexcept
{
	return text == other.text
			|| text.compareIgnoreCase (other.text) == 0;
}

int String::compare (const String& other) const noexcept           { return (text == other.text) ? 0 : text.compare (other.text); }
int String::compare (const char* const other) const noexcept       { return text.compare (CharPointer_UTF8 (other)); }
int String::compare (const wchar_t* const other) const noexcept    { return text.compare (castToCharPointer_wchar_t (other)); }
int String::compareIgnoreCase (const String& other) const noexcept { return (text == other.text) ? 0 : text.compareIgnoreCase (other.text); }

int String::compareLexicographically (const String& other) const noexcept
{
	CharPointerType s1 (text);

	while (! (s1.isEmpty() || s1.isLetterOrDigit()))
		++s1;

	CharPointerType s2 (other.text);

	while (! (s2.isEmpty() || s2.isLetterOrDigit()))
		++s2;

	return s1.compareIgnoreCase (s2);
}

void String::append (const String& textToAppend, size_t maxCharsToTake)
{
	appendCharPointer (textToAppend.text, maxCharsToTake);
}

String& String::operator+= (const wchar_t* const t)
{
	appendCharPointer (castToCharPointer_wchar_t (t));
	return *this;
}

String& String::operator+= (const char* const t)
{
	/*  If you get an assertion here, then you're trying to create a string from 8-bit data
		that contains values greater than 127. These can NOT be correctly converted to unicode
		because there's no way for the String class to know what encoding was used to
		create them. The source data could be UTF-8, ASCII or one of many local code-pages.

		To get around this problem, you must be more explicit when you pass an ambiguous 8-bit
		string to the String class - so for example if your source data is actually UTF-8,
		you'd call String (CharPointer_UTF8 ("my utf8 string..")), and it would be able to
		correctly convert the multi-byte characters to unicode. It's *highly* recommended that
		you use UTF-8 with escape characters in your source code to represent extended characters,
		because there's no other way to represent these strings in a way that isn't dependent on
		the compiler, source code editor and platform.
	*/
	jassert (t == nullptr || CharPointer_ASCII::isValidString (t, std::numeric_limits<int>::max()));

	appendCharPointer (CharPointer_ASCII (t));
	return *this;
}

String& String::operator+= (const String& other)
{
	if (isEmpty())
		return operator= (other);

	appendCharPointer (other.text);
	return *this;
}

String& String::operator+= (const char ch)
{
	const char asString[] = { ch, 0 };
	return operator+= (asString);
}

String& String::operator+= (const wchar_t ch)
{
	const wchar_t asString[] = { ch, 0 };
	return operator+= (asString);
}

#if ! JUCE_NATIVE_WCHAR_IS_UTF32
String& String::operator+= (const juce_wchar ch)
{
	const juce_wchar asString[] = { ch, 0 };
	appendCharPointer (CharPointer_UTF32 (asString));
	return *this;
}
#endif

String& String::operator+= (const int number)
{
	char buffer [16];
	char* const end = buffer + numElementsInArray (buffer);
	char* const start = NumberToStringConverters::numberToString (end, number);

	const int numExtraChars = (int) (end - start);

	if (numExtraChars > 0)
	{
		const size_t byteOffsetOfNull = getByteOffsetOfEnd();
		const size_t newBytesNeeded = sizeof (CharPointerType::CharType) + byteOffsetOfNull
										+ sizeof (CharPointerType::CharType) * numExtraChars;

		text = StringHolder::makeUniqueWithByteSize (text, newBytesNeeded);

		CharPointerType newEnd (addBytesToPointer (text.getAddress(), (int) byteOffsetOfNull));
		newEnd.writeWithCharLimit (CharPointer_ASCII (start), numExtraChars);
	}

	return *this;
}

JUCE_API String JUCE_CALLTYPE operator+ (const char* const string1, const String& string2)
{
	String s (string1);
	return s += string2;
}

JUCE_API String JUCE_CALLTYPE operator+ (const wchar_t* const string1, const String& string2)
{
	String s (string1);
	return s += string2;
}

JUCE_API String JUCE_CALLTYPE operator+ (const char s1, const String& s2)       { return String::charToString ((juce_wchar) (uint8) s1) + s2; }
JUCE_API String JUCE_CALLTYPE operator+ (const wchar_t s1, const String& s2)    { return String::charToString (s1) + s2; }
#if ! JUCE_NATIVE_WCHAR_IS_UTF32
JUCE_API String JUCE_CALLTYPE operator+ (const juce_wchar s1, const String& s2) { return String::charToString (s1) + s2; }
#endif

JUCE_API String JUCE_CALLTYPE operator+ (String s1, const String& s2)       { return s1 += s2; }
JUCE_API String JUCE_CALLTYPE operator+ (String s1, const char* const s2)   { return s1 += s2; }
JUCE_API String JUCE_CALLTYPE operator+ (String s1, const wchar_t* s2)      { return s1 += s2; }

JUCE_API String JUCE_CALLTYPE operator+ (String s1, const char s2)          { return s1 += s2; }
JUCE_API String JUCE_CALLTYPE operator+ (String s1, const wchar_t s2)       { return s1 += s2; }
#if ! JUCE_NATIVE_WCHAR_IS_UTF32
JUCE_API String JUCE_CALLTYPE operator+ (String s1, const juce_wchar s2)    { return s1 += s2; }
#endif

JUCE_API String& JUCE_CALLTYPE operator<< (String& s1, const char s2)             { return s1 += s2; }
JUCE_API String& JUCE_CALLTYPE operator<< (String& s1, const wchar_t s2)          { return s1 += s2; }
#if ! JUCE_NATIVE_WCHAR_IS_UTF32
JUCE_API String& JUCE_CALLTYPE operator<< (String& s1, const juce_wchar s2)       { return s1 += s2; }
#endif

JUCE_API String& JUCE_CALLTYPE operator<< (String& s1, const char* const s2)      { return s1 += s2; }
JUCE_API String& JUCE_CALLTYPE operator<< (String& s1, const wchar_t* const s2)   { return s1 += s2; }
JUCE_API String& JUCE_CALLTYPE operator<< (String& s1, const String& s2)          { return s1 += s2; }

JUCE_API String& JUCE_CALLTYPE operator<< (String& s1, const short number)        { return s1 += (int) number; }
JUCE_API String& JUCE_CALLTYPE operator<< (String& s1, const int number)          { return s1 += number; }
JUCE_API String& JUCE_CALLTYPE operator<< (String& s1, const long number)         { return s1 += (int) number; }
JUCE_API String& JUCE_CALLTYPE operator<< (String& s1, const float number)        { return s1 += String (number); }
JUCE_API String& JUCE_CALLTYPE operator<< (String& s1, const double number)       { return s1 += String (number); }

JUCE_API OutputStream& JUCE_CALLTYPE operator<< (OutputStream& stream, const String& text)
{
	const int numBytes = text.getNumBytesAsUTF8();

   #if (JUCE_STRING_UTF_TYPE == 8)
	stream.write (text.getCharPointer().getAddress(), numBytes);
   #else
	// (This avoids using toUTF8() to prevent the memory bloat that it would leave behind
	// if lots of large, persistent strings were to be written to streams).
	HeapBlock<char> temp (numBytes + 1);
	CharPointer_UTF8 (temp).writeAll (text.getCharPointer());
	stream.write (temp, numBytes);
   #endif

	return stream;
}

JUCE_API String& JUCE_CALLTYPE operator<< (String& string1, const NewLine&)
{
	return string1 += NewLine::getDefault();
}

int String::indexOfChar (const juce_wchar character) const noexcept
{
	return text.indexOf (character);
}

int String::indexOfChar (const int startIndex, const juce_wchar character) const noexcept
{
	CharPointerType t (text);

	for (int i = 0; ! t.isEmpty(); ++i)
	{
		if (i >= startIndex)
		{
			if (t.getAndAdvance() == character)
				return i;
		}
		else
		{
			++t;
		}
	}

	return -1;
}

int String::lastIndexOfChar (const juce_wchar character) const noexcept
{
	CharPointerType t (text);
	int last = -1;

	for (int i = 0; ! t.isEmpty(); ++i)
		if (t.getAndAdvance() == character)
			last = i;

	return last;
}

int String::indexOfAnyOf (const String& charactersToLookFor, const int startIndex, const bool ignoreCase) const noexcept
{
	CharPointerType t (text);

	for (int i = 0; ! t.isEmpty(); ++i)
	{
		if (i >= startIndex)
		{
			if (charactersToLookFor.text.indexOf (t.getAndAdvance(), ignoreCase) >= 0)
				return i;
		}
		else
		{
			++t;
		}
	}

	return -1;
}

int String::indexOf (const String& other) const noexcept
{
	return other.isEmpty() ? 0 : text.indexOf (other.text);
}

int String::indexOfIgnoreCase (const String& other) const noexcept
{
	return other.isEmpty() ? 0 : CharacterFunctions::indexOfIgnoreCase (text, other.text);
}

int String::indexOf (const int startIndex, const String& other) const noexcept
{
	if (other.isEmpty())
		return -1;

	CharPointerType t (text);

	for (int i = startIndex; --i >= 0;)
	{
		if (t.isEmpty())
			return -1;

		++t;
	}

	int found = t.indexOf (other.text);
	if (found >= 0)
		found += startIndex;
	return found;
}

int String::indexOfIgnoreCase (const int startIndex, const String& other) const noexcept
{
	if (other.isEmpty())
		return -1;

	CharPointerType t (text);

	for (int i = startIndex; --i >= 0;)
	{
		if (t.isEmpty())
			return -1;

		++t;
	}

	int found = CharacterFunctions::indexOfIgnoreCase (t, other.text);
	if (found >= 0)
		found += startIndex;
	return found;
}

int String::lastIndexOf (const String& other) const noexcept
{
	if (other.isNotEmpty())
	{
		const int len = other.length();
		int i = length() - len;

		if (i >= 0)
		{
			CharPointerType n (text + i);

			while (i >= 0)
			{
				if (n.compareUpTo (other.text, len) == 0)
					return i;

				--n;
				--i;
			}
		}
	}

	return -1;
}

int String::lastIndexOfIgnoreCase (const String& other) const noexcept
{
	if (other.isNotEmpty())
	{
		const int len = other.length();
		int i = length() - len;

		if (i >= 0)
		{
			CharPointerType n (text + i);

			while (i >= 0)
			{
				if (n.compareIgnoreCaseUpTo (other.text, len) == 0)
					return i;

				--n;
				--i;
			}
		}
	}

	return -1;
}

int String::lastIndexOfAnyOf (const String& charactersToLookFor, const bool ignoreCase) const noexcept
{
	CharPointerType t (text);
	int last = -1;

	for (int i = 0; ! t.isEmpty(); ++i)
		if (charactersToLookFor.text.indexOf (t.getAndAdvance(), ignoreCase) >= 0)
			last = i;

	return last;
}

bool String::contains (const String& other) const noexcept
{
	return indexOf (other) >= 0;
}

bool String::containsChar (const juce_wchar character) const noexcept
{
	return text.indexOf (character) >= 0;
}

bool String::containsIgnoreCase (const String& t) const noexcept
{
	return indexOfIgnoreCase (t) >= 0;
}

int String::indexOfWholeWord (const String& word) const noexcept
{
	if (word.isNotEmpty())
	{
		CharPointerType t (text);
		const int wordLen = word.length();
		const int end = (int) t.length() - wordLen;

		for (int i = 0; i <= end; ++i)
		{
			if (t.compareUpTo (word.text, wordLen) == 0
				  && (i == 0 || ! (t - 1).isLetterOrDigit())
				  && ! (t + wordLen).isLetterOrDigit())
				return i;

			++t;
		}
	}

	return -1;
}

int String::indexOfWholeWordIgnoreCase (const String& word) const noexcept
{
	if (word.isNotEmpty())
	{
		CharPointerType t (text);
		const int wordLen = word.length();
		const int end = (int) t.length() - wordLen;

		for (int i = 0; i <= end; ++i)
		{
			if (t.compareIgnoreCaseUpTo (word.text, wordLen) == 0
				  && (i == 0 || ! (t - 1).isLetterOrDigit())
				  && ! (t + wordLen).isLetterOrDigit())
				return i;

			++t;
		}
	}

	return -1;
}

bool String::containsWholeWord (const String& wordToLookFor) const noexcept
{
	return indexOfWholeWord (wordToLookFor) >= 0;
}

bool String::containsWholeWordIgnoreCase (const String& wordToLookFor) const noexcept
{
	return indexOfWholeWordIgnoreCase (wordToLookFor) >= 0;
}

template <typename CharPointer>
struct WildCardMatcher
{
	static bool matches (CharPointer wildcard, CharPointer test, const bool ignoreCase) noexcept
	{
		for (;;)
		{
			const juce_wchar wc = wildcard.getAndAdvance();

			if (wc == '*')
				return wildcard.isEmpty() || matchesAnywhere (wildcard, test, ignoreCase);

			if (! characterMatches (wc, test.getAndAdvance(), ignoreCase))
				return false;

			if (wc == 0)
				return true;
		}
	}

	static bool characterMatches (const juce_wchar wc, const juce_wchar tc, const bool ignoreCase) noexcept
	{
		return (wc == tc) || (wc == '?' && tc != 0)
				|| (ignoreCase && CharacterFunctions::toLowerCase (wc) == CharacterFunctions::toLowerCase (tc));
	}

	static bool matchesAnywhere (const CharPointer& wildcard, CharPointer test, const bool ignoreCase) noexcept
	{
		for (; ! test.isEmpty(); ++test)
			if (matches (wildcard, test, ignoreCase))
				return true;

		return false;
	}
};

bool String::matchesWildcard (const String& wildcard, const bool ignoreCase) const noexcept
{
	return WildCardMatcher<CharPointerType>::matches (wildcard.text, text, ignoreCase);
}

String String::repeatedString (const String& stringToRepeat, int numberOfTimesToRepeat)
{
	if (numberOfTimesToRepeat <= 0)
		return empty;

	String result (PreallocationBytes (stringToRepeat.getByteOffsetOfEnd() * numberOfTimesToRepeat));
	CharPointerType n (result.text);

	while (--numberOfTimesToRepeat >= 0)
		n.writeAll (stringToRepeat.text);

	return result;
}

String String::paddedLeft (const juce_wchar padCharacter, int minimumLength) const
{
	jassert (padCharacter != 0);

	int extraChars = minimumLength;
	CharPointerType end (text);

	while (! end.isEmpty())
	{
		--extraChars;
		++end;
	}

	if (extraChars <= 0 || padCharacter == 0)
		return *this;

	const size_t currentByteSize = (size_t) (((char*) end.getAddress()) - (char*) text.getAddress());
	String result (PreallocationBytes (currentByteSize + extraChars * CharPointerType::getBytesRequiredFor (padCharacter)));
	CharPointerType n (result.text);

	while (--extraChars >= 0)
		n.write (padCharacter);

	n.writeAll (text);
	return result;
}

String String::paddedRight (const juce_wchar padCharacter, int minimumLength) const
{
	jassert (padCharacter != 0);

	int extraChars = minimumLength;
	CharPointerType end (text);

	while (! end.isEmpty())
	{
		--extraChars;
		++end;
	}

	if (extraChars <= 0 || padCharacter == 0)
		return *this;

	const size_t currentByteSize = (size_t) (((char*) end.getAddress()) - (char*) text.getAddress());
	String result (PreallocationBytes (currentByteSize + extraChars * CharPointerType::getBytesRequiredFor (padCharacter)));
	CharPointerType n (result.text);

	n.writeAll (text);

	while (--extraChars >= 0)
		n.write (padCharacter);

	n.writeNull();
	return result;
}

String String::replaceSection (int index, int numCharsToReplace, const String& stringToInsert) const
{
	if (index < 0)
	{
		// a negative index to replace from?
		jassertfalse;
		index = 0;
	}

	if (numCharsToReplace < 0)
	{
		// replacing a negative number of characters?
		numCharsToReplace = 0;
		jassertfalse;
	}

	int i = 0;
	CharPointerType insertPoint (text);

	while (i < index)
	{
		if (insertPoint.isEmpty())
		{
			// replacing beyond the end of the string?
			jassertfalse;
			return *this + stringToInsert;
		}

		++insertPoint;
		++i;
	}

	CharPointerType startOfRemainder (insertPoint);

	i = 0;
	while (i < numCharsToReplace && ! startOfRemainder.isEmpty())
	{
		++startOfRemainder;
		++i;
	}

	if (insertPoint == text && startOfRemainder.isEmpty())
		return stringToInsert;

	const size_t initialBytes = (size_t) (((char*) insertPoint.getAddress()) - (char*) text.getAddress());
	const size_t newStringBytes = stringToInsert.getByteOffsetOfEnd();
	const size_t remainderBytes = (size_t) (((char*) startOfRemainder.findTerminatingNull().getAddress()) - (char*) startOfRemainder.getAddress());

	const size_t newTotalBytes = initialBytes + newStringBytes + remainderBytes;
	if (newTotalBytes <= 0)
		return String::empty;

	String result (PreallocationBytes ((size_t) newTotalBytes));

	char* dest = (char*) result.text.getAddress();
	memcpy (dest, text.getAddress(), initialBytes);
	dest += initialBytes;
	memcpy (dest, stringToInsert.text.getAddress(), newStringBytes);
	dest += newStringBytes;
	memcpy (dest, startOfRemainder.getAddress(), remainderBytes);
	dest += remainderBytes;
	CharPointerType ((CharPointerType::CharType*) dest).writeNull();

	return result;
}

String String::replace (const String& stringToReplace, const String& stringToInsert, const bool ignoreCase) const
{
	const int stringToReplaceLen = stringToReplace.length();
	const int stringToInsertLen = stringToInsert.length();

	int i = 0;
	String result (*this);

	while ((i = (ignoreCase ? result.indexOfIgnoreCase (i, stringToReplace)
							: result.indexOf (i, stringToReplace))) >= 0)
	{
		result = result.replaceSection (i, stringToReplaceLen, stringToInsert);
		i += stringToInsertLen;
	}

	return result;
}

class StringCreationHelper
{
public:
	StringCreationHelper (const size_t initialBytes)
		: source (nullptr), dest (nullptr), allocatedBytes (initialBytes), bytesWritten (0)
	{
		result.preallocateBytes (allocatedBytes);
		dest = result.getCharPointer();
	}

	StringCreationHelper (const String::CharPointerType& source_)
		: source (source_), dest (nullptr), allocatedBytes (StringHolder::getAllocatedNumBytes (source)), bytesWritten (0)
	{
		result.preallocateBytes (allocatedBytes);
		dest = result.getCharPointer();
	}

	void write (juce_wchar c)
	{
		bytesWritten += String::CharPointerType::getBytesRequiredFor (c);

		if (bytesWritten > allocatedBytes)
		{
			allocatedBytes += jmax ((size_t) 8, allocatedBytes / 16);
			const size_t destOffset = (size_t) (((char*) dest.getAddress()) - (char*) result.getCharPointer().getAddress());
			result.preallocateBytes (allocatedBytes);
			dest = addBytesToPointer (result.getCharPointer().getAddress(), (int) destOffset);
		}

		dest.write (c);
	}

	String result;
	String::CharPointerType source;

private:
	String::CharPointerType dest;
	size_t allocatedBytes, bytesWritten;
};

String String::replaceCharacter (const juce_wchar charToReplace, const juce_wchar charToInsert) const
{
	if (! containsChar (charToReplace))
		return *this;

	StringCreationHelper builder (text);

	for (;;)
	{
		juce_wchar c = builder.source.getAndAdvance();

		if (c == charToReplace)
			c = charToInsert;

		builder.write (c);

		if (c == 0)
			break;
	}

	return builder.result;
}

String String::replaceCharacters (const String& charactersToReplace, const String& charactersToInsertInstead) const
{
	StringCreationHelper builder (text);

	for (;;)
	{
		juce_wchar c = builder.source.getAndAdvance();

		const int index = charactersToReplace.indexOfChar (c);
		if (index >= 0)
			c = charactersToInsertInstead [index];

		builder.write (c);

		if (c == 0)
			break;
	}

	return builder.result;
}

bool String::startsWith (const String& other) const noexcept
{
	return text.compareUpTo (other.text, other.length()) == 0;
}

bool String::startsWithIgnoreCase (const String& other) const noexcept
{
	return text.compareIgnoreCaseUpTo (other.text, other.length()) == 0;
}

bool String::startsWithChar (const juce_wchar character) const noexcept
{
	jassert (character != 0); // strings can't contain a null character!

	return *text == character;
}

bool String::endsWithChar (const juce_wchar character) const noexcept
{
	jassert (character != 0); // strings can't contain a null character!

	if (text.isEmpty())
		return false;

	CharPointerType t (text.findTerminatingNull());
	return *--t == character;
}

bool String::endsWith (const String& other) const noexcept
{
	CharPointerType end (text.findTerminatingNull());
	CharPointerType otherEnd (other.text.findTerminatingNull());

	while (end > text && otherEnd > other.text)
	{
		--end;
		--otherEnd;

		if (*end != *otherEnd)
			return false;
	}

	return otherEnd == other.text;
}

bool String::endsWithIgnoreCase (const String& other) const noexcept
{
	CharPointerType end (text.findTerminatingNull());
	CharPointerType otherEnd (other.text.findTerminatingNull());

	while (end > text && otherEnd > other.text)
	{
		--end;
		--otherEnd;

		if (end.toLowerCase() != otherEnd.toLowerCase())
			return false;
	}

	return otherEnd == other.text;
}

String String::toUpperCase() const
{
	StringCreationHelper builder (text);

	for (;;)
	{
		const juce_wchar c = builder.source.toUpperCase();
		++(builder.source);
		builder.write (c);

		if (c == 0)
			break;
	}

	return builder.result;
}

String String::toLowerCase() const
{
	StringCreationHelper builder (text);

	for (;;)
	{
		const juce_wchar c = builder.source.toLowerCase();
		++(builder.source);
		builder.write (c);

		if (c == 0)
			break;
	}

	return builder.result;
}

juce_wchar String::getLastCharacter() const noexcept
{
	return isEmpty() ? juce_wchar() : text [length() - 1];
}

String String::substring (int start, const int end) const
{
	if (start < 0)
		start = 0;

	if (end <= start)
		return empty;

	int i = 0;
	CharPointerType t1 (text);

	while (i < start)
	{
		if (t1.isEmpty())
			return empty;

		++i;
		++t1;
	}

	CharPointerType t2 (t1);
	while (i < end)
	{
		if (t2.isEmpty())
		{
			if (start == 0)
				return *this;

			break;
		}

		++i;
		++t2;
	}

	return String (t1, t2);
}

String String::substring (int start) const
{
	if (start <= 0)
		return *this;

	CharPointerType t (text);

	while (--start >= 0)
	{
		if (t.isEmpty())
			return empty;

		++t;
	}

	return String (t);
}

String String::dropLastCharacters (const int numberToDrop) const
{
	return String (text, (size_t) jmax (0, length() - numberToDrop));
}

String String::getLastCharacters (const int numCharacters) const
{
	return String (text + jmax (0, length() - jmax (0, numCharacters)));
}

String String::fromFirstOccurrenceOf (const String& sub,
									  const bool includeSubString,
									  const bool ignoreCase) const
{
	const int i = ignoreCase ? indexOfIgnoreCase (sub)
							 : indexOf (sub);
	if (i < 0)
		return empty;

	return substring (includeSubString ? i : i + sub.length());
}

String String::fromLastOccurrenceOf (const String& sub,
									 const bool includeSubString,
									 const bool ignoreCase) const
{
	const int i = ignoreCase ? lastIndexOfIgnoreCase (sub)
							 : lastIndexOf (sub);
	if (i < 0)
		return *this;

	return substring (includeSubString ? i : i + sub.length());
}

String String::upToFirstOccurrenceOf (const String& sub,
									  const bool includeSubString,
									  const bool ignoreCase) const
{
	const int i = ignoreCase ? indexOfIgnoreCase (sub)
							 : indexOf (sub);
	if (i < 0)
		return *this;

	return substring (0, includeSubString ? i + sub.length() : i);
}

String String::upToLastOccurrenceOf (const String& sub,
									 const bool includeSubString,
									 const bool ignoreCase) const
{
	const int i = ignoreCase ? lastIndexOfIgnoreCase (sub)
							 : lastIndexOf (sub);
	if (i < 0)
		return *this;

	return substring (0, includeSubString ? i + sub.length() : i);
}

bool String::isQuotedString() const
{
	const String trimmed (trimStart());

	return trimmed[0] == '"'
		|| trimmed[0] == '\'';
}

String String::unquoted() const
{
	const int len = length();

	if (len == 0)
		return empty;

	const juce_wchar lastChar = text [len - 1];
	const int dropAtStart = (*text == '"' || *text == '\'') ? 1 : 0;
	const int dropAtEnd = (lastChar == '"' || lastChar == '\'') ? 1 : 0;

	return substring (dropAtStart, len - dropAtEnd);
}

String String::quoted (const juce_wchar quoteCharacter) const
{
	if (isEmpty())
		return charToString (quoteCharacter) + quoteCharacter;

	String t (*this);

	if (! t.startsWithChar (quoteCharacter))
		t = charToString (quoteCharacter) + t;

	if (! t.endsWithChar (quoteCharacter))
		t += quoteCharacter;

	return t;
}

static String::CharPointerType findTrimmedEnd (const String::CharPointerType& start, String::CharPointerType end)
{
	while (end > start)
	{
		if (! (--end).isWhitespace())
		{
			++end;
			break;
		}
	}

	return end;
}

String String::trim() const
{
	if (isNotEmpty())
	{
		CharPointerType start (text.findEndOfWhitespace());

		const CharPointerType end (start.findTerminatingNull());
		CharPointerType trimmedEnd (findTrimmedEnd (start, end));

		if (trimmedEnd <= start)
			return empty;
		else if (text < start || trimmedEnd < end)
			return String (start, trimmedEnd);
	}

	return *this;
}

String String::trimStart() const
{
	if (isNotEmpty())
	{
		const CharPointerType t (text.findEndOfWhitespace());

		if (t != text)
			return String (t);
	}

	return *this;
}

String String::trimEnd() const
{
	if (isNotEmpty())
	{
		const CharPointerType end (text.findTerminatingNull());
		CharPointerType trimmedEnd (findTrimmedEnd (text, end));

		if (trimmedEnd < end)
			return String (text, trimmedEnd);
	}

	return *this;
}

String String::trimCharactersAtStart (const String& charactersToTrim) const
{
	CharPointerType t (text);

	while (charactersToTrim.containsChar (*t))
		++t;

	return t == text ? *this : String (t);
}

String String::trimCharactersAtEnd (const String& charactersToTrim) const
{
	if (isNotEmpty())
	{
		const CharPointerType end (text.findTerminatingNull());
		CharPointerType trimmedEnd (end);

		while (trimmedEnd > text)
		{
			if (! charactersToTrim.containsChar (*--trimmedEnd))
			{
				++trimmedEnd;
				break;
			}
		}

		if (trimmedEnd < end)
			return String (text, trimmedEnd);
	}

	return *this;
}

String String::retainCharacters (const String& charactersToRetain) const
{
	if (isEmpty())
		return empty;

	StringCreationHelper builder (text);

	for (;;)
	{
		juce_wchar c = builder.source.getAndAdvance();

		if (charactersToRetain.containsChar (c))
			builder.write (c);

		if (c == 0)
			break;
	}

	builder.write (0);
	return builder.result;
}

String String::removeCharacters (const String& charactersToRemove) const
{
	if (isEmpty())
		return empty;

	StringCreationHelper builder (text);

	for (;;)
	{
		juce_wchar c = builder.source.getAndAdvance();

		if (! charactersToRemove.containsChar (c))
			builder.write (c);

		if (c == 0)
			break;
	}

	return builder.result;
}

String String::initialSectionContainingOnly (const String& permittedCharacters) const
{
	CharPointerType t (text);

	while (! t.isEmpty())
	{
		if (! permittedCharacters.containsChar (*t))
			return String (text, t);

		++t;
	}

	return *this;
}

String String::initialSectionNotContaining (const String& charactersToStopAt) const
{
	CharPointerType t (text);

	while (! t.isEmpty())
	{
		if (charactersToStopAt.containsChar (*t))
			return String (text, t);

		++t;
	}

	return *this;
}

bool String::containsOnly (const String& chars) const noexcept
{
	CharPointerType t (text);

	while (! t.isEmpty())
		if (! chars.containsChar (t.getAndAdvance()))
			return false;

	return true;
}

bool String::containsAnyOf (const String& chars) const noexcept
{
	CharPointerType t (text);

	while (! t.isEmpty())
		if (chars.containsChar (t.getAndAdvance()))
			return true;

	return false;
}

bool String::containsNonWhitespaceChars() const noexcept
{
	CharPointerType t (text);

	while (! t.isEmpty())
	{
		if (! t.isWhitespace())
			return true;

		++t;
	}

	return false;
}

// Note! The format parameter here MUST NOT be a reference, otherwise MS's va_start macro fails to work (but still compiles).
String String::formatted (const String pf, ... )
{
	size_t bufferSize = 256;

	for (;;)
	{
		va_list args;
		va_start (args, pf);

	   #if JUCE_WINDOWS
		HeapBlock <wchar_t> temp (bufferSize);
		const int num = (int) _vsnwprintf (temp.getData(), bufferSize - 1, pf.toWideCharPointer(), args);
	   #elif JUCE_ANDROID
		HeapBlock <char> temp (bufferSize);
		const int num = (int) vsnprintf (temp.getData(), bufferSize - 1, pf.toUTF8(), args);
	   #else
		HeapBlock <wchar_t> temp (bufferSize);
		const int num = (int) vswprintf (temp.getData(), bufferSize - 1, pf.toWideCharPointer(), args);
	   #endif

		va_end (args);

		if (num > 0)
			return String (temp);

		bufferSize += 256;

		if (num == 0 || bufferSize > 65536) // the upper limit is a sanity check to avoid situations where vprintf repeatedly
			break;                          // returns -1 because of an error rather than because it needs more space.
	}

	return empty;
}

int String::getIntValue() const noexcept
{
	return text.getIntValue32();
}

int String::getTrailingIntValue() const noexcept
{
	int n = 0;
	int mult = 1;
	CharPointerType t (text.findTerminatingNull());

	while (--t >= text)
	{
		if (! t.isDigit())
		{
			if (*t == '-')
				n = -n;

			break;
		}

		n += mult * (*t - '0');
		mult *= 10;
	}

	return n;
}

int64 String::getLargeIntValue() const noexcept
{
	return text.getIntValue64();
}

float String::getFloatValue() const noexcept
{
	return (float) getDoubleValue();
}

double String::getDoubleValue() const noexcept
{
	return text.getDoubleValue();
}

static const char hexDigits[] = "0123456789abcdef";

template <typename Type>
struct HexConverter
{
	static String hexToString (Type v)
	{
		char buffer[32];
		char* const end = buffer + 32;
		char* t = end;
		*--t = 0;

		do
		{
			*--t = hexDigits [(int) (v & 15)];
			v >>= 4;

		} while (v != 0);

		return String (t, (size_t) (end - t) - 1);
	}

	static Type stringToHex (String::CharPointerType t) noexcept
	{
		Type result = 0;

		while (! t.isEmpty())
		{
			const int hexValue = CharacterFunctions::getHexDigitValue (t.getAndAdvance());

			if (hexValue >= 0)
				result = (result << 4) | hexValue;
		}

		return result;
	}
};

String String::toHexString (const int number)
{
	return HexConverter <unsigned int>::hexToString ((unsigned int) number);
}

String String::toHexString (const int64 number)
{
	return HexConverter <uint64>::hexToString ((uint64) number);
}

String String::toHexString (const short number)
{
	return toHexString ((int) (unsigned short) number);
}

String String::toHexString (const void* const d, const int size, const int groupSize)
{
	if (size <= 0)
		return empty;

	int numChars = (size * 2) + 2;
	if (groupSize > 0)
		numChars += size / groupSize;

	String s (PreallocationBytes (sizeof (CharPointerType::CharType) * (size_t) numChars));

	const unsigned char* data = static_cast <const unsigned char*> (d);
	CharPointerType dest (s.text);

	for (int i = 0; i < size; ++i)
	{
		const unsigned char nextByte = *data++;
		dest.write ((juce_wchar) hexDigits [nextByte >> 4]);
		dest.write ((juce_wchar) hexDigits [nextByte & 0xf]);

		if (groupSize > 0 && (i % groupSize) == (groupSize - 1) && i < (size - 1))
			dest.write ((juce_wchar) ' ');
	}

	dest.writeNull();
	return s;
}

int String::getHexValue32() const noexcept
{
	return HexConverter<int>::stringToHex (text);
}

int64 String::getHexValue64() const noexcept
{
	return HexConverter<int64>::stringToHex (text);
}

String String::createStringFromData (const void* const data_, const int size)
{
	const uint8* const data = static_cast <const uint8*> (data_);

	if (size <= 0 || data == nullptr)
	{
		return empty;
	}
	else if (size == 1)
	{
		return charToString ((juce_wchar) data[0]);
	}
	else if ((data[0] == (uint8) CharPointer_UTF16::byteOrderMarkBE1 && data[1] == (uint8) CharPointer_UTF16::byteOrderMarkBE2)
		  || (data[0] == (uint8) CharPointer_UTF16::byteOrderMarkLE1 && data[1] == (uint8) CharPointer_UTF16::byteOrderMarkLE2))
	{
		const bool bigEndian = (data[0] == (uint8) CharPointer_UTF16::byteOrderMarkBE1);
		const int numChars = size / 2 - 1;

		StringCreationHelper builder ((size_t) numChars);

		const uint16* const src = (const uint16*) (data + 2);

		if (bigEndian)
		{
			for (int i = 0; i < numChars; ++i)
				builder.write ((juce_wchar) ByteOrder::swapIfLittleEndian (src[i]));
		}
		else
		{
			for (int i = 0; i < numChars; ++i)
				builder.write ((juce_wchar) ByteOrder::swapIfBigEndian (src[i]));
		}

		builder.write (0);
		return builder.result;
	}
	else
	{
		const uint8* start = data;
		const uint8* end = data + size;

		if (size >= 3
			  && data[0] == (uint8) CharPointer_UTF8::byteOrderMark1
			  && data[1] == (uint8) CharPointer_UTF8::byteOrderMark2
			  && data[2] == (uint8) CharPointer_UTF8::byteOrderMark3)
			start += 3;

		return String (CharPointer_UTF8 ((const char*) start),
					   CharPointer_UTF8 ((const char*) end));
	}
}

static const juce_wchar emptyChar = 0;

template <class CharPointerType_Src, class CharPointerType_Dest>
struct StringEncodingConverter
{
	static CharPointerType_Dest convert (const String& s)
	{
		String& source = const_cast <String&> (s);

		typedef typename CharPointerType_Dest::CharType DestChar;

		if (source.isEmpty())
			return CharPointerType_Dest (reinterpret_cast <const DestChar*> (&emptyChar));

		CharPointerType_Src text (source.getCharPointer());
		const size_t extraBytesNeeded = CharPointerType_Dest::getBytesRequiredFor (text);
		const size_t endOffset = (text.sizeInBytes() + 3) & ~3; // the new string must be word-aligned or many Windows
																// functions will fail to read it correctly!
		source.preallocateBytes (endOffset + extraBytesNeeded);
		text = source.getCharPointer();

		void* const newSpace = addBytesToPointer (text.getAddress(), (int) endOffset);
		const CharPointerType_Dest extraSpace (static_cast <DestChar*> (newSpace));

	   #if JUCE_DEBUG // (This just avoids spurious warnings from valgrind about the uninitialised bytes at the end of the buffer..)
		const int bytesToClear = jmin ((int) extraBytesNeeded, 4);
		zeromem (addBytesToPointer (newSpace, (int) (extraBytesNeeded - bytesToClear)), (size_t) bytesToClear);
	   #endif

		CharPointerType_Dest (extraSpace).writeAll (text);
		return extraSpace;
	}
};

template <>
struct StringEncodingConverter <CharPointer_UTF8, CharPointer_UTF8>
{
	static CharPointer_UTF8 convert (const String& source) noexcept   { return CharPointer_UTF8 ((CharPointer_UTF8::CharType*) source.getCharPointer().getAddress()); }
};

template <>
struct StringEncodingConverter <CharPointer_UTF16, CharPointer_UTF16>
{
	static CharPointer_UTF16 convert (const String& source) noexcept  { return CharPointer_UTF16 ((CharPointer_UTF16::CharType*) source.getCharPointer().getAddress()); }
};

template <>
struct StringEncodingConverter <CharPointer_UTF32, CharPointer_UTF32>
{
	static CharPointer_UTF32 convert (const String& source) noexcept  { return CharPointer_UTF32 ((CharPointer_UTF32::CharType*) source.getCharPointer().getAddress()); }
};

CharPointer_UTF8  String::toUTF8()  const { return StringEncodingConverter <CharPointerType, CharPointer_UTF8 >::convert (*this); }
CharPointer_UTF16 String::toUTF16() const { return StringEncodingConverter <CharPointerType, CharPointer_UTF16>::convert (*this); }
CharPointer_UTF32 String::toUTF32() const { return StringEncodingConverter <CharPointerType, CharPointer_UTF32>::convert (*this); }

const wchar_t* String::toWideCharPointer() const
{
	return StringEncodingConverter <CharPointerType, CharPointer_wchar_t>::convert (*this).getAddress();
}

template <class CharPointerType_Src, class CharPointerType_Dest>
struct StringCopier
{
	static int copyToBuffer (const CharPointerType_Src& source, typename CharPointerType_Dest::CharType* const buffer, const int maxBufferSizeBytes)
	{
		jassert (maxBufferSizeBytes >= 0); // keep this value positive, or no characters will be copied!

		if (buffer == nullptr)
			return (int) (CharPointerType_Dest::getBytesRequiredFor (source) + sizeof (typename CharPointerType_Dest::CharType));

		return CharPointerType_Dest (buffer).writeWithDestByteLimit (source, maxBufferSizeBytes);
	}
};

int String::copyToUTF8 (CharPointer_UTF8::CharType* const buffer, const int maxBufferSizeBytes) const noexcept
{
	return StringCopier <CharPointerType, CharPointer_UTF8>::copyToBuffer (text, buffer, maxBufferSizeBytes);
}

int String::copyToUTF16 (CharPointer_UTF16::CharType* const buffer, int maxBufferSizeBytes) const noexcept
{
	return StringCopier <CharPointerType, CharPointer_UTF16>::copyToBuffer (text, buffer, maxBufferSizeBytes);
}

int String::copyToUTF32 (CharPointer_UTF32::CharType* const buffer, int maxBufferSizeBytes) const noexcept
{
	return StringCopier <CharPointerType, CharPointer_UTF32>::copyToBuffer (text, buffer, maxBufferSizeBytes);
}

int String::getNumBytesAsUTF8() const noexcept
{
	return (int) CharPointer_UTF8::getBytesRequiredFor (text);
}

String String::fromUTF8 (const char* const buffer, int bufferSizeBytes)
{
	if (buffer != nullptr)
	{
		if (bufferSizeBytes < 0)
			return String (CharPointer_UTF8 (buffer));
		else if (bufferSizeBytes > 0)
			return String (CharPointer_UTF8 (buffer), CharPointer_UTF8 (buffer + bufferSizeBytes));
	}

	return String::empty;
}

#if JUCE_MSVC
 #pragma warning (pop)
#endif

#if JUCE_UNIT_TESTS

class StringTests  : public UnitTest
{
public:
	StringTests() : UnitTest ("String class") {}

	template <class CharPointerType>
	struct TestUTFConversion
	{
		static void test (UnitTest& test)
		{
			String s (createRandomWideCharString());

			typename CharPointerType::CharType buffer [300];

			memset (buffer, 0xff, sizeof (buffer));
			CharPointerType (buffer).writeAll (s.toUTF32());
			test.expectEquals (String (CharPointerType (buffer)), s);

			memset (buffer, 0xff, sizeof (buffer));
			CharPointerType (buffer).writeAll (s.toUTF16());
			test.expectEquals (String (CharPointerType (buffer)), s);

			memset (buffer, 0xff, sizeof (buffer));
			CharPointerType (buffer).writeAll (s.toUTF8());
			test.expectEquals (String (CharPointerType (buffer)), s);
		}
	};

	static String createRandomWideCharString()
	{
		juce_wchar buffer[50] = { 0 };
		Random r;

		for (int i = 0; i < numElementsInArray (buffer) - 1; ++i)
		{
			if (r.nextBool())
			{
				do
				{
					buffer[i] = (juce_wchar) (1 + r.nextInt (0x10ffff - 1));
				}
				while (! CharPointer_UTF16::canRepresent (buffer[i]));
			}
			else
				buffer[i] = (juce_wchar) (1 + r.nextInt (0xff));
		}

		return CharPointer_UTF32 (buffer);
	}

	void runTest()
	{
		{
			beginTest ("Basics");

			expect (String().length() == 0);
			expect (String() == String::empty);
			String s1, s2 ("abcd");
			expect (s1.isEmpty() && ! s1.isNotEmpty());
			expect (s2.isNotEmpty() && ! s2.isEmpty());
			expect (s2.length() == 4);
			s1 = "abcd";
			expect (s2 == s1 && s1 == s2);
			expect (s1 == "abcd" && s1 == L"abcd");
			expect (String ("abcd") == String (L"abcd"));
			expect (String ("abcdefg", 4) == L"abcd");
			expect (String ("abcdefg", 4) == String (L"abcdefg", 4));
			expect (String::charToString ('x') == "x");
			expect (String::charToString (0) == String::empty);
			expect (s2 + "e" == "abcde" && s2 + 'e' == "abcde");
			expect (s2 + L'e' == "abcde" && s2 + L"e" == "abcde");
			expect (s1.equalsIgnoreCase ("abcD") && s1 < "abce" && s1 > "abbb");
			expect (s1.startsWith ("ab") && s1.startsWith ("abcd") && ! s1.startsWith ("abcde"));
			expect (s1.startsWithIgnoreCase ("aB") && s1.endsWithIgnoreCase ("CD"));
			expect (s1.endsWith ("bcd") && ! s1.endsWith ("aabcd"));
			expectEquals (s1.indexOf (String::empty), 0);
			expectEquals (s1.indexOfIgnoreCase (String::empty), 0);
			expect (s1.startsWith (String::empty) && s1.endsWith (String::empty) && s1.contains (String::empty));
			expect (s1.contains ("cd") && s1.contains ("ab") && s1.contains ("abcd"));
			expect (s1.containsChar ('a'));
			expect (! s1.containsChar ('x'));
			expect (! s1.containsChar (0));
			expect (String ("abc foo bar").containsWholeWord ("abc") && String ("abc foo bar").containsWholeWord ("abc"));
		}

		{
			beginTest ("Operations");

			String s ("012345678");
			expect (s.hashCode() != 0);
			expect (s.hashCode64() != 0);
			expect (s.hashCode() != (s + s).hashCode());
			expect (s.hashCode64() != (s + s).hashCode64());
			expect (s.compare (String ("012345678")) == 0);
			expect (s.compare (String ("012345679")) < 0);
			expect (s.compare (String ("012345676")) > 0);
			expect (s.substring (2, 3) == String::charToString (s[2]));
			expect (s.substring (0, 1) == String::charToString (s[0]));
			expect (s.getLastCharacter() == s [s.length() - 1]);
			expect (String::charToString (s.getLastCharacter()) == s.getLastCharacters (1));
			expect (s.substring (0, 3) == L"012");
			expect (s.substring (0, 100) == s);
			expect (s.substring (-1, 100) == s);
			expect (s.substring (3) == "345678");
			expect (s.indexOf (L"45") == 4);
			expect (String ("444445").indexOf ("45") == 4);
			expect (String ("444445").lastIndexOfChar ('4') == 4);
			expect (String ("45454545x").lastIndexOf (L"45") == 6);
			expect (String ("45454545x").lastIndexOfAnyOf ("456") == 7);
			expect (String ("45454545x").lastIndexOfAnyOf (L"456x") == 8);
			expect (String ("abABaBaBa").lastIndexOfIgnoreCase ("aB") == 6);
			expect (s.indexOfChar (L'4') == 4);
			expect (s + s == "012345678012345678");
			expect (s.startsWith (s));
			expect (s.startsWith (s.substring (0, 4)));
			expect (s.startsWith (s.dropLastCharacters (4)));
			expect (s.endsWith (s.substring (5)));
			expect (s.endsWith (s));
			expect (s.contains (s.substring (3, 6)));
			expect (s.contains (s.substring (3)));
			expect (s.startsWithChar (s[0]));
			expect (s.endsWithChar (s.getLastCharacter()));
			expect (s [s.length()] == 0);
			expect (String ("abcdEFGH").toLowerCase() == String ("abcdefgh"));
			expect (String ("abcdEFGH").toUpperCase() == String ("ABCDEFGH"));

			String s2 ("123");
			s2 << ((int) 4) << ((short) 5) << "678" << L"9" << '0';
			s2 += "xyz";
			expect (s2 == "1234567890xyz");

			beginTest ("Numeric conversions");
			expect (String::empty.getIntValue() == 0);
			expect (String::empty.getDoubleValue() == 0.0);
			expect (String::empty.getFloatValue() == 0.0f);
			expect (s.getIntValue() == 12345678);
			expect (s.getLargeIntValue() == (int64) 12345678);
			expect (s.getDoubleValue() == 12345678.0);
			expect (s.getFloatValue() == 12345678.0f);
			expect (String (-1234).getIntValue() == -1234);
			expect (String ((int64) -1234).getLargeIntValue() == -1234);
			expect (String (-1234.56).getDoubleValue() == -1234.56);
			expect (String (-1234.56f).getFloatValue() == -1234.56f);
			expect (("xyz" + s).getTrailingIntValue() == s.getIntValue());
			expect (s.getHexValue32() == 0x12345678);
			expect (s.getHexValue64() == (int64) 0x12345678);
			expect (String::toHexString (0x1234abcd).equalsIgnoreCase ("1234abcd"));
			expect (String::toHexString ((int64) 0x1234abcd).equalsIgnoreCase ("1234abcd"));
			expect (String::toHexString ((short) 0x12ab).equalsIgnoreCase ("12ab"));

			unsigned char data[] = { 1, 2, 3, 4, 0xa, 0xb, 0xc, 0xd };
			expect (String::toHexString (data, 8, 0).equalsIgnoreCase ("010203040a0b0c0d"));
			expect (String::toHexString (data, 8, 1).equalsIgnoreCase ("01 02 03 04 0a 0b 0c 0d"));
			expect (String::toHexString (data, 8, 2).equalsIgnoreCase ("0102 0304 0a0b 0c0d"));

			beginTest ("Subsections");
			String s3;
			s3 = "abcdeFGHIJ";
			expect (s3.equalsIgnoreCase ("ABCdeFGhiJ"));
			expect (s3.compareIgnoreCase (L"ABCdeFGhiJ") == 0);
			expect (s3.containsIgnoreCase (s3.substring (3)));
			expect (s3.indexOfAnyOf ("xyzf", 2, true) == 5);
			expect (s3.indexOfAnyOf (L"xyzf", 2, false) == -1);
			expect (s3.indexOfAnyOf ("xyzF", 2, false) == 5);
			expect (s3.containsAnyOf (L"zzzFs"));
			expect (s3.startsWith ("abcd"));
			expect (s3.startsWithIgnoreCase (L"abCD"));
			expect (s3.startsWith (String::empty));
			expect (s3.startsWithChar ('a'));
			expect (s3.endsWith (String ("HIJ")));
			expect (s3.endsWithIgnoreCase (L"Hij"));
			expect (s3.endsWith (String::empty));
			expect (s3.endsWithChar (L'J'));
			expect (s3.indexOf ("HIJ") == 7);
			expect (s3.indexOf (L"HIJK") == -1);
			expect (s3.indexOfIgnoreCase ("hij") == 7);
			expect (s3.indexOfIgnoreCase (L"hijk") == -1);

			String s4 (s3);
			s4.append (String ("xyz123"), 3);
			expect (s4 == s3 + "xyz");

			expect (String (1234) < String (1235));
			expect (String (1235) > String (1234));
			expect (String (1234) >= String (1234));
			expect (String (1234) <= String (1234));
			expect (String (1235) >= String (1234));
			expect (String (1234) <= String (1235));

			String s5 ("word word2 word3");
			expect (s5.containsWholeWord (String ("word2")));
			expect (s5.indexOfWholeWord ("word2") == 5);
			expect (s5.containsWholeWord (L"word"));
			expect (s5.containsWholeWord ("word3"));
			expect (s5.containsWholeWord (s5));
			expect (s5.containsWholeWordIgnoreCase (L"Word2"));
			expect (s5.indexOfWholeWordIgnoreCase ("Word2") == 5);
			expect (s5.containsWholeWordIgnoreCase (L"Word"));
			expect (s5.containsWholeWordIgnoreCase ("Word3"));
			expect (! s5.containsWholeWordIgnoreCase (L"Wordx"));
			expect (!s5.containsWholeWordIgnoreCase ("xWord2"));
			expect (s5.containsNonWhitespaceChars());
			expect (s5.containsOnly ("ordw23 "));
			expect (! String (" \n\r\t").containsNonWhitespaceChars());

			expect (s5.matchesWildcard (L"wor*", false));
			expect (s5.matchesWildcard ("wOr*", true));
			expect (s5.matchesWildcard (L"*word3", true));
			expect (s5.matchesWildcard ("*word?", true));
			expect (s5.matchesWildcard (L"Word*3", true));
			expect (! s5.matchesWildcard (L"*34", true));
			expect (String ("xx**y").matchesWildcard ("*y", true));
			expect (String ("xx**y").matchesWildcard ("x*y", true));
			expect (String ("xx**y").matchesWildcard ("xx*y", true));
			expect (String ("xx**y").matchesWildcard ("xx*", true));
			expect (String ("xx?y").matchesWildcard ("x??y", true));
			expect (String ("xx?y").matchesWildcard ("xx?y", true));
			expect (! String ("xx?y").matchesWildcard ("xx?y?", true));
			expect (String ("xx?y").matchesWildcard ("xx??", true));

			expectEquals (s5.fromFirstOccurrenceOf (String::empty, true, false), s5);
			expectEquals (s5.fromFirstOccurrenceOf ("xword2", true, false), s5.substring (100));
			expectEquals (s5.fromFirstOccurrenceOf (L"word2", true, false), s5.substring (5));
			expectEquals (s5.fromFirstOccurrenceOf ("Word2", true, true), s5.substring (5));
			expectEquals (s5.fromFirstOccurrenceOf ("word2", false, false), s5.getLastCharacters (6));
			expectEquals (s5.fromFirstOccurrenceOf (L"Word2", false, true), s5.getLastCharacters (6));

			expectEquals (s5.fromLastOccurrenceOf (String::empty, true, false), s5);
			expectEquals (s5.fromLastOccurrenceOf (L"wordx", true, false), s5);
			expectEquals (s5.fromLastOccurrenceOf ("word", true, false), s5.getLastCharacters (5));
			expectEquals (s5.fromLastOccurrenceOf (L"worD", true, true), s5.getLastCharacters (5));
			expectEquals (s5.fromLastOccurrenceOf ("word", false, false), s5.getLastCharacters (1));
			expectEquals (s5.fromLastOccurrenceOf (L"worD", false, true), s5.getLastCharacters (1));

			expect (s5.upToFirstOccurrenceOf (String::empty, true, false).isEmpty());
			expectEquals (s5.upToFirstOccurrenceOf ("word4", true, false), s5);
			expectEquals (s5.upToFirstOccurrenceOf (L"word2", true, false), s5.substring (0, 10));
			expectEquals (s5.upToFirstOccurrenceOf ("Word2", true, true), s5.substring (0, 10));
			expectEquals (s5.upToFirstOccurrenceOf (L"word2", false, false), s5.substring (0, 5));
			expectEquals (s5.upToFirstOccurrenceOf ("Word2", false, true), s5.substring (0, 5));

			expectEquals (s5.upToLastOccurrenceOf (String::empty, true, false), s5);
			expectEquals (s5.upToLastOccurrenceOf ("zword", true, false), s5);
			expectEquals (s5.upToLastOccurrenceOf ("word", true, false), s5.dropLastCharacters (1));
			expectEquals (s5.dropLastCharacters(1).upToLastOccurrenceOf ("word", true, false), s5.dropLastCharacters (1));
			expectEquals (s5.upToLastOccurrenceOf ("Word", true, true), s5.dropLastCharacters (1));
			expectEquals (s5.upToLastOccurrenceOf ("word", false, false), s5.dropLastCharacters (5));
			expectEquals (s5.upToLastOccurrenceOf ("Word", false, true), s5.dropLastCharacters (5));

			expectEquals (s5.replace ("word", L"xyz", false), String ("xyz xyz2 xyz3"));
			expect (s5.replace (L"Word", "xyz", true) == "xyz xyz2 xyz3");
			expect (s5.dropLastCharacters (1).replace ("Word", String ("xyz"), true) == L"xyz xyz2 xyz");
			expect (s5.replace ("Word", "", true) == " 2 3");
			expectEquals (s5.replace ("Word2", L"xyz", true), String ("word xyz word3"));
			expect (s5.replaceCharacter (L'w', 'x') != s5);
			expectEquals (s5.replaceCharacter ('w', L'x').replaceCharacter ('x', 'w'), s5);
			expect (s5.replaceCharacters ("wo", "xy") != s5);
			expectEquals (s5.replaceCharacters ("wo", "xy").replaceCharacters ("xy", L"wo"), s5);
			expectEquals (s5.retainCharacters ("1wordxya"), String ("wordwordword"));
			expect (s5.retainCharacters (String::empty).isEmpty());
			expect (s5.removeCharacters ("1wordxya") == " 2 3");
			expectEquals (s5.removeCharacters (String::empty), s5);
			expect (s5.initialSectionContainingOnly ("word") == L"word");
			expect (String ("word").initialSectionContainingOnly ("word") == L"word");
			expectEquals (s5.initialSectionNotContaining (String ("xyz ")), String ("word"));
			expectEquals (s5.initialSectionNotContaining (String (";[:'/")), s5);
			expect (! s5.isQuotedString());
			expect (s5.quoted().isQuotedString());
			expect (! s5.quoted().unquoted().isQuotedString());
			expect (! String ("x'").isQuotedString());
			expect (String ("'x").isQuotedString());

			String s6 (" \t xyz  \t\r\n");
			expectEquals (s6.trim(), String ("xyz"));
			expect (s6.trim().trim() == "xyz");
			expectEquals (s5.trim(), s5);
			expectEquals (s6.trimStart().trimEnd(), s6.trim());
			expectEquals (s6.trimStart().trimEnd(), s6.trimEnd().trimStart());
			expectEquals (s6.trimStart().trimStart().trimEnd().trimEnd(), s6.trimEnd().trimStart());
			expect (s6.trimStart() != s6.trimEnd());
			expectEquals (("\t\r\n " + s6 + "\t\n \r").trim(), s6.trim());
			expect (String::repeatedString ("xyz", 3) == L"xyzxyzxyz");
		}

		{
			beginTest ("UTF conversions");

			TestUTFConversion <CharPointer_UTF32>::test (*this);
			TestUTFConversion <CharPointer_UTF8>::test (*this);
			TestUTFConversion <CharPointer_UTF16>::test (*this);
		}

		{
			beginTest ("StringArray");

			StringArray s;
			s.addTokens ("4,3,2,1,0", ";,", "x");
			expectEquals (s.size(), 5);

			expectEquals (s.joinIntoString ("-"), String ("4-3-2-1-0"));
			s.remove (2);
			expectEquals (s.joinIntoString ("--"), String ("4--3--1--0"));
			expectEquals (s.joinIntoString (String::empty), String ("4310"));
			s.clear();
			expectEquals (s.joinIntoString ("x"), String::empty);

			StringArray toks;
			toks.addTokens ("x,,", ";,", "");
			expectEquals (toks.size(), 3);
			expectEquals (toks.joinIntoString ("-"), String ("x--"));
			toks.clear();

			toks.addTokens (",x,", ";,", "");
			expectEquals (toks.size(), 3);
			expectEquals (toks.joinIntoString ("-"), String ("-x-"));
			toks.clear();

			toks.addTokens ("x,'y,z',", ";,", "'");
			expectEquals (toks.size(), 3);
			expectEquals (toks.joinIntoString ("-"), String ("x-'y,z'-"));
		}
	}
};

static StringTests stringUnitTests;

#endif

/*** End of inlined file: juce_String.cpp ***/


/*** Start of inlined file: juce_StringArray.cpp ***/
StringArray::StringArray() noexcept
{
}

StringArray::StringArray (const StringArray& other)
	: strings (other.strings)
{
}

#if JUCE_COMPILER_SUPPORTS_MOVE_SEMANTICS
StringArray::StringArray (StringArray&& other) noexcept
	: strings (static_cast <Array <String>&&> (other.strings))
{
}
#endif

StringArray::StringArray (const String& firstValue)
{
	strings.add (firstValue);
}

namespace StringArrayHelpers
{
	template <typename CharType>
	void addArray (Array<String>& dest, const CharType* const* strings)
	{
		if (strings != nullptr)
			while (*strings != nullptr)
				dest.add (*strings++);
	}

	template <typename CharType>
	void addArray (Array<String>& dest, const CharType* const* const strings, const int numberOfStrings)
	{
		for (int i = 0; i < numberOfStrings; ++i)
			dest.add (strings [i]);
	}
}

StringArray::StringArray (const char* const* const initialStrings)
{
	StringArrayHelpers::addArray (strings, initialStrings);
}

StringArray::StringArray (const char* const* const initialStrings, const int numberOfStrings)
{
	StringArrayHelpers::addArray (strings, initialStrings, numberOfStrings);
}

StringArray::StringArray (const wchar_t* const* const initialStrings)
{
	StringArrayHelpers::addArray (strings, initialStrings);
}

StringArray::StringArray (const wchar_t* const* const initialStrings, const int numberOfStrings)
{
	StringArrayHelpers::addArray (strings, initialStrings, numberOfStrings);
}

StringArray& StringArray::operator= (const StringArray& other)
{
	strings = other.strings;
	return *this;
}

#if JUCE_COMPILER_SUPPORTS_MOVE_SEMANTICS
StringArray& StringArray::operator= (StringArray&& other) noexcept
{
	strings = static_cast <Array<String>&&> (other.strings);
	return *this;
}
#endif

StringArray::~StringArray()
{
}

bool StringArray::operator== (const StringArray& other) const noexcept
{
	if (other.size() != size())
		return false;

	for (int i = size(); --i >= 0;)
		if (other.strings.getReference(i) != strings.getReference(i))
			return false;

	return true;
}

bool StringArray::operator!= (const StringArray& other) const noexcept
{
	return ! operator== (other);
}

void StringArray::clear()
{
	strings.clear();
}

const String& StringArray::operator[] (const int index) const noexcept
{
	if (isPositiveAndBelow (index, strings.size()))
		return strings.getReference (index);

	return String::empty;
}

String& StringArray::getReference (const int index) noexcept
{
	jassert (isPositiveAndBelow (index, strings.size()));
	return strings.getReference (index);
}

void StringArray::add (const String& newString)
{
	strings.add (newString);
}

void StringArray::insert (const int index, const String& newString)
{
	strings.insert (index, newString);
}

void StringArray::addIfNotAlreadyThere (const String& newString, const bool ignoreCase)
{
	if (! contains (newString, ignoreCase))
		add (newString);
}

void StringArray::addArray (const StringArray& otherArray, int startIndex, int numElementsToAdd)
{
	if (startIndex < 0)
	{
		jassertfalse;
		startIndex = 0;
	}

	if (numElementsToAdd < 0 || startIndex + numElementsToAdd > otherArray.size())
		numElementsToAdd = otherArray.size() - startIndex;

	while (--numElementsToAdd >= 0)
		strings.add (otherArray.strings.getReference (startIndex++));
}

void StringArray::set (const int index, const String& newString)
{
	strings.set (index, newString);
}

bool StringArray::contains (const String& stringToLookFor, const bool ignoreCase) const
{
	if (ignoreCase)
	{
		for (int i = size(); --i >= 0;)
			if (strings.getReference(i).equalsIgnoreCase (stringToLookFor))
				return true;
	}
	else
	{
		for (int i = size(); --i >= 0;)
			if (stringToLookFor == strings.getReference(i))
				return true;
	}

	return false;
}

int StringArray::indexOf (const String& stringToLookFor, const bool ignoreCase, int i) const
{
	if (i < 0)
		i = 0;

	const int numElements = size();

	if (ignoreCase)
	{
		while (i < numElements)
		{
			if (strings.getReference(i).equalsIgnoreCase (stringToLookFor))
				return i;

			++i;
		}
	}
	else
	{
		while (i < numElements)
		{
			if (stringToLookFor == strings.getReference (i))
				return i;

			++i;
		}
	}

	return -1;
}

void StringArray::remove (const int index)
{
	strings.remove (index);
}

void StringArray::removeString (const String& stringToRemove,
								const bool ignoreCase)
{
	if (ignoreCase)
	{
		for (int i = size(); --i >= 0;)
			if (strings.getReference(i).equalsIgnoreCase (stringToRemove))
				strings.remove (i);
	}
	else
	{
		for (int i = size(); --i >= 0;)
			if (stringToRemove == strings.getReference (i))
				strings.remove (i);
	}
}

void StringArray::removeRange (int startIndex, int numberToRemove)
{
	strings.removeRange (startIndex, numberToRemove);
}

void StringArray::removeEmptyStrings (const bool removeWhitespaceStrings)
{
	if (removeWhitespaceStrings)
	{
		for (int i = size(); --i >= 0;)
			if (! strings.getReference(i).containsNonWhitespaceChars())
				strings.remove (i);
	}
	else
	{
		for (int i = size(); --i >= 0;)
			if (strings.getReference(i).isEmpty())
				strings.remove (i);
	}
}

void StringArray::trim()
{
	for (int i = size(); --i >= 0;)
	{
		String& s = strings.getReference(i);
		s = s.trim();
	}
}

struct InternalStringArrayComparator_CaseSensitive
{
	static int compareElements (String& first, String& second)      { return first.compare (second); }
};

struct InternalStringArrayComparator_CaseInsensitive
{
	static int compareElements (String& first, String& second)      { return first.compareIgnoreCase (second); }
};

void StringArray::sort (const bool ignoreCase)
{
	if (ignoreCase)
	{
		InternalStringArrayComparator_CaseInsensitive comp;
		strings.sort (comp);
	}
	else
	{
		InternalStringArrayComparator_CaseSensitive comp;
		strings.sort (comp);
	}
}

void StringArray::move (const int currentIndex, int newIndex) noexcept
{
	strings.move (currentIndex, newIndex);
}

String StringArray::joinIntoString (const String& separator, int start, int numberToJoin) const
{
	const int last = (numberToJoin < 0) ? size()
										: jmin (size(), start + numberToJoin);

	if (start < 0)
		start = 0;

	if (start >= last)
		return String::empty;

	if (start == last - 1)
		return strings.getReference (start);

	const size_t separatorBytes = separator.getCharPointer().sizeInBytes() - sizeof (String::CharPointerType::CharType);
	size_t bytesNeeded = separatorBytes * (last - start - 1);

	for (int i = start; i < last; ++i)
		bytesNeeded += strings.getReference(i).getCharPointer().sizeInBytes() - sizeof (String::CharPointerType::CharType);

	String result;
	result.preallocateBytes (bytesNeeded);

	String::CharPointerType dest (result.getCharPointer());

	while (start < last)
	{
		const String& s = strings.getReference (start);

		if (! s.isEmpty())
			dest.writeAll (s.getCharPointer());

		if (++start < last && separatorBytes > 0)
			dest.writeAll (separator.getCharPointer());
	}

	dest.writeNull();

	return result;
}

int StringArray::addTokens (const String& text, const bool preserveQuotedStrings)
{
	return addTokens (text, " \n\r\t", preserveQuotedStrings ? "\"" : "");
}

int StringArray::addTokens (const String& text, const String& breakCharacters, const String& quoteCharacters)
{
	int num = 0;
	String::CharPointerType t (text.getCharPointer());

	if (! t.isEmpty())
	{
		for (;;)
		{
			String::CharPointerType tokenEnd (CharacterFunctions::findEndOfToken (t,
																				  breakCharacters.getCharPointer(),
																				  quoteCharacters.getCharPointer()));
			add (String (t, tokenEnd));
			++num;

			if (tokenEnd.isEmpty())
				break;

			t = ++tokenEnd;
		}
	}

	return num;
}

int StringArray::addLines (const String& sourceText)
{
	int numLines = 0;
	String::CharPointerType text (sourceText.getCharPointer());
	bool finished = text.isEmpty();

	while (! finished)
	{
		String::CharPointerType startOfLine (text);
		size_t numChars = 0;

		for (;;)
		{
			const juce_wchar c = text.getAndAdvance();

			if (c == 0)
			{
				finished = true;
				break;
			}

			if (c == '\n')
				break;

			if (c == '\r')
			{
				if (*text == '\n')
					++text;

				break;
			}

			++numChars;
		}

		add (String (startOfLine, numChars));
		++numLines;
	}

	return numLines;
}

void StringArray::removeDuplicates (const bool ignoreCase)
{
	for (int i = 0; i < size() - 1; ++i)
	{
		const String s (strings.getReference(i));

		int nextIndex = i + 1;

		for (;;)
		{
			nextIndex = indexOf (s, ignoreCase, nextIndex);

			if (nextIndex < 0)
				break;

			strings.remove (nextIndex);
		}
	}
}

void StringArray::appendNumbersToDuplicates (const bool ignoreCase,
											 const bool appendNumberToFirstInstance,
											 CharPointer_UTF8 preNumberString,
											 CharPointer_UTF8 postNumberString)
{
	CharPointer_UTF8 defaultPre (" ("), defaultPost (")");

	if (preNumberString.getAddress() == nullptr)
		preNumberString = defaultPre;

	if (postNumberString.getAddress() == nullptr)
		postNumberString = defaultPost;

	for (int i = 0; i < size() - 1; ++i)
	{
		String& s = strings.getReference(i);

		int nextIndex = indexOf (s, ignoreCase, i + 1);

		if (nextIndex >= 0)
		{
			const String original (s);

			int number = 0;

			if (appendNumberToFirstInstance)
				s = original + String (preNumberString) + String (++number) + String (postNumberString);
			else
				++number;

			while (nextIndex >= 0)
			{
				set (nextIndex, (*this)[nextIndex] + String (preNumberString) + String (++number) + String (postNumberString));
				nextIndex = indexOf (original, ignoreCase, nextIndex + 1);
			}
		}
	}
}

void StringArray::minimiseStorageOverheads()
{
	strings.minimiseStorageOverheads();
}

/*** End of inlined file: juce_StringArray.cpp ***/


/*** Start of inlined file: juce_StringPairArray.cpp ***/
StringPairArray::StringPairArray (const bool ignoreCase_)
	: ignoreCase (ignoreCase_)
{
}

StringPairArray::StringPairArray (const StringPairArray& other)
	: keys (other.keys),
	  values (other.values),
	  ignoreCase (other.ignoreCase)
{
}

StringPairArray::~StringPairArray()
{
}

StringPairArray& StringPairArray::operator= (const StringPairArray& other)
{
	keys = other.keys;
	values = other.values;
	return *this;
}

bool StringPairArray::operator== (const StringPairArray& other) const
{
	for (int i = keys.size(); --i >= 0;)
		if (other [keys[i]] != values[i])
			return false;

	return true;
}

bool StringPairArray::operator!= (const StringPairArray& other) const
{
	return ! operator== (other);
}

const String& StringPairArray::operator[] (const String& key) const
{
	return values [keys.indexOf (key, ignoreCase)];
}

String StringPairArray::getValue (const String& key, const String& defaultReturnValue) const
{
	const int i = keys.indexOf (key, ignoreCase);

	if (i >= 0)
		return values[i];

	return defaultReturnValue;
}

void StringPairArray::set (const String& key, const String& value)
{
	const int i = keys.indexOf (key, ignoreCase);

	if (i >= 0)
	{
		values.set (i, value);
	}
	else
	{
		keys.add (key);
		values.add (value);
	}
}

void StringPairArray::addArray (const StringPairArray& other)
{
	for (int i = 0; i < other.size(); ++i)
		set (other.keys[i], other.values[i]);
}

void StringPairArray::clear()
{
	keys.clear();
	values.clear();
}

void StringPairArray::remove (const String& key)
{
	remove (keys.indexOf (key, ignoreCase));
}

void StringPairArray::remove (const int index)
{
	keys.remove (index);
	values.remove (index);
}

void StringPairArray::setIgnoresCase (const bool shouldIgnoreCase)
{
	ignoreCase = shouldIgnoreCase;
}

String StringPairArray::getDescription() const
{
	String s;

	for (int i = 0; i < keys.size(); ++i)
	{
		s << keys[i] << " = " << values[i];
		if (i < keys.size())
			s << ", ";
	}

	return s;
}

void StringPairArray::minimiseStorageOverheads()
{
	keys.minimiseStorageOverheads();
	values.minimiseStorageOverheads();
}

/*** End of inlined file: juce_StringPairArray.cpp ***/


/*** Start of inlined file: juce_StringPool.cpp ***/
StringPool::StringPool() noexcept   {}
StringPool::~StringPool()           {}

namespace StringPoolHelpers
{
	template <class StringType>
	String::CharPointerType getPooledStringFromArray (Array<String>& strings,
													  StringType newString,
													  const CriticalSection& lock)
	{
		const ScopedLock sl (lock);
		int start = 0;
		int end = strings.size();

		for (;;)
		{
			if (start >= end)
			{
				jassert (start <= end);
				strings.insert (start, newString);
				return strings.getReference (start).getCharPointer();
			}
			else
			{
				const String& startString = strings.getReference (start);

				if (startString == newString)
					return startString.getCharPointer();

				const int halfway = (start + end) >> 1;

				if (halfway == start)
				{
					if (startString.compare (newString) < 0)
						++start;

					strings.insert (start, newString);
					return strings.getReference (start).getCharPointer();
				}

				const int comp = strings.getReference (halfway).compare (newString);

				if (comp == 0)
					return strings.getReference (halfway).getCharPointer();
				else if (comp < 0)
					start = halfway;
				else
					end = halfway;
			}
		}
	}
}

String::CharPointerType StringPool::getPooledString (const String& s)
{
	if (s.isEmpty())
		return String::empty.getCharPointer();

	return StringPoolHelpers::getPooledStringFromArray (strings, s, lock);
}

String::CharPointerType StringPool::getPooledString (const char* const s)
{
	if (s == nullptr || *s == 0)
		return String::empty.getCharPointer();

	return StringPoolHelpers::getPooledStringFromArray (strings, s, lock);
}

String::CharPointerType StringPool::getPooledString (const wchar_t* const s)
{
	if (s == nullptr || *s == 0)
		return String::empty.getCharPointer();

	return StringPoolHelpers::getPooledStringFromArray (strings, s, lock);
}

int StringPool::size() const noexcept
{
	return strings.size();
}

String::CharPointerType StringPool::operator[] (const int index) const noexcept
{
	return strings [index].getCharPointer();
}

/*** End of inlined file: juce_StringPool.cpp ***/


/*** Start of inlined file: juce_ChildProcess.cpp ***/
ChildProcess::ChildProcess() {}
ChildProcess::~ChildProcess() {}

bool ChildProcess::waitForProcessToFinish (const int timeoutMs) const
{
	const int64 timeoutTime = Time::getMillisecondCounter() + timeoutMs;

	do
	{
		if (! isRunning())
			return true;
	}
	while (timeoutMs < 0 || Time::getMillisecondCounter() < timeoutTime);

	return false;
}

String ChildProcess::readAllProcessOutput()
{
	MemoryOutputStream result;

	for (;;)
	{
		char buffer [512];
		const int num = readProcessOutput (buffer, sizeof (buffer));

		if (num <= 0)
			break;

		result.write (buffer, num);
	}

	return result.toString();
}

#if JUCE_UNIT_TESTS

class ChildProcessTests  : public UnitTest
{
public:
	ChildProcessTests() : UnitTest ("ChildProcess") {}

	void runTest()
	{
		beginTest ("Child Processes");

	  #if JUCE_WINDOWS || JUCE_MAC || JUCE_LINUX
		ChildProcess p;

	   #if JUCE_WINDOWS
		expect (p.start ("tasklist"));
	   #else
		expect (p.start ("ls /"));
	   #endif

		//String output (p.readAllProcessOutput());
		//expect (output.isNotEmpty());
	  #endif
	}
};

static ChildProcessTests childProcessUnitTests;

#endif

/*** End of inlined file: juce_ChildProcess.cpp ***/


/*** Start of inlined file: juce_ReadWriteLock.cpp ***/
ReadWriteLock::ReadWriteLock() noexcept
	: numWaitingWriters (0),
	  numWriters (0),
	  writerThreadId (0)
{
	readerThreads.ensureStorageAllocated (16);
}

ReadWriteLock::~ReadWriteLock() noexcept
{
	jassert (readerThreads.size() == 0);
	jassert (numWriters == 0);
}

void ReadWriteLock::enterRead() const noexcept
{
	const Thread::ThreadID threadId = Thread::getCurrentThreadId();
	const SpinLock::ScopedLockType sl (accessLock);

	for (;;)
	{
		jassert (readerThreads.size() % 2 == 0);

		int i;
		for (i = 0; i < readerThreads.size(); i += 2)
			if (readerThreads.getUnchecked(i) == threadId)
				break;

		if (i < readerThreads.size()
			  || numWriters + numWaitingWriters == 0
			  || (threadId == writerThreadId && numWriters > 0))
		{
			if (i < readerThreads.size())
			{
				readerThreads.set (i + 1, (Thread::ThreadID) (1 + (pointer_sized_int) readerThreads.getUnchecked (i + 1)));
			}
			else
			{
				readerThreads.add (threadId);
				readerThreads.add ((Thread::ThreadID) 1);
			}

			return;
		}

		const SpinLock::ScopedUnlockType ul (accessLock);
		waitEvent.wait (100);
	}
}

void ReadWriteLock::exitRead() const noexcept
{
	const Thread::ThreadID threadId = Thread::getCurrentThreadId();
	const SpinLock::ScopedLockType sl (accessLock);

	for (int i = 0; i < readerThreads.size(); i += 2)
	{
		if (readerThreads.getUnchecked(i) == threadId)
		{
			const pointer_sized_int newCount = ((pointer_sized_int) readerThreads.getUnchecked (i + 1)) - 1;

			if (newCount == 0)
			{
				readerThreads.removeRange (i, 2);
				waitEvent.signal();
			}
			else
			{
				readerThreads.set (i + 1, (Thread::ThreadID) newCount);
			}

			return;
		}
	}

	jassertfalse; // unlocking a lock that wasn't locked..
}

void ReadWriteLock::enterWrite() const noexcept
{
	const Thread::ThreadID threadId = Thread::getCurrentThreadId();
	const SpinLock::ScopedLockType sl (accessLock);

	for (;;)
	{
		if (readerThreads.size() + numWriters == 0
			 || threadId == writerThreadId
			 || (readerThreads.size() == 2
				  && readerThreads.getUnchecked(0) == threadId))
		{
			writerThreadId = threadId;
			++numWriters;
			break;
		}

		++numWaitingWriters;
		accessLock.exit();
		waitEvent.wait (100);
		accessLock.enter();
		--numWaitingWriters;
	}
}

bool ReadWriteLock::tryEnterWrite() const noexcept
{
	const Thread::ThreadID threadId = Thread::getCurrentThreadId();
	const SpinLock::ScopedLockType sl (accessLock);

	if (readerThreads.size() + numWriters == 0
		 || threadId == writerThreadId
		 || (readerThreads.size() == 2
			  && readerThreads.getUnchecked(0) == threadId))
	{
		writerThreadId = threadId;
		++numWriters;
		return true;
	}

	return false;
}

void ReadWriteLock::exitWrite() const noexcept
{
	const SpinLock::ScopedLockType sl (accessLock);

	// check this thread actually had the lock..
	jassert (numWriters > 0 && writerThreadId == Thread::getCurrentThreadId());

	if (--numWriters == 0)
	{
		writerThreadId = 0;
		waitEvent.signal();
	}
}

/*** End of inlined file: juce_ReadWriteLock.cpp ***/


/*** Start of inlined file: juce_Thread.cpp ***/
Thread::Thread (const String& threadName_)
	: threadName (threadName_),
	  threadHandle (nullptr),
	  threadId (0),
	  threadPriority (5),
	  affinityMask (0),
	  shouldExit (false)
{
}

Thread::~Thread()
{
	/* If your thread class's destructor has been called without first stopping the thread, that
	   means that this partially destructed object is still performing some work - and that's
	   probably a Bad Thing!

	   To avoid this type of nastiness, always make sure you call stopThread() before or during
	   your subclass's destructor.
	*/
	jassert (! isThreadRunning());

	stopThread (100);
}

// Use a ref-counted object to hold this shared data, so that it can outlive its static
// shared pointer when threads are still running during static shutdown.
struct CurrentThreadHolder   : public ReferenceCountedObject
{
	CurrentThreadHolder() noexcept {}

	typedef ReferenceCountedObjectPtr <CurrentThreadHolder> Ptr;
	ThreadLocalValue<Thread*> value;

	JUCE_DECLARE_NON_COPYABLE (CurrentThreadHolder);
};

static char currentThreadHolderLock [sizeof (SpinLock)]; // (statically initialised to zeros).

static CurrentThreadHolder::Ptr getCurrentThreadHolder()
{
	static CurrentThreadHolder::Ptr currentThreadHolder;
	SpinLock::ScopedLockType lock (*reinterpret_cast <SpinLock*> (currentThreadHolderLock));

	if (currentThreadHolder == nullptr)
		currentThreadHolder = new CurrentThreadHolder();

	return currentThreadHolder;
}

void Thread::threadEntryPoint()
{
	const CurrentThreadHolder::Ptr currentThreadHolder (getCurrentThreadHolder());
	currentThreadHolder->value = this;

	JUCE_TRY
	{
		if (threadName.isNotEmpty())
			setCurrentThreadName (threadName);

		if (startSuspensionEvent.wait (10000))
		{
			jassert (getCurrentThreadId() == threadId);

			if (affinityMask != 0)
				setCurrentThreadAffinityMask (affinityMask);

			run();
		}
	}
	JUCE_CATCH_ALL_ASSERT

	currentThreadHolder->value.releaseCurrentThreadStorage();
	closeThreadHandle();
}

// used to wrap the incoming call from the platform-specific code
void JUCE_API juce_threadEntryPoint (void* userData)
{
	static_cast <Thread*> (userData)->threadEntryPoint();
}

void Thread::startThread()
{
	const ScopedLock sl (startStopLock);

	shouldExit = false;

	if (threadHandle == nullptr)
	{
		launchThread();
		setThreadPriority (threadHandle, threadPriority);
		startSuspensionEvent.signal();
	}
}

void Thread::startThread (const int priority)
{
	const ScopedLock sl (startStopLock);

	if (threadHandle == nullptr)
	{
		threadPriority = priority;
		startThread();
	}
	else
	{
		setPriority (priority);
	}
}

bool Thread::isThreadRunning() const
{
	return threadHandle != nullptr;
}

Thread* Thread::getCurrentThread()
{
	return getCurrentThreadHolder()->value.get();
}

void Thread::signalThreadShouldExit()
{
	shouldExit = true;
}

bool Thread::waitForThreadToExit (const int timeOutMilliseconds) const
{
	// Doh! So how exactly do you expect this thread to wait for itself to stop??
	jassert (getThreadId() != getCurrentThreadId() || getCurrentThreadId() == 0);

	const int sleepMsPerIteration = 5;
	int count = timeOutMilliseconds / sleepMsPerIteration;

	while (isThreadRunning())
	{
		if (timeOutMilliseconds >= 0 && --count < 0)
			return false;

		sleep (sleepMsPerIteration);
	}

	return true;
}

void Thread::stopThread (const int timeOutMilliseconds)
{
	// agh! You can't stop the thread that's calling this method! How on earth
	// would that work??
	jassert (getCurrentThreadId() != getThreadId());

	const ScopedLock sl (startStopLock);

	if (isThreadRunning())
	{
		signalThreadShouldExit();
		notify();

		if (timeOutMilliseconds != 0)
			waitForThreadToExit (timeOutMilliseconds);

		if (isThreadRunning())
		{
			// very bad karma if this point is reached, as there are bound to be
			// locks and events left in silly states when a thread is killed by force..
			jassertfalse;
			Logger::writeToLog ("!! killing thread by force !!");

			killThread();

			threadHandle = nullptr;
			threadId = 0;
		}
	}
}

bool Thread::setPriority (const int newPriority)
{
	const ScopedLock sl (startStopLock);

	if (setThreadPriority (threadHandle, newPriority))
	{
		threadPriority = newPriority;
		return true;
	}

	return false;
}

bool Thread::setCurrentThreadPriority (const int newPriority)
{
	return setThreadPriority (0, newPriority);
}

void Thread::setAffinityMask (const uint32 newAffinityMask)
{
	affinityMask = newAffinityMask;
}

bool Thread::wait (const int timeOutMilliseconds) const
{
	return defaultEvent.wait (timeOutMilliseconds);
}

void Thread::notify() const
{
	defaultEvent.signal();
}

void SpinLock::enter() const noexcept
{
	if (! tryEnter())
	{
		for (int i = 20; --i >= 0;)
			if (tryEnter())
				return;

		while (! tryEnter())
			Thread::yield();
	}
}

#if JUCE_UNIT_TESTS

class AtomicTests  : public UnitTest
{
public:
	AtomicTests() : UnitTest ("Atomics") {}

	void runTest()
	{
		beginTest ("Misc");

		char a1[7];
		expect (numElementsInArray(a1) == 7);
		int a2[3];
		expect (numElementsInArray(a2) == 3);

		expect (ByteOrder::swap ((uint16) 0x1122) == 0x2211);
		expect (ByteOrder::swap ((uint32) 0x11223344) == 0x44332211);
		expect (ByteOrder::swap ((uint64) literal64bit (0x1122334455667788)) == literal64bit (0x8877665544332211));

		beginTest ("Atomic int");
		AtomicTester <int>::testInteger (*this);
		beginTest ("Atomic unsigned int");
		AtomicTester <unsigned int>::testInteger (*this);
		beginTest ("Atomic int32");
		AtomicTester <int32>::testInteger (*this);
		beginTest ("Atomic uint32");
		AtomicTester <uint32>::testInteger (*this);
		beginTest ("Atomic long");
		AtomicTester <long>::testInteger (*this);
		beginTest ("Atomic void*");
		AtomicTester <void*>::testInteger (*this);
		beginTest ("Atomic int*");
		AtomicTester <int*>::testInteger (*this);
		beginTest ("Atomic float");
		AtomicTester <float>::testFloat (*this);
	  #if ! JUCE_64BIT_ATOMICS_UNAVAILABLE  // 64-bit intrinsics aren't available on some old platforms
		beginTest ("Atomic int64");
		AtomicTester <int64>::testInteger (*this);
		beginTest ("Atomic uint64");
		AtomicTester <uint64>::testInteger (*this);
		beginTest ("Atomic double");
		AtomicTester <double>::testFloat (*this);
	  #endif
	}

	template <typename Type>
	class AtomicTester
	{
	public:
		AtomicTester() {}

		static void testInteger (UnitTest& test)
		{
			Atomic<Type> a, b;
			a.set ((Type) 10);
			test.expect (a.value == (Type) 10);
			test.expect (a.get() == (Type) 10);
			a += (Type) 15;
			test.expect (a.get() == (Type) 25);
			a.memoryBarrier();
			a -= (Type) 5;
			test.expect (a.get() == (Type) 20);
			test.expect (++a == (Type) 21);
			++a;
			test.expect (--a == (Type) 21);
			test.expect (a.get() == (Type) 21);
			a.memoryBarrier();

			testFloat (test);
		}

		static void testFloat (UnitTest& test)
		{
			Atomic<Type> a, b;
			a = (Type) 21;
			a.memoryBarrier();

			/*  These are some simple test cases to check the atomics - let me know
				if any of these assertions fail on your system!
			*/
			test.expect (a.get() == (Type) 21);
			test.expect (a.compareAndSetValue ((Type) 100, (Type) 50) == (Type) 21);
			test.expect (a.get() == (Type) 21);
			test.expect (a.compareAndSetValue ((Type) 101, a.get()) == (Type) 21);
			test.expect (a.get() == (Type) 101);
			test.expect (! a.compareAndSetBool ((Type) 300, (Type) 200));
			test.expect (a.get() == (Type) 101);
			test.expect (a.compareAndSetBool ((Type) 200, a.get()));
			test.expect (a.get() == (Type) 200);

			test.expect (a.exchange ((Type) 300) == (Type) 200);
			test.expect (a.get() == (Type) 300);

			b = a;
			test.expect (b.get() == a.get());
		}
	};
};

static AtomicTests atomicUnitTests;

#endif

/*** End of inlined file: juce_Thread.cpp ***/


/*** Start of inlined file: juce_ThreadPool.cpp ***/
ThreadPoolJob::ThreadPoolJob (const String& name)
	: jobName (name),
	  pool (nullptr),
	  shouldStop (false),
	  isActive (false),
	  shouldBeDeleted (false)
{
}

ThreadPoolJob::~ThreadPoolJob()
{
	// you mustn't delete a job while it's still in a pool! Use ThreadPool::removeJob()
	// to remove it first!
	jassert (pool == nullptr || ! pool->contains (this));
}

String ThreadPoolJob::getJobName() const
{
	return jobName;
}

void ThreadPoolJob::setJobName (const String& newName)
{
	jobName = newName;
}

void ThreadPoolJob::signalJobShouldExit()
{
	shouldStop = true;
}

class ThreadPool::ThreadPoolThread  : public Thread
{
public:
	ThreadPoolThread (ThreadPool& pool_)
		: Thread ("Pool"),
		  pool (pool_)
	{
	}

	void run()
	{
		while (! threadShouldExit())
		{
			if (! pool.runNextJob())
				wait (500);
		}
	}

private:
	ThreadPool& pool;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ThreadPoolThread);
};

ThreadPool::ThreadPool (const int numThreads)
{
	jassert (numThreads > 0); // not much point having a pool without any threads!

	createThreads (numThreads);
}

ThreadPool::ThreadPool()
{
	createThreads (SystemStats::getNumCpus());
}

ThreadPool::~ThreadPool()
{
	removeAllJobs (true, 5000);
	stopThreads();
}

void ThreadPool::createThreads (int numThreads)
{
	for (int i = jmax (1, numThreads); --i >= 0;)
		threads.add (new ThreadPoolThread (*this));

	for (int i = threads.size(); --i >= 0;)
		threads.getUnchecked(i)->startThread();
}

void ThreadPool::stopThreads()
{
	for (int i = threads.size(); --i >= 0;)
		threads.getUnchecked(i)->signalThreadShouldExit();

	for (int i = threads.size(); --i >= 0;)
		threads.getUnchecked(i)->stopThread (500);
}

void ThreadPool::addJob (ThreadPoolJob* const job, const bool deleteJobWhenFinished)
{
	jassert (job != nullptr);
	jassert (job->pool == nullptr);

	if (job->pool == nullptr)
	{
		job->pool = this;
		job->shouldStop = false;
		job->isActive = false;
		job->shouldBeDeleted = deleteJobWhenFinished;

		{
			const ScopedLock sl (lock);
			jobs.add (job);
		}

		for (int i = threads.size(); --i >= 0;)
			threads.getUnchecked(i)->notify();
	}
}

int ThreadPool::getNumJobs() const
{
	return jobs.size();
}

ThreadPoolJob* ThreadPool::getJob (const int index) const
{
	const ScopedLock sl (lock);
	return jobs [index];
}

bool ThreadPool::contains (const ThreadPoolJob* const job) const
{
	const ScopedLock sl (lock);
	return jobs.contains (const_cast <ThreadPoolJob*> (job));
}

bool ThreadPool::isJobRunning (const ThreadPoolJob* const job) const
{
	const ScopedLock sl (lock);
	return jobs.contains (const_cast <ThreadPoolJob*> (job)) && job->isActive;
}

bool ThreadPool::waitForJobToFinish (const ThreadPoolJob* const job,
									 const int timeOutMs) const
{
	if (job != nullptr)
	{
		const uint32 start = Time::getMillisecondCounter();

		while (contains (job))
		{
			if (timeOutMs >= 0 && Time::getMillisecondCounter() >= start + timeOutMs)
				return false;

			jobFinishedSignal.wait (2);
		}
	}

	return true;
}

bool ThreadPool::removeJob (ThreadPoolJob* const job,
							const bool interruptIfRunning,
							const int timeOutMs)
{
	bool dontWait = true;
	OwnedArray<ThreadPoolJob> deletionList;

	if (job != nullptr)
	{
		const ScopedLock sl (lock);

		if (jobs.contains (job))
		{
			if (job->isActive)
			{
				if (interruptIfRunning)
					job->signalJobShouldExit();

				dontWait = false;
			}
			else
			{
				jobs.removeValue (job);
				addToDeleteList (deletionList, job);
			}
		}
	}

	return dontWait || waitForJobToFinish (job, timeOutMs);
}

bool ThreadPool::removeAllJobs (const bool interruptRunningJobs, const int timeOutMs,
								ThreadPool::JobSelector* selectedJobsToRemove)
{
	Array <ThreadPoolJob*> jobsToWaitFor;

	{
		OwnedArray<ThreadPoolJob> deletionList;

		{
			const ScopedLock sl (lock);

			for (int i = jobs.size(); --i >= 0;)
			{
				ThreadPoolJob* const job = jobs.getUnchecked(i);

				if (selectedJobsToRemove == nullptr || selectedJobsToRemove->isJobSuitable (job))
				{
					if (job->isActive)
					{
						jobsToWaitFor.add (job);

						if (interruptRunningJobs)
							job->signalJobShouldExit();
					}
					else
					{
						jobs.remove (i);
						addToDeleteList (deletionList, job);
					}
				}
			}
		}
	}

	const uint32 start = Time::getMillisecondCounter();

	for (;;)
	{
		for (int i = jobsToWaitFor.size(); --i >= 0;)
		{
			ThreadPoolJob* const job = jobsToWaitFor.getUnchecked (i);

			if (! isJobRunning (job))
				jobsToWaitFor.remove (i);
		}

		if (jobsToWaitFor.size() == 0)
			break;

		if (timeOutMs >= 0 && Time::getMillisecondCounter() >= start + timeOutMs)
			return false;

		jobFinishedSignal.wait (20);
	}

	return true;
}

StringArray ThreadPool::getNamesOfAllJobs (const bool onlyReturnActiveJobs) const
{
	StringArray s;
	const ScopedLock sl (lock);

	for (int i = 0; i < jobs.size(); ++i)
	{
		const ThreadPoolJob* const job = jobs.getUnchecked(i);
		if (job->isActive || ! onlyReturnActiveJobs)
			s.add (job->getJobName());
	}

	return s;
}

bool ThreadPool::setThreadPriorities (const int newPriority)
{
	bool ok = true;

	for (int i = threads.size(); --i >= 0;)
		if (! threads.getUnchecked(i)->setPriority (newPriority))
			ok = false;

	return ok;
}

ThreadPoolJob* ThreadPool::pickNextJobToRun()
{
	OwnedArray<ThreadPoolJob> deletionList;

	{
		const ScopedLock sl (lock);

		for (int i = 0; i < jobs.size(); ++i)
		{
			ThreadPoolJob* job = jobs[i];

			if (job != nullptr && ! job->isActive)
			{
				if (job->shouldStop)
				{
					jobs.remove (i);
					addToDeleteList (deletionList, job);
					--i;
					continue;
				}

				job->isActive = true;
				return job;
			}
		}
	}

	return nullptr;
}

bool ThreadPool::runNextJob()
{
	ThreadPoolJob* const job = pickNextJobToRun();

	if (job == nullptr)
		return false;

	ThreadPoolJob::JobStatus result = ThreadPoolJob::jobHasFinished;

	JUCE_TRY
	{
		result = job->runJob();
	}
	JUCE_CATCH_ALL_ASSERT

	OwnedArray<ThreadPoolJob> deletionList;

	{
		const ScopedLock sl (lock);

		if (jobs.contains (job))
		{
			job->isActive = false;

			if (result != ThreadPoolJob::jobNeedsRunningAgain || job->shouldStop)
			{
				jobs.removeValue (job);
				addToDeleteList (deletionList, job);

				jobFinishedSignal.signal();
			}
			else
			{
				// move the job to the end of the queue if it wants another go
				jobs.move (jobs.indexOf (job), -1);
			}
		}
	}

	return true;
}

void ThreadPool::addToDeleteList (OwnedArray<ThreadPoolJob>& deletionList, ThreadPoolJob* const job) const
{
	job->shouldStop = true;
	job->pool = nullptr;

	if (job->shouldBeDeleted)
		deletionList.add (job);
}

/*** End of inlined file: juce_ThreadPool.cpp ***/


/*** Start of inlined file: juce_TimeSliceThread.cpp ***/
TimeSliceThread::TimeSliceThread (const String& threadName)
	: Thread (threadName),
	  clientBeingCalled (nullptr)
{
}

TimeSliceThread::~TimeSliceThread()
{
	stopThread (2000);
}

void TimeSliceThread::addTimeSliceClient (TimeSliceClient* const client, int millisecondsBeforeStarting)
{
	if (client != nullptr)
	{
		const ScopedLock sl (listLock);
		client->nextCallTime = Time::getCurrentTime() + RelativeTime::milliseconds (millisecondsBeforeStarting);
		clients.addIfNotAlreadyThere (client);
		notify();
	}
}

void TimeSliceThread::removeTimeSliceClient (TimeSliceClient* const client)
{
	const ScopedLock sl1 (listLock);

	// if there's a chance we're in the middle of calling this client, we need to
	// also lock the outer lock..
	if (clientBeingCalled == client)
	{
		const ScopedUnlock ul (listLock); // unlock first to get the order right..

		const ScopedLock sl2 (callbackLock);
		const ScopedLock sl3 (listLock);

		clients.removeValue (client);
	}
	else
	{
		clients.removeValue (client);
	}
}

void TimeSliceThread::moveToFrontOfQueue (TimeSliceClient* client)
{
	const ScopedLock sl (listLock);

	if (clients.contains (client))
	{
		client->nextCallTime = Time::getCurrentTime();
		notify();
	}
}

int TimeSliceThread::getNumClients() const
{
	return clients.size();
}

TimeSliceClient* TimeSliceThread::getClient (const int i) const
{
	const ScopedLock sl (listLock);
	return clients [i];
}

TimeSliceClient* TimeSliceThread::getNextClient (int index) const
{
	Time soonest;
	TimeSliceClient* client = nullptr;

	for (int i = clients.size(); --i >= 0;)
	{
		TimeSliceClient* const c = clients.getUnchecked ((i + index) % clients.size());

		if (client == nullptr || c->nextCallTime < soonest)
		{
			client = c;
			soonest = c->nextCallTime;
		}
	}

	return client;
}

void TimeSliceThread::run()
{
	int index = 0;

	while (! threadShouldExit())
	{
		int timeToWait = 500;

		{
			Time nextClientTime;

			{
				const ScopedLock sl2 (listLock);

				index = clients.size() > 0 ? ((index + 1) % clients.size()) : 0;

				TimeSliceClient* const firstClient = getNextClient (index);
				if (firstClient != nullptr)
					nextClientTime = firstClient->nextCallTime;
			}

			const Time now (Time::getCurrentTime());

			if (nextClientTime > now)
			{
				timeToWait = (int) jmin ((int64) 500, (nextClientTime - now).inMilliseconds());
			}
			else
			{
				timeToWait = index == 0 ? 1 : 0;

				const ScopedLock sl (callbackLock);

				{
					const ScopedLock sl2 (listLock);
					clientBeingCalled = getNextClient (index);
				}

				if (clientBeingCalled != nullptr)
				{
					const int msUntilNextCall = clientBeingCalled->useTimeSlice();

					const ScopedLock sl2 (listLock);

					if (msUntilNextCall >= 0)
						clientBeingCalled->nextCallTime += RelativeTime::milliseconds (msUntilNextCall);
					else
						clients.removeValue (clientBeingCalled);

					clientBeingCalled = nullptr;
				}
			}
		}

		if (timeToWait > 0)
			wait (timeToWait);
	}
}

/*** End of inlined file: juce_TimeSliceThread.cpp ***/


/*** Start of inlined file: juce_PerformanceCounter.cpp ***/
PerformanceCounter::PerformanceCounter (const String& name_,
										const int runsPerPrintout,
										const File& loggingFile)
	: name (name_),
	  numRuns (0),
	  runsPerPrint (runsPerPrintout),
	  totalTime (0),
	  outputFile (loggingFile)
{
	if (outputFile != File::nonexistent)
	{
		String s ("**** Counter for \"");
		s << name_ << "\" started at: "
		  << Time::getCurrentTime().toString (true, true)
		  << newLine;

		outputFile.appendText (s, false, false);
	}
}

PerformanceCounter::~PerformanceCounter()
{
	printStatistics();
}

void PerformanceCounter::start()
{
	started = Time::getHighResolutionTicks();
}

void PerformanceCounter::stop()
{
	const int64 now = Time::getHighResolutionTicks();

	totalTime += 1000.0 * Time::highResolutionTicksToSeconds (now - started);

	if (++numRuns == runsPerPrint)
		printStatistics();
}

void PerformanceCounter::printStatistics()
{
	if (numRuns > 0)
	{
		String s ("Performance count for \"");
		s << name << "\" - average over " << numRuns << " run(s) = ";

		const int micros = (int) (totalTime * (1000.0 / numRuns));

		if (micros > 10000)
			s << (micros/1000) << " millisecs";
		else
			s << micros << " microsecs";

		s << ", total = " << String (totalTime / 1000, 5) << " seconds";

		Logger::outputDebugString (s);

		s << newLine;

		if (outputFile != File::nonexistent)
			outputFile.appendText (s, false, false);

		numRuns = 0;
		totalTime = 0;
	}
}

/*** End of inlined file: juce_PerformanceCounter.cpp ***/


/*** Start of inlined file: juce_RelativeTime.cpp ***/
RelativeTime::RelativeTime (const double seconds_) noexcept
	: seconds (seconds_)
{
}

RelativeTime::RelativeTime (const RelativeTime& other) noexcept
	: seconds (other.seconds)
{
}

RelativeTime::~RelativeTime() noexcept
{
}

const RelativeTime RelativeTime::milliseconds (const int milliseconds) noexcept   { return RelativeTime (milliseconds * 0.001); }
const RelativeTime RelativeTime::milliseconds (const int64 milliseconds) noexcept { return RelativeTime (milliseconds * 0.001); }
const RelativeTime RelativeTime::minutes (const double numberOfMinutes) noexcept  { return RelativeTime (numberOfMinutes * 60.0); }
const RelativeTime RelativeTime::hours (const double numberOfHours) noexcept      { return RelativeTime (numberOfHours * (60.0 * 60.0)); }
const RelativeTime RelativeTime::days (const double numberOfDays) noexcept        { return RelativeTime (numberOfDays  * (60.0 * 60.0 * 24.0)); }
const RelativeTime RelativeTime::weeks (const double numberOfWeeks) noexcept      { return RelativeTime (numberOfWeeks * (60.0 * 60.0 * 24.0 * 7.0)); }

int64 RelativeTime::inMilliseconds() const noexcept { return (int64) (seconds * 1000.0); }
double RelativeTime::inMinutes() const noexcept     { return seconds / 60.0; }
double RelativeTime::inHours() const noexcept       { return seconds / (60.0 * 60.0); }
double RelativeTime::inDays() const noexcept        { return seconds / (60.0 * 60.0 * 24.0); }
double RelativeTime::inWeeks() const noexcept       { return seconds / (60.0 * 60.0 * 24.0 * 7.0); }

String RelativeTime::getDescription (const String& returnValueForZeroTime) const
{
	if (seconds < 0.001 && seconds > -0.001)
		return returnValueForZeroTime;

	String result;
	result.preallocateBytes (32);

	if (seconds < 0)
		result << '-';

	int fieldsShown = 0;
	int n = std::abs ((int) inWeeks());
	if (n > 0)
	{
		result << n << (n == 1 ? TRANS(" week ")
							   : TRANS(" weeks "));
		++fieldsShown;
	}

	n = std::abs ((int) inDays()) % 7;
	if (n > 0)
	{
		result << n << (n == 1 ? TRANS(" day ")
							   : TRANS(" days "));
		++fieldsShown;
	}

	if (fieldsShown < 2)
	{
		n = std::abs ((int) inHours()) % 24;
		if (n > 0)
		{
			result << n << (n == 1 ? TRANS(" hr ")
								   : TRANS(" hrs "));
			++fieldsShown;
		}

		if (fieldsShown < 2)
		{
			n = std::abs ((int) inMinutes()) % 60;
			if (n > 0)
			{
				result << n << (n == 1 ? TRANS(" min ")
									   : TRANS(" mins "));
				++fieldsShown;
			}

			if (fieldsShown < 2)
			{
				n = std::abs ((int) inSeconds()) % 60;
				if (n > 0)
				{
					result << n << (n == 1 ? TRANS(" sec ")
										   : TRANS(" secs "));
					++fieldsShown;
				}

				if (fieldsShown == 0)
				{
					n = std::abs ((int) inMilliseconds()) % 1000;
					if (n > 0)
						result << n << TRANS(" ms");
				}
			}
		}
	}

	return result.trimEnd();
}

RelativeTime& RelativeTime::operator= (const RelativeTime& other) noexcept
{
	seconds = other.seconds;
	return *this;
}

const RelativeTime& RelativeTime::operator+= (const RelativeTime& timeToAdd) noexcept
{
	seconds += timeToAdd.seconds;
	return *this;
}

const RelativeTime& RelativeTime::operator-= (const RelativeTime& timeToSubtract) noexcept
{
	seconds -= timeToSubtract.seconds;
	return *this;
}

const RelativeTime& RelativeTime::operator+= (const double secondsToAdd) noexcept
{
	seconds += secondsToAdd;
	return *this;
}

const RelativeTime& RelativeTime::operator-= (const double secondsToSubtract) noexcept
{
	seconds -= secondsToSubtract;
	return *this;
}

bool operator== (const RelativeTime& t1, const RelativeTime& t2) noexcept { return t1.inSeconds() == t2.inSeconds(); }
bool operator!= (const RelativeTime& t1, const RelativeTime& t2) noexcept { return t1.inSeconds() != t2.inSeconds(); }
bool operator>  (const RelativeTime& t1, const RelativeTime& t2) noexcept { return t1.inSeconds() >  t2.inSeconds(); }
bool operator<  (const RelativeTime& t1, const RelativeTime& t2) noexcept { return t1.inSeconds() <  t2.inSeconds(); }
bool operator>= (const RelativeTime& t1, const RelativeTime& t2) noexcept { return t1.inSeconds() >= t2.inSeconds(); }
bool operator<= (const RelativeTime& t1, const RelativeTime& t2) noexcept { return t1.inSeconds() <= t2.inSeconds(); }

RelativeTime operator+ (const RelativeTime&  t1, const RelativeTime& t2) noexcept   { RelativeTime t (t1); return t += t2; }
RelativeTime operator- (const RelativeTime&  t1, const RelativeTime& t2) noexcept   { RelativeTime t (t1); return t -= t2; }

/*** End of inlined file: juce_RelativeTime.cpp ***/


/*** Start of inlined file: juce_Time.cpp ***/
#if JUCE_MSVC
 #pragma warning (push)
 #pragma warning (disable: 4514)
#endif

#if JUCE_MSVC
 #pragma warning (pop)

 #ifdef _INC_TIME_INL
  #define USE_NEW_SECURE_TIME_FNS
 #endif
#endif

namespace TimeHelpers
{
	static struct tm millisToLocal (const int64 millis) noexcept
	{
		struct tm result;
		const int64 seconds = millis / 1000;

		if (seconds < literal64bit (86400) || seconds >= literal64bit (2145916800))
		{
			// use extended maths for dates beyond 1970 to 2037..
			const int timeZoneAdjustment = 31536000 - (int) (Time (1971, 0, 1, 0, 0).toMilliseconds() / 1000);
			const int64 jdm = seconds + timeZoneAdjustment + literal64bit (210866803200);

			const int days = (int) (jdm / literal64bit (86400));
			const int a = 32044 + days;
			const int b = (4 * a + 3) / 146097;
			const int c = a - (b * 146097) / 4;
			const int d = (4 * c + 3) / 1461;
			const int e = c - (d * 1461) / 4;
			const int m = (5 * e + 2) / 153;

			result.tm_mday  = e - (153 * m + 2) / 5 + 1;
			result.tm_mon   = m + 2 - 12 * (m / 10);
			result.tm_year  = b * 100 + d - 6700 + (m / 10);
			result.tm_wday  = (days + 1) % 7;
			result.tm_yday  = -1;

			int t = (int) (jdm % literal64bit (86400));
			result.tm_hour  = t / 3600;
			t %= 3600;
			result.tm_min   = t / 60;
			result.tm_sec   = t % 60;
			result.tm_isdst = -1;
		}
		else
		{
			time_t now = static_cast <time_t> (seconds);

		  #if JUCE_WINDOWS
		   #ifdef USE_NEW_SECURE_TIME_FNS
			if (now >= 0 && now <= 0x793406fff)
				localtime_s (&result, &now);
			else
				zerostruct (result);
		   #else
			result = *localtime (&now);
		   #endif
		  #else

			localtime_r (&now, &result); // more thread-safe
		  #endif
		}

		return result;
	}

	static int extendedModulo (const int64 value, const int modulo) noexcept
	{
		return (int) (value >= 0 ? (value % modulo)
								 : (value - ((value / modulo) + 1) * modulo));
	}

	static int doFTime (CharPointer_UTF32 dest, const size_t maxChars,
						const String& format, const struct tm* const tm) noexcept
	{
	   #if JUCE_ANDROID
		HeapBlock <char> tempDest;
		tempDest.calloc (maxChars + 2);
		const int result = (int) strftime (tempDest, maxChars, format.toUTF8(), tm);
		if (result > 0)
			dest.writeAll (CharPointer_UTF8 (tempDest.getData()));
		return result;
	   #elif JUCE_WINDOWS
		HeapBlock <wchar_t> tempDest;
		tempDest.calloc (maxChars + 2);
		const int result = (int) wcsftime (tempDest, maxChars, format.toWideCharPointer(), tm);
		if (result > 0)
			dest.writeAll (CharPointer_UTF16 (tempDest.getData()));
		return result;
	   #else
		return (int) wcsftime (dest.getAddress(), maxChars, format.toUTF32(), tm);
	   #endif
	}

	static uint32 lastMSCounterValue = 0;
}

Time::Time() noexcept
	: millisSinceEpoch (0)
{
}

Time::Time (const Time& other) noexcept
	: millisSinceEpoch (other.millisSinceEpoch)
{
}

Time::Time (const int64 ms) noexcept
	: millisSinceEpoch (ms)
{
}

Time::Time (const int year,
			const int month,
			const int day,
			const int hours,
			const int minutes,
			const int seconds,
			const int milliseconds,
			const bool useLocalTime) noexcept
{
	jassert (year > 100); // year must be a 4-digit version

	if (year < 1971 || year >= 2038 || ! useLocalTime)
	{
		// use extended maths for dates beyond 1970 to 2037..
		const int timeZoneAdjustment = useLocalTime ? (31536000 - (int) (Time (1971, 0, 1, 0, 0).toMilliseconds() / 1000))
													: 0;
		const int a = (13 - month) / 12;
		const int y = year + 4800 - a;
		const int jd = day + (153 * (month + 12 * a - 2) + 2) / 5
						   + (y * 365) + (y /  4) - (y / 100) + (y / 400)
						   - 32045;

		const int64 s = ((int64) jd) * literal64bit (86400) - literal64bit (210866803200);

		millisSinceEpoch = 1000 * (s + (hours * 3600 + minutes * 60 + seconds - timeZoneAdjustment))
							 + milliseconds;
	}
	else
	{
		struct tm t;
		t.tm_year   = year - 1900;
		t.tm_mon    = month;
		t.tm_mday   = day;
		t.tm_hour   = hours;
		t.tm_min    = minutes;
		t.tm_sec    = seconds;
		t.tm_isdst  = -1;

		millisSinceEpoch = 1000 * (int64) mktime (&t);

		if (millisSinceEpoch < 0)
			millisSinceEpoch = 0;
		else
			millisSinceEpoch += milliseconds;
	}
}

Time::~Time() noexcept
{
}

Time& Time::operator= (const Time& other) noexcept
{
	millisSinceEpoch = other.millisSinceEpoch;
	return *this;
}

int64 Time::currentTimeMillis() noexcept
{
	static uint32 lastCounterResult = 0xffffffff;
	static int64 correction = 0;

	const uint32 now = getMillisecondCounter();

	// check the counter hasn't wrapped (also triggered the first time this function is called)
	if (now < lastCounterResult)
	{
		// double-check it's actually wrapped, in case multi-cpu machines have timers that drift a bit.
		if (lastCounterResult == 0xffffffff || now < lastCounterResult - 10)
		{
			// get the time once using normal library calls, and store the difference needed to
			// turn the millisecond counter into a real time.
		  #if JUCE_WINDOWS
			struct _timeb t;
		   #ifdef USE_NEW_SECURE_TIME_FNS
			_ftime_s (&t);
		   #else
			_ftime (&t);
		   #endif
			correction = (((int64) t.time) * 1000 + t.millitm) - now;
		  #else
			struct timeval tv;
			struct timezone tz;
			gettimeofday (&tv, &tz);
			correction = (((int64) tv.tv_sec) * 1000 + tv.tv_usec / 1000) - now;
		  #endif
		}
	}

	lastCounterResult = now;

	return correction + now;
}

uint32 juce_millisecondsSinceStartup() noexcept;

uint32 Time::getMillisecondCounter() noexcept
{
	const uint32 now = juce_millisecondsSinceStartup();

	if (now < TimeHelpers::lastMSCounterValue)
	{
		// in multi-threaded apps this might be called concurrently, so
		// make sure that our last counter value only increases and doesn't
		// go backwards..
		if (now < TimeHelpers::lastMSCounterValue - 1000)
			TimeHelpers::lastMSCounterValue = now;
	}
	else
	{
		TimeHelpers::lastMSCounterValue = now;
	}

	return now;
}

uint32 Time::getApproximateMillisecondCounter() noexcept
{
	if (TimeHelpers::lastMSCounterValue == 0)
		getMillisecondCounter();

	return TimeHelpers::lastMSCounterValue;
}

void Time::waitForMillisecondCounter (const uint32 targetTime) noexcept
{
	for (;;)
	{
		const uint32 now = getMillisecondCounter();

		if (now >= targetTime)
			break;

		const int toWait = (int) (targetTime - now);

		if (toWait > 2)
		{
			Thread::sleep (jmin (20, toWait >> 1));
		}
		else
		{
			// xxx should consider using mutex_pause on the mac as it apparently
			// makes it seem less like a spinlock and avoids lowering the thread pri.
			for (int i = 10; --i >= 0;)
				Thread::yield();
		}
	}
}

double Time::highResolutionTicksToSeconds (const int64 ticks) noexcept
{
	return ticks / (double) getHighResolutionTicksPerSecond();
}

int64 Time::secondsToHighResolutionTicks (const double seconds) noexcept
{
	return (int64) (seconds * (double) getHighResolutionTicksPerSecond());
}

Time JUCE_CALLTYPE Time::getCurrentTime() noexcept
{
	return Time (currentTimeMillis());
}

String Time::toString (const bool includeDate,
					   const bool includeTime,
					   const bool includeSeconds,
					   const bool use24HourClock) const noexcept
{
	String result;

	if (includeDate)
	{
		result << getDayOfMonth() << ' '
			   << getMonthName (true) << ' '
			   << getYear();

		if (includeTime)
			result << ' ';
	}

	if (includeTime)
	{
		const int mins = getMinutes();

		result << (use24HourClock ? getHours() : getHoursInAmPmFormat())
			   << (mins < 10 ? ":0" : ":") << mins;

		if (includeSeconds)
		{
			const int secs = getSeconds();
			result << (secs < 10 ? ":0" : ":") << secs;
		}

		if (! use24HourClock)
			result << (isAfternoon() ? "pm" : "am");
	}

	return result.trimEnd();
}

String Time::formatted (const String& format) const
{
	size_t bufferSize = 128;
	HeapBlock<juce_wchar> buffer (128);

	struct tm t (TimeHelpers::millisToLocal (millisSinceEpoch));

	while (TimeHelpers::doFTime (CharPointer_UTF32 (buffer.getData()), bufferSize, format, &t) <= 0)
	{
		bufferSize += 128;
		buffer.malloc (bufferSize);
	}

	return CharPointer_UTF32 (buffer.getData());
}

int Time::getYear() const noexcept          { return TimeHelpers::millisToLocal (millisSinceEpoch).tm_year + 1900; }
int Time::getMonth() const noexcept         { return TimeHelpers::millisToLocal (millisSinceEpoch).tm_mon; }
int Time::getDayOfYear() const noexcept     { return TimeHelpers::millisToLocal (millisSinceEpoch).tm_yday; }
int Time::getDayOfMonth() const noexcept    { return TimeHelpers::millisToLocal (millisSinceEpoch).tm_mday; }
int Time::getDayOfWeek() const noexcept     { return TimeHelpers::millisToLocal (millisSinceEpoch).tm_wday; }
int Time::getHours() const noexcept         { return TimeHelpers::millisToLocal (millisSinceEpoch).tm_hour; }
int Time::getMinutes() const noexcept       { return TimeHelpers::millisToLocal (millisSinceEpoch).tm_min; }
int Time::getSeconds() const noexcept       { return TimeHelpers::extendedModulo (millisSinceEpoch / 1000, 60); }
int Time::getMilliseconds() const noexcept  { return TimeHelpers::extendedModulo (millisSinceEpoch, 1000); }

int Time::getHoursInAmPmFormat() const noexcept
{
	const int hours = getHours();

	if (hours == 0)
		return 12;
	else if (hours <= 12)
		return hours;
	else
		return hours - 12;
}

bool Time::isAfternoon() const noexcept
{
	return getHours() >= 12;
}

bool Time::isDaylightSavingTime() const noexcept
{
	return TimeHelpers::millisToLocal (millisSinceEpoch).tm_isdst != 0;
}

String Time::getTimeZone() const noexcept
{
	String zone[2];

  #if JUCE_WINDOWS
	_tzset();

   #ifdef USE_NEW_SECURE_TIME_FNS
	for (int i = 0; i < 2; ++i)
	{
		char name[128] = { 0 };
		size_t length;
		_get_tzname (&length, name, 127, i);
		zone[i] = name;
	}
   #else
	const char** const zonePtr = (const char**) _tzname;
	zone[0] = zonePtr[0];
	zone[1] = zonePtr[1];
   #endif
  #else
	tzset();
	const char** const zonePtr = (const char**) tzname;
	zone[0] = zonePtr[0];
	zone[1] = zonePtr[1];
  #endif

	if (isDaylightSavingTime())
	{
		zone[0] = zone[1];

		if (zone[0].length() > 3
			 && zone[0].containsIgnoreCase ("daylight")
			 && zone[0].contains ("GMT"))
			zone[0] = "BST";
	}

	return zone[0].substring (0, 3);
}

String Time::getMonthName (const bool threeLetterVersion) const
{
	return getMonthName (getMonth(), threeLetterVersion);
}

String Time::getWeekdayName (const bool threeLetterVersion) const
{
	return getWeekdayName (getDayOfWeek(), threeLetterVersion);
}

String Time::getMonthName (int monthNumber, const bool threeLetterVersion)
{
	const char* const shortMonthNames[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
	const char* const longMonthNames[]  = { "January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December" };

	monthNumber %= 12;

	return TRANS (threeLetterVersion ? shortMonthNames [monthNumber]
									 : longMonthNames [monthNumber]);
}

String Time::getWeekdayName (int day, const bool threeLetterVersion)
{
	const char* const shortDayNames[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
	const char* const longDayNames[]  = { "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" };

	day %= 7;

	return TRANS (threeLetterVersion ? shortDayNames [day]
									 : longDayNames [day]);
}

Time& Time::operator+= (const RelativeTime& delta)        { millisSinceEpoch += delta.inMilliseconds(); return *this; }
Time& Time::operator-= (const RelativeTime& delta)        { millisSinceEpoch -= delta.inMilliseconds(); return *this; }

Time operator+ (const Time& time, const RelativeTime& delta)        { Time t (time); return t += delta; }
Time operator- (const Time& time, const RelativeTime& delta)        { Time t (time); return t -= delta; }
Time operator+ (const RelativeTime& delta, const Time& time)        { Time t (time); return t += delta; }
const RelativeTime operator- (const Time& time1, const Time& time2) { return RelativeTime::milliseconds (time1.toMilliseconds() - time2.toMilliseconds()); }

bool operator== (const Time& time1, const Time& time2)      { return time1.toMilliseconds() == time2.toMilliseconds(); }
bool operator!= (const Time& time1, const Time& time2)      { return time1.toMilliseconds() != time2.toMilliseconds(); }
bool operator<  (const Time& time1, const Time& time2)      { return time1.toMilliseconds() <  time2.toMilliseconds(); }
bool operator>  (const Time& time1, const Time& time2)      { return time1.toMilliseconds() >  time2.toMilliseconds(); }
bool operator<= (const Time& time1, const Time& time2)      { return time1.toMilliseconds() <= time2.toMilliseconds(); }
bool operator>= (const Time& time1, const Time& time2)      { return time1.toMilliseconds() >= time2.toMilliseconds(); }

/*** End of inlined file: juce_Time.cpp ***/


/*** Start of inlined file: juce_UnitTest.cpp ***/
UnitTest::UnitTest (const String& name_)
	: name (name_), runner (nullptr)
{
	getAllTests().add (this);
}

UnitTest::~UnitTest()
{
	getAllTests().removeValue (this);
}

Array<UnitTest*>& UnitTest::getAllTests()
{
	static Array<UnitTest*> tests;
	return tests;
}

void UnitTest::initialise()  {}
void UnitTest::shutdown()   {}

void UnitTest::performTest (UnitTestRunner* const runner_)
{
	jassert (runner_ != nullptr);
	runner = runner_;

	initialise();
	runTest();
	shutdown();
}

void UnitTest::logMessage (const String& message)
{
	runner->logMessage (message);
}

void UnitTest::beginTest (const String& testName)
{
	runner->beginNewTest (this, testName);
}

void UnitTest::expect (const bool result, const String& failureMessage)
{
	if (result)
		runner->addPass();
	else
		runner->addFail (failureMessage);
}

UnitTestRunner::UnitTestRunner()
	: currentTest (nullptr),
	  assertOnFailure (true),
	  logPasses (false)
{
}

UnitTestRunner::~UnitTestRunner()
{
}

void UnitTestRunner::setAssertOnFailure (bool shouldAssert) noexcept
{
	assertOnFailure = shouldAssert;
}

void UnitTestRunner::setPassesAreLogged (bool shouldDisplayPasses) noexcept
{
	logPasses = shouldDisplayPasses;
}

int UnitTestRunner::getNumResults() const noexcept
{
	return results.size();
}

const UnitTestRunner::TestResult* UnitTestRunner::getResult (int index) const noexcept
{
	return results [index];
}

void UnitTestRunner::resultsUpdated()
{
}

void UnitTestRunner::runTests (const Array<UnitTest*>& tests)
{
	results.clear();
	resultsUpdated();

	for (int i = 0; i < tests.size(); ++i)
	{
		if (shouldAbortTests())
			break;

		try
		{
			tests.getUnchecked(i)->performTest (this);
		}
		catch (...)
		{
			addFail ("An unhandled exception was thrown!");
		}
	}

	endTest();
}

void UnitTestRunner::runAllTests()
{
	runTests (UnitTest::getAllTests());
}

void UnitTestRunner::logMessage (const String& message)
{
	Logger::writeToLog (message);
}

bool UnitTestRunner::shouldAbortTests()
{
	return false;
}

void UnitTestRunner::beginNewTest (UnitTest* const test, const String& subCategory)
{
	endTest();
	currentTest = test;

	TestResult* const r = new TestResult();
	results.add (r);
	r->unitTestName = test->getName();
	r->subcategoryName = subCategory;
	r->passes = 0;
	r->failures = 0;

	logMessage ("-----------------------------------------------------------------");
	logMessage ("Starting test: " + r->unitTestName + " / " + subCategory + "...");

	resultsUpdated();
}

void UnitTestRunner::endTest()
{
	if (results.size() > 0)
	{
		TestResult* const r = results.getLast();

		if (r->failures > 0)
		{
			String m ("FAILED!!  ");
			m << r->failures << (r->failures == 1 ? " test" : " tests")
			  << " failed, out of a total of " << (r->passes + r->failures);

			logMessage (String::empty);
			logMessage (m);
			logMessage (String::empty);
		}
		else
		{
			logMessage ("All tests completed successfully");
		}
	}
}

void UnitTestRunner::addPass()
{
	{
		const ScopedLock sl (results.getLock());

		TestResult* const r = results.getLast();
		jassert (r != nullptr); // You need to call UnitTest::beginTest() before performing any tests!

		r->passes++;

		if (logPasses)
		{
			String message ("Test ");
			message << (r->failures + r->passes) << " passed";
			logMessage (message);
		}
	}

	resultsUpdated();
}

void UnitTestRunner::addFail (const String& failureMessage)
{
	{
		const ScopedLock sl (results.getLock());

		TestResult* const r = results.getLast();
		jassert (r != nullptr); // You need to call UnitTest::beginTest() before performing any tests!

		r->failures++;

		String message ("!!! Test ");
		message << (r->failures + r->passes) << " failed";

		if (failureMessage.isNotEmpty())
			message << ": " << failureMessage;

		r->messages.add (message);

		logMessage (message);
	}

	resultsUpdated();

	if (assertOnFailure) { jassertfalse }
}

/*** End of inlined file: juce_UnitTest.cpp ***/


/*** Start of inlined file: juce_XmlDocument.cpp ***/
XmlDocument::XmlDocument (const String& documentText)
	: originalText (documentText),
	  input (nullptr),
	  ignoreEmptyTextElements (true)
{
}

XmlDocument::XmlDocument (const File& file)
	: input (nullptr),
	  ignoreEmptyTextElements (true),
	  inputSource (new FileInputSource (file))
{
}

XmlDocument::~XmlDocument()
{
}

XmlElement* XmlDocument::parse (const File& file)
{
	XmlDocument doc (file);
	return doc.getDocumentElement();
}

XmlElement* XmlDocument::parse (const String& xmlData)
{
	XmlDocument doc (xmlData);
	return doc.getDocumentElement();
}

void XmlDocument::setInputSource (InputSource* const newSource) noexcept
{
	inputSource = newSource;
}

void XmlDocument::setEmptyTextElementsIgnored (const bool shouldBeIgnored) noexcept
{
	ignoreEmptyTextElements = shouldBeIgnored;
}

namespace XmlIdentifierChars
{
	static bool isIdentifierCharSlow (const juce_wchar c) noexcept
	{
		return CharacterFunctions::isLetterOrDigit (c)
				 || c == '_' || c == '-' || c == ':' || c == '.';
	}

	static bool isIdentifierChar (const juce_wchar c) noexcept
	{
		static const uint32 legalChars[] = { 0, 0x7ff6000, 0x87fffffe, 0x7fffffe, 0 };

		return ((int) c < (int) numElementsInArray (legalChars) * 32) ? ((legalChars [c >> 5] & (1 << (c & 31))) != 0)
																	  : isIdentifierCharSlow (c);
	}

	/*static void generateIdentifierCharConstants()
	{
		uint32 n[8] = { 0 };
		for (int i = 0; i < 256; ++i)
			if (isIdentifierCharSlow (i))
				n[i >> 5] |= (1 << (i & 31));

		String s;
		for (int i = 0; i < 8; ++i)
			s << "0x" << String::toHexString ((int) n[i]) << ", ";

		DBG (s);
	}*/
}

XmlElement* XmlDocument::getDocumentElement (const bool onlyReadOuterDocumentElement)
{
	String textToParse (originalText);

	if (textToParse.isEmpty() && inputSource != nullptr)
	{
		ScopedPointer <InputStream> in (inputSource->createInputStream());

		if (in != nullptr)
		{
			MemoryOutputStream data;
			data.writeFromInputStream (*in, onlyReadOuterDocumentElement ? 8192 : -1);
			textToParse = data.toString();

			if (! onlyReadOuterDocumentElement)
				originalText = textToParse;
		}
	}

	input = textToParse.getCharPointer();
	lastError = String::empty;
	errorOccurred = false;
	outOfData = false;
	needToLoadDTD = true;

	if (textToParse.isEmpty())
	{
		lastError = "not enough input";
	}
	else
	{
		skipHeader();

		if (input.getAddress() != nullptr)
		{
			ScopedPointer <XmlElement> result (readNextElement (! onlyReadOuterDocumentElement));

			if (! errorOccurred)
				return result.release();
		}
		else
		{
			lastError = "incorrect xml header";
		}
	}

	return nullptr;
}

const String& XmlDocument::getLastParseError() const noexcept
{
	return lastError;
}

void XmlDocument::setLastError (const String& desc, const bool carryOn)
{
	lastError = desc;
	errorOccurred = ! carryOn;
}

String XmlDocument::getFileContents (const String& filename) const
{
	if (inputSource != nullptr)
	{
		const ScopedPointer <InputStream> in (inputSource->createInputStreamFor (filename.trim().unquoted()));

		if (in != nullptr)
			return in->readEntireStreamAsString();
	}

	return String::empty;
}

juce_wchar XmlDocument::readNextChar() noexcept
{
	const juce_wchar c = input.getAndAdvance();

	if (c == 0)
	{
		outOfData = true;
		--input;
	}

	return c;
}

int XmlDocument::findNextTokenLength() noexcept
{
	int len = 0;
	juce_wchar c = *input;

	while (XmlIdentifierChars::isIdentifierChar (c))
		c = input [++len];

	return len;
}

void XmlDocument::skipHeader()
{
	const int headerStart = input.indexOf (CharPointer_UTF8 ("<?xml"));

	if (headerStart >= 0)
	{
		const int headerEnd = (input + headerStart).indexOf (CharPointer_UTF8 ("?>"));
		if (headerEnd < 0)
			return;

	   #if JUCE_DEBUG
		const String header (input + headerStart, (size_t) (headerEnd - headerStart));
		const String encoding (header.fromFirstOccurrenceOf ("encoding", false, true)
									 .fromFirstOccurrenceOf ("=", false, false)
									 .fromFirstOccurrenceOf ("\"", false, false)
									 .upToFirstOccurrenceOf ("\"", false, false).trim());

		/* If you load an XML document with a non-UTF encoding type, it may have been
		   loaded wrongly.. Since all the files are read via the normal juce file streams,
		   they're treated as UTF-8, so by the time it gets to the parser, the encoding will
		   have been lost. Best plan is to stick to utf-8 or if you have specific files to
		   read, use your own code to convert them to a unicode String, and pass that to the
		   XML parser.
		*/
		jassert (encoding.isEmpty() || encoding.startsWithIgnoreCase ("utf-"));
	   #endif

		input += headerEnd + 2;
	}

	skipNextWhiteSpace();

	const int docTypeIndex = input.indexOf (CharPointer_UTF8 ("<!DOCTYPE"));
	if (docTypeIndex < 0)
		return;

	input += docTypeIndex + 9;
	const String::CharPointerType docType (input);

	int n = 1;

	while (n > 0)
	{
		const juce_wchar c = readNextChar();

		if (outOfData)
			return;

		if (c == '<')
			++n;
		else if (c == '>')
			--n;
	}

	dtdText = String (docType, (size_t) (input.getAddress() - (docType.getAddress() + 1))).trim();
}

void XmlDocument::skipNextWhiteSpace()
{
	for (;;)
	{
		juce_wchar c = *input;

		while (CharacterFunctions::isWhitespace (c))
			c = *++input;

		if (c == 0)
		{
			outOfData = true;
			break;
		}
		else if (c == '<')
		{
			if (input[1] == '!'
				 && input[2] == '-'
				 && input[3] == '-')
			{
				input += 4;
				const int closeComment = input.indexOf (CharPointer_UTF8 ("-->"));

				if (closeComment < 0)
				{
					outOfData = true;
					break;
				}

				input += closeComment + 3;
				continue;
			}
			else if (input[1] == '?')
			{
				input += 2;
				const int closeBracket = input.indexOf (CharPointer_UTF8 ("?>"));

				if (closeBracket < 0)
				{
					outOfData = true;
					break;
				}

				input += closeBracket + 2;
				continue;
			}
		}

		break;
	}
}

void XmlDocument::readQuotedString (String& result)
{
	const juce_wchar quote = readNextChar();

	while (! outOfData)
	{
		const juce_wchar c = readNextChar();

		if (c == quote)
			break;

		--input;

		if (c == '&')
		{
			readEntity (result);
		}
		else
		{
			const String::CharPointerType start (input);
			size_t numChars = 0;

			for (;;)
			{
				const juce_wchar character = *input;

				if (character == quote)
				{
					result.appendCharPointer (start, numChars);
					++input;
					return;
				}
				else if (character == '&')
				{
					result.appendCharPointer (start, numChars);
					break;
				}
				else if (character == 0)
				{
					outOfData = true;
					setLastError ("unmatched quotes", false);
					break;
				}

				++input;
				++numChars;
			}
		}
	}
}

XmlElement* XmlDocument::readNextElement (const bool alsoParseSubElements)
{
	XmlElement* node = nullptr;

	skipNextWhiteSpace();
	if (outOfData)
		return nullptr;

	const int openBracket = input.indexOf ((juce_wchar) '<');

	if (openBracket >= 0)
	{
		input += openBracket + 1;
		int tagLen = findNextTokenLength();

		if (tagLen == 0)
		{
			// no tag name - but allow for a gap after the '<' before giving an error
			skipNextWhiteSpace();
			tagLen = findNextTokenLength();

			if (tagLen == 0)
			{
				setLastError ("tag name missing", false);
				return node;
			}
		}

		node = new XmlElement (String (input, (size_t) tagLen));
		input += tagLen;
		LinkedListPointer<XmlElement::XmlAttributeNode>::Appender attributeAppender (node->attributes);

		// look for attributes
		for (;;)
		{
			skipNextWhiteSpace();

			const juce_wchar c = *input;

			// empty tag..
			if (c == '/' && input[1] == '>')
			{
				input += 2;
				break;
			}

			// parse the guts of the element..
			if (c == '>')
			{
				++input;

				if (alsoParseSubElements)
					readChildElements (node);

				break;
			}

			// get an attribute..
			if (XmlIdentifierChars::isIdentifierChar (c))
			{
				const int attNameLen = findNextTokenLength();

				if (attNameLen > 0)
				{
					const String::CharPointerType attNameStart (input);
					input += attNameLen;

					skipNextWhiteSpace();

					if (readNextChar() == '=')
					{
						skipNextWhiteSpace();

						const juce_wchar nextChar = *input;

						if (nextChar == '"' || nextChar == '\'')
						{
							XmlElement::XmlAttributeNode* const newAtt
								= new XmlElement::XmlAttributeNode (String (attNameStart, (size_t) attNameLen),
																	String::empty);

							readQuotedString (newAtt->value);
							attributeAppender.append (newAtt);
							continue;
						}
					}
				}
			}
			else
			{
				if (! outOfData)
					setLastError ("illegal character found in " + node->getTagName() + ": '" + c + "'", false);
			}

			break;
		}
	}

	return node;
}

void XmlDocument::readChildElements (XmlElement* parent)
{
	LinkedListPointer<XmlElement>::Appender childAppender (parent->firstChildElement);

	for (;;)
	{
		const String::CharPointerType preWhitespaceInput (input);
		skipNextWhiteSpace();

		if (outOfData)
		{
			setLastError ("unmatched tags", false);
			break;
		}

		if (*input == '<')
		{
			if (input[1] == '/')
			{
				// our close tag..
				const int closeTag = input.indexOf ((juce_wchar) '>');

				if (closeTag >= 0)
					input += closeTag + 1;

				break;
			}
			else if (input[1] == '!'
				  && input[2] == '['
				  && input[3] == 'C'
				  && input[4] == 'D'
				  && input[5] == 'A'
				  && input[6] == 'T'
				  && input[7] == 'A'
				  && input[8] == '[')
			{
				input += 9;
				const String::CharPointerType inputStart (input);

				size_t len = 0;

				for (;;)
				{
					if (*input == 0)
					{
						setLastError ("unterminated CDATA section", false);
						outOfData = true;
						break;
					}
					else if (input[0] == ']'
							  && input[1] == ']'
							  && input[2] == '>')
					{
						input += 3;
						break;
					}

					++input;
					++len;
				}

				childAppender.append (XmlElement::createTextElement (String (inputStart, len)));
			}
			else
			{
				// this is some other element, so parse and add it..
				XmlElement* const n = readNextElement (true);

				if (n != nullptr)
					childAppender.append (n);
				else
					break;
			}
		}
		else  // must be a character block
		{
			input = preWhitespaceInput; // roll back to include the leading whitespace
			String textElementContent;

			for (;;)
			{
				const juce_wchar c = *input;

				if (c == '<')
					break;

				if (c == 0)
				{
					setLastError ("unmatched tags", false);
					outOfData = true;
					return;
				}

				if (c == '&')
				{
					String entity;
					readEntity (entity);

					if (entity.startsWithChar ('<') && entity [1] != 0)
					{
						const String::CharPointerType oldInput (input);
						const bool oldOutOfData = outOfData;

						input = entity.getCharPointer();
						outOfData = false;

						for (;;)
						{
							XmlElement* const n = readNextElement (true);

							if (n == nullptr)
								break;

							childAppender.append (n);
						}

						input = oldInput;
						outOfData = oldOutOfData;
					}
					else
					{
						textElementContent += entity;
					}
				}
				else
				{
					const String::CharPointerType start (input);
					size_t len = 0;

					for (;;)
					{
						const juce_wchar nextChar = *input;

						if (nextChar == '<' || nextChar == '&')
						{
							break;
						}
						else if (nextChar == 0)
						{
							setLastError ("unmatched tags", false);
							outOfData = true;
							return;
						}

						++input;
						++len;
					}

					textElementContent.appendCharPointer (start, len);
				}
			}

			if ((! ignoreEmptyTextElements) || textElementContent.containsNonWhitespaceChars())
			{
				childAppender.append (XmlElement::createTextElement (textElementContent));
			}
		}
	}
}

void XmlDocument::readEntity (String& result)
{
	// skip over the ampersand
	++input;

	if (input.compareIgnoreCaseUpTo (CharPointer_UTF8 ("amp;"), 4) == 0)
	{
		input += 4;
		result += '&';
	}
	else if (input.compareIgnoreCaseUpTo (CharPointer_UTF8 ("quot;"), 5) == 0)
	{
		input += 5;
		result += '"';
	}
	else if (input.compareIgnoreCaseUpTo (CharPointer_UTF8 ("apos;"), 5) == 0)
	{
		input += 5;
		result += '\'';
	}
	else if (input.compareIgnoreCaseUpTo (CharPointer_UTF8 ("lt;"), 3) == 0)
	{
		input += 3;
		result += '<';
	}
	else if (input.compareIgnoreCaseUpTo (CharPointer_UTF8 ("gt;"), 3) == 0)
	{
		input += 3;
		result += '>';
	}
	else if (*input == '#')
	{
		int charCode = 0;
		++input;

		if (*input == 'x' || *input == 'X')
		{
			++input;
			int numChars = 0;

			while (input[0] != ';')
			{
				const int hexValue = CharacterFunctions::getHexDigitValue (input[0]);

				if (hexValue < 0 || ++numChars > 8)
				{
					setLastError ("illegal escape sequence", true);
					break;
				}

				charCode = (charCode << 4) | hexValue;
				++input;
			}

			++input;
		}
		else if (input[0] >= '0' && input[0] <= '9')
		{
			int numChars = 0;

			while (input[0] != ';')
			{
				if (++numChars > 12)
				{
					setLastError ("illegal escape sequence", true);
					break;
				}

				charCode = charCode * 10 + ((int) input[0] - '0');
				++input;
			}

			++input;
		}
		else
		{
			setLastError ("illegal escape sequence", true);
			result += '&';
			return;
		}

		result << (juce_wchar) charCode;
	}
	else
	{
		const String::CharPointerType entityNameStart (input);
		const int closingSemiColon = input.indexOf ((juce_wchar) ';');

		if (closingSemiColon < 0)
		{
			outOfData = true;
			result += '&';
		}
		else
		{
			input += closingSemiColon + 1;

			result += expandExternalEntity (String (entityNameStart, (size_t) closingSemiColon));
		}
	}
}

String XmlDocument::expandEntity (const String& ent)
{
	if (ent.equalsIgnoreCase ("amp"))   return String::charToString ('&');
	if (ent.equalsIgnoreCase ("quot"))  return String::charToString ('"');
	if (ent.equalsIgnoreCase ("apos"))  return String::charToString ('\'');
	if (ent.equalsIgnoreCase ("lt"))    return String::charToString ('<');
	if (ent.equalsIgnoreCase ("gt"))    return String::charToString ('>');

	if (ent[0] == '#')
	{
		const juce_wchar char1 = ent[1];

		if (char1 == 'x' || char1 == 'X')
			return String::charToString (static_cast <juce_wchar> (ent.substring (2).getHexValue32()));

		if (char1 >= '0' && char1 <= '9')
			return String::charToString (static_cast <juce_wchar> (ent.substring (1).getIntValue()));

		setLastError ("illegal escape sequence", false);
		return String::charToString ('&');
	}

	return expandExternalEntity (ent);
}

String XmlDocument::expandExternalEntity (const String& entity)
{
	if (needToLoadDTD)
	{
		if (dtdText.isNotEmpty())
		{
			dtdText = dtdText.trimCharactersAtEnd (">");
			tokenisedDTD.addTokens (dtdText, true);

			if (tokenisedDTD [tokenisedDTD.size() - 2].equalsIgnoreCase ("system")
				 && tokenisedDTD [tokenisedDTD.size() - 1].isQuotedString())
			{
				const String fn (tokenisedDTD [tokenisedDTD.size() - 1]);

				tokenisedDTD.clear();
				tokenisedDTD.addTokens (getFileContents (fn), true);
			}
			else
			{
				tokenisedDTD.clear();
				const int openBracket = dtdText.indexOfChar ('[');

				if (openBracket > 0)
				{
					const int closeBracket = dtdText.lastIndexOfChar (']');

					if (closeBracket > openBracket)
						tokenisedDTD.addTokens (dtdText.substring (openBracket + 1,
																   closeBracket), true);
				}
			}

			for (int i = tokenisedDTD.size(); --i >= 0;)
			{
				if (tokenisedDTD[i].startsWithChar ('%')
					 && tokenisedDTD[i].endsWithChar (';'))
				{
					const String parsed (getParameterEntity (tokenisedDTD[i].substring (1, tokenisedDTD[i].length() - 1)));
					StringArray newToks;
					newToks.addTokens (parsed, true);

					tokenisedDTD.remove (i);

					for (int j = newToks.size(); --j >= 0;)
						tokenisedDTD.insert (i, newToks[j]);
				}
			}
		}

		needToLoadDTD = false;
	}

	for (int i = 0; i < tokenisedDTD.size(); ++i)
	{
		if (tokenisedDTD[i] == entity)
		{
			if (tokenisedDTD[i - 1].equalsIgnoreCase ("<!entity"))
			{
				String ent (tokenisedDTD [i + 1].trimCharactersAtEnd (">").trim().unquoted());

				// check for sub-entities..
				int ampersand = ent.indexOfChar ('&');

				while (ampersand >= 0)
				{
					const int semiColon = ent.indexOf (i + 1, ";");

					if (semiColon < 0)
					{
						setLastError ("entity without terminating semi-colon", false);
						break;
					}

					const String resolved (expandEntity (ent.substring (i + 1, semiColon)));

					ent = ent.substring (0, ampersand)
						   + resolved
						   + ent.substring (semiColon + 1);

					ampersand = ent.indexOfChar (semiColon + 1, '&');
				}

				return ent;
			}
		}
	}

	setLastError ("unknown entity", true);

	return entity;
}

String XmlDocument::getParameterEntity (const String& entity)
{
	for (int i = 0; i < tokenisedDTD.size(); ++i)
	{
		if (tokenisedDTD[i] == entity
			 && tokenisedDTD [i - 1] == "%"
			 && tokenisedDTD [i - 2].equalsIgnoreCase ("<!entity"))
		{
			const String ent (tokenisedDTD [i + 1].trimCharactersAtEnd (">"));

			if (ent.equalsIgnoreCase ("system"))
				return getFileContents (tokenisedDTD [i + 2].trimCharactersAtEnd (">"));
			else
				return ent.trim().unquoted();
		}
	}

	return entity;
}

/*** End of inlined file: juce_XmlDocument.cpp ***/


/*** Start of inlined file: juce_XmlElement.cpp ***/
XmlElement::XmlAttributeNode::XmlAttributeNode (const XmlAttributeNode& other) noexcept
	: name (other.name),
	  value (other.value)
{
}

XmlElement::XmlAttributeNode::XmlAttributeNode (const String& name_, const String& value_) noexcept
	: name (name_),
	  value (value_)
{
   #if JUCE_DEBUG
	// this checks whether the attribute name string contains any illegal characters..
	for (String::CharPointerType t (name.getCharPointer()); ! t.isEmpty(); ++t)
		jassert (t.isLetterOrDigit() || *t == '_' || *t == '-' || *t == ':');
   #endif
}

inline bool XmlElement::XmlAttributeNode::hasName (const String& nameToMatch) const noexcept
{
	return name.equalsIgnoreCase (nameToMatch);
}

XmlElement::XmlElement (const String& tagName_) noexcept
	: tagName (tagName_)
{
	// the tag name mustn't be empty, or it'll look like a text element!
	jassert (tagName_.containsNonWhitespaceChars())

	// The tag can't contain spaces or other characters that would create invalid XML!
	jassert (! tagName_.containsAnyOf (" <>/&"));
}

XmlElement::XmlElement (int /*dummy*/) noexcept
{
}

XmlElement::XmlElement (const XmlElement& other)
	: tagName (other.tagName)
{
	copyChildrenAndAttributesFrom (other);
}

XmlElement& XmlElement::operator= (const XmlElement& other)
{
	if (this != &other)
	{
		removeAllAttributes();
		deleteAllChildElements();

		tagName = other.tagName;

		copyChildrenAndAttributesFrom (other);
	}

	return *this;
}

#if JUCE_COMPILER_SUPPORTS_MOVE_SEMANTICS
XmlElement::XmlElement (XmlElement&& other) noexcept
	: nextListItem      (static_cast <LinkedListPointer <XmlElement>&&> (other.nextListItem)),
	  firstChildElement (static_cast <LinkedListPointer <XmlElement>&&> (other.firstChildElement)),
	  attributes        (static_cast <LinkedListPointer <XmlAttributeNode>&&> (other.attributes)),
	  tagName           (static_cast <String&&> (other.tagName))
{
}

XmlElement& XmlElement::operator= (XmlElement&& other) noexcept
{
	jassert (this != &other); // hopefully the compiler should make this situation impossible!

	removeAllAttributes();
	deleteAllChildElements();

	nextListItem      = static_cast <LinkedListPointer <XmlElement>&&> (other.nextListItem);
	firstChildElement = static_cast <LinkedListPointer <XmlElement>&&> (other.firstChildElement);
	attributes        = static_cast <LinkedListPointer <XmlAttributeNode>&&> (other.attributes);
	tagName           = static_cast <String&&> (other.tagName);

	return *this;
}
#endif

void XmlElement::copyChildrenAndAttributesFrom (const XmlElement& other)
{
	jassert (firstChildElement.get() == nullptr);
	firstChildElement.addCopyOfList (other.firstChildElement);

	jassert (attributes.get() == nullptr);
	attributes.addCopyOfList (other.attributes);
}

XmlElement::~XmlElement() noexcept
{
	firstChildElement.deleteAll();
	attributes.deleteAll();
}

namespace XmlOutputFunctions
{
   #if 0 // (These functions are just used to generate the lookup table used below)
	bool isLegalXmlCharSlow (const juce_wchar character) noexcept
	{
		if ((character >= 'a' && character <= 'z')
			 || (character >= 'A' && character <= 'Z')
				|| (character >= '0' && character <= '9'))
			return true;

		const char* t = " .,;:-()_+=?!'#@[]/\\*%~{}$|";

		do
		{
			if (((juce_wchar) (uint8) *t) == character)
				return true;
		}
		while (*++t != 0);

		return false;
	}

	void generateLegalCharLookupTable()
	{
		uint8 n[32] = { 0 };
		for (int i = 0; i < 256; ++i)
			if (isLegalXmlCharSlow (i))
				n[i >> 3] |= (1 << (i & 7));

		String s;
		for (int i = 0; i < 32; ++i)
			s << (int) n[i] << ", ";

		DBG (s);
	}
   #endif

	static bool isLegalXmlChar (const uint32 c) noexcept
	{
		static const unsigned char legalChars[] = { 0, 0, 0, 0, 187, 255, 255, 175, 255, 255, 255, 191, 254, 255, 255, 127 };

		return c < sizeof (legalChars) * 8
				 && (legalChars [c >> 3] & (1 << (c & 7))) != 0;
	}

	static void escapeIllegalXmlChars (OutputStream& outputStream, const String& text, const bool changeNewLines)
	{
		String::CharPointerType t (text.getCharPointer());

		for (;;)
		{
			const uint32 character = (uint32) t.getAndAdvance();

			if (character == 0)
				break;

			if (isLegalXmlChar (character))
			{
				outputStream << (char) character;
			}
			else
			{
				switch (character)
				{
				case '&':   outputStream << "&amp;"; break;
				case '"':   outputStream << "&quot;"; break;
				case '>':   outputStream << "&gt;"; break;
				case '<':   outputStream << "&lt;"; break;

				case '\n':
				case '\r':
					if (! changeNewLines)
					{
						outputStream << (char) character;
						break;
					}
					// Note: deliberate fall-through here!
				default:
					outputStream << "&#" << ((int) character) << ';';
					break;
				}
			}
		}
	}

	static void writeSpaces (OutputStream& out, const int numSpaces)
	{
		out.writeRepeatedByte (' ', numSpaces);
	}
}

void XmlElement::writeElementAsText (OutputStream& outputStream,
									 const int indentationLevel,
									 const int lineWrapLength) const
{
	using namespace XmlOutputFunctions;

	if (indentationLevel >= 0)
		writeSpaces (outputStream, indentationLevel);

	if (! isTextElement())
	{
		outputStream.writeByte ('<');
		outputStream << tagName;

		{
			const int attIndent = indentationLevel + tagName.length() + 1;
			int lineLen = 0;

			for (const XmlAttributeNode* att = attributes; att != nullptr; att = att->nextListItem)
			{
				if (lineLen > lineWrapLength && indentationLevel >= 0)
				{
					outputStream << newLine;
					writeSpaces (outputStream, attIndent);
					lineLen = 0;
				}

				const int64 startPos = outputStream.getPosition();
				outputStream.writeByte (' ');
				outputStream << att->name;
				outputStream.write ("=\"", 2);
				escapeIllegalXmlChars (outputStream, att->value, true);
				outputStream.writeByte ('"');
				lineLen += (int) (outputStream.getPosition() - startPos);
			}
		}

		if (firstChildElement != nullptr)
		{
			outputStream.writeByte ('>');

			bool lastWasTextNode = false;

			for (XmlElement* child = firstChildElement; child != nullptr; child = child->nextListItem)
			{
				if (child->isTextElement())
				{
					escapeIllegalXmlChars (outputStream, child->getText(), false);
					lastWasTextNode = true;
				}
				else
				{
					if (indentationLevel >= 0 && ! lastWasTextNode)
						outputStream << newLine;

					child->writeElementAsText (outputStream,
											   lastWasTextNode ? 0 : (indentationLevel + (indentationLevel >= 0 ? 2 : 0)), lineWrapLength);
					lastWasTextNode = false;
				}
			}

			if (indentationLevel >= 0 && ! lastWasTextNode)
			{
				outputStream << newLine;
				writeSpaces (outputStream, indentationLevel);
			}

			outputStream.write ("</", 2);
			outputStream << tagName;
			outputStream.writeByte ('>');
		}
		else
		{
			outputStream.write ("/>", 2);
		}
	}
	else
	{
		escapeIllegalXmlChars (outputStream, getText(), false);
	}
}

String XmlElement::createDocument (const String& dtdToUse,
								   const bool allOnOneLine,
								   const bool includeXmlHeader,
								   const String& encodingType,
								   const int lineWrapLength) const
{
	MemoryOutputStream mem (2048);
	writeToStream (mem, dtdToUse, allOnOneLine, includeXmlHeader, encodingType, lineWrapLength);

	return mem.toUTF8();
}

void XmlElement::writeToStream (OutputStream& output,
								const String& dtdToUse,
								const bool allOnOneLine,
								const bool includeXmlHeader,
								const String& encodingType,
								const int lineWrapLength) const
{
	using namespace XmlOutputFunctions;

	if (includeXmlHeader)
	{
		output << "<?xml version=\"1.0\" encoding=\"" << encodingType << "\"?>";

		if (allOnOneLine)
			output.writeByte (' ');
		else
			output << newLine << newLine;
	}

	if (dtdToUse.isNotEmpty())
	{
		output << dtdToUse;

		if (allOnOneLine)
			output.writeByte (' ');
		else
			output << newLine;
	}

	writeElementAsText (output, allOnOneLine ? -1 : 0, lineWrapLength);

	if (! allOnOneLine)
		output << newLine;
}

bool XmlElement::writeToFile (const File& file,
							  const String& dtdToUse,
							  const String& encodingType,
							  const int lineWrapLength) const
{
	if (file.hasWriteAccess())
	{
		TemporaryFile tempFile (file);
		ScopedPointer <FileOutputStream> out (tempFile.getFile().createOutputStream());

		if (out != nullptr)
		{
			writeToStream (*out, dtdToUse, false, true, encodingType, lineWrapLength);
			out = nullptr;

			return tempFile.overwriteTargetFileWithTemporary();
		}
	}

	return false;
}

bool XmlElement::hasTagName (const String& tagNameWanted) const noexcept
{
   #if JUCE_DEBUG
	// if debugging, check that the case is actually the same, because
	// valid xml is case-sensitive, and although this lets it pass, it's
	// better not to..
	if (tagName.equalsIgnoreCase (tagNameWanted))
	{
		jassert (tagName == tagNameWanted);
		return true;
	}
	else
	{
		return false;
	}
   #else
	return tagName.equalsIgnoreCase (tagNameWanted);
   #endif
}

XmlElement* XmlElement::getNextElementWithTagName (const String& requiredTagName) const
{
	XmlElement* e = nextListItem;

	while (e != nullptr && ! e->hasTagName (requiredTagName))
		e = e->nextListItem;

	return e;
}

int XmlElement::getNumAttributes() const noexcept
{
	return attributes.size();
}

const String& XmlElement::getAttributeName (const int index) const noexcept
{
	const XmlAttributeNode* const att = attributes [index];
	return att != nullptr ? att->name : String::empty;
}

const String& XmlElement::getAttributeValue (const int index) const noexcept
{
	const XmlAttributeNode* const att = attributes [index];
	return att != nullptr ? att->value : String::empty;
}

bool XmlElement::hasAttribute (const String& attributeName) const noexcept
{
	for (const XmlAttributeNode* att = attributes; att != nullptr; att = att->nextListItem)
		if (att->hasName (attributeName))
			return true;

	return false;
}

const String& XmlElement::getStringAttribute (const String& attributeName) const noexcept
{
	for (const XmlAttributeNode* att = attributes; att != nullptr; att = att->nextListItem)
		if (att->hasName (attributeName))
			return att->value;

	return String::empty;
}

String XmlElement::getStringAttribute (const String& attributeName, const String& defaultReturnValue) const
{
	for (const XmlAttributeNode* att = attributes; att != nullptr; att = att->nextListItem)
		if (att->hasName (attributeName))
			return att->value;

	return defaultReturnValue;
}

int XmlElement::getIntAttribute (const String& attributeName, const int defaultReturnValue) const
{
	for (const XmlAttributeNode* att = attributes; att != nullptr; att = att->nextListItem)
		if (att->hasName (attributeName))
			return att->value.getIntValue();

	return defaultReturnValue;
}

double XmlElement::getDoubleAttribute (const String& attributeName, const double defaultReturnValue) const
{
	for (const XmlAttributeNode* att = attributes; att != nullptr; att = att->nextListItem)
		if (att->hasName (attributeName))
			return att->value.getDoubleValue();

	return defaultReturnValue;
}

bool XmlElement::getBoolAttribute (const String& attributeName, const bool defaultReturnValue) const
{
	for (const XmlAttributeNode* att = attributes; att != nullptr; att = att->nextListItem)
	{
		if (att->hasName (attributeName))
		{
			juce_wchar firstChar = att->value[0];

			if (CharacterFunctions::isWhitespace (firstChar))
				firstChar = att->value.trimStart() [0];

			return firstChar == '1'
				|| firstChar == 't'
				|| firstChar == 'y'
				|| firstChar == 'T'
				|| firstChar == 'Y';
		}
	}

	return defaultReturnValue;
}

bool XmlElement::compareAttribute (const String& attributeName,
								   const String& stringToCompareAgainst,
								   const bool ignoreCase) const noexcept
{
	for (const XmlAttributeNode* att = attributes; att != nullptr; att = att->nextListItem)
		if (att->hasName (attributeName))
			return ignoreCase ? att->value.equalsIgnoreCase (stringToCompareAgainst)
							  : att->value == stringToCompareAgainst;

	return false;
}

void XmlElement::setAttribute (const String& attributeName, const String& value)
{
	if (attributes == nullptr)
	{
		attributes = new XmlAttributeNode (attributeName, value);
	}
	else
	{
		XmlAttributeNode* att = attributes;

		for (;;)
		{
			if (att->hasName (attributeName))
			{
				att->value = value;
				break;
			}
			else if (att->nextListItem == nullptr)
			{
				att->nextListItem = new XmlAttributeNode (attributeName, value);
				break;
			}

			att = att->nextListItem;
		}
	}
}

void XmlElement::setAttribute (const String& attributeName, const int number)
{
	setAttribute (attributeName, String (number));
}

void XmlElement::setAttribute (const String& attributeName, const double number)
{
	setAttribute (attributeName, String (number));
}

void XmlElement::removeAttribute (const String& attributeName) noexcept
{
	LinkedListPointer<XmlAttributeNode>* att = &attributes;

	while (att->get() != nullptr)
	{
		if (att->get()->hasName (attributeName))
		{
			delete att->removeNext();
			break;
		}

		att = &(att->get()->nextListItem);
	}
}

void XmlElement::removeAllAttributes() noexcept
{
	attributes.deleteAll();
}

int XmlElement::getNumChildElements() const noexcept
{
	return firstChildElement.size();
}

XmlElement* XmlElement::getChildElement (const int index) const noexcept
{
	return firstChildElement [index].get();
}

XmlElement* XmlElement::getChildByName (const String& childName) const noexcept
{
	for (XmlElement* child = firstChildElement; child != nullptr; child = child->nextListItem)
		if (child->hasTagName (childName))
			return child;

	return nullptr;
}

void XmlElement::addChildElement (XmlElement* const newNode) noexcept
{
	if (newNode != nullptr)
		firstChildElement.append (newNode);
}

void XmlElement::insertChildElement (XmlElement* const newNode,
									 int indexToInsertAt) noexcept
{
	if (newNode != nullptr)
	{
		removeChildElement (newNode, false);
		firstChildElement.insertAtIndex (indexToInsertAt, newNode);
	}
}

XmlElement* XmlElement::createNewChildElement (const String& childTagName)
{
	XmlElement* const newElement = new XmlElement (childTagName);
	addChildElement (newElement);
	return newElement;
}

bool XmlElement::replaceChildElement (XmlElement* const currentChildElement,
									  XmlElement* const newNode) noexcept
{
	if (newNode != nullptr)
	{
		LinkedListPointer<XmlElement>* const p = firstChildElement.findPointerTo (currentChildElement);

		if (p != nullptr)
		{
			if (currentChildElement != newNode)
				delete p->replaceNext (newNode);

			return true;
		}
	}

	return false;
}

void XmlElement::removeChildElement (XmlElement* const childToRemove,
									 const bool shouldDeleteTheChild) noexcept
{
	if (childToRemove != nullptr)
	{
		firstChildElement.remove (childToRemove);

		if (shouldDeleteTheChild)
			delete childToRemove;
	}
}

bool XmlElement::isEquivalentTo (const XmlElement* const other,
								 const bool ignoreOrderOfAttributes) const noexcept
{
	if (this != other)
	{
		if (other == nullptr || tagName != other->tagName)
			return false;

		if (ignoreOrderOfAttributes)
		{
			int totalAtts = 0;

			for (const XmlAttributeNode* att = attributes; att != nullptr; att = att->nextListItem)
			{
				if (! other->compareAttribute (att->name, att->value))
					return false;

				++totalAtts;
			}

			if (totalAtts != other->getNumAttributes())
				return false;
		}
		else
		{
			const XmlAttributeNode* thisAtt = attributes;
			const XmlAttributeNode* otherAtt = other->attributes;

			for (;;)
			{
				if (thisAtt == nullptr || otherAtt == nullptr)
				{
					if (thisAtt == otherAtt) // both 0, so it's a match
						break;

					return false;
				}

				if (thisAtt->name != otherAtt->name
					 || thisAtt->value != otherAtt->value)
				{
					return false;
				}

				thisAtt = thisAtt->nextListItem;
				otherAtt = otherAtt->nextListItem;
			}
		}

		const XmlElement* thisChild = firstChildElement;
		const XmlElement* otherChild = other->firstChildElement;

		for (;;)
		{
			if (thisChild == nullptr || otherChild == nullptr)
			{
				if (thisChild == otherChild) // both 0, so it's a match
					break;

				return false;
			}

			if (! thisChild->isEquivalentTo (otherChild, ignoreOrderOfAttributes))
				return false;

			thisChild = thisChild->nextListItem;
			otherChild = otherChild->nextListItem;
		}
	}

	return true;
}

void XmlElement::deleteAllChildElements() noexcept
{
	firstChildElement.deleteAll();
}

void XmlElement::deleteAllChildElementsWithTagName (const String& name) noexcept
{
	XmlElement* child = firstChildElement;

	while (child != nullptr)
	{
		XmlElement* const nextChild = child->nextListItem;

		if (child->hasTagName (name))
			removeChildElement (child, true);

		child = nextChild;
	}
}

bool XmlElement::containsChildElement (const XmlElement* const possibleChild) const noexcept
{
	return firstChildElement.contains (possibleChild);
}

XmlElement* XmlElement::findParentElementOf (const XmlElement* const elementToLookFor) noexcept
{
	if (this == elementToLookFor || elementToLookFor == nullptr)
		return nullptr;

	for (XmlElement* child = firstChildElement; child != nullptr; child = child->nextListItem)
	{
		if (elementToLookFor == child)
			return this;

		XmlElement* const found = child->findParentElementOf (elementToLookFor);

		if (found != nullptr)
			return found;
	}

	return nullptr;
}

void XmlElement::getChildElementsAsArray (XmlElement** elems) const noexcept
{
	firstChildElement.copyToArray (elems);
}

void XmlElement::reorderChildElements (XmlElement** const elems, const int num) noexcept
{
	XmlElement* e = firstChildElement = elems[0];

	for (int i = 1; i < num; ++i)
	{
		e->nextListItem = elems[i];
		e = e->nextListItem;
	}

	e->nextListItem = nullptr;
}

bool XmlElement::isTextElement() const noexcept
{
	return tagName.isEmpty();
}

static const String juce_xmltextContentAttributeName ("text");

const String& XmlElement::getText() const noexcept
{
	jassert (isTextElement());  // you're trying to get the text from an element that
								// isn't actually a text element.. If this contains text sub-nodes, you
								// probably want to use getAllSubText instead.

	return getStringAttribute (juce_xmltextContentAttributeName);
}

void XmlElement::setText (const String& newText)
{
	if (isTextElement())
		setAttribute (juce_xmltextContentAttributeName, newText);
	else
		jassertfalse; // you can only change the text in a text element, not a normal one.
}

String XmlElement::getAllSubText() const
{
	if (isTextElement())
		return getText();

	MemoryOutputStream mem (1024);

	for (const XmlElement* child = firstChildElement; child != nullptr; child = child->nextListItem)
		mem << child->getAllSubText();

	return mem.toString();
}

String XmlElement::getChildElementAllSubText (const String& childTagName,
											  const String& defaultReturnValue) const
{
	const XmlElement* const child = getChildByName (childTagName);

	if (child != nullptr)
		return child->getAllSubText();

	return defaultReturnValue;
}

XmlElement* XmlElement::createTextElement (const String& text)
{
	XmlElement* const e = new XmlElement ((int) 0);
	e->setAttribute (juce_xmltextContentAttributeName, text);
	return e;
}

void XmlElement::addTextElement (const String& text)
{
	addChildElement (createTextElement (text));
}

void XmlElement::deleteAllTextElements() noexcept
{
	XmlElement* child = firstChildElement;

	while (child != nullptr)
	{
		XmlElement* const next = child->nextListItem;

		if (child->isTextElement())
			removeChildElement (child, true);

		child = next;
	}
}

/*** End of inlined file: juce_XmlElement.cpp ***/


/*** Start of inlined file: juce_GZIPDecompressorInputStream.cpp ***/
#if JUCE_MSVC
 #pragma warning (push)
 #pragma warning (disable: 4309 4305 4365)
#endif

namespace zlibNamespace
{
 #if JUCE_INCLUDE_ZLIB_CODE
  #undef OS_CODE
  #undef fdopen
  #define ZLIB_INTERNAL
  #define NO_DUMMY_DECL

/*** Start of inlined file: zlib.h ***/
#ifndef ZLIB_H
#define ZLIB_H


/*** Start of inlined file: zconf.h ***/
/* @(#) $Id: zconf.h,v 1.1 2007/06/07 17:54:37 jules_rms Exp $ */

#ifndef ZCONF_H
#define ZCONF_H

// *** Just a few hacks here to make it compile nicely with Juce..
#define Z_PREFIX 1
#undef __MACTYPES__

#ifdef _MSC_VER
  #pragma warning (disable : 4131 4127 4244 4267)
#endif

/*
 * If you *really* need a unique prefix for all types and library functions,
 * compile with -DZ_PREFIX. The "standard" zlib should be compiled without it.
 */
#ifdef Z_PREFIX
#  define deflateInit_          z_deflateInit_
#  define deflate               z_deflate
#  define deflateEnd            z_deflateEnd
#  define inflateInit_          z_inflateInit_
#  define inflate               z_inflate
#  define inflateEnd            z_inflateEnd
#  define inflatePrime          z_inflatePrime
#  define inflateGetHeader      z_inflateGetHeader
#  define adler32_combine       z_adler32_combine
#  define crc32_combine         z_crc32_combine
#  define deflateInit2_         z_deflateInit2_
#  define deflateSetDictionary  z_deflateSetDictionary
#  define deflateCopy           z_deflateCopy
#  define deflateReset          z_deflateReset
#  define deflateParams         z_deflateParams
#  define deflateBound          z_deflateBound
#  define deflatePrime          z_deflatePrime
#  define inflateInit2_         z_inflateInit2_
#  define inflateSetDictionary  z_inflateSetDictionary
#  define inflateSync           z_inflateSync
#  define inflateSyncPoint      z_inflateSyncPoint
#  define inflateCopy           z_inflateCopy
#  define inflateReset          z_inflateReset
#  define inflateBack           z_inflateBack
#  define inflateBackEnd        z_inflateBackEnd
#  define compress              z_compress
#  define compress2             z_compress2
#  define compressBound         z_compressBound
#  define uncompress            z_uncompress
#  define adler32               z_adler32
#  define crc32                 z_crc32
#  define get_crc_table         z_get_crc_table
#  define zError                z_zError

#  define alloc_func            z_alloc_func
#  define free_func             z_free_func
#  define in_func               z_in_func
#  define out_func              z_out_func
#  define Byte                  z_Byte
#  define uInt                  z_uInt
#  define uLong                 z_uLong
#  define Bytef                 z_Bytef
#  define charf                 z_charf
#  define intf                  z_intf
#  define uIntf                 z_uIntf
#  define uLongf                z_uLongf
#  define voidpf                z_voidpf
#  define voidp                 z_voidp
#endif

#if defined(__MSDOS__) && !defined(MSDOS)
#  define MSDOS
#endif
#if (defined(OS_2) || defined(__OS2__)) && !defined(OS2)
#  define OS2
#endif
#if defined(_WINDOWS) && !defined(WINDOWS)
#  define WINDOWS
#endif
#if defined(_WIN32) || defined(_WIN32_WCE) || defined(__WIN32__)
#  ifndef WIN32
#    define WIN32
#  endif
#endif
#if (defined(MSDOS) || defined(OS2) || defined(WINDOWS)) && !defined(WIN32)
#  if !defined(__GNUC__) && !defined(__FLAT__) && !defined(__386__)
#    ifndef SYS16BIT
#      define SYS16BIT
#    endif
#  endif
#endif

/*
 * Compile with -DMAXSEG_64K if the alloc function cannot allocate more
 * than 64k bytes at a time (needed on systems with 16-bit int).
 */
#ifdef SYS16BIT
#  define MAXSEG_64K
#endif
#ifdef MSDOS
#  define UNALIGNED_OK
#endif

#ifdef __STDC_VERSION__
#  ifndef STDC
#    define STDC
#  endif
#  if __STDC_VERSION__ >= 199901L
#    ifndef STDC99
#      define STDC99
#    endif
#  endif
#endif
#if !defined(STDC) && (defined(__STDC__) || defined(__cplusplus))
#  define STDC
#endif
#if !defined(STDC) && (defined(__GNUC__) || defined(__BORLANDC__))
#  define STDC
#endif
#if !defined(STDC) && (defined(MSDOS) || defined(WINDOWS) || defined(WIN32))
#  define STDC
#endif
#if !defined(STDC) && (defined(OS2) || defined(__HOS_AIX__))
#  define STDC
#endif

#if defined(__OS400__) && !defined(STDC)    /* iSeries (formerly AS/400). */
#  define STDC
#endif

#ifndef STDC
#  ifndef const /* cannot use !defined(STDC) && !defined(const) on Mac */
#    define const       /* note: need a more gentle solution here */
#  endif
#endif

/* Some Mac compilers merge all .h files incorrectly: */
#if defined(__MWERKS__)||defined(applec)||defined(THINK_C)||defined(__SC__)
#  define NO_DUMMY_DECL
#endif

/* Maximum value for memLevel in deflateInit2 */
#ifndef MAX_MEM_LEVEL
#  ifdef MAXSEG_64K
#    define MAX_MEM_LEVEL 8
#  else
#    define MAX_MEM_LEVEL 9
#  endif
#endif

/* Maximum value for windowBits in deflateInit2 and inflateInit2.
 * WARNING: reducing MAX_WBITS makes minigzip unable to extract .gz files
 * created by gzip. (Files created by minigzip can still be extracted by
 * gzip.)
 */
#ifndef MAX_WBITS
#  define MAX_WBITS   15 /* 32K LZ77 window */
#endif

/* The memory requirements for deflate are (in bytes):
			(1 << (windowBits+2)) +  (1 << (memLevel+9))
 that is: 128K for windowBits=15  +  128K for memLevel = 8  (default values)
 plus a few kilobytes for small objects. For example, if you want to reduce
 the default memory requirements from 256K to 128K, compile with
	 make CFLAGS="-O -DMAX_WBITS=14 -DMAX_MEM_LEVEL=7"
 Of course this will generally degrade compression (there's no free lunch).

   The memory requirements for inflate are (in bytes) 1 << windowBits
 that is, 32K for windowBits=15 (default value) plus a few kilobytes
 for small objects.
*/

						/* Type declarations */

#ifndef OF /* function prototypes */
#  ifdef STDC
#    define OF(args)  args
#  else
#    define OF(args)  ()
#  endif
#endif

/* The following definitions for FAR are needed only for MSDOS mixed
 * model programming (small or medium model with some far allocations).
 * This was tested only with MSC; for other MSDOS compilers you may have
 * to define NO_MEMCPY in zutil.h.  If you don't need the mixed model,
 * just define FAR to be empty.
 */
#ifdef SYS16BIT
#  if defined(M_I86SM) || defined(M_I86MM)
	 /* MSC small or medium model */
#    define SMALL_MEDIUM
#    ifdef _MSC_VER
#      define FAR _far
#    else
#      define FAR far
#    endif
#  endif
#  if (defined(__SMALL__) || defined(__MEDIUM__))
	 /* Turbo C small or medium model */
#    define SMALL_MEDIUM
#    ifdef __BORLANDC__
#      define FAR _far
#    else
#      define FAR far
#    endif
#  endif
#endif

#if defined(WINDOWS) || defined(WIN32)
   /* If building or using zlib as a DLL, define ZLIB_DLL.
	* This is not mandatory, but it offers a little performance increase.
	*/
#  ifdef ZLIB_DLL
#    if defined(WIN32) && (!defined(__BORLANDC__) || (__BORLANDC__ >= 0x500))
#      ifdef ZLIB_INTERNAL
#        define ZEXTERN extern __declspec(dllexport)
#      else
#        define ZEXTERN extern __declspec(dllimport)
#      endif
#    endif
#  endif  /* ZLIB_DLL */
   /* If building or using zlib with the WINAPI/WINAPIV calling convention,
	* define ZLIB_WINAPI.
	* Caution: the standard ZLIB1.DLL is NOT compiled using ZLIB_WINAPI.
	*/
#  ifdef ZLIB_WINAPI
#    ifdef FAR
#      undef FAR
#    endif
#    include <windows.h>
	 /* No need for _export, use ZLIB.DEF instead. */
	 /* For complete Windows compatibility, use WINAPI, not __stdcall. */
#    define ZEXPORT WINAPI
#    ifdef WIN32
#      define ZEXPORTVA WINAPIV
#    else
#      define ZEXPORTVA FAR CDECL
#    endif
#  endif
#endif

#if defined (__BEOS__)
#  ifdef ZLIB_DLL
#    ifdef ZLIB_INTERNAL
#      define ZEXPORT   __declspec(dllexport)
#      define ZEXPORTVA __declspec(dllexport)
#    else
#      define ZEXPORT   __declspec(dllimport)
#      define ZEXPORTVA __declspec(dllimport)
#    endif
#  endif
#endif

#ifndef ZEXTERN
#  define ZEXTERN extern
#endif
#ifndef ZEXPORT
#  define ZEXPORT
#endif
#ifndef ZEXPORTVA
#  define ZEXPORTVA
#endif

#ifndef FAR
#  define FAR
#endif

#if !defined(__MACTYPES__)
typedef unsigned char  Byte;  /* 8 bits */
#endif
typedef unsigned int   uInt;  /* 16 bits or more */
typedef unsigned long  uLong; /* 32 bits or more */

#ifdef SMALL_MEDIUM
   /* Borland C/C++ and some old MSC versions ignore FAR inside typedef */
#  define Bytef Byte FAR
#else
   typedef Byte  FAR Bytef;
#endif
typedef char  FAR charf;
typedef int   FAR intf;
typedef uInt  FAR uIntf;
typedef uLong FAR uLongf;

#ifdef STDC
   typedef void const *voidpc;
   typedef void FAR   *voidpf;
   typedef void       *voidp;
#else
   typedef Byte const *voidpc;
   typedef Byte FAR   *voidpf;
   typedef Byte       *voidp;
#endif

#if 0           /* HAVE_UNISTD_H -- this line is updated by ./configure */
#  include <sys/types.h> /* for off_t */
#  include <unistd.h>    /* for SEEK_* and off_t */
#  ifdef VMS
#    include <unixio.h>   /* for off_t */
#  endif
#  define z_off_t off_t
#endif
#ifndef SEEK_SET
#  define SEEK_SET        0       /* Seek from beginning of file.  */
#  define SEEK_CUR        1       /* Seek from current position.  */
#  define SEEK_END        2       /* Set file pointer to EOF plus "offset" */
#endif
#ifndef z_off_t
#  define z_off_t long
#endif

#if defined(__OS400__)
#  define NO_vsnprintf
#endif

#if defined(__MVS__)
#  define NO_vsnprintf
#  ifdef FAR
#    undef FAR
#  endif
#endif

/* MVS linker does not support external names larger than 8 bytes */
#if defined(__MVS__)
#   pragma map(deflateInit_,"DEIN")
#   pragma map(deflateInit2_,"DEIN2")
#   pragma map(deflateEnd,"DEEND")
#   pragma map(deflateBound,"DEBND")
#   pragma map(inflateInit_,"ININ")
#   pragma map(inflateInit2_,"ININ2")
#   pragma map(inflateEnd,"INEND")
#   pragma map(inflateSync,"INSY")
#   pragma map(inflateSetDictionary,"INSEDI")
#   pragma map(compressBound,"CMBND")
#   pragma map(inflate_table,"INTABL")
#   pragma map(inflate_fast,"INFA")
#   pragma map(inflate_copyright,"INCOPY")
#endif

#endif /* ZCONF_H */

/*** End of inlined file: zconf.h ***/

#ifdef __cplusplus
//extern "C" {
#endif

#define ZLIB_VERSION "1.2.3"
#define ZLIB_VERNUM 0x1230

/*
	 The 'zlib' compression library provides in-memory compression and
  decompression functions, including integrity checks of the uncompressed
  data.  This version of the library supports only one compression method
  (deflation) but other algorithms will be added later and will have the same
  stream interface.

	 Compression can be done in a single step if the buffers are large
  enough (for example if an input file is mmap'ed), or can be done by
  repeated calls of the compression function.  In the latter case, the
  application must provide more input and/or consume the output
  (providing more output space) before each call.

	 The compressed data format used by default by the in-memory functions is
  the zlib format, which is a zlib wrapper documented in RFC 1950, wrapped
  around a deflate stream, which is itself documented in RFC 1951.

	 The library also supports reading and writing files in gzip (.gz) format
  with an interface similar to that of stdio using the functions that start
  with "gz".  The gzip format is different from the zlib format.  gzip is a
  gzip wrapper, documented in RFC 1952, wrapped around a deflate stream.

	 This library can optionally read and write gzip streams in memory as well.

	 The zlib format was designed to be compact and fast for use in memory
  and on communications channels.  The gzip format was designed for single-
  file compression on file systems, has a larger header than zlib to maintain
  directory information, and uses a different, slower check method than zlib.

	 The library does not install any signal handler. The decoder checks
  the consistency of the compressed data, so the library should never
  crash even in case of corrupted input.
*/

typedef voidpf (*alloc_func) OF((voidpf opaque, uInt items, uInt size));
typedef void   (*free_func)  OF((voidpf opaque, voidpf address));

struct internal_state;

typedef struct z_stream_s {
	Bytef    *next_in;  /* next input byte */
	uInt     avail_in;  /* number of bytes available at next_in */
	uLong    total_in;  /* total nb of input bytes read so far */

	Bytef    *next_out; /* next output byte should be put there */
	uInt     avail_out; /* remaining free space at next_out */
	uLong    total_out; /* total nb of bytes output so far */

	char     *msg;      /* last error message, NULL if no error */
	struct internal_state FAR *state; /* not visible by applications */

	alloc_func zalloc;  /* used to allocate the internal state */
	free_func  zfree;   /* used to free the internal state */
	voidpf     opaque;  /* private data object passed to zalloc and zfree */

	int     data_type;  /* best guess about the data type: binary or text */
	uLong   adler;      /* adler32 value of the uncompressed data */
	uLong   reserved;   /* reserved for future use */
} z_stream;

typedef z_stream FAR *z_streamp;

/*
	 gzip header information passed to and from zlib routines.  See RFC 1952
  for more details on the meanings of these fields.
*/
typedef struct gz_header_s {
	int     text;       /* true if compressed data believed to be text */
	uLong   time;       /* modification time */
	int     xflags;     /* extra flags (not used when writing a gzip file) */
	int     os;         /* operating system */
	Bytef   *extra;     /* pointer to extra field or Z_NULL if none */
	uInt    extra_len;  /* extra field length (valid if extra != Z_NULL) */
	uInt    extra_max;  /* space at extra (only when reading header) */
	Bytef   *name;      /* pointer to zero-terminated file name or Z_NULL */
	uInt    name_max;   /* space at name (only when reading header) */
	Bytef   *comment;   /* pointer to zero-terminated comment or Z_NULL */
	uInt    comm_max;   /* space at comment (only when reading header) */
	int     hcrc;       /* true if there was or will be a header crc */
	int     done;       /* true when done reading gzip header (not used
						   when writing a gzip file) */
} gz_header;

typedef gz_header FAR *gz_headerp;

/*
   The application must update next_in and avail_in when avail_in has
   dropped to zero. It must update next_out and avail_out when avail_out
   has dropped to zero. The application must initialize zalloc, zfree and
   opaque before calling the init function. All other fields are set by the
   compression library and must not be updated by the application.

   The opaque value provided by the application will be passed as the first
   parameter for calls of zalloc and zfree. This can be useful for custom
   memory management. The compression library attaches no meaning to the
   opaque value.

   zalloc must return Z_NULL if there is not enough memory for the object.
   If zlib is used in a multi-threaded application, zalloc and zfree must be
   thread safe.

   On 16-bit systems, the functions zalloc and zfree must be able to allocate
   exactly 65536 bytes, but will not be required to allocate more than this
   if the symbol MAXSEG_64K is defined (see zconf.h). WARNING: On MSDOS,
   pointers returned by zalloc for objects of exactly 65536 bytes *must*
   have their offset normalized to zero. The default allocation function
   provided by this library ensures this (see zutil.c). To reduce memory
   requirements and avoid any allocation of 64K objects, at the expense of
   compression ratio, compile the library with -DMAX_WBITS=14 (see zconf.h).

   The fields total_in and total_out can be used for statistics or
   progress reports. After compression, total_in holds the total size of
   the uncompressed data and may be saved for use in the decompressor
   (particularly if the decompressor wants to decompress everything in
   a single step).
*/

						/* constants */

#define Z_NO_FLUSH      0
#define Z_PARTIAL_FLUSH 1 /* will be removed, use Z_SYNC_FLUSH instead */
#define Z_SYNC_FLUSH    2
#define Z_FULL_FLUSH    3
#define Z_FINISH        4
#define Z_BLOCK         5
/* Allowed flush values; see deflate() and inflate() below for details */

#define Z_OK            0
#define Z_STREAM_END    1
#define Z_NEED_DICT     2
#define Z_ERRNO        (-1)
#define Z_STREAM_ERROR (-2)
#define Z_DATA_ERROR   (-3)
#define Z_MEM_ERROR    (-4)
#define Z_BUF_ERROR    (-5)
#define Z_VERSION_ERROR (-6)
/* Return codes for the compression/decompression functions. Negative
 * values are errors, positive values are used for special but normal events.
 */

#define Z_NO_COMPRESSION         0
#define Z_BEST_SPEED             1
#define Z_BEST_COMPRESSION       9
#define Z_DEFAULT_COMPRESSION  (-1)
/* compression levels */

#define Z_FILTERED            1
#define Z_HUFFMAN_ONLY        2
#define Z_RLE                 3
#define Z_FIXED               4
#define Z_DEFAULT_STRATEGY    0
/* compression strategy; see deflateInit2() below for details */

#define Z_BINARY   0
#define Z_TEXT     1
#define Z_ASCII    Z_TEXT   /* for compatibility with 1.2.2 and earlier */
#define Z_UNKNOWN  2
/* Possible values of the data_type field (though see inflate()) */

#define Z_DEFLATED   8
/* The deflate compression method (the only one supported in this version) */

#define Z_NULL  0  /* for initializing zalloc, zfree, opaque */

#define zlib_version zlibVersion()
/* for compatibility with versions < 1.0.2 */

						/* basic functions */

//ZEXTERN const char * ZEXPORT zlibVersion OF((void));
/* The application can compare zlibVersion and ZLIB_VERSION for consistency.
   If the first character differs, the library code actually used is
   not compatible with the zlib.h header file used by the application.
   This check is automatically made by deflateInit and inflateInit.
 */

/*
ZEXTERN int ZEXPORT deflateInit OF((z_streamp strm, int level));

	 Initializes the internal stream state for compression. The fields
   zalloc, zfree and opaque must be initialized before by the caller.
   If zalloc and zfree are set to Z_NULL, deflateInit updates them to
   use default allocation functions.

	 The compression level must be Z_DEFAULT_COMPRESSION, or between 0 and 9:
   1 gives best speed, 9 gives best compression, 0 gives no compression at
   all (the input data is simply copied a block at a time).
   Z_DEFAULT_COMPRESSION requests a default compromise between speed and
   compression (currently equivalent to level 6).

	 deflateInit returns Z_OK if success, Z_MEM_ERROR if there was not
   enough memory, Z_STREAM_ERROR if level is not a valid compression level,
   Z_VERSION_ERROR if the zlib library version (zlib_version) is incompatible
   with the version assumed by the caller (ZLIB_VERSION).
   msg is set to null if there is no error message.  deflateInit does not
   perform any compression: this will be done by deflate().
*/

ZEXTERN int ZEXPORT deflate OF((z_streamp strm, int flush));
/*
	deflate compresses as much data as possible, and stops when the input
  buffer becomes empty or the output buffer becomes full. It may introduce some
  output latency (reading input without producing any output) except when
  forced to flush.

	The detailed semantics are as follows. deflate performs one or both of the
  following actions:

  - Compress more input starting at next_in and update next_in and avail_in
	accordingly. If not all input can be processed (because there is not
	enough room in the output buffer), next_in and avail_in are updated and
	processing will resume at this point for the next call of deflate().

  - Provide more output starting at next_out and update next_out and avail_out
	accordingly. This action is forced if the parameter flush is non zero.
	Forcing flush frequently degrades the compression ratio, so this parameter
	should be set only when necessary (in interactive applications).
	Some output may be provided even if flush is not set.

  Before the call of deflate(), the application should ensure that at least
  one of the actions is possible, by providing more input and/or consuming
  more output, and updating avail_in or avail_out accordingly; avail_out
  should never be zero before the call. The application can consume the
  compressed output when it wants, for example when the output buffer is full
  (avail_out == 0), or after each call of deflate(). If deflate returns Z_OK
  and with zero avail_out, it must be called again after making room in the
  output buffer because there might be more output pending.

	Normally the parameter flush is set to Z_NO_FLUSH, which allows deflate to
  decide how much data to accumualte before producing output, in order to
  maximize compression.

	If the parameter flush is set to Z_SYNC_FLUSH, all pending output is
  flushed to the output buffer and the output is aligned on a byte boundary, so
  that the decompressor can get all input data available so far. (In particular
  avail_in is zero after the call if enough output space has been provided
  before the call.)  Flushing may degrade compression for some compression
  algorithms and so it should be used only when necessary.

	If flush is set to Z_FULL_FLUSH, all output is flushed as with
  Z_SYNC_FLUSH, and the compression state is reset so that decompression can
  restart from this point if previous compressed data has been damaged or if
  random access is desired. Using Z_FULL_FLUSH too often can seriously degrade
  compression.

	If deflate returns with avail_out == 0, this function must be called again
  with the same value of the flush parameter and more output space (updated
  avail_out), until the flush is complete (deflate returns with non-zero
  avail_out). In the case of a Z_FULL_FLUSH or Z_SYNC_FLUSH, make sure that
  avail_out is greater than six to avoid repeated flush markers due to
  avail_out == 0 on return.

	If the parameter flush is set to Z_FINISH, pending input is processed,
  pending output is flushed and deflate returns with Z_STREAM_END if there
  was enough output space; if deflate returns with Z_OK, this function must be
  called again with Z_FINISH and more output space (updated avail_out) but no
  more input data, until it returns with Z_STREAM_END or an error. After
  deflate has returned Z_STREAM_END, the only possible operations on the
  stream are deflateReset or deflateEnd.

	Z_FINISH can be used immediately after deflateInit if all the compression
  is to be done in a single step. In this case, avail_out must be at least
  the value returned by deflateBound (see below). If deflate does not return
  Z_STREAM_END, then it must be called again as described above.

	deflate() sets strm->adler to the adler32 checksum of all input read
  so far (that is, total_in bytes).

	deflate() may update strm->data_type if it can make a good guess about
  the input data type (Z_BINARY or Z_TEXT). In doubt, the data is considered
  binary. This field is only for information purposes and does not affect
  the compression algorithm in any manner.

	deflate() returns Z_OK if some progress has been made (more input
  processed or more output produced), Z_STREAM_END if all input has been
  consumed and all output has been produced (only when flush is set to
  Z_FINISH), Z_STREAM_ERROR if the stream state was inconsistent (for example
  if next_in or next_out was NULL), Z_BUF_ERROR if no progress is possible
  (for example avail_in or avail_out was zero). Note that Z_BUF_ERROR is not
  fatal, and deflate() can be called again with more input and more output
  space to continue compressing.
*/

ZEXTERN int ZEXPORT deflateEnd OF((z_streamp strm));
/*
	 All dynamically allocated data structures for this stream are freed.
   This function discards any unprocessed input and does not flush any
   pending output.

	 deflateEnd returns Z_OK if success, Z_STREAM_ERROR if the
   stream state was inconsistent, Z_DATA_ERROR if the stream was freed
   prematurely (some input or output was discarded). In the error case,
   msg may be set but then points to a static string (which must not be
   deallocated).
*/

/*
ZEXTERN int ZEXPORT inflateInit OF((z_streamp strm));

	 Initializes the internal stream state for decompression. The fields
   next_in, avail_in, zalloc, zfree and opaque must be initialized before by
   the caller. If next_in is not Z_NULL and avail_in is large enough (the exact
   value depends on the compression method), inflateInit determines the
   compression method from the zlib header and allocates all data structures
   accordingly; otherwise the allocation will be deferred to the first call of
   inflate.  If zalloc and zfree are set to Z_NULL, inflateInit updates them to
   use default allocation functions.

	 inflateInit returns Z_OK if success, Z_MEM_ERROR if there was not enough
   memory, Z_VERSION_ERROR if the zlib library version is incompatible with the
   version assumed by the caller.  msg is set to null if there is no error
   message. inflateInit does not perform any decompression apart from reading
   the zlib header if present: this will be done by inflate().  (So next_in and
   avail_in may be modified, but next_out and avail_out are unchanged.)
*/

ZEXTERN int ZEXPORT inflate OF((z_streamp strm, int flush));
/*
	inflate decompresses as much data as possible, and stops when the input
  buffer becomes empty or the output buffer becomes full. It may introduce
  some output latency (reading input without producing any output) except when
  forced to flush.

  The detailed semantics are as follows. inflate performs one or both of the
  following actions:

  - Decompress more input starting at next_in and update next_in and avail_in
	accordingly. If not all input can be processed (because there is not
	enough room in the output buffer), next_in is updated and processing
	will resume at this point for the next call of inflate().

  - Provide more output starting at next_out and update next_out and avail_out
	accordingly.  inflate() provides as much output as possible, until there
	is no more input data or no more space in the output buffer (see below
	about the flush parameter).

  Before the call of inflate(), the application should ensure that at least
  one of the actions is possible, by providing more input and/or consuming
  more output, and updating the next_* and avail_* values accordingly.
  The application can consume the uncompressed output when it wants, for
  example when the output buffer is full (avail_out == 0), or after each
  call of inflate(). If inflate returns Z_OK and with zero avail_out, it
  must be called again after making room in the output buffer because there
  might be more output pending.

	The flush parameter of inflate() can be Z_NO_FLUSH, Z_SYNC_FLUSH,
  Z_FINISH, or Z_BLOCK. Z_SYNC_FLUSH requests that inflate() flush as much
  output as possible to the output buffer. Z_BLOCK requests that inflate() stop
  if and when it gets to the next deflate block boundary. When decoding the
  zlib or gzip format, this will cause inflate() to return immediately after
  the header and before the first block. When doing a raw inflate, inflate()
  will go ahead and process the first block, and will return when it gets to
  the end of that block, or when it runs out of data.

	The Z_BLOCK option assists in appending to or combining deflate streams.
  Also to assist in this, on return inflate() will set strm->data_type to the
  number of unused bits in the last byte taken from strm->next_in, plus 64
  if inflate() is currently decoding the last block in the deflate stream,
  plus 128 if inflate() returned immediately after decoding an end-of-block
  code or decoding the complete header up to just before the first byte of the
  deflate stream. The end-of-block will not be indicated until all of the
  uncompressed data from that block has been written to strm->next_out.  The
  number of unused bits may in general be greater than seven, except when
  bit 7 of data_type is set, in which case the number of unused bits will be
  less than eight.

	inflate() should normally be called until it returns Z_STREAM_END or an
  error. However if all decompression is to be performed in a single step
  (a single call of inflate), the parameter flush should be set to
  Z_FINISH. In this case all pending input is processed and all pending
  output is flushed; avail_out must be large enough to hold all the
  uncompressed data. (The size of the uncompressed data may have been saved
  by the compressor for this purpose.) The next operation on this stream must
  be inflateEnd to deallocate the decompression state. The use of Z_FINISH
  is never required, but can be used to inform inflate that a faster approach
  may be used for the single inflate() call.

	 In this implementation, inflate() always flushes as much output as
  possible to the output buffer, and always uses the faster approach on the
  first call. So the only effect of the flush parameter in this implementation
  is on the return value of inflate(), as noted below, or when it returns early
  because Z_BLOCK is used.

	 If a preset dictionary is needed after this call (see inflateSetDictionary
  below), inflate sets strm->adler to the adler32 checksum of the dictionary
  chosen by the compressor and returns Z_NEED_DICT; otherwise it sets
  strm->adler to the adler32 checksum of all output produced so far (that is,
  total_out bytes) and returns Z_OK, Z_STREAM_END or an error code as described
  below. At the end of the stream, inflate() checks that its computed adler32
  checksum is equal to that saved by the compressor and returns Z_STREAM_END
  only if the checksum is correct.

	inflate() will decompress and check either zlib-wrapped or gzip-wrapped
  deflate data.  The header type is detected automatically.  Any information
  contained in the gzip header is not retained, so applications that need that
  information should instead use raw inflate, see inflateInit2() below, or
  inflateBack() and perform their own processing of the gzip header and
  trailer.

	inflate() returns Z_OK if some progress has been made (more input processed
  or more output produced), Z_STREAM_END if the end of the compressed data has
  been reached and all uncompressed output has been produced, Z_NEED_DICT if a
  preset dictionary is needed at this point, Z_DATA_ERROR if the input data was
  corrupted (input stream not conforming to the zlib format or incorrect check
  value), Z_STREAM_ERROR if the stream structure was inconsistent (for example
  if next_in or next_out was NULL), Z_MEM_ERROR if there was not enough memory,
  Z_BUF_ERROR if no progress is possible or if there was not enough room in the
  output buffer when Z_FINISH is used. Note that Z_BUF_ERROR is not fatal, and
  inflate() can be called again with more input and more output space to
  continue decompressing. If Z_DATA_ERROR is returned, the application may then
  call inflateSync() to look for a good compression block if a partial recovery
  of the data is desired.
*/

ZEXTERN int ZEXPORT inflateEnd OF((z_streamp strm));
/*
	 All dynamically allocated data structures for this stream are freed.
   This function discards any unprocessed input and does not flush any
   pending output.

	 inflateEnd returns Z_OK if success, Z_STREAM_ERROR if the stream state
   was inconsistent. In the error case, msg may be set but then points to a
   static string (which must not be deallocated).
*/

						/* Advanced functions */

/*
	The following functions are needed only in some special applications.
*/

/*
ZEXTERN int ZEXPORT deflateInit2 OF((z_streamp strm,
									 int  level,
									 int  method,
									 int  windowBits,
									 int  memLevel,
									 int  strategy));

	 This is another version of deflateInit with more compression options. The
   fields next_in, zalloc, zfree and opaque must be initialized before by
   the caller.

	 The method parameter is the compression method. It must be Z_DEFLATED in
   this version of the library.

	 The windowBits parameter is the base two logarithm of the window size
   (the size of the history buffer). It should be in the range 8..15 for this
   version of the library. Larger values of this parameter result in better
   compression at the expense of memory usage. The default value is 15 if
   deflateInit is used instead.

	 windowBits can also be -8..-15 for raw deflate. In this case, -windowBits
   determines the window size. deflate() will then generate raw deflate data
   with no zlib header or trailer, and will not compute an adler32 check value.

	 windowBits can also be greater than 15 for optional gzip encoding. Add
   16 to windowBits to write a simple gzip header and trailer around the
   compressed data instead of a zlib wrapper. The gzip header will have no
   file name, no extra data, no comment, no modification time (set to zero),
   no header crc, and the operating system will be set to 255 (unknown).  If a
   gzip stream is being written, strm->adler is a crc32 instead of an adler32.

	 The memLevel parameter specifies how much memory should be allocated
   for the internal compression state. memLevel=1 uses minimum memory but
   is slow and reduces compression ratio; memLevel=9 uses maximum memory
   for optimal speed. The default value is 8. See zconf.h for total memory
   usage as a function of windowBits and memLevel.

	 The strategy parameter is used to tune the compression algorithm. Use the
   value Z_DEFAULT_STRATEGY for normal data, Z_FILTERED for data produced by a
   filter (or predictor), Z_HUFFMAN_ONLY to force Huffman encoding only (no
   string match), or Z_RLE to limit match distances to one (run-length
   encoding). Filtered data consists mostly of small values with a somewhat
   random distribution. In this case, the compression algorithm is tuned to
   compress them better. The effect of Z_FILTERED is to force more Huffman
   coding and less string matching; it is somewhat intermediate between
   Z_DEFAULT and Z_HUFFMAN_ONLY. Z_RLE is designed to be almost as fast as
   Z_HUFFMAN_ONLY, but give better compression for PNG image data. The strategy
   parameter only affects the compression ratio but not the correctness of the
   compressed output even if it is not set appropriately.  Z_FIXED prevents the
   use of dynamic Huffman codes, allowing for a simpler decoder for special
   applications.

	  deflateInit2 returns Z_OK if success, Z_MEM_ERROR if there was not enough
   memory, Z_STREAM_ERROR if a parameter is invalid (such as an invalid
   method). msg is set to null if there is no error message.  deflateInit2 does
   not perform any compression: this will be done by deflate().
*/

ZEXTERN int ZEXPORT deflateSetDictionary OF((z_streamp strm,
											 const Bytef *dictionary,
											 uInt  dictLength));
/*
	 Initializes the compression dictionary from the given byte sequence
   without producing any compressed output. This function must be called
   immediately after deflateInit, deflateInit2 or deflateReset, before any
   call of deflate. The compressor and decompressor must use exactly the same
   dictionary (see inflateSetDictionary).

	 The dictionary should consist of strings (byte sequences) that are likely
   to be encountered later in the data to be compressed, with the most commonly
   used strings preferably put towards the end of the dictionary. Using a
   dictionary is most useful when the data to be compressed is short and can be
   predicted with good accuracy; the data can then be compressed better than
   with the default empty dictionary.

	 Depending on the size of the compression data structures selected by
   deflateInit or deflateInit2, a part of the dictionary may in effect be
   discarded, for example if the dictionary is larger than the window size in
   deflate or deflate2. Thus the strings most likely to be useful should be
   put at the end of the dictionary, not at the front. In addition, the
   current implementation of deflate will use at most the window size minus
   262 bytes of the provided dictionary.

	 Upon return of this function, strm->adler is set to the adler32 value
   of the dictionary; the decompressor may later use this value to determine
   which dictionary has been used by the compressor. (The adler32 value
   applies to the whole dictionary even if only a subset of the dictionary is
   actually used by the compressor.) If a raw deflate was requested, then the
   adler32 value is not computed and strm->adler is not set.

	 deflateSetDictionary returns Z_OK if success, or Z_STREAM_ERROR if a
   parameter is invalid (such as NULL dictionary) or the stream state is
   inconsistent (for example if deflate has already been called for this stream
   or if the compression method is bsort). deflateSetDictionary does not
   perform any compression: this will be done by deflate().
*/

ZEXTERN int ZEXPORT deflateCopy OF((z_streamp dest,
									z_streamp source));
/*
	 Sets the destination stream as a complete copy of the source stream.

	 This function can be useful when several compression strategies will be
   tried, for example when there are several ways of pre-processing the input
   data with a filter. The streams that will be discarded should then be freed
   by calling deflateEnd.  Note that deflateCopy duplicates the internal
   compression state which can be quite large, so this strategy is slow and
   can consume lots of memory.

	 deflateCopy returns Z_OK if success, Z_MEM_ERROR if there was not
   enough memory, Z_STREAM_ERROR if the source stream state was inconsistent
   (such as zalloc being NULL). msg is left unchanged in both source and
   destination.
*/

ZEXTERN int ZEXPORT deflateReset OF((z_streamp strm));
/*
	 This function is equivalent to deflateEnd followed by deflateInit,
   but does not free and reallocate all the internal compression state.
   The stream will keep the same compression level and any other attributes
   that may have been set by deflateInit2.

	  deflateReset returns Z_OK if success, or Z_STREAM_ERROR if the source
   stream state was inconsistent (such as zalloc or state being NULL).
*/

ZEXTERN int ZEXPORT deflateParams OF((z_streamp strm,
									  int level,
									  int strategy));
/*
	 Dynamically update the compression level and compression strategy.  The
   interpretation of level and strategy is as in deflateInit2.  This can be
   used to switch between compression and straight copy of the input data, or
   to switch to a different kind of input data requiring a different
   strategy. If the compression level is changed, the input available so far
   is compressed with the old level (and may be flushed); the new level will
   take effect only at the next call of deflate().

	 Before the call of deflateParams, the stream state must be set as for
   a call of deflate(), since the currently available input may have to
   be compressed and flushed. In particular, strm->avail_out must be non-zero.

	 deflateParams returns Z_OK if success, Z_STREAM_ERROR if the source
   stream state was inconsistent or if a parameter was invalid, Z_BUF_ERROR
   if strm->avail_out was zero.
*/

ZEXTERN int ZEXPORT deflateTune OF((z_streamp strm,
									int good_length,
									int max_lazy,
									int nice_length,
									int max_chain));
/*
	 Fine tune deflate's internal compression parameters.  This should only be
   used by someone who understands the algorithm used by zlib's deflate for
   searching for the best matching string, and even then only by the most
   fanatic optimizer trying to squeeze out the last compressed bit for their
   specific input data.  Read the deflate.c source code for the meaning of the
   max_lazy, good_length, nice_length, and max_chain parameters.

	 deflateTune() can be called after deflateInit() or deflateInit2(), and
   returns Z_OK on success, or Z_STREAM_ERROR for an invalid deflate stream.
 */

ZEXTERN uLong ZEXPORT deflateBound OF((z_streamp strm,
									   uLong sourceLen));
/*
	 deflateBound() returns an upper bound on the compressed size after
   deflation of sourceLen bytes.  It must be called after deflateInit()
   or deflateInit2().  This would be used to allocate an output buffer
   for deflation in a single pass, and so would be called before deflate().
*/

ZEXTERN int ZEXPORT deflatePrime OF((z_streamp strm,
									 int bits,
									 int value));
/*
	 deflatePrime() inserts bits in the deflate output stream.  The intent
  is that this function is used to start off the deflate output with the
  bits leftover from a previous deflate stream when appending to it.  As such,
  this function can only be used for raw deflate, and must be used before the
  first deflate() call after a deflateInit2() or deflateReset().  bits must be
  less than or equal to 16, and that many of the least significant bits of
  value will be inserted in the output.

	  deflatePrime returns Z_OK if success, or Z_STREAM_ERROR if the source
   stream state was inconsistent.
*/

ZEXTERN int ZEXPORT deflateSetHeader OF((z_streamp strm,
										 gz_headerp head));
/*
	  deflateSetHeader() provides gzip header information for when a gzip
   stream is requested by deflateInit2().  deflateSetHeader() may be called
   after deflateInit2() or deflateReset() and before the first call of
   deflate().  The text, time, os, extra field, name, and comment information
   in the provided gz_header structure are written to the gzip header (xflag is
   ignored -- the extra flags are set according to the compression level).  The
   caller must assure that, if not Z_NULL, name and comment are terminated with
   a zero byte, and that if extra is not Z_NULL, that extra_len bytes are
   available there.  If hcrc is true, a gzip header crc is included.  Note that
   the current versions of the command-line version of gzip (up through version
   1.3.x) do not support header crc's, and will report that it is a "multi-part
   gzip file" and give up.

	  If deflateSetHeader is not used, the default gzip header has text false,
   the time set to zero, and os set to 255, with no extra, name, or comment
   fields.  The gzip header is returned to the default state by deflateReset().

	  deflateSetHeader returns Z_OK if success, or Z_STREAM_ERROR if the source
   stream state was inconsistent.
*/

/*
ZEXTERN int ZEXPORT inflateInit2 OF((z_streamp strm,
									 int  windowBits));

	 This is another version of inflateInit with an extra parameter. The
   fields next_in, avail_in, zalloc, zfree and opaque must be initialized
   before by the caller.

	 The windowBits parameter is the base two logarithm of the maximum window
   size (the size of the history buffer).  It should be in the range 8..15 for
   this version of the library. The default value is 15 if inflateInit is used
   instead. windowBits must be greater than or equal to the windowBits value
   provided to deflateInit2() while compressing, or it must be equal to 15 if
   deflateInit2() was not used. If a compressed stream with a larger window
   size is given as input, inflate() will return with the error code
   Z_DATA_ERROR instead of trying to allocate a larger window.

	 windowBits can also be -8..-15 for raw inflate. In this case, -windowBits
   determines the window size. inflate() will then process raw deflate data,
   not looking for a zlib or gzip header, not generating a check value, and not
   looking for any check values for comparison at the end of the stream. This
   is for use with other formats that use the deflate compressed data format
   such as zip.  Those formats provide their own check values. If a custom
   format is developed using the raw deflate format for compressed data, it is
   recommended that a check value such as an adler32 or a crc32 be applied to
   the uncompressed data as is done in the zlib, gzip, and zip formats.  For
   most applications, the zlib format should be used as is. Note that comments
   above on the use in deflateInit2() applies to the magnitude of windowBits.

	 windowBits can also be greater than 15 for optional gzip decoding. Add
   32 to windowBits to enable zlib and gzip decoding with automatic header
   detection, or add 16 to decode only the gzip format (the zlib format will
   return a Z_DATA_ERROR).  If a gzip stream is being decoded, strm->adler is
   a crc32 instead of an adler32.

	 inflateInit2 returns Z_OK if success, Z_MEM_ERROR if there was not enough
   memory, Z_STREAM_ERROR if a parameter is invalid (such as a null strm). msg
   is set to null if there is no error message.  inflateInit2 does not perform
   any decompression apart from reading the zlib header if present: this will
   be done by inflate(). (So next_in and avail_in may be modified, but next_out
   and avail_out are unchanged.)
*/

ZEXTERN int ZEXPORT inflateSetDictionary OF((z_streamp strm,
											 const Bytef *dictionary,
											 uInt  dictLength));
/*
	 Initializes the decompression dictionary from the given uncompressed byte
   sequence. This function must be called immediately after a call of inflate,
   if that call returned Z_NEED_DICT. The dictionary chosen by the compressor
   can be determined from the adler32 value returned by that call of inflate.
   The compressor and decompressor must use exactly the same dictionary (see
   deflateSetDictionary).  For raw inflate, this function can be called
   immediately after inflateInit2() or inflateReset() and before any call of
   inflate() to set the dictionary.  The application must insure that the
   dictionary that was used for compression is provided.

	 inflateSetDictionary returns Z_OK if success, Z_STREAM_ERROR if a
   parameter is invalid (such as NULL dictionary) or the stream state is
   inconsistent, Z_DATA_ERROR if the given dictionary doesn't match the
   expected one (incorrect adler32 value). inflateSetDictionary does not
   perform any decompression: this will be done by subsequent calls of
   inflate().
*/

ZEXTERN int ZEXPORT inflateSync OF((z_streamp strm));
/*
	Skips invalid compressed data until a full flush point (see above the
  description of deflate with Z_FULL_FLUSH) can be found, or until all
  available input is skipped. No output is provided.

	inflateSync returns Z_OK if a full flush point has been found, Z_BUF_ERROR
  if no more input was provided, Z_DATA_ERROR if no flush point has been found,
  or Z_STREAM_ERROR if the stream structure was inconsistent. In the success
  case, the application may save the current current value of total_in which
  indicates where valid compressed data was found. In the error case, the
  application may repeatedly call inflateSync, providing more input each time,
  until success or end of the input data.
*/

ZEXTERN int ZEXPORT inflateCopy OF((z_streamp dest,
									z_streamp source));
/*
	 Sets the destination stream as a complete copy of the source stream.

	 This function can be useful when randomly accessing a large stream.  The
   first pass through the stream can periodically record the inflate state,
   allowing restarting inflate at those points when randomly accessing the
   stream.

	 inflateCopy returns Z_OK if success, Z_MEM_ERROR if there was not
   enough memory, Z_STREAM_ERROR if the source stream state was inconsistent
   (such as zalloc being NULL). msg is left unchanged in both source and
   destination.
*/

ZEXTERN int ZEXPORT inflateReset OF((z_streamp strm));
/*
	 This function is equivalent to inflateEnd followed by inflateInit,
   but does not free and reallocate all the internal decompression state.
   The stream will keep attributes that may have been set by inflateInit2.

	  inflateReset returns Z_OK if success, or Z_STREAM_ERROR if the source
   stream state was inconsistent (such as zalloc or state being NULL).
*/

ZEXTERN int ZEXPORT inflatePrime OF((z_streamp strm,
									 int bits,
									 int value));
/*
	 This function inserts bits in the inflate input stream.  The intent is
  that this function is used to start inflating at a bit position in the
  middle of a byte.  The provided bits will be used before any bytes are used
  from next_in.  This function should only be used with raw inflate, and
  should be used before the first inflate() call after inflateInit2() or
  inflateReset().  bits must be less than or equal to 16, and that many of the
  least significant bits of value will be inserted in the input.

	  inflatePrime returns Z_OK if success, or Z_STREAM_ERROR if the source
   stream state was inconsistent.
*/

ZEXTERN int ZEXPORT inflateGetHeader OF((z_streamp strm,
										 gz_headerp head));
/*
	  inflateGetHeader() requests that gzip header information be stored in the
   provided gz_header structure.  inflateGetHeader() may be called after
   inflateInit2() or inflateReset(), and before the first call of inflate().
   As inflate() processes the gzip stream, head->done is zero until the header
   is completed, at which time head->done is set to one.  If a zlib stream is
   being decoded, then head->done is set to -1 to indicate that there will be
   no gzip header information forthcoming.  Note that Z_BLOCK can be used to
   force inflate() to return immediately after header processing is complete
   and before any actual data is decompressed.

	  The text, time, xflags, and os fields are filled in with the gzip header
   contents.  hcrc is set to true if there is a header CRC.  (The header CRC
   was valid if done is set to one.)  If extra is not Z_NULL, then extra_max
   contains the maximum number of bytes to write to extra.  Once done is true,
   extra_len contains the actual extra field length, and extra contains the
   extra field, or that field truncated if extra_max is less than extra_len.
   If name is not Z_NULL, then up to name_max characters are written there,
   terminated with a zero unless the length is greater than name_max.  If
   comment is not Z_NULL, then up to comm_max characters are written there,
   terminated with a zero unless the length is greater than comm_max.  When
   any of extra, name, or comment are not Z_NULL and the respective field is
   not present in the header, then that field is set to Z_NULL to signal its
   absence.  This allows the use of deflateSetHeader() with the returned
   structure to duplicate the header.  However if those fields are set to
   allocated memory, then the application will need to save those pointers
   elsewhere so that they can be eventually freed.

	  If inflateGetHeader is not used, then the header information is simply
   discarded.  The header is always checked for validity, including the header
   CRC if present.  inflateReset() will reset the process to discard the header
   information.  The application would need to call inflateGetHeader() again to
   retrieve the header from the next gzip stream.

	  inflateGetHeader returns Z_OK if success, or Z_STREAM_ERROR if the source
   stream state was inconsistent.
*/

/*
ZEXTERN int ZEXPORT inflateBackInit OF((z_streamp strm, int windowBits,
										unsigned char FAR *window));

	 Initialize the internal stream state for decompression using inflateBack()
   calls.  The fields zalloc, zfree and opaque in strm must be initialized
   before the call.  If zalloc and zfree are Z_NULL, then the default library-
   derived memory allocation routines are used.  windowBits is the base two
   logarithm of the window size, in the range 8..15.  window is a caller
   supplied buffer of that size.  Except for special applications where it is
   assured that deflate was used with small window sizes, windowBits must be 15
   and a 32K byte window must be supplied to be able to decompress general
   deflate streams.

	 See inflateBack() for the usage of these routines.

	 inflateBackInit will return Z_OK on success, Z_STREAM_ERROR if any of
   the paramaters are invalid, Z_MEM_ERROR if the internal state could not
   be allocated, or Z_VERSION_ERROR if the version of the library does not
   match the version of the header file.
*/

typedef unsigned (*in_func) OF((void FAR *, unsigned char FAR * FAR *));
typedef int (*out_func) OF((void FAR *, unsigned char FAR *, unsigned));

ZEXTERN int ZEXPORT inflateBack OF((z_streamp strm,
									in_func in, void FAR *in_desc,
									out_func out, void FAR *out_desc));
/*
	 inflateBack() does a raw inflate with a single call using a call-back
   interface for input and output.  This is more efficient than inflate() for
   file i/o applications in that it avoids copying between the output and the
   sliding window by simply making the window itself the output buffer.  This
   function trusts the application to not change the output buffer passed by
   the output function, at least until inflateBack() returns.

	 inflateBackInit() must be called first to allocate the internal state
   and to initialize the state with the user-provided window buffer.
   inflateBack() may then be used multiple times to inflate a complete, raw
   deflate stream with each call.  inflateBackEnd() is then called to free
   the allocated state.

	 A raw deflate stream is one with no zlib or gzip header or trailer.
   This routine would normally be used in a utility that reads zip or gzip
   files and writes out uncompressed files.  The utility would decode the
   header and process the trailer on its own, hence this routine expects
   only the raw deflate stream to decompress.  This is different from the
   normal behavior of inflate(), which expects either a zlib or gzip header and
   trailer around the deflate stream.

	 inflateBack() uses two subroutines supplied by the caller that are then
   called by inflateBack() for input and output.  inflateBack() calls those
   routines until it reads a complete deflate stream and writes out all of the
   uncompressed data, or until it encounters an error.  The function's
   parameters and return types are defined above in the in_func and out_func
   typedefs.  inflateBack() will call in(in_desc, &buf) which should return the
   number of bytes of provided input, and a pointer to that input in buf.  If
   there is no input available, in() must return zero--buf is ignored in that
   case--and inflateBack() will return a buffer error.  inflateBack() will call
   out(out_desc, buf, len) to write the uncompressed data buf[0..len-1].  out()
   should return zero on success, or non-zero on failure.  If out() returns
   non-zero, inflateBack() will return with an error.  Neither in() nor out()
   are permitted to change the contents of the window provided to
   inflateBackInit(), which is also the buffer that out() uses to write from.
   The length written by out() will be at most the window size.  Any non-zero
   amount of input may be provided by in().

	 For convenience, inflateBack() can be provided input on the first call by
   setting strm->next_in and strm->avail_in.  If that input is exhausted, then
   in() will be called.  Therefore strm->next_in must be initialized before
   calling inflateBack().  If strm->next_in is Z_NULL, then in() will be called
   immediately for input.  If strm->next_in is not Z_NULL, then strm->avail_in
   must also be initialized, and then if strm->avail_in is not zero, input will
   initially be taken from strm->next_in[0 .. strm->avail_in - 1].

	 The in_desc and out_desc parameters of inflateBack() is passed as the
   first parameter of in() and out() respectively when they are called.  These
   descriptors can be optionally used to pass any information that the caller-
   supplied in() and out() functions need to do their job.

	 On return, inflateBack() will set strm->next_in and strm->avail_in to
   pass back any unused input that was provided by the last in() call.  The
   return values of inflateBack() can be Z_STREAM_END on success, Z_BUF_ERROR
   if in() or out() returned an error, Z_DATA_ERROR if there was a format
   error in the deflate stream (in which case strm->msg is set to indicate the
   nature of the error), or Z_STREAM_ERROR if the stream was not properly
   initialized.  In the case of Z_BUF_ERROR, an input or output error can be
   distinguished using strm->next_in which will be Z_NULL only if in() returned
   an error.  If strm->next is not Z_NULL, then the Z_BUF_ERROR was due to
   out() returning non-zero.  (in() will always be called before out(), so
   strm->next_in is assured to be defined if out() returns non-zero.)  Note
   that inflateBack() cannot return Z_OK.
*/

ZEXTERN int ZEXPORT inflateBackEnd OF((z_streamp strm));
/*
	 All memory allocated by inflateBackInit() is freed.

	 inflateBackEnd() returns Z_OK on success, or Z_STREAM_ERROR if the stream
   state was inconsistent.
*/

//ZEXTERN uLong ZEXPORT zlibCompileFlags OF((void));
/* Return flags indicating compile-time options.

	Type sizes, two bits each, 00 = 16 bits, 01 = 32, 10 = 64, 11 = other:
	 1.0: size of uInt
	 3.2: size of uLong
	 5.4: size of voidpf (pointer)
	 7.6: size of z_off_t

	Compiler, assembler, and debug options:
	 8: DEBUG
	 9: ASMV or ASMINF -- use ASM code
	 10: ZLIB_WINAPI -- exported functions use the WINAPI calling convention
	 11: 0 (reserved)

	One-time table building (smaller code, but not thread-safe if true):
	 12: BUILDFIXED -- build static block decoding tables when needed
	 13: DYNAMIC_CRC_TABLE -- build CRC calculation tables when needed
	 14,15: 0 (reserved)

	Library content (indicates missing functionality):
	 16: NO_GZCOMPRESS -- gz* functions cannot compress (to avoid linking
						  deflate code when not needed)
	 17: NO_GZIP -- deflate can't write gzip streams, and inflate can't detect
					and decode gzip streams (to avoid linking crc code)
	 18-19: 0 (reserved)

	Operation variations (changes in library functionality):
	 20: PKZIP_BUG_WORKAROUND -- slightly more permissive inflate
	 21: FASTEST -- deflate algorithm with only one, lowest compression level
	 22,23: 0 (reserved)

	The sprintf variant used by gzprintf (zero is best):
	 24: 0 = vs*, 1 = s* -- 1 means limited to 20 arguments after the format
	 25: 0 = *nprintf, 1 = *printf -- 1 means gzprintf() not secure!
	 26: 0 = returns value, 1 = void -- 1 means inferred string length returned

	Remainder:
	 27-31: 0 (reserved)
 */

						/* utility functions */

/*
	 The following utility functions are implemented on top of the
   basic stream-oriented functions. To simplify the interface, some
   default options are assumed (compression level and memory usage,
   standard memory allocation functions). The source code of these
   utility functions can easily be modified if you need special options.
*/

ZEXTERN int ZEXPORT compress OF((Bytef *dest,   uLongf *destLen,
								 const Bytef *source, uLong sourceLen));
/*
	 Compresses the source buffer into the destination buffer.  sourceLen is
   the byte length of the source buffer. Upon entry, destLen is the total
   size of the destination buffer, which must be at least the value returned
   by compressBound(sourceLen). Upon exit, destLen is the actual size of the
   compressed buffer.
	 This function can be used to compress a whole file at once if the
   input file is mmap'ed.
	 compress returns Z_OK if success, Z_MEM_ERROR if there was not
   enough memory, Z_BUF_ERROR if there was not enough room in the output
   buffer.
*/

ZEXTERN int ZEXPORT compress2 OF((Bytef *dest,   uLongf *destLen,
								  const Bytef *source, uLong sourceLen,
								  int level));
/*
	 Compresses the source buffer into the destination buffer. The level
   parameter has the same meaning as in deflateInit.  sourceLen is the byte
   length of the source buffer. Upon entry, destLen is the total size of the
   destination buffer, which must be at least the value returned by
   compressBound(sourceLen). Upon exit, destLen is the actual size of the
   compressed buffer.

	 compress2 returns Z_OK if success, Z_MEM_ERROR if there was not enough
   memory, Z_BUF_ERROR if there was not enough room in the output buffer,
   Z_STREAM_ERROR if the level parameter is invalid.
*/

ZEXTERN uLong ZEXPORT compressBound OF((uLong sourceLen));
/*
	 compressBound() returns an upper bound on the compressed size after
   compress() or compress2() on sourceLen bytes.  It would be used before
   a compress() or compress2() call to allocate the destination buffer.
*/

ZEXTERN int ZEXPORT uncompress OF((Bytef *dest,   uLongf *destLen,
								   const Bytef *source, uLong sourceLen));
/*
	 Decompresses the source buffer into the destination buffer.  sourceLen is
   the byte length of the source buffer. Upon entry, destLen is the total
   size of the destination buffer, which must be large enough to hold the
   entire uncompressed data. (The size of the uncompressed data must have
   been saved previously by the compressor and transmitted to the decompressor
   by some mechanism outside the scope of this compression library.)
   Upon exit, destLen is the actual size of the compressed buffer.
	 This function can be used to decompress a whole file at once if the
   input file is mmap'ed.

	 uncompress returns Z_OK if success, Z_MEM_ERROR if there was not
   enough memory, Z_BUF_ERROR if there was not enough room in the output
   buffer, or Z_DATA_ERROR if the input data was corrupted or incomplete.
*/

typedef voidp gzFile;

ZEXTERN gzFile ZEXPORT gzopen  OF((const char *path, const char *mode));
/*
	 Opens a gzip (.gz) file for reading or writing. The mode parameter
   is as in fopen ("rb" or "wb") but can also include a compression level
   ("wb9") or a strategy: 'f' for filtered data as in "wb6f", 'h' for
   Huffman only compression as in "wb1h", or 'R' for run-length encoding
   as in "wb1R". (See the description of deflateInit2 for more information
   about the strategy parameter.)

	 gzopen can be used to read a file which is not in gzip format; in this
   case gzread will directly read from the file without decompression.

	 gzopen returns NULL if the file could not be opened or if there was
   insufficient memory to allocate the (de)compression state; errno
   can be checked to distinguish the two cases (if errno is zero, the
   zlib error is Z_MEM_ERROR).  */

ZEXTERN gzFile ZEXPORT gzdopen  OF((int fd, const char *mode));
/*
	 gzdopen() associates a gzFile with the file descriptor fd.  File
   descriptors are obtained from calls like open, dup, creat, pipe or
   fileno (in the file has been previously opened with fopen).
   The mode parameter is as in gzopen.
	 The next call of gzclose on the returned gzFile will also close the
   file descriptor fd, just like fclose(fdopen(fd), mode) closes the file
   descriptor fd. If you want to keep fd open, use gzdopen(dup(fd), mode).
	 gzdopen returns NULL if there was insufficient memory to allocate
   the (de)compression state.
*/

ZEXTERN int ZEXPORT gzsetparams OF((gzFile file, int level, int strategy));
/*
	 Dynamically update the compression level or strategy. See the description
   of deflateInit2 for the meaning of these parameters.
	 gzsetparams returns Z_OK if success, or Z_STREAM_ERROR if the file was not
   opened for writing.
*/

ZEXTERN int ZEXPORT    gzread  OF((gzFile file, voidp buf, unsigned len));
/*
	 Reads the given number of uncompressed bytes from the compressed file.
   If the input file was not in gzip format, gzread copies the given number
   of bytes into the buffer.
	 gzread returns the number of uncompressed bytes actually read (0 for
   end of file, -1 for error). */

ZEXTERN int ZEXPORT    gzwrite OF((gzFile file,
								   voidpc buf, unsigned len));
/*
	 Writes the given number of uncompressed bytes into the compressed file.
   gzwrite returns the number of uncompressed bytes actually written
   (0 in case of error).
*/

ZEXTERN int ZEXPORTVA   gzprintf OF((gzFile file, const char *format, ...));
/*
	 Converts, formats, and writes the args to the compressed file under
   control of the format string, as in fprintf. gzprintf returns the number of
   uncompressed bytes actually written (0 in case of error).  The number of
   uncompressed bytes written is limited to 4095. The caller should assure that
   this limit is not exceeded. If it is exceeded, then gzprintf() will return
   return an error (0) with nothing written. In this case, there may also be a
   buffer overflow with unpredictable consequences, which is possible only if
   zlib was compiled with the insecure functions sprintf() or vsprintf()
   because the secure snprintf() or vsnprintf() functions were not available.
*/

ZEXTERN int ZEXPORT gzputs OF((gzFile file, const char *s));
/*
	  Writes the given null-terminated string to the compressed file, excluding
   the terminating null character.
	  gzputs returns the number of characters written, or -1 in case of error.
*/

ZEXTERN char * ZEXPORT gzgets OF((gzFile file, char *buf, int len));
/*
	  Reads bytes from the compressed file until len-1 characters are read, or
   a newline character is read and transferred to buf, or an end-of-file
   condition is encountered.  The string is then terminated with a null
   character.
	  gzgets returns buf, or Z_NULL in case of error.
*/

ZEXTERN int ZEXPORT    gzputc OF((gzFile file, int c));
/*
	  Writes c, converted to an unsigned char, into the compressed file.
   gzputc returns the value that was written, or -1 in case of error.
*/

ZEXTERN int ZEXPORT    gzgetc OF((gzFile file));
/*
	  Reads one byte from the compressed file. gzgetc returns this byte
   or -1 in case of end of file or error.
*/

ZEXTERN int ZEXPORT    gzungetc OF((int c, gzFile file));
/*
	  Push one character back onto the stream to be read again later.
   Only one character of push-back is allowed.  gzungetc() returns the
   character pushed, or -1 on failure.  gzungetc() will fail if a
   character has been pushed but not read yet, or if c is -1. The pushed
   character will be discarded if the stream is repositioned with gzseek()
   or gzrewind().
*/

ZEXTERN int ZEXPORT    gzflush OF((gzFile file, int flush));
/*
	 Flushes all pending output into the compressed file. The parameter
   flush is as in the deflate() function. The return value is the zlib
   error number (see function gzerror below). gzflush returns Z_OK if
   the flush parameter is Z_FINISH and all output could be flushed.
	 gzflush should be called only when strictly necessary because it can
   degrade compression.
*/

ZEXTERN z_off_t ZEXPORT    gzseek OF((gzFile file,
									  z_off_t offset, int whence));
/*
	  Sets the starting position for the next gzread or gzwrite on the
   given compressed file. The offset represents a number of bytes in the
   uncompressed data stream. The whence parameter is defined as in lseek(2);
   the value SEEK_END is not supported.
	 If the file is opened for reading, this function is emulated but can be
   extremely slow. If the file is opened for writing, only forward seeks are
   supported; gzseek then compresses a sequence of zeroes up to the new
   starting position.

	  gzseek returns the resulting offset location as measured in bytes from
   the beginning of the uncompressed stream, or -1 in case of error, in
   particular if the file is opened for writing and the new starting position
   would be before the current position.
*/

ZEXTERN int ZEXPORT    gzrewind OF((gzFile file));
/*
	 Rewinds the given file. This function is supported only for reading.

   gzrewind(file) is equivalent to (int)gzseek(file, 0L, SEEK_SET)
*/

ZEXTERN z_off_t ZEXPORT    gztell OF((gzFile file));
/*
	 Returns the starting position for the next gzread or gzwrite on the
   given compressed file. This position represents a number of bytes in the
   uncompressed data stream.

   gztell(file) is equivalent to gzseek(file, 0L, SEEK_CUR)
*/

ZEXTERN int ZEXPORT gzeof OF((gzFile file));
/*
	 Returns 1 when EOF has previously been detected reading the given
   input stream, otherwise zero.
*/

ZEXTERN int ZEXPORT gzdirect OF((gzFile file));
/*
	 Returns 1 if file is being read directly without decompression, otherwise
   zero.
*/

ZEXTERN int ZEXPORT    gzclose OF((gzFile file));
/*
	 Flushes all pending output if necessary, closes the compressed file
   and deallocates all the (de)compression state. The return value is the zlib
   error number (see function gzerror below).
*/

ZEXTERN const char * ZEXPORT gzerror OF((gzFile file, int *errnum));
/*
	 Returns the error message for the last error which occurred on the
   given compressed file. errnum is set to zlib error number. If an
   error occurred in the file system and not in the compression library,
   errnum is set to Z_ERRNO and the application may consult errno
   to get the exact error code.
*/

ZEXTERN void ZEXPORT gzclearerr OF((gzFile file));
/*
	 Clears the error and end-of-file flags for file. This is analogous to the
   clearerr() function in stdio. This is useful for continuing to read a gzip
   file that is being written concurrently.
*/

						/* checksum functions */

/*
	 These functions are not related to compression but are exported
   anyway because they might be useful in applications using the
   compression library.
*/

ZEXTERN uLong ZEXPORT adler32 OF((uLong adler, const Bytef *buf, uInt len));
/*
	 Update a running Adler-32 checksum with the bytes buf[0..len-1] and
   return the updated checksum. If buf is NULL, this function returns
   the required initial value for the checksum.
   An Adler-32 checksum is almost as reliable as a CRC32 but can be computed
   much faster. Usage example:

	 uLong adler = adler32(0L, Z_NULL, 0);

	 while (read_buffer(buffer, length) != EOF) {
	   adler = adler32(adler, buffer, length);
	 }
	 if (adler != original_adler) error();
*/

ZEXTERN uLong ZEXPORT adler32_combine OF((uLong adler1, uLong adler2,
										  z_off_t len2));
/*
	 Combine two Adler-32 checksums into one.  For two sequences of bytes, seq1
   and seq2 with lengths len1 and len2, Adler-32 checksums were calculated for
   each, adler1 and adler2.  adler32_combine() returns the Adler-32 checksum of
   seq1 and seq2 concatenated, requiring only adler1, adler2, and len2.
*/

ZEXTERN uLong ZEXPORT crc32   OF((uLong crc, const Bytef *buf, uInt len));
/*
	 Update a running CRC-32 with the bytes buf[0..len-1] and return the
   updated CRC-32. If buf is NULL, this function returns the required initial
   value for the for the crc. Pre- and post-conditioning (one's complement) is
   performed within this function so it shouldn't be done by the application.
   Usage example:

	 uLong crc = crc32(0L, Z_NULL, 0);

	 while (read_buffer(buffer, length) != EOF) {
	   crc = crc32(crc, buffer, length);
	 }
	 if (crc != original_crc) error();
*/

ZEXTERN uLong ZEXPORT crc32_combine OF((uLong crc1, uLong crc2, z_off_t len2));

/*
	 Combine two CRC-32 check values into one.  For two sequences of bytes,
   seq1 and seq2 with lengths len1 and len2, CRC-32 check values were
   calculated for each, crc1 and crc2.  crc32_combine() returns the CRC-32
   check value of seq1 and seq2 concatenated, requiring only crc1, crc2, and
   len2.
*/

						/* various hacks, don't look :) */

/* deflateInit and inflateInit are macros to allow checking the zlib version
 * and the compiler's view of z_stream:
 */
ZEXTERN int ZEXPORT deflateInit_ OF((z_streamp strm, int level,
									 const char *version, int stream_size));
ZEXTERN int ZEXPORT inflateInit_ OF((z_streamp strm,
									 const char *version, int stream_size));
ZEXTERN int ZEXPORT deflateInit2_ OF((z_streamp strm, int  level, int  method,
									  int windowBits, int memLevel,
									  int strategy, const char *version,
									  int stream_size));
ZEXTERN int ZEXPORT inflateInit2_ OF((z_streamp strm, int  windowBits,
									  const char *version, int stream_size));
ZEXTERN int ZEXPORT inflateBackInit_ OF((z_streamp strm, int windowBits,
										 unsigned char FAR *window,
										 const char *version,
										 int stream_size));
#define deflateInit(strm, level) \
		deflateInit_((strm), (level),       ZLIB_VERSION, sizeof(z_stream))
#define inflateInit(strm) \
		inflateInit_((strm),                ZLIB_VERSION, sizeof(z_stream))
#define deflateInit2(strm, level, method, windowBits, memLevel, strategy) \
		deflateInit2_((strm),(level),(method),(windowBits),(memLevel),\
					  (strategy),           ZLIB_VERSION, sizeof(z_stream))
#define inflateInit2(strm, windowBits) \
		inflateInit2_((strm), (windowBits), ZLIB_VERSION, sizeof(z_stream))
#define inflateBackInit(strm, windowBits, window) \
		inflateBackInit_((strm), (windowBits), (window), \
		ZLIB_VERSION, sizeof(z_stream))

#if !defined(ZUTIL_H) && !defined(NO_DUMMY_DECL)
	struct internal_state {int dummy;}; /* hack for buggy compilers */
#endif

ZEXTERN const char   * ZEXPORT zError           OF((int));
ZEXTERN int            ZEXPORT inflateSyncPoint OF((z_streamp z));
ZEXTERN const uLongf * ZEXPORT get_crc_table    OF((void));

#ifdef __cplusplus
//}
#endif

#endif /* ZLIB_H */

/*** End of inlined file: zlib.h ***/



/*** Start of inlined file: adler32.c ***/
/* @(#) $Id: adler32.c,v 1.1 2007/06/07 17:54:37 jules_rms Exp $ */

#define ZLIB_INTERNAL

#define BASE 65521UL    /* largest prime smaller than 65536 */
#define NMAX 5552
/* NMAX is the largest n such that 255n(n+1)/2 + (n+1)(BASE-1) <= 2^32-1 */

#define DO1(buf,i)  {adler += (buf)[i]; sum2 += adler;}
#define DO2(buf,i)  DO1(buf,i); DO1(buf,i+1);
#define DO4(buf,i)  DO2(buf,i); DO2(buf,i+2);
#define DO8(buf,i)  DO4(buf,i); DO4(buf,i+4);
#define DO16(buf)   DO8(buf,0); DO8(buf,8);

/* use NO_DIVIDE if your processor does not do division in hardware */
#ifdef NO_DIVIDE
#  define MOD(a) \
	do { \
		if (a >= (BASE << 16)) a -= (BASE << 16); \
		if (a >= (BASE << 15)) a -= (BASE << 15); \
		if (a >= (BASE << 14)) a -= (BASE << 14); \
		if (a >= (BASE << 13)) a -= (BASE << 13); \
		if (a >= (BASE << 12)) a -= (BASE << 12); \
		if (a >= (BASE << 11)) a -= (BASE << 11); \
		if (a >= (BASE << 10)) a -= (BASE << 10); \
		if (a >= (BASE << 9)) a -= (BASE << 9); \
		if (a >= (BASE << 8)) a -= (BASE << 8); \
		if (a >= (BASE << 7)) a -= (BASE << 7); \
		if (a >= (BASE << 6)) a -= (BASE << 6); \
		if (a >= (BASE << 5)) a -= (BASE << 5); \
		if (a >= (BASE << 4)) a -= (BASE << 4); \
		if (a >= (BASE << 3)) a -= (BASE << 3); \
		if (a >= (BASE << 2)) a -= (BASE << 2); \
		if (a >= (BASE << 1)) a -= (BASE << 1); \
		if (a >= BASE) a -= BASE; \
	} while (0)
#  define MOD4(a) \
	do { \
		if (a >= (BASE << 4)) a -= (BASE << 4); \
		if (a >= (BASE << 3)) a -= (BASE << 3); \
		if (a >= (BASE << 2)) a -= (BASE << 2); \
		if (a >= (BASE << 1)) a -= (BASE << 1); \
		if (a >= BASE) a -= BASE; \
	} while (0)
#else
#  define MOD(a) a %= BASE
#  define MOD4(a) a %= BASE
#endif

/* ========================================================================= */
uLong ZEXPORT adler32(uLong adler, const Bytef *buf, uInt len)
{
	unsigned long sum2;
	unsigned n;

	/* split Adler-32 into component sums */
	sum2 = (adler >> 16) & 0xffff;
	adler &= 0xffff;

	/* in case user likes doing a byte at a time, keep it fast */
	if (len == 1) {
		adler += buf[0];
		if (adler >= BASE)
			adler -= BASE;
		sum2 += adler;
		if (sum2 >= BASE)
			sum2 -= BASE;
		return adler | (sum2 << 16);
	}

	/* initial Adler-32 value (deferred check for len == 1 speed) */
	if (buf == Z_NULL)
		return 1L;

	/* in case short lengths are provided, keep it somewhat fast */
	if (len < 16) {
		while (len--) {
			adler += *buf++;
			sum2 += adler;
		}
		if (adler >= BASE)
			adler -= BASE;
		MOD4(sum2);             /* only added so many BASE's */
		return adler | (sum2 << 16);
	}

	/* do length NMAX blocks -- requires just one modulo operation */
	while (len >= NMAX) {
		len -= NMAX;
		n = NMAX / 16;          /* NMAX is divisible by 16 */
		do {
			DO16(buf);          /* 16 sums unrolled */
			buf += 16;
		} while (--n);
		MOD(adler);
		MOD(sum2);
	}

	/* do remaining bytes (less than NMAX, still just one modulo) */
	if (len) {                  /* avoid modulos if none remaining */
		while (len >= 16) {
			len -= 16;
			DO16(buf);
			buf += 16;
		}
		while (len--) {
			adler += *buf++;
			sum2 += adler;
		}
		MOD(adler);
		MOD(sum2);
	}

	/* return recombined sums */
	return adler | (sum2 << 16);
}

/* ========================================================================= */
uLong ZEXPORT adler32_combine(uLong adler1, uLong adler2, z_off_t len2)
{
	unsigned long sum1;
	unsigned long sum2;
	unsigned rem;

	/* the derivation of this formula is left as an exercise for the reader */
	rem = (unsigned)(len2 % BASE);
	sum1 = adler1 & 0xffff;
	sum2 = rem * sum1;
	MOD(sum2);
	sum1 += (adler2 & 0xffff) + BASE - 1;
	sum2 += ((adler1 >> 16) & 0xffff) + ((adler2 >> 16) & 0xffff) + BASE - rem;
	if (sum1 > BASE) sum1 -= BASE;
	if (sum1 > BASE) sum1 -= BASE;
	if (sum2 > (BASE << 1)) sum2 -= (BASE << 1);
	if (sum2 > BASE) sum2 -= BASE;
	return sum1 | (sum2 << 16);
}

/*** End of inlined file: adler32.c ***/


/*** Start of inlined file: compress.c ***/
/* @(#) $Id: compress.c,v 1.1 2007/06/07 17:54:37 jules_rms Exp $ */

#define ZLIB_INTERNAL

/* ===========================================================================
	 Compresses the source buffer into the destination buffer. The level
   parameter has the same meaning as in deflateInit.  sourceLen is the byte
   length of the source buffer. Upon entry, destLen is the total size of the
   destination buffer, which must be at least 0.1% larger than sourceLen plus
   12 bytes. Upon exit, destLen is the actual size of the compressed buffer.

	 compress2 returns Z_OK if success, Z_MEM_ERROR if there was not enough
   memory, Z_BUF_ERROR if there was not enough room in the output buffer,
   Z_STREAM_ERROR if the level parameter is invalid.
*/
int ZEXPORT compress2 (Bytef *dest, uLongf *destLen, const Bytef *source,
					   uLong sourceLen, int level)
{
	z_stream stream;
	int err;

	stream.next_in = (Bytef*)source;
	stream.avail_in = (uInt)sourceLen;
#ifdef MAXSEG_64K
	/* Check for source > 64K on 16-bit machine: */
	if ((uLong)stream.avail_in != sourceLen) return Z_BUF_ERROR;
#endif
	stream.next_out = dest;
	stream.avail_out = (uInt)*destLen;
	if ((uLong)stream.avail_out != *destLen) return Z_BUF_ERROR;

	stream.zalloc = (alloc_func)0;
	stream.zfree = (free_func)0;
	stream.opaque = (voidpf)0;

	err = deflateInit(&stream, level);
	if (err != Z_OK) return err;

	err = deflate(&stream, Z_FINISH);
	if (err != Z_STREAM_END) {
		deflateEnd(&stream);
		return err == Z_OK ? Z_BUF_ERROR : err;
	}
	*destLen = stream.total_out;

	err = deflateEnd(&stream);
	return err;
}

/* ===========================================================================
 */
int ZEXPORT compress (Bytef *dest, uLongf *destLen, const Bytef *source, uLong sourceLen)
{
	return compress2(dest, destLen, source, sourceLen, Z_DEFAULT_COMPRESSION);
}

/* ===========================================================================
	 If the default memLevel or windowBits for deflateInit() is changed, then
   this function needs to be updated.
 */
uLong ZEXPORT compressBound (uLong sourceLen)
{
	return sourceLen + (sourceLen >> 12) + (sourceLen >> 14) + 11;
}

/*** End of inlined file: compress.c ***/

  #undef DO1
  #undef DO8

/*** Start of inlined file: crc32.c ***/
/* @(#) $Id: crc32.c,v 1.1 2007/06/07 17:54:37 jules_rms Exp $ */

/*
  Note on the use of DYNAMIC_CRC_TABLE: there is no mutex or semaphore
  protection on the static variables used to control the first-use generation
  of the crc tables.  Therefore, if you #define DYNAMIC_CRC_TABLE, you should
  first call get_crc_table() to initialize the tables before allowing more than
  one thread to use crc32().
 */

#ifdef MAKECRCH
#  include <stdio.h>
#  ifndef DYNAMIC_CRC_TABLE
#    define DYNAMIC_CRC_TABLE
#  endif /* !DYNAMIC_CRC_TABLE */
#endif /* MAKECRCH */


/*** Start of inlined file: zutil.h ***/
/* WARNING: this file should *not* be used by applications. It is
   part of the implementation of the compression library and is
   subject to change. Applications should only use zlib.h.
 */

/* @(#) $Id: zutil.h,v 1.1 2007/06/07 17:54:37 jules_rms Exp $ */

#ifndef ZUTIL_H
#define ZUTIL_H

#define ZLIB_INTERNAL

#ifdef STDC
#  ifndef _WIN32_WCE
#    include <stddef.h>
#  endif
#  include <string.h>
#  include <stdlib.h>
#endif
#ifdef NO_ERRNO_H
#   ifdef _WIN32_WCE
	  /* The Microsoft C Run-Time Library for Windows CE doesn't have
	   * errno.  We define it as a global variable to simplify porting.
	   * Its value is always 0 and should not be used.  We rename it to
	   * avoid conflict with other libraries that use the same workaround.
	   */
#     define errno z_errno
#   endif
	extern int errno;
#else
#  ifndef _WIN32_WCE
#    include <errno.h>
#  endif
#endif

#ifndef local
#  define local static
#endif
/* compile with -Dlocal if your debugger can't find static symbols */

typedef unsigned char  uch;
typedef uch FAR uchf;
typedef unsigned short ush;
typedef ush FAR ushf;
typedef unsigned long  ulg;

extern const char * const z_errmsg[10]; /* indexed by 2-zlib_error */
/* (size given to avoid silly warnings with Visual C++) */

#define ERR_MSG(err) z_errmsg[Z_NEED_DICT-(err)]

#define ERR_RETURN(strm,err) \
  return (strm->msg = (char*)ERR_MSG(err), (err))
/* To be used only when the state is known to be valid */

		/* common constants */

#ifndef DEF_WBITS
#  define DEF_WBITS MAX_WBITS
#endif
/* default windowBits for decompression. MAX_WBITS is for compression only */

#if MAX_MEM_LEVEL >= 8
#  define DEF_MEM_LEVEL 8
#else
#  define DEF_MEM_LEVEL  MAX_MEM_LEVEL
#endif
/* default memLevel */

#define STORED_BLOCK 0
#define STATIC_TREES 1
#define DYN_TREES    2
/* The three kinds of block type */

#define MIN_MATCH  3
#define MAX_MATCH  258
/* The minimum and maximum match lengths */

#define PRESET_DICT 0x20 /* preset dictionary flag in zlib header */

		/* target dependencies */

#if defined(MSDOS) || (defined(WINDOWS) && !defined(WIN32))
#  define OS_CODE  0x00
#  if defined(__TURBOC__) || defined(__BORLANDC__)
#    if(__STDC__ == 1) && (defined(__LARGE__) || defined(__COMPACT__))
	   /* Allow compilation with ANSI keywords only enabled */
	   void _Cdecl farfree( void *block );
	   void *_Cdecl farmalloc( unsigned long nbytes );
#    else
#      include <alloc.h>
#    endif
#  else /* MSC or DJGPP */
#    include <malloc.h>
#  endif
#endif

#ifdef AMIGA
#  define OS_CODE  0x01
#endif

#if defined(VAXC) || defined(VMS)
#  define OS_CODE  0x02
#  define F_OPEN(name, mode) \
	 fopen((name), (mode), "mbc=60", "ctx=stm", "rfm=fix", "mrs=512")
#endif

#if defined(ATARI) || defined(atarist)
#  define OS_CODE  0x05
#endif

#ifdef OS2
#  define OS_CODE  0x06
#  ifdef M_I86
	 #include <malloc.h>
#  endif
#endif

#if defined(MACOS) || TARGET_OS_MAC
#  define OS_CODE  0x07
#  if defined(__MWERKS__) && __dest_os != __be_os && __dest_os != __win32_os
#    include <unix.h> /* for fdopen */
#  else
#    ifndef fdopen
#      define fdopen(fd,mode) NULL /* No fdopen() */
#    endif
#  endif
#endif

#ifdef TOPS20
#  define OS_CODE  0x0a
#endif

#ifdef WIN32
#  ifndef __CYGWIN__  /* Cygwin is Unix, not Win32 */
#    define OS_CODE  0x0b
#  endif
#endif

#ifdef __50SERIES /* Prime/PRIMOS */
#  define OS_CODE  0x0f
#endif

#if defined(_BEOS_) || defined(RISCOS)
#  define fdopen(fd,mode) NULL /* No fdopen() */
#endif

#if (defined(_MSC_VER) && (_MSC_VER > 600))
#  if defined(_WIN32_WCE)
#    define fdopen(fd,mode) NULL /* No fdopen() */
#    ifndef _PTRDIFF_T_DEFINED
	   typedef int ptrdiff_t;
#      define _PTRDIFF_T_DEFINED
#    endif
#  else
#    define fdopen(fd,type)  _fdopen(fd,type)
#  endif
#endif

		/* common defaults */

#ifndef OS_CODE
#  define OS_CODE  0x03  /* assume Unix */
#endif

#ifndef F_OPEN
#  define F_OPEN(name, mode) fopen((name), (mode))
#endif

		 /* functions */

#if defined(STDC99) || (defined(__TURBOC__) && __TURBOC__ >= 0x550)
#  ifndef HAVE_VSNPRINTF
#    define HAVE_VSNPRINTF
#  endif
#endif
#if defined(__CYGWIN__)
#  ifndef HAVE_VSNPRINTF
#    define HAVE_VSNPRINTF
#  endif
#endif
#ifndef HAVE_VSNPRINTF
#  ifdef MSDOS
	 /* vsnprintf may exist on some MS-DOS compilers (DJGPP?),
		but for now we just assume it doesn't. */
#    define NO_vsnprintf
#  endif
#  ifdef __TURBOC__
#    define NO_vsnprintf
#  endif
#  ifdef WIN32
	 /* In Win32, vsnprintf is available as the "non-ANSI" _vsnprintf. */
#    if !defined(vsnprintf) && !defined(NO_vsnprintf)
#      define vsnprintf _vsnprintf
#    endif
#  endif
#  ifdef __SASC
#    define NO_vsnprintf
#  endif
#endif
#ifdef VMS
#  define NO_vsnprintf
#endif

#if defined(pyr)
#  define NO_MEMCPY
#endif
#if defined(SMALL_MEDIUM) && !defined(_MSC_VER) && !defined(__SC__)
 /* Use our own functions for small and medium model with MSC <= 5.0.
  * You may have to use the same strategy for Borland C (untested).
  * The __SC__ check is for Symantec.
  */
#  define NO_MEMCPY
#endif
#if defined(STDC) && !defined(HAVE_MEMCPY) && !defined(NO_MEMCPY)
#  define HAVE_MEMCPY
#endif
#ifdef HAVE_MEMCPY
#  ifdef SMALL_MEDIUM /* MSDOS small or medium model */
#    define zmemcpy _fmemcpy
#    define zmemcmp _fmemcmp
#    define zmemzero(dest, len) _fmemset(dest, 0, len)
#  else
#    define zmemcpy memcpy
#    define zmemcmp memcmp
#    define zmemzero(dest, len) memset(dest, 0, len)
#  endif
#else
   extern void zmemcpy  OF((Bytef* dest, const Bytef* source, uInt len));
   extern int  zmemcmp  OF((const Bytef* s1, const Bytef* s2, uInt len));
   extern void zmemzero OF((Bytef* dest, uInt len));
#endif

/* Diagnostic functions */
#if 0
#  include <stdio.h>
   extern int z_verbose;
   extern void z_error    OF((const char *m));
#  define Assert(cond,msg) {if(!(cond)) z_error(msg);}
#  define Trace(x) {if (z_verbose>=0) fprintf x ;}
#  define Tracev(x) {if (z_verbose>0) fprintf x ;}
#  define Tracevv(x) {if (z_verbose>1) fprintf x ;}
#  define Tracec(c,x) {if (z_verbose>0 && (c)) fprintf x ;}
#  define Tracecv(c,x) {if (z_verbose>1 && (c)) fprintf x ;}
#else
#  define Assert(cond,msg)
#  define Trace(x)
#  define Tracev(x)
#  define Tracevv(x)
#  define Tracec(c,x)
#  define Tracecv(c,x)
#  define z_error(x)
#  define z_verbose 0
#endif

voidpf zcalloc OF((voidpf opaque, unsigned items, unsigned size));
void   zcfree  OF((voidpf opaque, voidpf ptr));

#define ZALLOC(strm, items, size) \
		   (*((strm)->zalloc))((strm)->opaque, (items), (size))
#define ZFREE(strm, addr)  (*((strm)->zfree))((strm)->opaque, (voidpf)(addr))
#define TRY_FREE(s, p) {if (p) ZFREE(s, p);}

#endif /* ZUTIL_H */

/*** End of inlined file: zutil.h ***/

#define local static

/* Find a four-byte integer type for crc32_little() and crc32_big(). */
#ifndef NOBYFOUR
#  ifdef STDC           /* need ANSI C limits.h to determine sizes */
#    include <limits.h>
#    define BYFOUR
#    if (UINT_MAX == 0xffffffffUL)
	   typedef unsigned int u4;
#    else
#      if (ULONG_MAX == 0xffffffffUL)
		 typedef unsigned long u4;
#      else
#        if (USHRT_MAX == 0xffffffffUL)
		   typedef unsigned short u4;
#        else
#          undef BYFOUR     /* can't find a four-byte integer type! */
#        endif
#      endif
#    endif
#  endif /* STDC */
#endif /* !NOBYFOUR */

/* Definitions for doing the crc four data bytes at a time. */
#ifdef BYFOUR
#  define REV(w) (((w)>>24)+(((w)>>8)&0xff00)+ \
				(((w)&0xff00)<<8)+(((w)&0xff)<<24))
   local unsigned long crc32_little OF((unsigned long,
						const unsigned char FAR *, unsigned));
   local unsigned long crc32_big OF((unsigned long,
						const unsigned char FAR *, unsigned));
#  define TBLS 8
#else
#  define TBLS 1
#endif /* BYFOUR */

/* Local functions for crc concatenation */
local unsigned long gf2_matrix_times OF((unsigned long *mat,
										 unsigned long vec));
local void gf2_matrix_square OF((unsigned long *square, unsigned long *mat));

#ifdef DYNAMIC_CRC_TABLE

local volatile int crc_table_empty = 1;
local unsigned long FAR crc_table[TBLS][256];
local void make_crc_table OF((void));
#ifdef MAKECRCH
   local void write_table OF((FILE *, const unsigned long FAR *));
#endif /* MAKECRCH */
/*
  Generate tables for a byte-wise 32-bit CRC calculation on the polynomial:
  x^32+x^26+x^23+x^22+x^16+x^12+x^11+x^10+x^8+x^7+x^5+x^4+x^2+x+1.

  Polynomials over GF(2) are represented in binary, one bit per coefficient,
  with the lowest powers in the most significant bit.  Then adding polynomials
  is just exclusive-or, and multiplying a polynomial by x is a right shift by
  one.  If we call the above polynomial p, and represent a byte as the
  polynomial q, also with the lowest power in the most significant bit (so the
  byte 0xb1 is the polynomial x^7+x^3+x+1), then the CRC is (q*x^32) mod p,
  where a mod b means the remainder after dividing a by b.

  This calculation is done using the shift-register method of multiplying and
  taking the remainder.  The register is initialized to zero, and for each
  incoming bit, x^32 is added mod p to the register if the bit is a one (where
  x^32 mod p is p+x^32 = x^26+...+1), and the register is multiplied mod p by
  x (which is shifting right by one and adding x^32 mod p if the bit shifted
  out is a one).  We start with the highest power (least significant bit) of
  q and repeat for all eight bits of q.

  The first table is simply the CRC of all possible eight bit values.  This is
  all the information needed to generate CRCs on data a byte at a time for all
  combinations of CRC register values and incoming bytes.  The remaining tables
  allow for word-at-a-time CRC calculation for both big-endian and little-
  endian machines, where a word is four bytes.
*/
local void make_crc_table()
{
	unsigned long c;
	int n, k;
	unsigned long poly;                 /* polynomial exclusive-or pattern */
	/* terms of polynomial defining this crc (except x^32): */
	static volatile int first = 1;      /* flag to limit concurrent making */
	static const unsigned char p[] = {0,1,2,4,5,7,8,10,11,12,16,22,23,26};

	/* See if another task is already doing this (not thread-safe, but better
	   than nothing -- significantly reduces duration of vulnerability in
	   case the advice about DYNAMIC_CRC_TABLE is ignored) */
	if (first) {
		first = 0;

		/* make exclusive-or pattern from polynomial (0xedb88320UL) */
		poly = 0UL;
		for (n = 0; n < sizeof(p)/sizeof(unsigned char); n++)
			poly |= 1UL << (31 - p[n]);

		/* generate a crc for every 8-bit value */
		for (n = 0; n < 256; n++) {
			c = (unsigned long)n;
			for (k = 0; k < 8; k++)
				c = c & 1 ? poly ^ (c >> 1) : c >> 1;
			crc_table[0][n] = c;
		}

#ifdef BYFOUR
		/* generate crc for each value followed by one, two, and three zeros,
		   and then the byte reversal of those as well as the first table */
		for (n = 0; n < 256; n++) {
			c = crc_table[0][n];
			crc_table[4][n] = REV(c);
			for (k = 1; k < 4; k++) {
				c = crc_table[0][c & 0xff] ^ (c >> 8);
				crc_table[k][n] = c;
				crc_table[k + 4][n] = REV(c);
			}
		}
#endif /* BYFOUR */

		crc_table_empty = 0;
	}
	else {      /* not first */
		/* wait for the other guy to finish (not efficient, but rare) */
		while (crc_table_empty)
			;
	}

#ifdef MAKECRCH
	/* write out CRC tables to crc32.h */
	{
		FILE *out;

		out = fopen("crc32.h", "w");
		if (out == NULL) return;
		fprintf(out, "/* crc32.h -- tables for rapid CRC calculation\n");
		fprintf(out, " * Generated automatically by crc32.c\n */\n\n");
		fprintf(out, "local const unsigned long FAR ");
		fprintf(out, "crc_table[TBLS][256] =\n{\n  {\n");
		write_table(out, crc_table[0]);
#  ifdef BYFOUR
		fprintf(out, "#ifdef BYFOUR\n");
		for (k = 1; k < 8; k++) {
			fprintf(out, "  },\n  {\n");
			write_table(out, crc_table[k]);
		}
		fprintf(out, "#endif\n");
#  endif /* BYFOUR */
		fprintf(out, "  }\n};\n");
		fclose(out);
	}
#endif /* MAKECRCH */
}

#ifdef MAKECRCH
local void write_table(out, table)
	FILE *out;
	const unsigned long FAR *table;
{
	int n;

	for (n = 0; n < 256; n++)
		fprintf(out, "%s0x%08lxUL%s", n % 5 ? "" : "    ", table[n],
				n == 255 ? "\n" : (n % 5 == 4 ? ",\n" : ", "));
}
#endif /* MAKECRCH */

#else /* !DYNAMIC_CRC_TABLE */
/* ========================================================================
 * Tables of CRC-32s of all single-byte values, made by make_crc_table().
 */

/*** Start of inlined file: crc32.h ***/
local const unsigned long FAR crc_table[TBLS][256] =
{
  {
	0x00000000UL, 0x77073096UL, 0xee0e612cUL, 0x990951baUL, 0x076dc419UL,
	0x706af48fUL, 0xe963a535UL, 0x9e6495a3UL, 0x0edb8832UL, 0x79dcb8a4UL,
	0xe0d5e91eUL, 0x97d2d988UL, 0x09b64c2bUL, 0x7eb17cbdUL, 0xe7b82d07UL,
	0x90bf1d91UL, 0x1db71064UL, 0x6ab020f2UL, 0xf3b97148UL, 0x84be41deUL,
	0x1adad47dUL, 0x6ddde4ebUL, 0xf4d4b551UL, 0x83d385c7UL, 0x136c9856UL,
	0x646ba8c0UL, 0xfd62f97aUL, 0x8a65c9ecUL, 0x14015c4fUL, 0x63066cd9UL,
	0xfa0f3d63UL, 0x8d080df5UL, 0x3b6e20c8UL, 0x4c69105eUL, 0xd56041e4UL,
	0xa2677172UL, 0x3c03e4d1UL, 0x4b04d447UL, 0xd20d85fdUL, 0xa50ab56bUL,
	0x35b5a8faUL, 0x42b2986cUL, 0xdbbbc9d6UL, 0xacbcf940UL, 0x32d86ce3UL,
	0x45df5c75UL, 0xdcd60dcfUL, 0xabd13d59UL, 0x26d930acUL, 0x51de003aUL,
	0xc8d75180UL, 0xbfd06116UL, 0x21b4f4b5UL, 0x56b3c423UL, 0xcfba9599UL,
	0xb8bda50fUL, 0x2802b89eUL, 0x5f058808UL, 0xc60cd9b2UL, 0xb10be924UL,
	0x2f6f7c87UL, 0x58684c11UL, 0xc1611dabUL, 0xb6662d3dUL, 0x76dc4190UL,
	0x01db7106UL, 0x98d220bcUL, 0xefd5102aUL, 0x71b18589UL, 0x06b6b51fUL,
	0x9fbfe4a5UL, 0xe8b8d433UL, 0x7807c9a2UL, 0x0f00f934UL, 0x9609a88eUL,
	0xe10e9818UL, 0x7f6a0dbbUL, 0x086d3d2dUL, 0x91646c97UL, 0xe6635c01UL,
	0x6b6b51f4UL, 0x1c6c6162UL, 0x856530d8UL, 0xf262004eUL, 0x6c0695edUL,
	0x1b01a57bUL, 0x8208f4c1UL, 0xf50fc457UL, 0x65b0d9c6UL, 0x12b7e950UL,
	0x8bbeb8eaUL, 0xfcb9887cUL, 0x62dd1ddfUL, 0x15da2d49UL, 0x8cd37cf3UL,
	0xfbd44c65UL, 0x4db26158UL, 0x3ab551ceUL, 0xa3bc0074UL, 0xd4bb30e2UL,
	0x4adfa541UL, 0x3dd895d7UL, 0xa4d1c46dUL, 0xd3d6f4fbUL, 0x4369e96aUL,
	0x346ed9fcUL, 0xad678846UL, 0xda60b8d0UL, 0x44042d73UL, 0x33031de5UL,
	0xaa0a4c5fUL, 0xdd0d7cc9UL, 0x5005713cUL, 0x270241aaUL, 0xbe0b1010UL,
	0xc90c2086UL, 0x5768b525UL, 0x206f85b3UL, 0xb966d409UL, 0xce61e49fUL,
	0x5edef90eUL, 0x29d9c998UL, 0xb0d09822UL, 0xc7d7a8b4UL, 0x59b33d17UL,
	0x2eb40d81UL, 0xb7bd5c3bUL, 0xc0ba6cadUL, 0xedb88320UL, 0x9abfb3b6UL,
	0x03b6e20cUL, 0x74b1d29aUL, 0xead54739UL, 0x9dd277afUL, 0x04db2615UL,
	0x73dc1683UL, 0xe3630b12UL, 0x94643b84UL, 0x0d6d6a3eUL, 0x7a6a5aa8UL,
	0xe40ecf0bUL, 0x9309ff9dUL, 0x0a00ae27UL, 0x7d079eb1UL, 0xf00f9344UL,
	0x8708a3d2UL, 0x1e01f268UL, 0x6906c2feUL, 0xf762575dUL, 0x806567cbUL,
	0x196c3671UL, 0x6e6b06e7UL, 0xfed41b76UL, 0x89d32be0UL, 0x10da7a5aUL,
	0x67dd4accUL, 0xf9b9df6fUL, 0x8ebeeff9UL, 0x17b7be43UL, 0x60b08ed5UL,
	0xd6d6a3e8UL, 0xa1d1937eUL, 0x38d8c2c4UL, 0x4fdff252UL, 0xd1bb67f1UL,
	0xa6bc5767UL, 0x3fb506ddUL, 0x48b2364bUL, 0xd80d2bdaUL, 0xaf0a1b4cUL,
	0x36034af6UL, 0x41047a60UL, 0xdf60efc3UL, 0xa867df55UL, 0x316e8eefUL,
	0x4669be79UL, 0xcb61b38cUL, 0xbc66831aUL, 0x256fd2a0UL, 0x5268e236UL,
	0xcc0c7795UL, 0xbb0b4703UL, 0x220216b9UL, 0x5505262fUL, 0xc5ba3bbeUL,
	0xb2bd0b28UL, 0x2bb45a92UL, 0x5cb36a04UL, 0xc2d7ffa7UL, 0xb5d0cf31UL,
	0x2cd99e8bUL, 0x5bdeae1dUL, 0x9b64c2b0UL, 0xec63f226UL, 0x756aa39cUL,
	0x026d930aUL, 0x9c0906a9UL, 0xeb0e363fUL, 0x72076785UL, 0x05005713UL,
	0x95bf4a82UL, 0xe2b87a14UL, 0x7bb12baeUL, 0x0cb61b38UL, 0x92d28e9bUL,
	0xe5d5be0dUL, 0x7cdcefb7UL, 0x0bdbdf21UL, 0x86d3d2d4UL, 0xf1d4e242UL,
	0x68ddb3f8UL, 0x1fda836eUL, 0x81be16cdUL, 0xf6b9265bUL, 0x6fb077e1UL,
	0x18b74777UL, 0x88085ae6UL, 0xff0f6a70UL, 0x66063bcaUL, 0x11010b5cUL,
	0x8f659effUL, 0xf862ae69UL, 0x616bffd3UL, 0x166ccf45UL, 0xa00ae278UL,
	0xd70dd2eeUL, 0x4e048354UL, 0x3903b3c2UL, 0xa7672661UL, 0xd06016f7UL,
	0x4969474dUL, 0x3e6e77dbUL, 0xaed16a4aUL, 0xd9d65adcUL, 0x40df0b66UL,
	0x37d83bf0UL, 0xa9bcae53UL, 0xdebb9ec5UL, 0x47b2cf7fUL, 0x30b5ffe9UL,
	0xbdbdf21cUL, 0xcabac28aUL, 0x53b39330UL, 0x24b4a3a6UL, 0xbad03605UL,
	0xcdd70693UL, 0x54de5729UL, 0x23d967bfUL, 0xb3667a2eUL, 0xc4614ab8UL,
	0x5d681b02UL, 0x2a6f2b94UL, 0xb40bbe37UL, 0xc30c8ea1UL, 0x5a05df1bUL,
	0x2d02ef8dUL
#ifdef BYFOUR
  },
  {
	0x00000000UL, 0x191b3141UL, 0x32366282UL, 0x2b2d53c3UL, 0x646cc504UL,
	0x7d77f445UL, 0x565aa786UL, 0x4f4196c7UL, 0xc8d98a08UL, 0xd1c2bb49UL,
	0xfaefe88aUL, 0xe3f4d9cbUL, 0xacb54f0cUL, 0xb5ae7e4dUL, 0x9e832d8eUL,
	0x87981ccfUL, 0x4ac21251UL, 0x53d92310UL, 0x78f470d3UL, 0x61ef4192UL,
	0x2eaed755UL, 0x37b5e614UL, 0x1c98b5d7UL, 0x05838496UL, 0x821b9859UL,
	0x9b00a918UL, 0xb02dfadbUL, 0xa936cb9aUL, 0xe6775d5dUL, 0xff6c6c1cUL,
	0xd4413fdfUL, 0xcd5a0e9eUL, 0x958424a2UL, 0x8c9f15e3UL, 0xa7b24620UL,
	0xbea97761UL, 0xf1e8e1a6UL, 0xe8f3d0e7UL, 0xc3de8324UL, 0xdac5b265UL,
	0x5d5daeaaUL, 0x44469febUL, 0x6f6bcc28UL, 0x7670fd69UL, 0x39316baeUL,
	0x202a5aefUL, 0x0b07092cUL, 0x121c386dUL, 0xdf4636f3UL, 0xc65d07b2UL,
	0xed705471UL, 0xf46b6530UL, 0xbb2af3f7UL, 0xa231c2b6UL, 0x891c9175UL,
	0x9007a034UL, 0x179fbcfbUL, 0x0e848dbaUL, 0x25a9de79UL, 0x3cb2ef38UL,
	0x73f379ffUL, 0x6ae848beUL, 0x41c51b7dUL, 0x58de2a3cUL, 0xf0794f05UL,
	0xe9627e44UL, 0xc24f2d87UL, 0xdb541cc6UL, 0x94158a01UL, 0x8d0ebb40UL,
	0xa623e883UL, 0xbf38d9c2UL, 0x38a0c50dUL, 0x21bbf44cUL, 0x0a96a78fUL,
	0x138d96ceUL, 0x5ccc0009UL, 0x45d73148UL, 0x6efa628bUL, 0x77e153caUL,
	0xbabb5d54UL, 0xa3a06c15UL, 0x888d3fd6UL, 0x91960e97UL, 0xded79850UL,
	0xc7cca911UL, 0xece1fad2UL, 0xf5facb93UL, 0x7262d75cUL, 0x6b79e61dUL,
	0x4054b5deUL, 0x594f849fUL, 0x160e1258UL, 0x0f152319UL, 0x243870daUL,
	0x3d23419bUL, 0x65fd6ba7UL, 0x7ce65ae6UL, 0x57cb0925UL, 0x4ed03864UL,
	0x0191aea3UL, 0x188a9fe2UL, 0x33a7cc21UL, 0x2abcfd60UL, 0xad24e1afUL,
	0xb43fd0eeUL, 0x9f12832dUL, 0x8609b26cUL, 0xc94824abUL, 0xd05315eaUL,
	0xfb7e4629UL, 0xe2657768UL, 0x2f3f79f6UL, 0x362448b7UL, 0x1d091b74UL,
	0x04122a35UL, 0x4b53bcf2UL, 0x52488db3UL, 0x7965de70UL, 0x607eef31UL,
	0xe7e6f3feUL, 0xfefdc2bfUL, 0xd5d0917cUL, 0xcccba03dUL, 0x838a36faUL,
	0x9a9107bbUL, 0xb1bc5478UL, 0xa8a76539UL, 0x3b83984bUL, 0x2298a90aUL,
	0x09b5fac9UL, 0x10aecb88UL, 0x5fef5d4fUL, 0x46f46c0eUL, 0x6dd93fcdUL,
	0x74c20e8cUL, 0xf35a1243UL, 0xea412302UL, 0xc16c70c1UL, 0xd8774180UL,
	0x9736d747UL, 0x8e2de606UL, 0xa500b5c5UL, 0xbc1b8484UL, 0x71418a1aUL,
	0x685abb5bUL, 0x4377e898UL, 0x5a6cd9d9UL, 0x152d4f1eUL, 0x0c367e5fUL,
	0x271b2d9cUL, 0x3e001cddUL, 0xb9980012UL, 0xa0833153UL, 0x8bae6290UL,
	0x92b553d1UL, 0xddf4c516UL, 0xc4eff457UL, 0xefc2a794UL, 0xf6d996d5UL,
	0xae07bce9UL, 0xb71c8da8UL, 0x9c31de6bUL, 0x852aef2aUL, 0xca6b79edUL,
	0xd37048acUL, 0xf85d1b6fUL, 0xe1462a2eUL, 0x66de36e1UL, 0x7fc507a0UL,
	0x54e85463UL, 0x4df36522UL, 0x02b2f3e5UL, 0x1ba9c2a4UL, 0x30849167UL,
	0x299fa026UL, 0xe4c5aeb8UL, 0xfdde9ff9UL, 0xd6f3cc3aUL, 0xcfe8fd7bUL,
	0x80a96bbcUL, 0x99b25afdUL, 0xb29f093eUL, 0xab84387fUL, 0x2c1c24b0UL,
	0x350715f1UL, 0x1e2a4632UL, 0x07317773UL, 0x4870e1b4UL, 0x516bd0f5UL,
	0x7a468336UL, 0x635db277UL, 0xcbfad74eUL, 0xd2e1e60fUL, 0xf9ccb5ccUL,
	0xe0d7848dUL, 0xaf96124aUL, 0xb68d230bUL, 0x9da070c8UL, 0x84bb4189UL,
	0x03235d46UL, 0x1a386c07UL, 0x31153fc4UL, 0x280e0e85UL, 0x674f9842UL,
	0x7e54a903UL, 0x5579fac0UL, 0x4c62cb81UL, 0x8138c51fUL, 0x9823f45eUL,
	0xb30ea79dUL, 0xaa1596dcUL, 0xe554001bUL, 0xfc4f315aUL, 0xd7626299UL,
	0xce7953d8UL, 0x49e14f17UL, 0x50fa7e56UL, 0x7bd72d95UL, 0x62cc1cd4UL,
	0x2d8d8a13UL, 0x3496bb52UL, 0x1fbbe891UL, 0x06a0d9d0UL, 0x5e7ef3ecUL,
	0x4765c2adUL, 0x6c48916eUL, 0x7553a02fUL, 0x3a1236e8UL, 0x230907a9UL,
	0x0824546aUL, 0x113f652bUL, 0x96a779e4UL, 0x8fbc48a5UL, 0xa4911b66UL,
	0xbd8a2a27UL, 0xf2cbbce0UL, 0xebd08da1UL, 0xc0fdde62UL, 0xd9e6ef23UL,
	0x14bce1bdUL, 0x0da7d0fcUL, 0x268a833fUL, 0x3f91b27eUL, 0x70d024b9UL,
	0x69cb15f8UL, 0x42e6463bUL, 0x5bfd777aUL, 0xdc656bb5UL, 0xc57e5af4UL,
	0xee530937UL, 0xf7483876UL, 0xb809aeb1UL, 0xa1129ff0UL, 0x8a3fcc33UL,
	0x9324fd72UL
  },
  {
	0x00000000UL, 0x01c26a37UL, 0x0384d46eUL, 0x0246be59UL, 0x0709a8dcUL,
	0x06cbc2ebUL, 0x048d7cb2UL, 0x054f1685UL, 0x0e1351b8UL, 0x0fd13b8fUL,
	0x0d9785d6UL, 0x0c55efe1UL, 0x091af964UL, 0x08d89353UL, 0x0a9e2d0aUL,
	0x0b5c473dUL, 0x1c26a370UL, 0x1de4c947UL, 0x1fa2771eUL, 0x1e601d29UL,
	0x1b2f0bacUL, 0x1aed619bUL, 0x18abdfc2UL, 0x1969b5f5UL, 0x1235f2c8UL,
	0x13f798ffUL, 0x11b126a6UL, 0x10734c91UL, 0x153c5a14UL, 0x14fe3023UL,
	0x16b88e7aUL, 0x177ae44dUL, 0x384d46e0UL, 0x398f2cd7UL, 0x3bc9928eUL,
	0x3a0bf8b9UL, 0x3f44ee3cUL, 0x3e86840bUL, 0x3cc03a52UL, 0x3d025065UL,
	0x365e1758UL, 0x379c7d6fUL, 0x35dac336UL, 0x3418a901UL, 0x3157bf84UL,
	0x3095d5b3UL, 0x32d36beaUL, 0x331101ddUL, 0x246be590UL, 0x25a98fa7UL,
	0x27ef31feUL, 0x262d5bc9UL, 0x23624d4cUL, 0x22a0277bUL, 0x20e69922UL,
	0x2124f315UL, 0x2a78b428UL, 0x2bbade1fUL, 0x29fc6046UL, 0x283e0a71UL,
	0x2d711cf4UL, 0x2cb376c3UL, 0x2ef5c89aUL, 0x2f37a2adUL, 0x709a8dc0UL,
	0x7158e7f7UL, 0x731e59aeUL, 0x72dc3399UL, 0x7793251cUL, 0x76514f2bUL,
	0x7417f172UL, 0x75d59b45UL, 0x7e89dc78UL, 0x7f4bb64fUL, 0x7d0d0816UL,
	0x7ccf6221UL, 0x798074a4UL, 0x78421e93UL, 0x7a04a0caUL, 0x7bc6cafdUL,
	0x6cbc2eb0UL, 0x6d7e4487UL, 0x6f38fadeUL, 0x6efa90e9UL, 0x6bb5866cUL,
	0x6a77ec5bUL, 0x68315202UL, 0x69f33835UL, 0x62af7f08UL, 0x636d153fUL,
	0x612bab66UL, 0x60e9c151UL, 0x65a6d7d4UL, 0x6464bde3UL, 0x662203baUL,
	0x67e0698dUL, 0x48d7cb20UL, 0x4915a117UL, 0x4b531f4eUL, 0x4a917579UL,
	0x4fde63fcUL, 0x4e1c09cbUL, 0x4c5ab792UL, 0x4d98dda5UL, 0x46c49a98UL,
	0x4706f0afUL, 0x45404ef6UL, 0x448224c1UL, 0x41cd3244UL, 0x400f5873UL,
	0x4249e62aUL, 0x438b8c1dUL, 0x54f16850UL, 0x55330267UL, 0x5775bc3eUL,
	0x56b7d609UL, 0x53f8c08cUL, 0x523aaabbUL, 0x507c14e2UL, 0x51be7ed5UL,
	0x5ae239e8UL, 0x5b2053dfUL, 0x5966ed86UL, 0x58a487b1UL, 0x5deb9134UL,
	0x5c29fb03UL, 0x5e6f455aUL, 0x5fad2f6dUL, 0xe1351b80UL, 0xe0f771b7UL,
	0xe2b1cfeeUL, 0xe373a5d9UL, 0xe63cb35cUL, 0xe7fed96bUL, 0xe5b86732UL,
	0xe47a0d05UL, 0xef264a38UL, 0xeee4200fUL, 0xeca29e56UL, 0xed60f461UL,
	0xe82fe2e4UL, 0xe9ed88d3UL, 0xebab368aUL, 0xea695cbdUL, 0xfd13b8f0UL,
	0xfcd1d2c7UL, 0xfe976c9eUL, 0xff5506a9UL, 0xfa1a102cUL, 0xfbd87a1bUL,
	0xf99ec442UL, 0xf85cae75UL, 0xf300e948UL, 0xf2c2837fUL, 0xf0843d26UL,
	0xf1465711UL, 0xf4094194UL, 0xf5cb2ba3UL, 0xf78d95faUL, 0xf64fffcdUL,
	0xd9785d60UL, 0xd8ba3757UL, 0xdafc890eUL, 0xdb3ee339UL, 0xde71f5bcUL,
	0xdfb39f8bUL, 0xddf521d2UL, 0xdc374be5UL, 0xd76b0cd8UL, 0xd6a966efUL,
	0xd4efd8b6UL, 0xd52db281UL, 0xd062a404UL, 0xd1a0ce33UL, 0xd3e6706aUL,
	0xd2241a5dUL, 0xc55efe10UL, 0xc49c9427UL, 0xc6da2a7eUL, 0xc7184049UL,
	0xc25756ccUL, 0xc3953cfbUL, 0xc1d382a2UL, 0xc011e895UL, 0xcb4dafa8UL,
	0xca8fc59fUL, 0xc8c97bc6UL, 0xc90b11f1UL, 0xcc440774UL, 0xcd866d43UL,
	0xcfc0d31aUL, 0xce02b92dUL, 0x91af9640UL, 0x906dfc77UL, 0x922b422eUL,
	0x93e92819UL, 0x96a63e9cUL, 0x976454abUL, 0x9522eaf2UL, 0x94e080c5UL,
	0x9fbcc7f8UL, 0x9e7eadcfUL, 0x9c381396UL, 0x9dfa79a1UL, 0x98b56f24UL,
	0x99770513UL, 0x9b31bb4aUL, 0x9af3d17dUL, 0x8d893530UL, 0x8c4b5f07UL,
	0x8e0de15eUL, 0x8fcf8b69UL, 0x8a809decUL, 0x8b42f7dbUL, 0x89044982UL,
	0x88c623b5UL, 0x839a6488UL, 0x82580ebfUL, 0x801eb0e6UL, 0x81dcdad1UL,
	0x8493cc54UL, 0x8551a663UL, 0x8717183aUL, 0x86d5720dUL, 0xa9e2d0a0UL,
	0xa820ba97UL, 0xaa6604ceUL, 0xaba46ef9UL, 0xaeeb787cUL, 0xaf29124bUL,
	0xad6fac12UL, 0xacadc625UL, 0xa7f18118UL, 0xa633eb2fUL, 0xa4755576UL,
	0xa5b73f41UL, 0xa0f829c4UL, 0xa13a43f3UL, 0xa37cfdaaUL, 0xa2be979dUL,
	0xb5c473d0UL, 0xb40619e7UL, 0xb640a7beUL, 0xb782cd89UL, 0xb2cddb0cUL,
	0xb30fb13bUL, 0xb1490f62UL, 0xb08b6555UL, 0xbbd72268UL, 0xba15485fUL,
	0xb853f606UL, 0xb9919c31UL, 0xbcde8ab4UL, 0xbd1ce083UL, 0xbf5a5edaUL,
	0xbe9834edUL
  },
  {
	0x00000000UL, 0xb8bc6765UL, 0xaa09c88bUL, 0x12b5afeeUL, 0x8f629757UL,
	0x37def032UL, 0x256b5fdcUL, 0x9dd738b9UL, 0xc5b428efUL, 0x7d084f8aUL,
	0x6fbde064UL, 0xd7018701UL, 0x4ad6bfb8UL, 0xf26ad8ddUL, 0xe0df7733UL,
	0x58631056UL, 0x5019579fUL, 0xe8a530faUL, 0xfa109f14UL, 0x42acf871UL,
	0xdf7bc0c8UL, 0x67c7a7adUL, 0x75720843UL, 0xcdce6f26UL, 0x95ad7f70UL,
	0x2d111815UL, 0x3fa4b7fbUL, 0x8718d09eUL, 0x1acfe827UL, 0xa2738f42UL,
	0xb0c620acUL, 0x087a47c9UL, 0xa032af3eUL, 0x188ec85bUL, 0x0a3b67b5UL,
	0xb28700d0UL, 0x2f503869UL, 0x97ec5f0cUL, 0x8559f0e2UL, 0x3de59787UL,
	0x658687d1UL, 0xdd3ae0b4UL, 0xcf8f4f5aUL, 0x7733283fUL, 0xeae41086UL,
	0x525877e3UL, 0x40edd80dUL, 0xf851bf68UL, 0xf02bf8a1UL, 0x48979fc4UL,
	0x5a22302aUL, 0xe29e574fUL, 0x7f496ff6UL, 0xc7f50893UL, 0xd540a77dUL,
	0x6dfcc018UL, 0x359fd04eUL, 0x8d23b72bUL, 0x9f9618c5UL, 0x272a7fa0UL,
	0xbafd4719UL, 0x0241207cUL, 0x10f48f92UL, 0xa848e8f7UL, 0x9b14583dUL,
	0x23a83f58UL, 0x311d90b6UL, 0x89a1f7d3UL, 0x1476cf6aUL, 0xaccaa80fUL,
	0xbe7f07e1UL, 0x06c36084UL, 0x5ea070d2UL, 0xe61c17b7UL, 0xf4a9b859UL,
	0x4c15df3cUL, 0xd1c2e785UL, 0x697e80e0UL, 0x7bcb2f0eUL, 0xc377486bUL,
	0xcb0d0fa2UL, 0x73b168c7UL, 0x6104c729UL, 0xd9b8a04cUL, 0x446f98f5UL,
	0xfcd3ff90UL, 0xee66507eUL, 0x56da371bUL, 0x0eb9274dUL, 0xb6054028UL,
	0xa4b0efc6UL, 0x1c0c88a3UL, 0x81dbb01aUL, 0x3967d77fUL, 0x2bd27891UL,
	0x936e1ff4UL, 0x3b26f703UL, 0x839a9066UL, 0x912f3f88UL, 0x299358edUL,
	0xb4446054UL, 0x0cf80731UL, 0x1e4da8dfUL, 0xa6f1cfbaUL, 0xfe92dfecUL,
	0x462eb889UL, 0x549b1767UL, 0xec277002UL, 0x71f048bbUL, 0xc94c2fdeUL,
	0xdbf98030UL, 0x6345e755UL, 0x6b3fa09cUL, 0xd383c7f9UL, 0xc1366817UL,
	0x798a0f72UL, 0xe45d37cbUL, 0x5ce150aeUL, 0x4e54ff40UL, 0xf6e89825UL,
	0xae8b8873UL, 0x1637ef16UL, 0x048240f8UL, 0xbc3e279dUL, 0x21e91f24UL,
	0x99557841UL, 0x8be0d7afUL, 0x335cb0caUL, 0xed59b63bUL, 0x55e5d15eUL,
	0x47507eb0UL, 0xffec19d5UL, 0x623b216cUL, 0xda874609UL, 0xc832e9e7UL,
	0x708e8e82UL, 0x28ed9ed4UL, 0x9051f9b1UL, 0x82e4565fUL, 0x3a58313aUL,
	0xa78f0983UL, 0x1f336ee6UL, 0x0d86c108UL, 0xb53aa66dUL, 0xbd40e1a4UL,
	0x05fc86c1UL, 0x1749292fUL, 0xaff54e4aUL, 0x322276f3UL, 0x8a9e1196UL,
	0x982bbe78UL, 0x2097d91dUL, 0x78f4c94bUL, 0xc048ae2eUL, 0xd2fd01c0UL,
	0x6a4166a5UL, 0xf7965e1cUL, 0x4f2a3979UL, 0x5d9f9697UL, 0xe523f1f2UL,
	0x4d6b1905UL, 0xf5d77e60UL, 0xe762d18eUL, 0x5fdeb6ebUL, 0xc2098e52UL,
	0x7ab5e937UL, 0x680046d9UL, 0xd0bc21bcUL, 0x88df31eaUL, 0x3063568fUL,
	0x22d6f961UL, 0x9a6a9e04UL, 0x07bda6bdUL, 0xbf01c1d8UL, 0xadb46e36UL,
	0x15080953UL, 0x1d724e9aUL, 0xa5ce29ffUL, 0xb77b8611UL, 0x0fc7e174UL,
	0x9210d9cdUL, 0x2aacbea8UL, 0x38191146UL, 0x80a57623UL, 0xd8c66675UL,
	0x607a0110UL, 0x72cfaefeUL, 0xca73c99bUL, 0x57a4f122UL, 0xef189647UL,
	0xfdad39a9UL, 0x45115eccUL, 0x764dee06UL, 0xcef18963UL, 0xdc44268dUL,
	0x64f841e8UL, 0xf92f7951UL, 0x41931e34UL, 0x5326b1daUL, 0xeb9ad6bfUL,
	0xb3f9c6e9UL, 0x0b45a18cUL, 0x19f00e62UL, 0xa14c6907UL, 0x3c9b51beUL,
	0x842736dbUL, 0x96929935UL, 0x2e2efe50UL, 0x2654b999UL, 0x9ee8defcUL,
	0x8c5d7112UL, 0x34e11677UL, 0xa9362eceUL, 0x118a49abUL, 0x033fe645UL,
	0xbb838120UL, 0xe3e09176UL, 0x5b5cf613UL, 0x49e959fdUL, 0xf1553e98UL,
	0x6c820621UL, 0xd43e6144UL, 0xc68bceaaUL, 0x7e37a9cfUL, 0xd67f4138UL,
	0x6ec3265dUL, 0x7c7689b3UL, 0xc4caeed6UL, 0x591dd66fUL, 0xe1a1b10aUL,
	0xf3141ee4UL, 0x4ba87981UL, 0x13cb69d7UL, 0xab770eb2UL, 0xb9c2a15cUL,
	0x017ec639UL, 0x9ca9fe80UL, 0x241599e5UL, 0x36a0360bUL, 0x8e1c516eUL,
	0x866616a7UL, 0x3eda71c2UL, 0x2c6fde2cUL, 0x94d3b949UL, 0x090481f0UL,
	0xb1b8e695UL, 0xa30d497bUL, 0x1bb12e1eUL, 0x43d23e48UL, 0xfb6e592dUL,
	0xe9dbf6c3UL, 0x516791a6UL, 0xccb0a91fUL, 0x740cce7aUL, 0x66b96194UL,
	0xde0506f1UL
  },
  {
	0x00000000UL, 0x96300777UL, 0x2c610eeeUL, 0xba510999UL, 0x19c46d07UL,
	0x8ff46a70UL, 0x35a563e9UL, 0xa395649eUL, 0x3288db0eUL, 0xa4b8dc79UL,
	0x1ee9d5e0UL, 0x88d9d297UL, 0x2b4cb609UL, 0xbd7cb17eUL, 0x072db8e7UL,
	0x911dbf90UL, 0x6410b71dUL, 0xf220b06aUL, 0x4871b9f3UL, 0xde41be84UL,
	0x7dd4da1aUL, 0xebe4dd6dUL, 0x51b5d4f4UL, 0xc785d383UL, 0x56986c13UL,
	0xc0a86b64UL, 0x7af962fdUL, 0xecc9658aUL, 0x4f5c0114UL, 0xd96c0663UL,
	0x633d0ffaUL, 0xf50d088dUL, 0xc8206e3bUL, 0x5e10694cUL, 0xe44160d5UL,
	0x727167a2UL, 0xd1e4033cUL, 0x47d4044bUL, 0xfd850dd2UL, 0x6bb50aa5UL,
	0xfaa8b535UL, 0x6c98b242UL, 0xd6c9bbdbUL, 0x40f9bcacUL, 0xe36cd832UL,
	0x755cdf45UL, 0xcf0dd6dcUL, 0x593dd1abUL, 0xac30d926UL, 0x3a00de51UL,
	0x8051d7c8UL, 0x1661d0bfUL, 0xb5f4b421UL, 0x23c4b356UL, 0x9995bacfUL,
	0x0fa5bdb8UL, 0x9eb80228UL, 0x0888055fUL, 0xb2d90cc6UL, 0x24e90bb1UL,
	0x877c6f2fUL, 0x114c6858UL, 0xab1d61c1UL, 0x3d2d66b6UL, 0x9041dc76UL,
	0x0671db01UL, 0xbc20d298UL, 0x2a10d5efUL, 0x8985b171UL, 0x1fb5b606UL,
	0xa5e4bf9fUL, 0x33d4b8e8UL, 0xa2c90778UL, 0x34f9000fUL, 0x8ea80996UL,
	0x18980ee1UL, 0xbb0d6a7fUL, 0x2d3d6d08UL, 0x976c6491UL, 0x015c63e6UL,
	0xf4516b6bUL, 0x62616c1cUL, 0xd8306585UL, 0x4e0062f2UL, 0xed95066cUL,
	0x7ba5011bUL, 0xc1f40882UL, 0x57c40ff5UL, 0xc6d9b065UL, 0x50e9b712UL,
	0xeab8be8bUL, 0x7c88b9fcUL, 0xdf1ddd62UL, 0x492dda15UL, 0xf37cd38cUL,
	0x654cd4fbUL, 0x5861b24dUL, 0xce51b53aUL, 0x7400bca3UL, 0xe230bbd4UL,
	0x41a5df4aUL, 0xd795d83dUL, 0x6dc4d1a4UL, 0xfbf4d6d3UL, 0x6ae96943UL,
	0xfcd96e34UL, 0x468867adUL, 0xd0b860daUL, 0x732d0444UL, 0xe51d0333UL,
	0x5f4c0aaaUL, 0xc97c0dddUL, 0x3c710550UL, 0xaa410227UL, 0x10100bbeUL,
	0x86200cc9UL, 0x25b56857UL, 0xb3856f20UL, 0x09d466b9UL, 0x9fe461ceUL,
	0x0ef9de5eUL, 0x98c9d929UL, 0x2298d0b0UL, 0xb4a8d7c7UL, 0x173db359UL,
	0x810db42eUL, 0x3b5cbdb7UL, 0xad6cbac0UL, 0x2083b8edUL, 0xb6b3bf9aUL,
	0x0ce2b603UL, 0x9ad2b174UL, 0x3947d5eaUL, 0xaf77d29dUL, 0x1526db04UL,
	0x8316dc73UL, 0x120b63e3UL, 0x843b6494UL, 0x3e6a6d0dUL, 0xa85a6a7aUL,
	0x0bcf0ee4UL, 0x9dff0993UL, 0x27ae000aUL, 0xb19e077dUL, 0x44930ff0UL,
	0xd2a30887UL, 0x68f2011eUL, 0xfec20669UL, 0x5d5762f7UL, 0xcb676580UL,
	0x71366c19UL, 0xe7066b6eUL, 0x761bd4feUL, 0xe02bd389UL, 0x5a7ada10UL,
	0xcc4add67UL, 0x6fdfb9f9UL, 0xf9efbe8eUL, 0x43beb717UL, 0xd58eb060UL,
	0xe8a3d6d6UL, 0x7e93d1a1UL, 0xc4c2d838UL, 0x52f2df4fUL, 0xf167bbd1UL,
	0x6757bca6UL, 0xdd06b53fUL, 0x4b36b248UL, 0xda2b0dd8UL, 0x4c1b0aafUL,
	0xf64a0336UL, 0x607a0441UL, 0xc3ef60dfUL, 0x55df67a8UL, 0xef8e6e31UL,
	0x79be6946UL, 0x8cb361cbUL, 0x1a8366bcUL, 0xa0d26f25UL, 0x36e26852UL,
	0x95770cccUL, 0x03470bbbUL, 0xb9160222UL, 0x2f260555UL, 0xbe3bbac5UL,
	0x280bbdb2UL, 0x925ab42bUL, 0x046ab35cUL, 0xa7ffd7c2UL, 0x31cfd0b5UL,
	0x8b9ed92cUL, 0x1daede5bUL, 0xb0c2649bUL, 0x26f263ecUL, 0x9ca36a75UL,
	0x0a936d02UL, 0xa906099cUL, 0x3f360eebUL, 0x85670772UL, 0x13570005UL,
	0x824abf95UL, 0x147ab8e2UL, 0xae2bb17bUL, 0x381bb60cUL, 0x9b8ed292UL,
	0x0dbed5e5UL, 0xb7efdc7cUL, 0x21dfdb0bUL, 0xd4d2d386UL, 0x42e2d4f1UL,
	0xf8b3dd68UL, 0x6e83da1fUL, 0xcd16be81UL, 0x5b26b9f6UL, 0xe177b06fUL,
	0x7747b718UL, 0xe65a0888UL, 0x706a0fffUL, 0xca3b0666UL, 0x5c0b0111UL,
	0xff9e658fUL, 0x69ae62f8UL, 0xd3ff6b61UL, 0x45cf6c16UL, 0x78e20aa0UL,
	0xeed20dd7UL, 0x5483044eUL, 0xc2b30339UL, 0x612667a7UL, 0xf71660d0UL,
	0x4d476949UL, 0xdb776e3eUL, 0x4a6ad1aeUL, 0xdc5ad6d9UL, 0x660bdf40UL,
	0xf03bd837UL, 0x53aebca9UL, 0xc59ebbdeUL, 0x7fcfb247UL, 0xe9ffb530UL,
	0x1cf2bdbdUL, 0x8ac2bacaUL, 0x3093b353UL, 0xa6a3b424UL, 0x0536d0baUL,
	0x9306d7cdUL, 0x2957de54UL, 0xbf67d923UL, 0x2e7a66b3UL, 0xb84a61c4UL,
	0x021b685dUL, 0x942b6f2aUL, 0x37be0bb4UL, 0xa18e0cc3UL, 0x1bdf055aUL,
	0x8def022dUL
  },
  {
	0x00000000UL, 0x41311b19UL, 0x82623632UL, 0xc3532d2bUL, 0x04c56c64UL,
	0x45f4777dUL, 0x86a75a56UL, 0xc796414fUL, 0x088ad9c8UL, 0x49bbc2d1UL,
	0x8ae8effaUL, 0xcbd9f4e3UL, 0x0c4fb5acUL, 0x4d7eaeb5UL, 0x8e2d839eUL,
	0xcf1c9887UL, 0x5112c24aUL, 0x1023d953UL, 0xd370f478UL, 0x9241ef61UL,
	0x55d7ae2eUL, 0x14e6b537UL, 0xd7b5981cUL, 0x96848305UL, 0x59981b82UL,
	0x18a9009bUL, 0xdbfa2db0UL, 0x9acb36a9UL, 0x5d5d77e6UL, 0x1c6c6cffUL,
	0xdf3f41d4UL, 0x9e0e5acdUL, 0xa2248495UL, 0xe3159f8cUL, 0x2046b2a7UL,
	0x6177a9beUL, 0xa6e1e8f1UL, 0xe7d0f3e8UL, 0x2483dec3UL, 0x65b2c5daUL,
	0xaaae5d5dUL, 0xeb9f4644UL, 0x28cc6b6fUL, 0x69fd7076UL, 0xae6b3139UL,
	0xef5a2a20UL, 0x2c09070bUL, 0x6d381c12UL, 0xf33646dfUL, 0xb2075dc6UL,
	0x715470edUL, 0x30656bf4UL, 0xf7f32abbUL, 0xb6c231a2UL, 0x75911c89UL,
	0x34a00790UL, 0xfbbc9f17UL, 0xba8d840eUL, 0x79dea925UL, 0x38efb23cUL,
	0xff79f373UL, 0xbe48e86aUL, 0x7d1bc541UL, 0x3c2ade58UL, 0x054f79f0UL,
	0x447e62e9UL, 0x872d4fc2UL, 0xc61c54dbUL, 0x018a1594UL, 0x40bb0e8dUL,
	0x83e823a6UL, 0xc2d938bfUL, 0x0dc5a038UL, 0x4cf4bb21UL, 0x8fa7960aUL,
	0xce968d13UL, 0x0900cc5cUL, 0x4831d745UL, 0x8b62fa6eUL, 0xca53e177UL,
	0x545dbbbaUL, 0x156ca0a3UL, 0xd63f8d88UL, 0x970e9691UL, 0x5098d7deUL,
	0x11a9ccc7UL, 0xd2fae1ecUL, 0x93cbfaf5UL, 0x5cd76272UL, 0x1de6796bUL,
	0xdeb55440UL, 0x9f844f59UL, 0x58120e16UL, 0x1923150fUL, 0xda703824UL,
	0x9b41233dUL, 0xa76bfd65UL, 0xe65ae67cUL, 0x2509cb57UL, 0x6438d04eUL,
	0xa3ae9101UL, 0xe29f8a18UL, 0x21cca733UL, 0x60fdbc2aUL, 0xafe124adUL,
	0xeed03fb4UL, 0x2d83129fUL, 0x6cb20986UL, 0xab2448c9UL, 0xea1553d0UL,
	0x29467efbUL, 0x687765e2UL, 0xf6793f2fUL, 0xb7482436UL, 0x741b091dUL,
	0x352a1204UL, 0xf2bc534bUL, 0xb38d4852UL, 0x70de6579UL, 0x31ef7e60UL,
	0xfef3e6e7UL, 0xbfc2fdfeUL, 0x7c91d0d5UL, 0x3da0cbccUL, 0xfa368a83UL,
	0xbb07919aUL, 0x7854bcb1UL, 0x3965a7a8UL, 0x4b98833bUL, 0x0aa99822UL,
	0xc9fab509UL, 0x88cbae10UL, 0x4f5def5fUL, 0x0e6cf446UL, 0xcd3fd96dUL,
	0x8c0ec274UL, 0x43125af3UL, 0x022341eaUL, 0xc1706cc1UL, 0x804177d8UL,
	0x47d73697UL, 0x06e62d8eUL, 0xc5b500a5UL, 0x84841bbcUL, 0x1a8a4171UL,
	0x5bbb5a68UL, 0x98e87743UL, 0xd9d96c5aUL, 0x1e4f2d15UL, 0x5f7e360cUL,
	0x9c2d1b27UL, 0xdd1c003eUL, 0x120098b9UL, 0x533183a0UL, 0x9062ae8bUL,
	0xd153b592UL, 0x16c5f4ddUL, 0x57f4efc4UL, 0x94a7c2efUL, 0xd596d9f6UL,
	0xe9bc07aeUL, 0xa88d1cb7UL, 0x6bde319cUL, 0x2aef2a85UL, 0xed796bcaUL,
	0xac4870d3UL, 0x6f1b5df8UL, 0x2e2a46e1UL, 0xe136de66UL, 0xa007c57fUL,
	0x6354e854UL, 0x2265f34dUL, 0xe5f3b202UL, 0xa4c2a91bUL, 0x67918430UL,
	0x26a09f29UL, 0xb8aec5e4UL, 0xf99fdefdUL, 0x3accf3d6UL, 0x7bfde8cfUL,
	0xbc6ba980UL, 0xfd5ab299UL, 0x3e099fb2UL, 0x7f3884abUL, 0xb0241c2cUL,
	0xf1150735UL, 0x32462a1eUL, 0x73773107UL, 0xb4e17048UL, 0xf5d06b51UL,
	0x3683467aUL, 0x77b25d63UL, 0x4ed7facbUL, 0x0fe6e1d2UL, 0xccb5ccf9UL,
	0x8d84d7e0UL, 0x4a1296afUL, 0x0b238db6UL, 0xc870a09dUL, 0x8941bb84UL,
	0x465d2303UL, 0x076c381aUL, 0xc43f1531UL, 0x850e0e28UL, 0x42984f67UL,
	0x03a9547eUL, 0xc0fa7955UL, 0x81cb624cUL, 0x1fc53881UL, 0x5ef42398UL,
	0x9da70eb3UL, 0xdc9615aaUL, 0x1b0054e5UL, 0x5a314ffcUL, 0x996262d7UL,
	0xd85379ceUL, 0x174fe149UL, 0x567efa50UL, 0x952dd77bUL, 0xd41ccc62UL,
	0x138a8d2dUL, 0x52bb9634UL, 0x91e8bb1fUL, 0xd0d9a006UL, 0xecf37e5eUL,
	0xadc26547UL, 0x6e91486cUL, 0x2fa05375UL, 0xe836123aUL, 0xa9070923UL,
	0x6a542408UL, 0x2b653f11UL, 0xe479a796UL, 0xa548bc8fUL, 0x661b91a4UL,
	0x272a8abdUL, 0xe0bccbf2UL, 0xa18dd0ebUL, 0x62defdc0UL, 0x23efe6d9UL,
	0xbde1bc14UL, 0xfcd0a70dUL, 0x3f838a26UL, 0x7eb2913fUL, 0xb924d070UL,
	0xf815cb69UL, 0x3b46e642UL, 0x7a77fd5bUL, 0xb56b65dcUL, 0xf45a7ec5UL,
	0x370953eeUL, 0x763848f7UL, 0xb1ae09b8UL, 0xf09f12a1UL, 0x33cc3f8aUL,
	0x72fd2493UL
  },
  {
	0x00000000UL, 0x376ac201UL, 0x6ed48403UL, 0x59be4602UL, 0xdca80907UL,
	0xebc2cb06UL, 0xb27c8d04UL, 0x85164f05UL, 0xb851130eUL, 0x8f3bd10fUL,
	0xd685970dUL, 0xe1ef550cUL, 0x64f91a09UL, 0x5393d808UL, 0x0a2d9e0aUL,
	0x3d475c0bUL, 0x70a3261cUL, 0x47c9e41dUL, 0x1e77a21fUL, 0x291d601eUL,
	0xac0b2f1bUL, 0x9b61ed1aUL, 0xc2dfab18UL, 0xf5b56919UL, 0xc8f23512UL,
	0xff98f713UL, 0xa626b111UL, 0x914c7310UL, 0x145a3c15UL, 0x2330fe14UL,
	0x7a8eb816UL, 0x4de47a17UL, 0xe0464d38UL, 0xd72c8f39UL, 0x8e92c93bUL,
	0xb9f80b3aUL, 0x3cee443fUL, 0x0b84863eUL, 0x523ac03cUL, 0x6550023dUL,
	0x58175e36UL, 0x6f7d9c37UL, 0x36c3da35UL, 0x01a91834UL, 0x84bf5731UL,
	0xb3d59530UL, 0xea6bd332UL, 0xdd011133UL, 0x90e56b24UL, 0xa78fa925UL,
	0xfe31ef27UL, 0xc95b2d26UL, 0x4c4d6223UL, 0x7b27a022UL, 0x2299e620UL,
	0x15f32421UL, 0x28b4782aUL, 0x1fdeba2bUL, 0x4660fc29UL, 0x710a3e28UL,
	0xf41c712dUL, 0xc376b32cUL, 0x9ac8f52eUL, 0xada2372fUL, 0xc08d9a70UL,
	0xf7e75871UL, 0xae591e73UL, 0x9933dc72UL, 0x1c259377UL, 0x2b4f5176UL,
	0x72f11774UL, 0x459bd575UL, 0x78dc897eUL, 0x4fb64b7fUL, 0x16080d7dUL,
	0x2162cf7cUL, 0xa4748079UL, 0x931e4278UL, 0xcaa0047aUL, 0xfdcac67bUL,
	0xb02ebc6cUL, 0x87447e6dUL, 0xdefa386fUL, 0xe990fa6eUL, 0x6c86b56bUL,
	0x5bec776aUL, 0x02523168UL, 0x3538f369UL, 0x087faf62UL, 0x3f156d63UL,
	0x66ab2b61UL, 0x51c1e960UL, 0xd4d7a665UL, 0xe3bd6464UL, 0xba032266UL,
	0x8d69e067UL, 0x20cbd748UL, 0x17a11549UL, 0x4e1f534bUL, 0x7975914aUL,
	0xfc63de4fUL, 0xcb091c4eUL, 0x92b75a4cUL, 0xa5dd984dUL, 0x989ac446UL,
	0xaff00647UL, 0xf64e4045UL, 0xc1248244UL, 0x4432cd41UL, 0x73580f40UL,
	0x2ae64942UL, 0x1d8c8b43UL, 0x5068f154UL, 0x67023355UL, 0x3ebc7557UL,
	0x09d6b756UL, 0x8cc0f853UL, 0xbbaa3a52UL, 0xe2147c50UL, 0xd57ebe51UL,
	0xe839e25aUL, 0xdf53205bUL, 0x86ed6659UL, 0xb187a458UL, 0x3491eb5dUL,
	0x03fb295cUL, 0x5a456f5eUL, 0x6d2fad5fUL, 0x801b35e1UL, 0xb771f7e0UL,
	0xeecfb1e2UL, 0xd9a573e3UL, 0x5cb33ce6UL, 0x6bd9fee7UL, 0x3267b8e5UL,
	0x050d7ae4UL, 0x384a26efUL, 0x0f20e4eeUL, 0x569ea2ecUL, 0x61f460edUL,
	0xe4e22fe8UL, 0xd388ede9UL, 0x8a36abebUL, 0xbd5c69eaUL, 0xf0b813fdUL,
	0xc7d2d1fcUL, 0x9e6c97feUL, 0xa90655ffUL, 0x2c101afaUL, 0x1b7ad8fbUL,
	0x42c49ef9UL, 0x75ae5cf8UL, 0x48e900f3UL, 0x7f83c2f2UL, 0x263d84f0UL,
	0x115746f1UL, 0x944109f4UL, 0xa32bcbf5UL, 0xfa958df7UL, 0xcdff4ff6UL,
	0x605d78d9UL, 0x5737bad8UL, 0x0e89fcdaUL, 0x39e33edbUL, 0xbcf571deUL,
	0x8b9fb3dfUL, 0xd221f5ddUL, 0xe54b37dcUL, 0xd80c6bd7UL, 0xef66a9d6UL,
	0xb6d8efd4UL, 0x81b22dd5UL, 0x04a462d0UL, 0x33cea0d1UL, 0x6a70e6d3UL,
	0x5d1a24d2UL, 0x10fe5ec5UL, 0x27949cc4UL, 0x7e2adac6UL, 0x494018c7UL,
	0xcc5657c2UL, 0xfb3c95c3UL, 0xa282d3c1UL, 0x95e811c0UL, 0xa8af4dcbUL,
	0x9fc58fcaUL, 0xc67bc9c8UL, 0xf1110bc9UL, 0x740744ccUL, 0x436d86cdUL,
	0x1ad3c0cfUL, 0x2db902ceUL, 0x4096af91UL, 0x77fc6d90UL, 0x2e422b92UL,
	0x1928e993UL, 0x9c3ea696UL, 0xab546497UL, 0xf2ea2295UL, 0xc580e094UL,
	0xf8c7bc9fUL, 0xcfad7e9eUL, 0x9613389cUL, 0xa179fa9dUL, 0x246fb598UL,
	0x13057799UL, 0x4abb319bUL, 0x7dd1f39aUL, 0x3035898dUL, 0x075f4b8cUL,
	0x5ee10d8eUL, 0x698bcf8fUL, 0xec9d808aUL, 0xdbf7428bUL, 0x82490489UL,
	0xb523c688UL, 0x88649a83UL, 0xbf0e5882UL, 0xe6b01e80UL, 0xd1dadc81UL,
	0x54cc9384UL, 0x63a65185UL, 0x3a181787UL, 0x0d72d586UL, 0xa0d0e2a9UL,
	0x97ba20a8UL, 0xce0466aaUL, 0xf96ea4abUL, 0x7c78ebaeUL, 0x4b1229afUL,
	0x12ac6fadUL, 0x25c6adacUL, 0x1881f1a7UL, 0x2feb33a6UL, 0x765575a4UL,
	0x413fb7a5UL, 0xc429f8a0UL, 0xf3433aa1UL, 0xaafd7ca3UL, 0x9d97bea2UL,
	0xd073c4b5UL, 0xe71906b4UL, 0xbea740b6UL, 0x89cd82b7UL, 0x0cdbcdb2UL,
	0x3bb10fb3UL, 0x620f49b1UL, 0x55658bb0UL, 0x6822d7bbUL, 0x5f4815baUL,
	0x06f653b8UL, 0x319c91b9UL, 0xb48adebcUL, 0x83e01cbdUL, 0xda5e5abfUL,
	0xed3498beUL
  },
  {
	0x00000000UL, 0x6567bcb8UL, 0x8bc809aaUL, 0xeeafb512UL, 0x5797628fUL,
	0x32f0de37UL, 0xdc5f6b25UL, 0xb938d79dUL, 0xef28b4c5UL, 0x8a4f087dUL,
	0x64e0bd6fUL, 0x018701d7UL, 0xb8bfd64aUL, 0xddd86af2UL, 0x3377dfe0UL,
	0x56106358UL, 0x9f571950UL, 0xfa30a5e8UL, 0x149f10faUL, 0x71f8ac42UL,
	0xc8c07bdfUL, 0xada7c767UL, 0x43087275UL, 0x266fcecdUL, 0x707fad95UL,
	0x1518112dUL, 0xfbb7a43fUL, 0x9ed01887UL, 0x27e8cf1aUL, 0x428f73a2UL,
	0xac20c6b0UL, 0xc9477a08UL, 0x3eaf32a0UL, 0x5bc88e18UL, 0xb5673b0aUL,
	0xd00087b2UL, 0x6938502fUL, 0x0c5fec97UL, 0xe2f05985UL, 0x8797e53dUL,
	0xd1878665UL, 0xb4e03addUL, 0x5a4f8fcfUL, 0x3f283377UL, 0x8610e4eaUL,
	0xe3775852UL, 0x0dd8ed40UL, 0x68bf51f8UL, 0xa1f82bf0UL, 0xc49f9748UL,
	0x2a30225aUL, 0x4f579ee2UL, 0xf66f497fUL, 0x9308f5c7UL, 0x7da740d5UL,
	0x18c0fc6dUL, 0x4ed09f35UL, 0x2bb7238dUL, 0xc518969fUL, 0xa07f2a27UL,
	0x1947fdbaUL, 0x7c204102UL, 0x928ff410UL, 0xf7e848a8UL, 0x3d58149bUL,
	0x583fa823UL, 0xb6901d31UL, 0xd3f7a189UL, 0x6acf7614UL, 0x0fa8caacUL,
	0xe1077fbeUL, 0x8460c306UL, 0xd270a05eUL, 0xb7171ce6UL, 0x59b8a9f4UL,
	0x3cdf154cUL, 0x85e7c2d1UL, 0xe0807e69UL, 0x0e2fcb7bUL, 0x6b4877c3UL,
	0xa20f0dcbUL, 0xc768b173UL, 0x29c70461UL, 0x4ca0b8d9UL, 0xf5986f44UL,
	0x90ffd3fcUL, 0x7e5066eeUL, 0x1b37da56UL, 0x4d27b90eUL, 0x284005b6UL,
	0xc6efb0a4UL, 0xa3880c1cUL, 0x1ab0db81UL, 0x7fd76739UL, 0x9178d22bUL,
	0xf41f6e93UL, 0x03f7263bUL, 0x66909a83UL, 0x883f2f91UL, 0xed589329UL,
	0x546044b4UL, 0x3107f80cUL, 0xdfa84d1eUL, 0xbacff1a6UL, 0xecdf92feUL,
	0x89b82e46UL, 0x67179b54UL, 0x027027ecUL, 0xbb48f071UL, 0xde2f4cc9UL,
	0x3080f9dbUL, 0x55e74563UL, 0x9ca03f6bUL, 0xf9c783d3UL, 0x176836c1UL,
	0x720f8a79UL, 0xcb375de4UL, 0xae50e15cUL, 0x40ff544eUL, 0x2598e8f6UL,
	0x73888baeUL, 0x16ef3716UL, 0xf8408204UL, 0x9d273ebcUL, 0x241fe921UL,
	0x41785599UL, 0xafd7e08bUL, 0xcab05c33UL, 0x3bb659edUL, 0x5ed1e555UL,
	0xb07e5047UL, 0xd519ecffUL, 0x6c213b62UL, 0x094687daUL, 0xe7e932c8UL,
	0x828e8e70UL, 0xd49eed28UL, 0xb1f95190UL, 0x5f56e482UL, 0x3a31583aUL,
	0x83098fa7UL, 0xe66e331fUL, 0x08c1860dUL, 0x6da63ab5UL, 0xa4e140bdUL,
	0xc186fc05UL, 0x2f294917UL, 0x4a4ef5afUL, 0xf3762232UL, 0x96119e8aUL,
	0x78be2b98UL, 0x1dd99720UL, 0x4bc9f478UL, 0x2eae48c0UL, 0xc001fdd2UL,
	0xa566416aUL, 0x1c5e96f7UL, 0x79392a4fUL, 0x97969f5dUL, 0xf2f123e5UL,
	0x05196b4dUL, 0x607ed7f5UL, 0x8ed162e7UL, 0xebb6de5fUL, 0x528e09c2UL,
	0x37e9b57aUL, 0xd9460068UL, 0xbc21bcd0UL, 0xea31df88UL, 0x8f566330UL,
	0x61f9d622UL, 0x049e6a9aUL, 0xbda6bd07UL, 0xd8c101bfUL, 0x366eb4adUL,
	0x53090815UL, 0x9a4e721dUL, 0xff29cea5UL, 0x11867bb7UL, 0x74e1c70fUL,
	0xcdd91092UL, 0xa8beac2aUL, 0x46111938UL, 0x2376a580UL, 0x7566c6d8UL,
	0x10017a60UL, 0xfeaecf72UL, 0x9bc973caUL, 0x22f1a457UL, 0x479618efUL,
	0xa939adfdUL, 0xcc5e1145UL, 0x06ee4d76UL, 0x6389f1ceUL, 0x8d2644dcUL,
	0xe841f864UL, 0x51792ff9UL, 0x341e9341UL, 0xdab12653UL, 0xbfd69aebUL,
	0xe9c6f9b3UL, 0x8ca1450bUL, 0x620ef019UL, 0x07694ca1UL, 0xbe519b3cUL,
	0xdb362784UL, 0x35999296UL, 0x50fe2e2eUL, 0x99b95426UL, 0xfcdee89eUL,
	0x12715d8cUL, 0x7716e134UL, 0xce2e36a9UL, 0xab498a11UL, 0x45e63f03UL,
	0x208183bbUL, 0x7691e0e3UL, 0x13f65c5bUL, 0xfd59e949UL, 0x983e55f1UL,
	0x2106826cUL, 0x44613ed4UL, 0xaace8bc6UL, 0xcfa9377eUL, 0x38417fd6UL,
	0x5d26c36eUL, 0xb389767cUL, 0xd6eecac4UL, 0x6fd61d59UL, 0x0ab1a1e1UL,
	0xe41e14f3UL, 0x8179a84bUL, 0xd769cb13UL, 0xb20e77abUL, 0x5ca1c2b9UL,
	0x39c67e01UL, 0x80fea99cUL, 0xe5991524UL, 0x0b36a036UL, 0x6e511c8eUL,
	0xa7166686UL, 0xc271da3eUL, 0x2cde6f2cUL, 0x49b9d394UL, 0xf0810409UL,
	0x95e6b8b1UL, 0x7b490da3UL, 0x1e2eb11bUL, 0x483ed243UL, 0x2d596efbUL,
	0xc3f6dbe9UL, 0xa6916751UL, 0x1fa9b0ccUL, 0x7ace0c74UL, 0x9461b966UL,
	0xf10605deUL
#endif
  }
};

/*** End of inlined file: crc32.h ***/


#endif /* DYNAMIC_CRC_TABLE */

/* =========================================================================
 * This function can be used by asm versions of crc32()
 */
const unsigned long FAR * ZEXPORT get_crc_table()
{
#ifdef DYNAMIC_CRC_TABLE
	if (crc_table_empty)
		make_crc_table();
#endif /* DYNAMIC_CRC_TABLE */
	return (const unsigned long FAR *)crc_table;
}

/* ========================================================================= */
#define DO1 crc = crc_table[0][((int)crc ^ (*buf++)) & 0xff] ^ (crc >> 8)
#define DO8 DO1; DO1; DO1; DO1; DO1; DO1; DO1; DO1

/* ========================================================================= */
unsigned long ZEXPORT crc32 (unsigned long crc, const unsigned char FAR *buf, unsigned len)
{
	if (buf == Z_NULL) return 0UL;

#ifdef DYNAMIC_CRC_TABLE
	if (crc_table_empty)
		make_crc_table();
#endif /* DYNAMIC_CRC_TABLE */

#ifdef BYFOUR
	if (sizeof(void *) == sizeof(ptrdiff_t)) {
		u4 endian;

		endian = 1;
		if (*((unsigned char *)(&endian)))
			return crc32_little(crc, buf, len);
		else
			return crc32_big(crc, buf, len);
	}
#endif /* BYFOUR */
	crc = crc ^ 0xffffffffUL;
	while (len >= 8) {
		DO8;
		len -= 8;
	}
	if (len) do {
		DO1;
	} while (--len);
	return crc ^ 0xffffffffUL;
}

#ifdef BYFOUR

/* ========================================================================= */
#define DOLIT4 c ^= *buf4++; \
		c = crc_table[3][c & 0xff] ^ crc_table[2][(c >> 8) & 0xff] ^ \
			crc_table[1][(c >> 16) & 0xff] ^ crc_table[0][c >> 24]
#define DOLIT32 DOLIT4; DOLIT4; DOLIT4; DOLIT4; DOLIT4; DOLIT4; DOLIT4; DOLIT4

/* ========================================================================= */
local unsigned long crc32_little(unsigned long crc, const unsigned char FAR *buf, unsigned len)
{
	register u4 c;
	register const u4 FAR *buf4;

	c = (u4)crc;
	c = ~c;
	while (len && ((ptrdiff_t)buf & 3)) {
		c = crc_table[0][(c ^ *buf++) & 0xff] ^ (c >> 8);
		len--;
	}

	buf4 = (const u4 FAR *)(const void FAR *)buf;
	while (len >= 32) {
		DOLIT32;
		len -= 32;
	}
	while (len >= 4) {
		DOLIT4;
		len -= 4;
	}
	buf = (const unsigned char FAR *)buf4;

	if (len) do {
		c = crc_table[0][(c ^ *buf++) & 0xff] ^ (c >> 8);
	} while (--len);
	c = ~c;
	return (unsigned long)c;
}

/* ========================================================================= */
#define DOBIG4 c ^= *++buf4; \
		c = crc_table[4][c & 0xff] ^ crc_table[5][(c >> 8) & 0xff] ^ \
			crc_table[6][(c >> 16) & 0xff] ^ crc_table[7][c >> 24]
#define DOBIG32 DOBIG4; DOBIG4; DOBIG4; DOBIG4; DOBIG4; DOBIG4; DOBIG4; DOBIG4

/* ========================================================================= */
local unsigned long crc32_big (unsigned long crc, const unsigned char FAR *buf, unsigned len)
{
	register u4 c;
	register const u4 FAR *buf4;

	c = REV((u4)crc);
	c = ~c;
	while (len && ((ptrdiff_t)buf & 3)) {
		c = crc_table[4][(c >> 24) ^ *buf++] ^ (c << 8);
		len--;
	}

	buf4 = (const u4 FAR *)(const void FAR *)buf;
	buf4--;
	while (len >= 32) {
		DOBIG32;
		len -= 32;
	}
	while (len >= 4) {
		DOBIG4;
		len -= 4;
	}
	buf4++;
	buf = (const unsigned char FAR *)buf4;

	if (len) do {
		c = crc_table[4][(c >> 24) ^ *buf++] ^ (c << 8);
	} while (--len);
	c = ~c;
	return (unsigned long)(REV(c));
}

#endif /* BYFOUR */

#define GF2_DIM 32      /* dimension of GF(2) vectors (length of CRC) */

/* ========================================================================= */
local unsigned long gf2_matrix_times (unsigned long *mat, unsigned long vec)
{
	unsigned long sum;

	sum = 0;
	while (vec) {
		if (vec & 1)
			sum ^= *mat;
		vec >>= 1;
		mat++;
	}
	return sum;
}

/* ========================================================================= */
local void gf2_matrix_square (unsigned long *square, unsigned long *mat)
{
	int n;

	for (n = 0; n < GF2_DIM; n++)
		square[n] = gf2_matrix_times(mat, mat[n]);
}

/* ========================================================================= */
uLong ZEXPORT crc32_combine (uLong crc1, uLong crc2, z_off_t len2)
{
	int n;
	unsigned long row;
	unsigned long even[GF2_DIM];    /* even-power-of-two zeros operator */
	unsigned long odd[GF2_DIM];     /* odd-power-of-two zeros operator */

	/* degenerate case */
	if (len2 == 0)
		return crc1;

	/* put operator for one zero bit in odd */
	odd[0] = 0xedb88320L;           /* CRC-32 polynomial */
	row = 1;
	for (n = 1; n < GF2_DIM; n++) {
		odd[n] = row;
		row <<= 1;
	}

	/* put operator for two zero bits in even */
	gf2_matrix_square(even, odd);

	/* put operator for four zero bits in odd */
	gf2_matrix_square(odd, even);

	/* apply len2 zeros to crc1 (first square will put the operator for one
	   zero byte, eight zero bits, in even) */
	do {
		/* apply zeros operator for this bit of len2 */
		gf2_matrix_square(even, odd);
		if (len2 & 1)
			crc1 = gf2_matrix_times(even, crc1);
		len2 >>= 1;

		/* if no more bits set, then done */
		if (len2 == 0)
			break;

		/* another iteration of the loop with odd and even swapped */
		gf2_matrix_square(odd, even);
		if (len2 & 1)
			crc1 = gf2_matrix_times(odd, crc1);
		len2 >>= 1;

		/* if no more bits set, then done */
	} while (len2 != 0);

	/* return combined crc */
	crc1 ^= crc2;
	return crc1;
}

/*** End of inlined file: crc32.c ***/



/*** Start of inlined file: deflate.c ***/
/*
 *  ALGORITHM
 *
 *      The "deflation" process depends on being able to identify portions
 *      of the input text which are identical to earlier input (within a
 *      sliding window trailing behind the input currently being processed).
 *
 *      The most straightforward technique turns out to be the fastest for
 *      most input files: try all possible matches and select the longest.
 *      The key feature of this algorithm is that insertions into the string
 *      dictionary are very simple and thus fast, and deletions are avoided
 *      completely. Insertions are performed at each input character, whereas
 *      string matches are performed only when the previous match ends. So it
 *      is preferable to spend more time in matches to allow very fast string
 *      insertions and avoid deletions. The matching algorithm for small
 *      strings is inspired from that of Rabin & Karp. A brute force approach
 *      is used to find longer strings when a small match has been found.
 *      A similar algorithm is used in comic (by Jan-Mark Wams) and freeze
 *      (by Leonid Broukhis).
 *         A previous version of this file used a more sophisticated algorithm
 *      (by Fiala and Greene) which is guaranteed to run in linear amortized
 *      time, but has a larger average cost, uses more memory and is patented.
 *      However the F&G algorithm may be faster for some highly redundant
 *      files if the parameter max_chain_length (described below) is too large.
 *
 *  ACKNOWLEDGEMENTS
 *
 *      The idea of lazy evaluation of matches is due to Jan-Mark Wams, and
 *      I found it in 'freeze' written by Leonid Broukhis.
 *      Thanks to many people for bug reports and testing.
 *
 *  REFERENCES
 *
 *      Deutsch, L.P.,"DEFLATE Compressed Data Format Specification".
 *      Available in http://www.ietf.org/rfc/rfc1951.txt
 *
 *      A description of the Rabin and Karp algorithm is given in the book
 *         "Algorithms" by R. Sedgewick, Addison-Wesley, p252.
 *
 *      Fiala,E.R., and Greene,D.H.
 *         Data Compression with Finite Windows, Comm.ACM, 32,4 (1989) 490-595
 *
 */

/* @(#) $Id: deflate.c,v 1.1 2007/06/07 17:54:37 jules_rms Exp $ */


/*** Start of inlined file: deflate.h ***/
/* WARNING: this file should *not* be used by applications. It is
   part of the implementation of the compression library and is
   subject to change. Applications should only use zlib.h.
 */

/* @(#) $Id: deflate.h,v 1.1 2007/06/07 17:54:37 jules_rms Exp $ */

#ifndef DEFLATE_H
#define DEFLATE_H

/* define NO_GZIP when compiling if you want to disable gzip header and
   trailer creation by deflate().  NO_GZIP would be used to avoid linking in
   the crc code when it is not needed.  For shared libraries, gzip encoding
   should be left enabled. */
#ifndef NO_GZIP
#  define GZIP
#endif

#define NO_DUMMY_DECL

/* ===========================================================================
 * Internal compression state.
 */

#define LENGTH_CODES 29
/* number of length codes, not counting the special END_BLOCK code */

#define LITERALS  256
/* number of literal bytes 0..255 */

#define L_CODES (LITERALS+1+LENGTH_CODES)
/* number of Literal or Length codes, including the END_BLOCK code */

#define D_CODES   30
/* number of distance codes */

#define BL_CODES  19
/* number of codes used to transfer the bit lengths */

#define HEAP_SIZE (2*L_CODES+1)
/* maximum heap size */

#define MAX_BITS 15
/* All codes must not exceed MAX_BITS bits */

#define INIT_STATE    42
#define EXTRA_STATE   69
#define NAME_STATE    73
#define COMMENT_STATE 91
#define HCRC_STATE   103
#define BUSY_STATE   113
#define FINISH_STATE 666
/* Stream status */

/* Data structure describing a single value and its code string. */
typedef struct ct_data_s {
	union {
		ush  freq;       /* frequency count */
		ush  code;       /* bit string */
	} fc;
	union {
		ush  dad;        /* father node in Huffman tree */
		ush  len;        /* length of bit string */
	} dl;
} FAR ct_data;

#define Freq fc.freq
#define Code fc.code
#define Dad  dl.dad
#define Len  dl.len

typedef struct static_tree_desc_s  static_tree_desc;

typedef struct tree_desc_s {
	ct_data *dyn_tree;           /* the dynamic tree */
	int     max_code;            /* largest code with non zero frequency */
	static_tree_desc *stat_desc; /* the corresponding static tree */
} FAR tree_desc;

typedef ush Pos;
typedef Pos FAR Posf;
typedef unsigned IPos;

/* A Pos is an index in the character window. We use short instead of int to
 * save space in the various tables. IPos is used only for parameter passing.
 */

typedef struct internal_state {
	z_streamp strm;      /* pointer back to this zlib stream */
	int   status;        /* as the name implies */
	Bytef *pending_buf;  /* output still pending */
	ulg   pending_buf_size; /* size of pending_buf */
	Bytef *pending_out;  /* next pending byte to output to the stream */
	uInt   pending;      /* nb of bytes in the pending buffer */
	int   wrap;          /* bit 0 true for zlib, bit 1 true for gzip */
	gz_headerp  gzhead;  /* gzip header information to write */
	uInt   gzindex;      /* where in extra, name, or comment */
	Byte  method;        /* STORED (for zip only) or DEFLATED */
	int   last_flush;    /* value of flush param for previous deflate call */

				/* used by deflate.c: */

	uInt  w_size;        /* LZ77 window size (32K by default) */
	uInt  w_bits;        /* log2(w_size)  (8..16) */
	uInt  w_mask;        /* w_size - 1 */

	Bytef *window;
	/* Sliding window. Input bytes are read into the second half of the window,
	 * and move to the first half later to keep a dictionary of at least wSize
	 * bytes. With this organization, matches are limited to a distance of
	 * wSize-MAX_MATCH bytes, but this ensures that IO is always
	 * performed with a length multiple of the block size. Also, it limits
	 * the window size to 64K, which is quite useful on MSDOS.
	 * To do: use the user input buffer as sliding window.
	 */

	ulg window_size;
	/* Actual size of window: 2*wSize, except when the user input buffer
	 * is directly used as sliding window.
	 */

	Posf *prev;
	/* Link to older string with same hash index. To limit the size of this
	 * array to 64K, this link is maintained only for the last 32K strings.
	 * An index in this array is thus a window index modulo 32K.
	 */

	Posf *head; /* Heads of the hash chains or NIL. */

	uInt  ins_h;          /* hash index of string to be inserted */
	uInt  hash_size;      /* number of elements in hash table */
	uInt  hash_bits;      /* log2(hash_size) */
	uInt  hash_mask;      /* hash_size-1 */

	uInt  hash_shift;
	/* Number of bits by which ins_h must be shifted at each input
	 * step. It must be such that after MIN_MATCH steps, the oldest
	 * byte no longer takes part in the hash key, that is:
	 *   hash_shift * MIN_MATCH >= hash_bits
	 */

	long block_start;
	/* Window position at the beginning of the current output block. Gets
	 * negative when the window is moved backwards.
	 */

	uInt match_length;           /* length of best match */
	IPos prev_match;             /* previous match */
	int match_available;         /* set if previous match exists */
	uInt strstart;               /* start of string to insert */
	uInt match_start;            /* start of matching string */
	uInt lookahead;              /* number of valid bytes ahead in window */

	uInt prev_length;
	/* Length of the best match at previous step. Matches not greater than this
	 * are discarded. This is used in the lazy match evaluation.
	 */

	uInt max_chain_length;
	/* To speed up deflation, hash chains are never searched beyond this
	 * length.  A higher limit improves compression ratio but degrades the
	 * speed.
	 */

	uInt max_lazy_match;
	/* Attempt to find a better match only when the current match is strictly
	 * smaller than this value. This mechanism is used only for compression
	 * levels >= 4.
	 */
#   define max_insert_length  max_lazy_match
	/* Insert new strings in the hash table only if the match length is not
	 * greater than this length. This saves time but degrades compression.
	 * max_insert_length is used only for compression levels <= 3.
	 */

	int level;    /* compression level (1..9) */
	int strategy; /* favor or force Huffman coding*/

	uInt good_match;
	/* Use a faster search when the previous match is longer than this */

	int nice_match; /* Stop searching when current match exceeds this */

				/* used by trees.c: */
	/* Didn't use ct_data typedef below to supress compiler warning */
	struct ct_data_s dyn_ltree[HEAP_SIZE];   /* literal and length tree */
	struct ct_data_s dyn_dtree[2*D_CODES+1]; /* distance tree */
	struct ct_data_s bl_tree[2*BL_CODES+1];  /* Huffman tree for bit lengths */

	struct tree_desc_s l_desc;               /* desc. for literal tree */
	struct tree_desc_s d_desc;               /* desc. for distance tree */
	struct tree_desc_s bl_desc;              /* desc. for bit length tree */

	ush bl_count[MAX_BITS+1];
	/* number of codes at each bit length for an optimal tree */

	int heap[2*L_CODES+1];      /* heap used to build the Huffman trees */
	int heap_len;               /* number of elements in the heap */
	int heap_max;               /* element of largest frequency */
	/* The sons of heap[n] are heap[2*n] and heap[2*n+1]. heap[0] is not used.
	 * The same heap array is used to build all trees.
	 */

	uch depth[2*L_CODES+1];
	/* Depth of each subtree used as tie breaker for trees of equal frequency
	 */

	uchf *l_buf;          /* buffer for literals or lengths */

	uInt  lit_bufsize;
	/* Size of match buffer for literals/lengths.  There are 4 reasons for
	 * limiting lit_bufsize to 64K:
	 *   - frequencies can be kept in 16 bit counters
	 *   - if compression is not successful for the first block, all input
	 *     data is still in the window so we can still emit a stored block even
	 *     when input comes from standard input.  (This can also be done for
	 *     all blocks if lit_bufsize is not greater than 32K.)
	 *   - if compression is not successful for a file smaller than 64K, we can
	 *     even emit a stored file instead of a stored block (saving 5 bytes).
	 *     This is applicable only for zip (not gzip or zlib).
	 *   - creating new Huffman trees less frequently may not provide fast
	 *     adaptation to changes in the input data statistics. (Take for
	 *     example a binary file with poorly compressible code followed by
	 *     a highly compressible string table.) Smaller buffer sizes give
	 *     fast adaptation but have of course the overhead of transmitting
	 *     trees more frequently.
	 *   - I can't count above 4
	 */

	uInt last_lit;      /* running index in l_buf */

	ushf *d_buf;
	/* Buffer for distances. To simplify the code, d_buf and l_buf have
	 * the same number of elements. To use different lengths, an extra flag
	 * array would be necessary.
	 */

	ulg opt_len;        /* bit length of current block with optimal trees */
	ulg static_len;     /* bit length of current block with static trees */
	uInt matches;       /* number of string matches in current block */
	int last_eob_len;   /* bit length of EOB code for last block */

#ifdef DEBUG
	ulg compressed_len; /* total bit length of compressed file mod 2^32 */
	ulg bits_sent;      /* bit length of compressed data sent mod 2^32 */
#endif

	ush bi_buf;
	/* Output buffer. bits are inserted starting at the bottom (least
	 * significant bits).
	 */
	int bi_valid;
	/* Number of valid bits in bi_buf.  All bits above the last valid bit
	 * are always zero.
	 */

} FAR deflate_state;

/* Output a byte on the stream.
 * IN assertion: there is enough room in pending_buf.
 */
#define put_byte(s, c) {s->pending_buf[s->pending++] = (c);}

#define MIN_LOOKAHEAD (MAX_MATCH+MIN_MATCH+1)
/* Minimum amount of lookahead, except at the end of the input file.
 * See deflate.c for comments about the MIN_MATCH+1.
 */

#define MAX_DIST(s)  ((s)->w_size-MIN_LOOKAHEAD)
/* In order to simplify the code, particularly on 16 bit machines, match
 * distances are limited to MAX_DIST instead of WSIZE.
 */

		/* in trees.c */
void _tr_init         OF((deflate_state *s));
int  _tr_tally        OF((deflate_state *s, unsigned dist, unsigned lc));
void _tr_flush_block  OF((deflate_state *s, charf *buf, ulg stored_len,
						  int eof));
void _tr_align        OF((deflate_state *s));
void _tr_stored_block OF((deflate_state *s, charf *buf, ulg stored_len,
						  int eof));

#define d_code(dist) \
   ((dist) < 256 ? _dist_code[dist] : _dist_code[256+((dist)>>7)])
/* Mapping from a distance to a distance code. dist is the distance - 1 and
 * must not have side effects. _dist_code[256] and _dist_code[257] are never
 * used.
 */

#ifndef DEBUG
/* Inline versions of _tr_tally for speed: */

#if defined(GEN_TREES_H) || !defined(STDC)
  extern uch _length_code[];
  extern uch _dist_code[];
#else
  extern const uch _length_code[];
  extern const uch _dist_code[];
#endif

# define _tr_tally_lit(s, c, flush) \
  { uch cc = (c); \
	s->d_buf[s->last_lit] = 0; \
	s->l_buf[s->last_lit++] = cc; \
	s->dyn_ltree[cc].Freq++; \
	flush = (s->last_lit == s->lit_bufsize-1); \
   }
# define _tr_tally_dist(s, distance, length, flush) \
  { uch len = (length); \
	ush dist = (distance); \
	s->d_buf[s->last_lit] = dist; \
	s->l_buf[s->last_lit++] = len; \
	dist--; \
	s->dyn_ltree[_length_code[len]+LITERALS+1].Freq++; \
	s->dyn_dtree[d_code(dist)].Freq++; \
	flush = (s->last_lit == s->lit_bufsize-1); \
  }
#else
# define _tr_tally_lit(s, c, flush) flush = _tr_tally(s, 0, c)
# define _tr_tally_dist(s, distance, length, flush) \
			  flush = _tr_tally(s, distance, length)
#endif

#endif /* DEFLATE_H */

/*** End of inlined file: deflate.h ***/

const char deflate_copyright[] =
   " deflate 1.2.3 Copyright 1995-2005 Jean-loup Gailly ";
/*
  If you use the zlib library in a product, an acknowledgment is welcome
  in the documentation of your product. If for some reason you cannot
  include such an acknowledgment, I would appreciate that you keep this
  copyright string in the executable of your product.
 */

/* ===========================================================================
 *  Function prototypes.
 */
typedef enum {
	need_more,      /* block not completed, need more input or more output */
	block_done,     /* block flush performed */
	finish_started, /* finish started, need only more output at next deflate */
	finish_done     /* finish done, accept no more input or output */
} block_state;

typedef block_state (*compress_func) OF((deflate_state *s, int flush));
/* Compression function. Returns the block state after the call. */

local void fill_window    OF((deflate_state *s));
local block_state deflate_stored OF((deflate_state *s, int flush));
local block_state deflate_fast   OF((deflate_state *s, int flush));
#ifndef FASTEST
local block_state deflate_slow   OF((deflate_state *s, int flush));
#endif
local void lm_init        OF((deflate_state *s));
local void putShortMSB    OF((deflate_state *s, uInt b));
local void flush_pending  OF((z_streamp strm));
local int read_buf        OF((z_streamp strm, Bytef *buf, unsigned size));
#ifndef FASTEST
#ifdef ASMV
	  void match_init OF((void)); /* asm code initialization */
	  uInt longest_match  OF((deflate_state *s, IPos cur_match));
#else
local uInt longest_match  OF((deflate_state *s, IPos cur_match));
#endif
#endif
local uInt longest_match_fast OF((deflate_state *s, IPos cur_match));

#ifdef DEBUG
local  void check_match OF((deflate_state *s, IPos start, IPos match,
							int length));
#endif

/* ===========================================================================
 * Local data
 */

#define NIL 0
/* Tail of hash chains */

#ifndef TOO_FAR
#  define TOO_FAR 4096
#endif
/* Matches of length 3 are discarded if their distance exceeds TOO_FAR */

#define MIN_LOOKAHEAD (MAX_MATCH+MIN_MATCH+1)
/* Minimum amount of lookahead, except at the end of the input file.
 * See deflate.c for comments about the MIN_MATCH+1.
 */

/* Values for max_lazy_match, good_match and max_chain_length, depending on
 * the desired pack level (0..9). The values given below have been tuned to
 * exclude worst case performance for pathological files. Better values may be
 * found for specific files.
 */
typedef struct config_s {
   ush good_length; /* reduce lazy search above this match length */
   ush max_lazy;    /* do not perform lazy search above this match length */
   ush nice_length; /* quit search above this match length */
   ush max_chain;
   compress_func func;
} config;

#ifdef FASTEST
local const config configuration_table[2] = {
/*      good lazy nice chain */
/* 0 */ {0,    0,  0,    0, deflate_stored},  /* store only */
/* 1 */ {4,    4,  8,    4, deflate_fast}}; /* max speed, no lazy matches */
#else
local const config configuration_table[10] = {
/*      good lazy nice chain */
/* 0 */ {0,    0,  0,    0, deflate_stored},  /* store only */
/* 1 */ {4,    4,  8,    4, deflate_fast}, /* max speed, no lazy matches */
/* 2 */ {4,    5, 16,    8, deflate_fast},
/* 3 */ {4,    6, 32,   32, deflate_fast},

/* 4 */ {4,    4, 16,   16, deflate_slow},  /* lazy matches */
/* 5 */ {8,   16, 32,   32, deflate_slow},
/* 6 */ {8,   16, 128, 128, deflate_slow},
/* 7 */ {8,   32, 128, 256, deflate_slow},
/* 8 */ {32, 128, 258, 1024, deflate_slow},
/* 9 */ {32, 258, 258, 4096, deflate_slow}}; /* max compression */
#endif

/* Note: the deflate() code requires max_lazy >= MIN_MATCH and max_chain >= 4
 * For deflate_fast() (levels <= 3) good is ignored and lazy has a different
 * meaning.
 */

#define EQUAL 0
/* result of memcmp for equal strings */

#ifndef NO_DUMMY_DECL
struct static_tree_desc_s {int dummy;}; /* for buggy compilers */
#endif

/* ===========================================================================
 * Update a hash value with the given input byte
 * IN  assertion: all calls to to UPDATE_HASH are made with consecutive
 *    input characters, so that a running hash key can be computed from the
 *    previous key instead of complete recalculation each time.
 */
#define UPDATE_HASH(s,h,c) (h = (((h)<<s->hash_shift) ^ (c)) & s->hash_mask)

/* ===========================================================================
 * Insert string str in the dictionary and set match_head to the previous head
 * of the hash chain (the most recent string with same hash key). Return
 * the previous length of the hash chain.
 * If this file is compiled with -DFASTEST, the compression level is forced
 * to 1, and no hash chains are maintained.
 * IN  assertion: all calls to to INSERT_STRING are made with consecutive
 *    input characters and the first MIN_MATCH bytes of str are valid
 *    (except for the last MIN_MATCH-1 bytes of the input file).
 */
#ifdef FASTEST
#define INSERT_STRING(s, str, match_head) \
   (UPDATE_HASH(s, s->ins_h, s->window[(str) + (MIN_MATCH-1)]), \
	match_head = s->head[s->ins_h], \
	s->head[s->ins_h] = (Pos)(str))
#else
#define INSERT_STRING(s, str, match_head) \
   (UPDATE_HASH(s, s->ins_h, s->window[(str) + (MIN_MATCH-1)]), \
	match_head = s->prev[(str) & s->w_mask] = s->head[s->ins_h], \
	s->head[s->ins_h] = (Pos)(str))
#endif

/* ===========================================================================
 * Initialize the hash table (avoiding 64K overflow for 16 bit systems).
 * prev[] will be initialized on the fly.
 */
#define CLEAR_HASH(s) \
	s->head[s->hash_size-1] = NIL; \
	zmemzero((Bytef *)s->head, (unsigned)(s->hash_size-1)*sizeof(*s->head));

/* ========================================================================= */
int ZEXPORT deflateInit_(z_streamp strm, int level, const char *version, int stream_size)
{
	return deflateInit2_(strm, level, Z_DEFLATED, MAX_WBITS, DEF_MEM_LEVEL,
						 Z_DEFAULT_STRATEGY, version, stream_size);
	/* To do: ignore strm->next_in if we use it as window */
}

/* ========================================================================= */
int ZEXPORT deflateInit2_ (z_streamp strm, int  level, int  method, int  windowBits, int  memLevel, int  strategy, const char *version, int stream_size)
{
	deflate_state *s;
	int wrap = 1;
	static const char my_version[] = ZLIB_VERSION;

	ushf *overlay;
	/* We overlay pending_buf and d_buf+l_buf. This works since the average
	 * output size for (length,distance) codes is <= 24 bits.
	 */

	if (version == Z_NULL || version[0] != my_version[0] ||
		stream_size != sizeof(z_stream)) {
		return Z_VERSION_ERROR;
	}
	if (strm == Z_NULL) return Z_STREAM_ERROR;

	strm->msg = Z_NULL;
	if (strm->zalloc == (alloc_func)0) {
		strm->zalloc = zcalloc;
		strm->opaque = (voidpf)0;
	}
	if (strm->zfree == (free_func)0) strm->zfree = zcfree;

#ifdef FASTEST
	if (level != 0) level = 1;
#else
	if (level == Z_DEFAULT_COMPRESSION) level = 6;
#endif

	if (windowBits < 0) { /* suppress zlib wrapper */
		wrap = 0;
		windowBits = -windowBits;
	}
#ifdef GZIP
	else if (windowBits > 15) {
		wrap = 2;       /* write gzip wrapper instead */
		windowBits -= 16;
	}
#endif
	if (memLevel < 1 || memLevel > MAX_MEM_LEVEL || method != Z_DEFLATED ||
		windowBits < 8 || windowBits > 15 || level < 0 || level > 9 ||
		strategy < 0 || strategy > Z_FIXED) {
		return Z_STREAM_ERROR;
	}
	if (windowBits == 8) windowBits = 9;  /* until 256-byte window bug fixed */
	s = (deflate_state *) ZALLOC(strm, 1, sizeof(deflate_state));
	if (s == Z_NULL) return Z_MEM_ERROR;
	strm->state = (struct internal_state FAR *)s;
	s->strm = strm;

	s->wrap = wrap;
	s->gzhead = Z_NULL;
	s->w_bits = windowBits;
	s->w_size = 1 << s->w_bits;
	s->w_mask = s->w_size - 1;

	s->hash_bits = memLevel + 7;
	s->hash_size = 1 << s->hash_bits;
	s->hash_mask = s->hash_size - 1;
	s->hash_shift =  ((s->hash_bits+MIN_MATCH-1)/MIN_MATCH);

	s->window = (Bytef *) ZALLOC(strm, s->w_size, 2*sizeof(Byte));
	s->prev   = (Posf *)  ZALLOC(strm, s->w_size, sizeof(Pos));
	s->head   = (Posf *)  ZALLOC(strm, s->hash_size, sizeof(Pos));

	s->lit_bufsize = 1 << (memLevel + 6); /* 16K elements by default */

	overlay = (ushf *) ZALLOC(strm, s->lit_bufsize, sizeof(ush)+2);
	s->pending_buf = (uchf *) overlay;
	s->pending_buf_size = (ulg)s->lit_bufsize * (sizeof(ush)+2L);

	if (s->window == Z_NULL || s->prev == Z_NULL || s->head == Z_NULL ||
		s->pending_buf == Z_NULL) {
		s->status = FINISH_STATE;
		strm->msg = (char*)ERR_MSG(Z_MEM_ERROR);
		deflateEnd (strm);
		return Z_MEM_ERROR;
	}
	s->d_buf = overlay + s->lit_bufsize/sizeof(ush);
	s->l_buf = s->pending_buf + (1+sizeof(ush))*s->lit_bufsize;

	s->level = level;
	s->strategy = strategy;
	s->method = (Byte)method;

	return deflateReset(strm);
}

/* ========================================================================= */
int ZEXPORT deflateSetDictionary (z_streamp strm, const Bytef *dictionary, uInt  dictLength)
{
	deflate_state *s;
	uInt length = dictLength;
	uInt n;
	IPos hash_head = 0;

	if (strm == Z_NULL || strm->state == Z_NULL || dictionary == Z_NULL ||
		strm->state->wrap == 2 ||
		(strm->state->wrap == 1 && strm->state->status != INIT_STATE))
		return Z_STREAM_ERROR;

	s = strm->state;
	if (s->wrap)
		strm->adler = adler32(strm->adler, dictionary, dictLength);

	if (length < MIN_MATCH) return Z_OK;
	if (length > MAX_DIST(s)) {
		length = MAX_DIST(s);
		dictionary += dictLength - length; /* use the tail of the dictionary */
	}
	zmemcpy(s->window, dictionary, length);
	s->strstart = length;
	s->block_start = (long)length;

	/* Insert all strings in the hash table (except for the last two bytes).
	 * s->lookahead stays null, so s->ins_h will be recomputed at the next
	 * call of fill_window.
	 */
	s->ins_h = s->window[0];
	UPDATE_HASH(s, s->ins_h, s->window[1]);
	for (n = 0; n <= length - MIN_MATCH; n++) {
		INSERT_STRING(s, n, hash_head);
	}
	if (hash_head) hash_head = 0;  /* to make compiler happy */
	return Z_OK;
}

/* ========================================================================= */
int ZEXPORT deflateReset (z_streamp strm)
{
	deflate_state *s;

	if (strm == Z_NULL || strm->state == Z_NULL ||
		strm->zalloc == (alloc_func)0 || strm->zfree == (free_func)0) {
		return Z_STREAM_ERROR;
	}

	strm->total_in = strm->total_out = 0;
	strm->msg = Z_NULL; /* use zfree if we ever allocate msg dynamically */
	strm->data_type = Z_UNKNOWN;

	s = (deflate_state *)strm->state;
	s->pending = 0;
	s->pending_out = s->pending_buf;

	if (s->wrap < 0) {
		s->wrap = -s->wrap; /* was made negative by deflate(..., Z_FINISH); */
	}
	s->status = s->wrap ? INIT_STATE : BUSY_STATE;
	strm->adler =
#ifdef GZIP
		s->wrap == 2 ? crc32(0L, Z_NULL, 0) :
#endif
		adler32(0L, Z_NULL, 0);
	s->last_flush = Z_NO_FLUSH;

	_tr_init(s);
	lm_init(s);

	return Z_OK;
}

/* ========================================================================= */
int ZEXPORT deflateSetHeader (z_streamp strm, gz_headerp head)
{
	if (strm == Z_NULL || strm->state == Z_NULL) return Z_STREAM_ERROR;
	if (strm->state->wrap != 2) return Z_STREAM_ERROR;
	strm->state->gzhead = head;
	return Z_OK;
}

/* ========================================================================= */
int ZEXPORT deflatePrime (z_streamp strm, int bits, int value)
{
	if (strm == Z_NULL || strm->state == Z_NULL) return Z_STREAM_ERROR;
	strm->state->bi_valid = bits;
	strm->state->bi_buf = (ush)(value & ((1 << bits) - 1));
	return Z_OK;
}

/* ========================================================================= */
int ZEXPORT deflateParams (z_streamp strm, int level, int strategy)
{
	deflate_state *s;
	compress_func func;
	int err = Z_OK;

	if (strm == Z_NULL || strm->state == Z_NULL) return Z_STREAM_ERROR;
	s = strm->state;

#ifdef FASTEST
	if (level != 0) level = 1;
#else
	if (level == Z_DEFAULT_COMPRESSION) level = 6;
#endif
	if (level < 0 || level > 9 || strategy < 0 || strategy > Z_FIXED) {
		return Z_STREAM_ERROR;
	}
	func = configuration_table[s->level].func;

	if (func != configuration_table[level].func && strm->total_in != 0) {
		/* Flush the last buffer: */
		err = deflate(strm, Z_PARTIAL_FLUSH);
	}
	if (s->level != level) {
		s->level = level;
		s->max_lazy_match   = configuration_table[level].max_lazy;
		s->good_match       = configuration_table[level].good_length;
		s->nice_match       = configuration_table[level].nice_length;
		s->max_chain_length = configuration_table[level].max_chain;
	}
	s->strategy = strategy;
	return err;
}

/* ========================================================================= */
int ZEXPORT deflateTune (z_streamp strm, int good_length, int max_lazy, int nice_length, int max_chain)
{
	deflate_state *s;

	if (strm == Z_NULL || strm->state == Z_NULL) return Z_STREAM_ERROR;
	s = strm->state;
	s->good_match = good_length;
	s->max_lazy_match = max_lazy;
	s->nice_match = nice_length;
	s->max_chain_length = max_chain;
	return Z_OK;
}

/* =========================================================================
 * For the default windowBits of 15 and memLevel of 8, this function returns
 * a close to exact, as well as small, upper bound on the compressed size.
 * They are coded as constants here for a reason--if the #define's are
 * changed, then this function needs to be changed as well.  The return
 * value for 15 and 8 only works for those exact settings.
 *
 * For any setting other than those defaults for windowBits and memLevel,
 * the value returned is a conservative worst case for the maximum expansion
 * resulting from using fixed blocks instead of stored blocks, which deflate
 * can emit on compressed data for some combinations of the parameters.
 *
 * This function could be more sophisticated to provide closer upper bounds
 * for every combination of windowBits and memLevel, as well as wrap.
 * But even the conservative upper bound of about 14% expansion does not
 * seem onerous for output buffer allocation.
 */
uLong ZEXPORT deflateBound (z_streamp strm, uLong sourceLen)
{
	deflate_state *s;
	uLong destLen;

	/* conservative upper bound */
	destLen = sourceLen +
			  ((sourceLen + 7) >> 3) + ((sourceLen + 63) >> 6) + 11;

	/* if can't get parameters, return conservative bound */
	if (strm == Z_NULL || strm->state == Z_NULL)
		return destLen;

	/* if not default parameters, return conservative bound */
	s = strm->state;
	if (s->w_bits != 15 || s->hash_bits != 8 + 7)
		return destLen;

	/* default settings: return tight bound for that case */
	return compressBound(sourceLen);
}

/* =========================================================================
 * Put a short in the pending buffer. The 16-bit value is put in MSB order.
 * IN assertion: the stream state is correct and there is enough room in
 * pending_buf.
 */
local void putShortMSB (deflate_state *s, uInt b)
{
	put_byte(s, (Byte)(b >> 8));
	put_byte(s, (Byte)(b & 0xff));
}

/* =========================================================================
 * Flush as much pending output as possible. All deflate() output goes
 * through this function so some applications may wish to modify it
 * to avoid allocating a large strm->next_out buffer and copying into it.
 * (See also read_buf()).
 */
local void flush_pending (z_streamp strm)
{
	unsigned len = strm->state->pending;

	if (len > strm->avail_out) len = strm->avail_out;
	if (len == 0) return;

	zmemcpy(strm->next_out, strm->state->pending_out, len);
	strm->next_out  += len;
	strm->state->pending_out  += len;
	strm->total_out += len;
	strm->avail_out  -= len;
	strm->state->pending -= len;
	if (strm->state->pending == 0) {
		strm->state->pending_out = strm->state->pending_buf;
	}
}

/* ========================================================================= */
int ZEXPORT deflate (z_streamp strm, int flush)
{
	int old_flush; /* value of flush param for previous deflate call */
	deflate_state *s;

	if (strm == Z_NULL || strm->state == Z_NULL ||
		flush > Z_FINISH || flush < 0) {
		return Z_STREAM_ERROR;
	}
	s = strm->state;

	if (strm->next_out == Z_NULL ||
		(strm->next_in == Z_NULL && strm->avail_in != 0) ||
		(s->status == FINISH_STATE && flush != Z_FINISH)) {
		ERR_RETURN(strm, Z_STREAM_ERROR);
	}
	if (strm->avail_out == 0) ERR_RETURN(strm, Z_BUF_ERROR);

	s->strm = strm; /* just in case */
	old_flush = s->last_flush;
	s->last_flush = flush;

	/* Write the header */
	if (s->status == INIT_STATE) {
#ifdef GZIP
		if (s->wrap == 2) {
			strm->adler = crc32(0L, Z_NULL, 0);
			put_byte(s, 31);
			put_byte(s, 139);
			put_byte(s, 8);
			if (s->gzhead == NULL) {
				put_byte(s, 0);
				put_byte(s, 0);
				put_byte(s, 0);
				put_byte(s, 0);
				put_byte(s, 0);
				put_byte(s, s->level == 9 ? 2 :
							(s->strategy >= Z_HUFFMAN_ONLY || s->level < 2 ?
							 4 : 0));
				put_byte(s, OS_CODE);
				s->status = BUSY_STATE;
			}
			else {
				put_byte(s, (s->gzhead->text ? 1 : 0) +
							(s->gzhead->hcrc ? 2 : 0) +
							(s->gzhead->extra == Z_NULL ? 0 : 4) +
							(s->gzhead->name == Z_NULL ? 0 : 8) +
							(s->gzhead->comment == Z_NULL ? 0 : 16)
						);
				put_byte(s, (Byte)(s->gzhead->time & 0xff));
				put_byte(s, (Byte)((s->gzhead->time >> 8) & 0xff));
				put_byte(s, (Byte)((s->gzhead->time >> 16) & 0xff));
				put_byte(s, (Byte)((s->gzhead->time >> 24) & 0xff));
				put_byte(s, s->level == 9 ? 2 :
							(s->strategy >= Z_HUFFMAN_ONLY || s->level < 2 ?
							 4 : 0));
				put_byte(s, s->gzhead->os & 0xff);
				if (s->gzhead->extra != NULL) {
					put_byte(s, s->gzhead->extra_len & 0xff);
					put_byte(s, (s->gzhead->extra_len >> 8) & 0xff);
				}
				if (s->gzhead->hcrc)
					strm->adler = crc32(strm->adler, s->pending_buf,
										s->pending);
				s->gzindex = 0;
				s->status = EXTRA_STATE;
			}
		}
		else
#endif
		{
			uInt header = (Z_DEFLATED + ((s->w_bits-8)<<4)) << 8;
			uInt level_flags;

			if (s->strategy >= Z_HUFFMAN_ONLY || s->level < 2)
				level_flags = 0;
			else if (s->level < 6)
				level_flags = 1;
			else if (s->level == 6)
				level_flags = 2;
			else
				level_flags = 3;
			header |= (level_flags << 6);
			if (s->strstart != 0) header |= PRESET_DICT;
			header += 31 - (header % 31);

			s->status = BUSY_STATE;
			putShortMSB(s, header);

			/* Save the adler32 of the preset dictionary: */
			if (s->strstart != 0) {
				putShortMSB(s, (uInt)(strm->adler >> 16));
				putShortMSB(s, (uInt)(strm->adler & 0xffff));
			}
			strm->adler = adler32(0L, Z_NULL, 0);
		}
	}
#ifdef GZIP
	if (s->status == EXTRA_STATE) {
		if (s->gzhead->extra != NULL) {
			uInt beg = s->pending;  /* start of bytes to update crc */

			while (s->gzindex < (s->gzhead->extra_len & 0xffff)) {
				if (s->pending == s->pending_buf_size) {
					if (s->gzhead->hcrc && s->pending > beg)
						strm->adler = crc32(strm->adler, s->pending_buf + beg,
											s->pending - beg);
					flush_pending(strm);
					beg = s->pending;
					if (s->pending == s->pending_buf_size)
						break;
				}
				put_byte(s, s->gzhead->extra[s->gzindex]);
				s->gzindex++;
			}
			if (s->gzhead->hcrc && s->pending > beg)
				strm->adler = crc32(strm->adler, s->pending_buf + beg,
									s->pending - beg);
			if (s->gzindex == s->gzhead->extra_len) {
				s->gzindex = 0;
				s->status = NAME_STATE;
			}
		}
		else
			s->status = NAME_STATE;
	}
	if (s->status == NAME_STATE) {
		if (s->gzhead->name != NULL) {
			uInt beg = s->pending;  /* start of bytes to update crc */
			int val;

			do {
				if (s->pending == s->pending_buf_size) {
					if (s->gzhead->hcrc && s->pending > beg)
						strm->adler = crc32(strm->adler, s->pending_buf + beg,
											s->pending - beg);
					flush_pending(strm);
					beg = s->pending;
					if (s->pending == s->pending_buf_size) {
						val = 1;
						break;
					}
				}
				val = s->gzhead->name[s->gzindex++];
				put_byte(s, val);
			} while (val != 0);
			if (s->gzhead->hcrc && s->pending > beg)
				strm->adler = crc32(strm->adler, s->pending_buf + beg,
									s->pending - beg);
			if (val == 0) {
				s->gzindex = 0;
				s->status = COMMENT_STATE;
			}
		}
		else
			s->status = COMMENT_STATE;
	}
	if (s->status == COMMENT_STATE) {
		if (s->gzhead->comment != NULL) {
			uInt beg = s->pending;  /* start of bytes to update crc */
			int val;

			do {
				if (s->pending == s->pending_buf_size) {
					if (s->gzhead->hcrc && s->pending > beg)
						strm->adler = crc32(strm->adler, s->pending_buf + beg,
											s->pending - beg);
					flush_pending(strm);
					beg = s->pending;
					if (s->pending == s->pending_buf_size) {
						val = 1;
						break;
					}
				}
				val = s->gzhead->comment[s->gzindex++];
				put_byte(s, val);
			} while (val != 0);
			if (s->gzhead->hcrc && s->pending > beg)
				strm->adler = crc32(strm->adler, s->pending_buf + beg,
									s->pending - beg);
			if (val == 0)
				s->status = HCRC_STATE;
		}
		else
			s->status = HCRC_STATE;
	}
	if (s->status == HCRC_STATE) {
		if (s->gzhead->hcrc) {
			if (s->pending + 2 > s->pending_buf_size)
				flush_pending(strm);
			if (s->pending + 2 <= s->pending_buf_size) {
				put_byte(s, (Byte)(strm->adler & 0xff));
				put_byte(s, (Byte)((strm->adler >> 8) & 0xff));
				strm->adler = crc32(0L, Z_NULL, 0);
				s->status = BUSY_STATE;
			}
		}
		else
			s->status = BUSY_STATE;
	}
#endif

	/* Flush as much pending output as possible */
	if (s->pending != 0) {
		flush_pending(strm);
		if (strm->avail_out == 0) {
			/* Since avail_out is 0, deflate will be called again with
			 * more output space, but possibly with both pending and
			 * avail_in equal to zero. There won't be anything to do,
			 * but this is not an error situation so make sure we
			 * return OK instead of BUF_ERROR at next call of deflate:
			 */
			s->last_flush = -1;
			return Z_OK;
		}

	/* Make sure there is something to do and avoid duplicate consecutive
	 * flushes. For repeated and useless calls with Z_FINISH, we keep
	 * returning Z_STREAM_END instead of Z_BUF_ERROR.
	 */
	} else if (strm->avail_in == 0 && flush <= old_flush &&
			   flush != Z_FINISH) {
		ERR_RETURN(strm, Z_BUF_ERROR);
	}

	/* User must not provide more input after the first FINISH: */
	if (s->status == FINISH_STATE && strm->avail_in != 0) {
		ERR_RETURN(strm, Z_BUF_ERROR);
	}

	/* Start a new block or continue the current one.
	 */
	if (strm->avail_in != 0 || s->lookahead != 0 ||
		(flush != Z_NO_FLUSH && s->status != FINISH_STATE)) {
		block_state bstate;

		bstate = (*(configuration_table[s->level].func))(s, flush);

		if (bstate == finish_started || bstate == finish_done) {
			s->status = FINISH_STATE;
		}
		if (bstate == need_more || bstate == finish_started) {
			if (strm->avail_out == 0) {
				s->last_flush = -1; /* avoid BUF_ERROR next call, see above */
			}
			return Z_OK;
			/* If flush != Z_NO_FLUSH && avail_out == 0, the next call
			 * of deflate should use the same flush parameter to make sure
			 * that the flush is complete. So we don't have to output an
			 * empty block here, this will be done at next call. This also
			 * ensures that for a very small output buffer, we emit at most
			 * one empty block.
			 */
		}
		if (bstate == block_done) {
			if (flush == Z_PARTIAL_FLUSH) {
				_tr_align(s);
			} else { /* FULL_FLUSH or SYNC_FLUSH */
				_tr_stored_block(s, (char*)0, 0L, 0);
				/* For a full flush, this empty block will be recognized
				 * as a special marker by inflate_sync().
				 */
				if (flush == Z_FULL_FLUSH) {
					CLEAR_HASH(s);             /* forget history */
				}
			}
			flush_pending(strm);
			if (strm->avail_out == 0) {
			  s->last_flush = -1; /* avoid BUF_ERROR at next call, see above */
			  return Z_OK;
			}
		}
	}
	Assert(strm->avail_out > 0, "bug2");

	if (flush != Z_FINISH) return Z_OK;
	if (s->wrap <= 0) return Z_STREAM_END;

	/* Write the trailer */
#ifdef GZIP
	if (s->wrap == 2) {
		put_byte(s, (Byte)(strm->adler & 0xff));
		put_byte(s, (Byte)((strm->adler >> 8) & 0xff));
		put_byte(s, (Byte)((strm->adler >> 16) & 0xff));
		put_byte(s, (Byte)((strm->adler >> 24) & 0xff));
		put_byte(s, (Byte)(strm->total_in & 0xff));
		put_byte(s, (Byte)((strm->total_in >> 8) & 0xff));
		put_byte(s, (Byte)((strm->total_in >> 16) & 0xff));
		put_byte(s, (Byte)((strm->total_in >> 24) & 0xff));
	}
	else
#endif
	{
		putShortMSB(s, (uInt)(strm->adler >> 16));
		putShortMSB(s, (uInt)(strm->adler & 0xffff));
	}
	flush_pending(strm);
	/* If avail_out is zero, the application will call deflate again
	 * to flush the rest.
	 */
	if (s->wrap > 0) s->wrap = -s->wrap; /* write the trailer only once! */
	return s->pending != 0 ? Z_OK : Z_STREAM_END;
}

/* ========================================================================= */
int ZEXPORT deflateEnd (z_streamp strm)
{
	int status;

	if (strm == Z_NULL || strm->state == Z_NULL) return Z_STREAM_ERROR;

	status = strm->state->status;
	if (status != INIT_STATE &&
		status != EXTRA_STATE &&
		status != NAME_STATE &&
		status != COMMENT_STATE &&
		status != HCRC_STATE &&
		status != BUSY_STATE &&
		status != FINISH_STATE) {
	  return Z_STREAM_ERROR;
	}

	/* Deallocate in reverse order of allocations: */
	TRY_FREE(strm, strm->state->pending_buf);
	TRY_FREE(strm, strm->state->head);
	TRY_FREE(strm, strm->state->prev);
	TRY_FREE(strm, strm->state->window);

	ZFREE(strm, strm->state);
	strm->state = Z_NULL;

	return status == BUSY_STATE ? Z_DATA_ERROR : Z_OK;
}

/* =========================================================================
 * Copy the source state to the destination state.
 * To simplify the source, this is not supported for 16-bit MSDOS (which
 * doesn't have enough memory anyway to duplicate compression states).
 */
int ZEXPORT deflateCopy (z_streamp dest, z_streamp source)
{
#ifdef MAXSEG_64K
	return Z_STREAM_ERROR;
#else
	deflate_state *ds;
	deflate_state *ss;
	ushf *overlay;

	if (source == Z_NULL || dest == Z_NULL || source->state == Z_NULL) {
		return Z_STREAM_ERROR;
	}

	ss = source->state;

	zmemcpy(dest, source, sizeof(z_stream));

	ds = (deflate_state *) ZALLOC(dest, 1, sizeof(deflate_state));
	if (ds == Z_NULL) return Z_MEM_ERROR;
	dest->state = (struct internal_state FAR *) ds;
	zmemcpy(ds, ss, sizeof(deflate_state));
	ds->strm = dest;

	ds->window = (Bytef *) ZALLOC(dest, ds->w_size, 2*sizeof(Byte));
	ds->prev   = (Posf *)  ZALLOC(dest, ds->w_size, sizeof(Pos));
	ds->head   = (Posf *)  ZALLOC(dest, ds->hash_size, sizeof(Pos));
	overlay = (ushf *) ZALLOC(dest, ds->lit_bufsize, sizeof(ush)+2);
	ds->pending_buf = (uchf *) overlay;

	if (ds->window == Z_NULL || ds->prev == Z_NULL || ds->head == Z_NULL ||
		ds->pending_buf == Z_NULL) {
		deflateEnd (dest);
		return Z_MEM_ERROR;
	}
	/* following zmemcpy do not work for 16-bit MSDOS */
	zmemcpy(ds->window, ss->window, ds->w_size * 2 * sizeof(Byte));
	zmemcpy(ds->prev, ss->prev, ds->w_size * sizeof(Pos));
	zmemcpy(ds->head, ss->head, ds->hash_size * sizeof(Pos));
	zmemcpy(ds->pending_buf, ss->pending_buf, (uInt)ds->pending_buf_size);

	ds->pending_out = ds->pending_buf + (ss->pending_out - ss->pending_buf);
	ds->d_buf = overlay + ds->lit_bufsize/sizeof(ush);
	ds->l_buf = ds->pending_buf + (1+sizeof(ush))*ds->lit_bufsize;

	ds->l_desc.dyn_tree = ds->dyn_ltree;
	ds->d_desc.dyn_tree = ds->dyn_dtree;
	ds->bl_desc.dyn_tree = ds->bl_tree;

	return Z_OK;
#endif /* MAXSEG_64K */
}

/* ===========================================================================
 * Read a new buffer from the current input stream, update the adler32
 * and total number of bytes read.  All deflate() input goes through
 * this function so some applications may wish to modify it to avoid
 * allocating a large strm->next_in buffer and copying from it.
 * (See also flush_pending()).
 */
local int read_buf (z_streamp strm, Bytef *buf, unsigned size)
{
	unsigned len = strm->avail_in;

	if (len > size) len = size;
	if (len == 0) return 0;

	strm->avail_in  -= len;

	if (strm->state->wrap == 1) {
		strm->adler = adler32(strm->adler, strm->next_in, len);
	}
#ifdef GZIP
	else if (strm->state->wrap == 2) {
		strm->adler = crc32(strm->adler, strm->next_in, len);
	}
#endif
	zmemcpy(buf, strm->next_in, len);
	strm->next_in  += len;
	strm->total_in += len;

	return (int)len;
}

/* ===========================================================================
 * Initialize the "longest match" routines for a new zlib stream
 */
local void lm_init (deflate_state *s)
{
	s->window_size = (ulg)2L*s->w_size;

	CLEAR_HASH(s);

	/* Set the default configuration parameters:
	 */
	s->max_lazy_match   = configuration_table[s->level].max_lazy;
	s->good_match       = configuration_table[s->level].good_length;
	s->nice_match       = configuration_table[s->level].nice_length;
	s->max_chain_length = configuration_table[s->level].max_chain;

	s->strstart = 0;
	s->block_start = 0L;
	s->lookahead = 0;
	s->match_length = s->prev_length = MIN_MATCH-1;
	s->match_available = 0;
	s->ins_h = 0;
#ifndef FASTEST
#ifdef ASMV
	match_init(); /* initialize the asm code */
#endif
#endif
}

#ifndef FASTEST
/* ===========================================================================
 * Set match_start to the longest match starting at the given string and
 * return its length. Matches shorter or equal to prev_length are discarded,
 * in which case the result is equal to prev_length and match_start is
 * garbage.
 * IN assertions: cur_match is the head of the hash chain for the current
 *   string (strstart) and its distance is <= MAX_DIST, and prev_length >= 1
 * OUT assertion: the match length is not greater than s->lookahead.
 */
#ifndef ASMV
/* For 80x86 and 680x0, an optimized version will be provided in match.asm or
 * match.S. The code will be functionally equivalent.
 */
local uInt longest_match(deflate_state *s, IPos cur_match)
{
	unsigned chain_length = s->max_chain_length;/* max hash chain length */
	register Bytef *scan = s->window + s->strstart; /* current string */
	register Bytef *match;                       /* matched string */
	register int len;                           /* length of current match */
	int best_len = s->prev_length;              /* best match length so far */
	int nice_match = s->nice_match;             /* stop if match long enough */
	IPos limit = s->strstart > (IPos)MAX_DIST(s) ?
		s->strstart - (IPos)MAX_DIST(s) : NIL;
	/* Stop when cur_match becomes <= limit. To simplify the code,
	 * we prevent matches with the string of window index 0.
	 */
	Posf *prev = s->prev;
	uInt wmask = s->w_mask;

#ifdef UNALIGNED_OK
	/* Compare two bytes at a time. Note: this is not always beneficial.
	 * Try with and without -DUNALIGNED_OK to check.
	 */
	register Bytef *strend = s->window + s->strstart + MAX_MATCH - 1;
	register ush scan_start = *(ushf*)scan;
	register ush scan_end   = *(ushf*)(scan+best_len-1);
#else
	register Bytef *strend = s->window + s->strstart + MAX_MATCH;
	register Byte scan_end1  = scan[best_len-1];
	register Byte scan_end   = scan[best_len];
#endif

	/* The code is optimized for HASH_BITS >= 8 and MAX_MATCH-2 multiple of 16.
	 * It is easy to get rid of this optimization if necessary.
	 */
	Assert(s->hash_bits >= 8 && MAX_MATCH == 258, "Code too clever");

	/* Do not waste too much time if we already have a good match: */
	if (s->prev_length >= s->good_match) {
		chain_length >>= 2;
	}
	/* Do not look for matches beyond the end of the input. This is necessary
	 * to make deflate deterministic.
	 */
	if ((uInt)nice_match > s->lookahead) nice_match = s->lookahead;

	Assert((ulg)s->strstart <= s->window_size-MIN_LOOKAHEAD, "need lookahead");

	do {
		Assert(cur_match < s->strstart, "no future");
		match = s->window + cur_match;

		/* Skip to next match if the match length cannot increase
		 * or if the match length is less than 2.  Note that the checks below
		 * for insufficient lookahead only occur occasionally for performance
		 * reasons.  Therefore uninitialized memory will be accessed, and
		 * conditional jumps will be made that depend on those values.
		 * However the length of the match is limited to the lookahead, so
		 * the output of deflate is not affected by the uninitialized values.
		 */
#if (defined(UNALIGNED_OK) && MAX_MATCH == 258)
		/* This code assumes sizeof(unsigned short) == 2. Do not use
		 * UNALIGNED_OK if your compiler uses a different size.
		 */
		if (*(ushf*)(match+best_len-1) != scan_end ||
			*(ushf*)match != scan_start) continue;

		/* It is not necessary to compare scan[2] and match[2] since they are
		 * always equal when the other bytes match, given that the hash keys
		 * are equal and that HASH_BITS >= 8. Compare 2 bytes at a time at
		 * strstart+3, +5, ... up to strstart+257. We check for insufficient
		 * lookahead only every 4th comparison; the 128th check will be made
		 * at strstart+257. If MAX_MATCH-2 is not a multiple of 8, it is
		 * necessary to put more guard bytes at the end of the window, or
		 * to check more often for insufficient lookahead.
		 */
		Assert(scan[2] == match[2], "scan[2]?");
		scan++, match++;
		do {
		} while (*(ushf*)(scan+=2) == *(ushf*)(match+=2) &&
				 *(ushf*)(scan+=2) == *(ushf*)(match+=2) &&
				 *(ushf*)(scan+=2) == *(ushf*)(match+=2) &&
				 *(ushf*)(scan+=2) == *(ushf*)(match+=2) &&
				 scan < strend);
		/* The funny "do {}" generates better code on most compilers */

		/* Here, scan <= window+strstart+257 */
		Assert(scan <= s->window+(unsigned)(s->window_size-1), "wild scan");
		if (*scan == *match) scan++;

		len = (MAX_MATCH - 1) - (int)(strend-scan);
		scan = strend - (MAX_MATCH-1);

#else /* UNALIGNED_OK */

		if (match[best_len]   != scan_end  ||
			match[best_len-1] != scan_end1 ||
			*match            != *scan     ||
			*++match          != scan[1])      continue;

		/* The check at best_len-1 can be removed because it will be made
		 * again later. (This heuristic is not always a win.)
		 * It is not necessary to compare scan[2] and match[2] since they
		 * are always equal when the other bytes match, given that
		 * the hash keys are equal and that HASH_BITS >= 8.
		 */
		scan += 2, match++;
		Assert(*scan == *match, "match[2]?");

		/* We check for insufficient lookahead only every 8th comparison;
		 * the 256th check will be made at strstart+258.
		 */
		do {
		} while (*++scan == *++match && *++scan == *++match &&
				 *++scan == *++match && *++scan == *++match &&
				 *++scan == *++match && *++scan == *++match &&
				 *++scan == *++match && *++scan == *++match &&
				 scan < strend);

		Assert(scan <= s->window+(unsigned)(s->window_size-1), "wild scan");

		len = MAX_MATCH - (int)(strend - scan);
		scan = strend - MAX_MATCH;

#endif /* UNALIGNED_OK */

		if (len > best_len) {
			s->match_start = cur_match;
			best_len = len;
			if (len >= nice_match) break;
#ifdef UNALIGNED_OK
			scan_end = *(ushf*)(scan+best_len-1);
#else
			scan_end1  = scan[best_len-1];
			scan_end   = scan[best_len];
#endif
		}
	} while ((cur_match = prev[cur_match & wmask]) > limit
			 && --chain_length != 0);

	if ((uInt)best_len <= s->lookahead) return (uInt)best_len;
	return s->lookahead;
}
#endif /* ASMV */
#endif /* FASTEST */

/* ---------------------------------------------------------------------------
 * Optimized version for level == 1 or strategy == Z_RLE only
 */
local uInt longest_match_fast (deflate_state *s, IPos cur_match)
{
	register Bytef *scan = s->window + s->strstart; /* current string */
	register Bytef *match;                       /* matched string */
	register int len;                           /* length of current match */
	register Bytef *strend = s->window + s->strstart + MAX_MATCH;

	/* The code is optimized for HASH_BITS >= 8 and MAX_MATCH-2 multiple of 16.
	 * It is easy to get rid of this optimization if necessary.
	 */
	Assert(s->hash_bits >= 8 && MAX_MATCH == 258, "Code too clever");

	Assert((ulg)s->strstart <= s->window_size-MIN_LOOKAHEAD, "need lookahead");

	Assert(cur_match < s->strstart, "no future");

	match = s->window + cur_match;

	/* Return failure if the match length is less than 2:
	 */
	if (match[0] != scan[0] || match[1] != scan[1]) return MIN_MATCH-1;

	/* The check at best_len-1 can be removed because it will be made
	 * again later. (This heuristic is not always a win.)
	 * It is not necessary to compare scan[2] and match[2] since they
	 * are always equal when the other bytes match, given that
	 * the hash keys are equal and that HASH_BITS >= 8.
	 */
	scan += 2, match += 2;
	Assert(*scan == *match, "match[2]?");

	/* We check for insufficient lookahead only every 8th comparison;
	 * the 256th check will be made at strstart+258.
	 */
	do {
	} while (*++scan == *++match && *++scan == *++match &&
			 *++scan == *++match && *++scan == *++match &&
			 *++scan == *++match && *++scan == *++match &&
			 *++scan == *++match && *++scan == *++match &&
			 scan < strend);

	Assert(scan <= s->window+(unsigned)(s->window_size-1), "wild scan");

	len = MAX_MATCH - (int)(strend - scan);

	if (len < MIN_MATCH) return MIN_MATCH - 1;

	s->match_start = cur_match;
	return (uInt)len <= s->lookahead ? (uInt)len : s->lookahead;
}

#ifdef DEBUG
/* ===========================================================================
 * Check that the match at match_start is indeed a match.
 */
local void check_match(deflate_state *s, IPos start, IPos match, int length)
{
	/* check that the match is indeed a match */
	if (zmemcmp(s->window + match,
				s->window + start, length) != EQUAL) {
		fprintf(stderr, " start %u, match %u, length %d\n",
				start, match, length);
		do {
			fprintf(stderr, "%c%c", s->window[match++], s->window[start++]);
		} while (--length != 0);
		z_error("invalid match");
	}
	if (z_verbose > 1) {
		fprintf(stderr,"\\[%d,%d]", start-match, length);
		do { putc(s->window[start++], stderr); } while (--length != 0);
	}
}
#else
#  define check_match(s, start, match, length)
#endif /* DEBUG */

/* ===========================================================================
 * Fill the window when the lookahead becomes insufficient.
 * Updates strstart and lookahead.
 *
 * IN assertion: lookahead < MIN_LOOKAHEAD
 * OUT assertions: strstart <= window_size-MIN_LOOKAHEAD
 *    At least one byte has been read, or avail_in == 0; reads are
 *    performed for at least two bytes (required for the zip translate_eol
 *    option -- not supported here).
 */
local void fill_window (deflate_state *s)
{
	register unsigned n, m;
	register Posf *p;
	unsigned more;    /* Amount of free space at the end of the window. */
	uInt wsize = s->w_size;

	do {
		more = (unsigned)(s->window_size -(ulg)s->lookahead -(ulg)s->strstart);

		/* Deal with !@#$% 64K limit: */
		if (sizeof(int) <= 2) {
			if (more == 0 && s->strstart == 0 && s->lookahead == 0) {
				more = wsize;

			} else if (more == (unsigned)(-1)) {
				/* Very unlikely, but possible on 16 bit machine if
				 * strstart == 0 && lookahead == 1 (input done a byte at time)
				 */
				more--;
			}
		}

		/* If the window is almost full and there is insufficient lookahead,
		 * move the upper half to the lower one to make room in the upper half.
		 */
		if (s->strstart >= wsize+MAX_DIST(s)) {

			zmemcpy(s->window, s->window+wsize, (unsigned)wsize);
			s->match_start -= wsize;
			s->strstart    -= wsize; /* we now have strstart >= MAX_DIST */
			s->block_start -= (long) wsize;

			/* Slide the hash table (could be avoided with 32 bit values
			   at the expense of memory usage). We slide even when level == 0
			   to keep the hash table consistent if we switch back to level > 0
			   later. (Using level 0 permanently is not an optimal usage of
			   zlib, so we don't care about this pathological case.)
			 */
			/* %%% avoid this when Z_RLE */
			n = s->hash_size;
			p = &s->head[n];
			do {
				m = *--p;
				*p = (Pos)(m >= wsize ? m-wsize : NIL);
			} while (--n);

			n = wsize;
#ifndef FASTEST
			p = &s->prev[n];
			do {
				m = *--p;
				*p = (Pos)(m >= wsize ? m-wsize : NIL);
				/* If n is not on any hash chain, prev[n] is garbage but
				 * its value will never be used.
				 */
			} while (--n);
#endif
			more += wsize;
		}
		if (s->strm->avail_in == 0) return;

		/* If there was no sliding:
		 *    strstart <= WSIZE+MAX_DIST-1 && lookahead <= MIN_LOOKAHEAD - 1 &&
		 *    more == window_size - lookahead - strstart
		 * => more >= window_size - (MIN_LOOKAHEAD-1 + WSIZE + MAX_DIST-1)
		 * => more >= window_size - 2*WSIZE + 2
		 * In the BIG_MEM or MMAP case (not yet supported),
		 *   window_size == input_size + MIN_LOOKAHEAD  &&
		 *   strstart + s->lookahead <= input_size => more >= MIN_LOOKAHEAD.
		 * Otherwise, window_size == 2*WSIZE so more >= 2.
		 * If there was sliding, more >= WSIZE. So in all cases, more >= 2.
		 */
		Assert(more >= 2, "more < 2");

		n = read_buf(s->strm, s->window + s->strstart + s->lookahead, more);
		s->lookahead += n;

		/* Initialize the hash value now that we have some input: */
		if (s->lookahead >= MIN_MATCH) {
			s->ins_h = s->window[s->strstart];
			UPDATE_HASH(s, s->ins_h, s->window[s->strstart+1]);
#if MIN_MATCH != 3
			Call UPDATE_HASH() MIN_MATCH-3 more times
#endif
		}
		/* If the whole input has less than MIN_MATCH bytes, ins_h is garbage,
		 * but this is not important since only literal bytes will be emitted.
		 */

	} while (s->lookahead < MIN_LOOKAHEAD && s->strm->avail_in != 0);
}

/* ===========================================================================
 * Flush the current block, with given end-of-file flag.
 * IN assertion: strstart is set to the end of the current match.
 */
#define FLUSH_BLOCK_ONLY(s, eof) { \
   _tr_flush_block(s, (s->block_start >= 0L ? \
				   (charf *)&s->window[(unsigned)s->block_start] : \
				   (charf *)Z_NULL), \
				(ulg)((long)s->strstart - s->block_start), \
				(eof)); \
   s->block_start = s->strstart; \
   flush_pending(s->strm); \
   Tracev((stderr,"[FLUSH]")); \
}

/* Same but force premature exit if necessary. */
#define FLUSH_BLOCK(s, eof) { \
   FLUSH_BLOCK_ONLY(s, eof); \
   if (s->strm->avail_out == 0) return (eof) ? finish_started : need_more; \
}

/* ===========================================================================
 * Copy without compression as much as possible from the input stream, return
 * the current block state.
 * This function does not insert new strings in the dictionary since
 * uncompressible data is probably not useful. This function is used
 * only for the level=0 compression option.
 * NOTE: this function should be optimized to avoid extra copying from
 * window to pending_buf.
 */
local block_state deflate_stored(deflate_state *s, int flush)
{
	/* Stored blocks are limited to 0xffff bytes, pending_buf is limited
	 * to pending_buf_size, and each stored block has a 5 byte header:
	 */
	ulg max_block_size = 0xffff;
	ulg max_start;

	if (max_block_size > s->pending_buf_size - 5) {
		max_block_size = s->pending_buf_size - 5;
	}

	/* Copy as much as possible from input to output: */
	for (;;) {
		/* Fill the window as much as possible: */
		if (s->lookahead <= 1) {

			Assert(s->strstart < s->w_size+MAX_DIST(s) ||
				   s->block_start >= (long)s->w_size, "slide too late");

			fill_window(s);
			if (s->lookahead == 0 && flush == Z_NO_FLUSH) return need_more;

			if (s->lookahead == 0) break; /* flush the current block */
		}
		Assert(s->block_start >= 0L, "block gone");

		s->strstart += s->lookahead;
		s->lookahead = 0;

		/* Emit a stored block if pending_buf will be full: */
		max_start = s->block_start + max_block_size;
		if (s->strstart == 0 || (ulg)s->strstart >= max_start) {
			/* strstart == 0 is possible when wraparound on 16-bit machine */
			s->lookahead = (uInt)(s->strstart - max_start);
			s->strstart = (uInt)max_start;
			FLUSH_BLOCK(s, 0);
		}
		/* Flush if we may have to slide, otherwise block_start may become
		 * negative and the data will be gone:
		 */
		if (s->strstart - (uInt)s->block_start >= MAX_DIST(s)) {
			FLUSH_BLOCK(s, 0);
		}
	}
	FLUSH_BLOCK(s, flush == Z_FINISH);
	return flush == Z_FINISH ? finish_done : block_done;
}

/* ===========================================================================
 * Compress as much as possible from the input stream, return the current
 * block state.
 * This function does not perform lazy evaluation of matches and inserts
 * new strings in the dictionary only for unmatched strings or for short
 * matches. It is used only for the fast compression options.
 */
local block_state deflate_fast(deflate_state *s, int flush)
{
	IPos hash_head = NIL; /* head of the hash chain */
	int bflush;           /* set if current block must be flushed */

	for (;;) {
		/* Make sure that we always have enough lookahead, except
		 * at the end of the input file. We need MAX_MATCH bytes
		 * for the next match, plus MIN_MATCH bytes to insert the
		 * string following the next match.
		 */
		if (s->lookahead < MIN_LOOKAHEAD) {
			fill_window(s);
			if (s->lookahead < MIN_LOOKAHEAD && flush == Z_NO_FLUSH) {
				return need_more;
			}
			if (s->lookahead == 0) break; /* flush the current block */
		}

		/* Insert the string window[strstart .. strstart+2] in the
		 * dictionary, and set hash_head to the head of the hash chain:
		 */
		if (s->lookahead >= MIN_MATCH) {
			INSERT_STRING(s, s->strstart, hash_head);
		}

		/* Find the longest match, discarding those <= prev_length.
		 * At this point we have always match_length < MIN_MATCH
		 */
		if (hash_head != NIL && s->strstart - hash_head <= MAX_DIST(s)) {
			/* To simplify the code, we prevent matches with the string
			 * of window index 0 (in particular we have to avoid a match
			 * of the string with itself at the start of the input file).
			 */
#ifdef FASTEST
			if ((s->strategy != Z_HUFFMAN_ONLY && s->strategy != Z_RLE) ||
				(s->strategy == Z_RLE && s->strstart - hash_head == 1)) {
				s->match_length = longest_match_fast (s, hash_head);
			}
#else
			if (s->strategy != Z_HUFFMAN_ONLY && s->strategy != Z_RLE) {
				s->match_length = longest_match (s, hash_head);
			} else if (s->strategy == Z_RLE && s->strstart - hash_head == 1) {
				s->match_length = longest_match_fast (s, hash_head);
			}
#endif
			/* longest_match() or longest_match_fast() sets match_start */
		}
		if (s->match_length >= MIN_MATCH) {
			check_match(s, s->strstart, s->match_start, s->match_length);

			_tr_tally_dist(s, s->strstart - s->match_start,
						   s->match_length - MIN_MATCH, bflush);

			s->lookahead -= s->match_length;

			/* Insert new strings in the hash table only if the match length
			 * is not too large. This saves time but degrades compression.
			 */
#ifndef FASTEST
			if (s->match_length <= s->max_insert_length &&
				s->lookahead >= MIN_MATCH) {
				s->match_length--; /* string at strstart already in table */
				do {
					s->strstart++;
					INSERT_STRING(s, s->strstart, hash_head);
					/* strstart never exceeds WSIZE-MAX_MATCH, so there are
					 * always MIN_MATCH bytes ahead.
					 */
				} while (--s->match_length != 0);
				s->strstart++;
			} else
#endif
			{
				s->strstart += s->match_length;
				s->match_length = 0;
				s->ins_h = s->window[s->strstart];
				UPDATE_HASH(s, s->ins_h, s->window[s->strstart+1]);
#if MIN_MATCH != 3
				Call UPDATE_HASH() MIN_MATCH-3 more times
#endif
				/* If lookahead < MIN_MATCH, ins_h is garbage, but it does not
				 * matter since it will be recomputed at next deflate call.
				 */
			}
		} else {
			/* No match, output a literal byte */
			Tracevv((stderr,"%c", s->window[s->strstart]));
			_tr_tally_lit (s, s->window[s->strstart], bflush);
			s->lookahead--;
			s->strstart++;
		}
		if (bflush) FLUSH_BLOCK(s, 0);
	}
	FLUSH_BLOCK(s, flush == Z_FINISH);
	return flush == Z_FINISH ? finish_done : block_done;
}

#ifndef FASTEST
/* ===========================================================================
 * Same as above, but achieves better compression. We use a lazy
 * evaluation for matches: a match is finally adopted only if there is
 * no better match at the next window position.
 */
local block_state deflate_slow(deflate_state *s, int flush)
{
	IPos hash_head = NIL;    /* head of hash chain */
	int bflush;              /* set if current block must be flushed */

	/* Process the input block. */
	for (;;) {
		/* Make sure that we always have enough lookahead, except
		 * at the end of the input file. We need MAX_MATCH bytes
		 * for the next match, plus MIN_MATCH bytes to insert the
		 * string following the next match.
		 */
		if (s->lookahead < MIN_LOOKAHEAD) {
			fill_window(s);
			if (s->lookahead < MIN_LOOKAHEAD && flush == Z_NO_FLUSH) {
				return need_more;
			}
			if (s->lookahead == 0) break; /* flush the current block */
		}

		/* Insert the string window[strstart .. strstart+2] in the
		 * dictionary, and set hash_head to the head of the hash chain:
		 */
		if (s->lookahead >= MIN_MATCH) {
			INSERT_STRING(s, s->strstart, hash_head);
		}

		/* Find the longest match, discarding those <= prev_length.
		 */
		s->prev_length = s->match_length, s->prev_match = s->match_start;
		s->match_length = MIN_MATCH-1;

		if (hash_head != NIL && s->prev_length < s->max_lazy_match &&
			s->strstart - hash_head <= MAX_DIST(s)) {
			/* To simplify the code, we prevent matches with the string
			 * of window index 0 (in particular we have to avoid a match
			 * of the string with itself at the start of the input file).
			 */
			if (s->strategy != Z_HUFFMAN_ONLY && s->strategy != Z_RLE) {
				s->match_length = longest_match (s, hash_head);
			} else if (s->strategy == Z_RLE && s->strstart - hash_head == 1) {
				s->match_length = longest_match_fast (s, hash_head);
			}
			/* longest_match() or longest_match_fast() sets match_start */

			if (s->match_length <= 5 && (s->strategy == Z_FILTERED
#if TOO_FAR <= 32767
				|| (s->match_length == MIN_MATCH &&
					s->strstart - s->match_start > TOO_FAR)
#endif
				)) {

				/* If prev_match is also MIN_MATCH, match_start is garbage
				 * but we will ignore the current match anyway.
				 */
				s->match_length = MIN_MATCH-1;
			}
		}
		/* If there was a match at the previous step and the current
		 * match is not better, output the previous match:
		 */
		if (s->prev_length >= MIN_MATCH && s->match_length <= s->prev_length) {
			uInt max_insert = s->strstart + s->lookahead - MIN_MATCH;
			/* Do not insert strings in hash table beyond this. */

			check_match(s, s->strstart-1, s->prev_match, s->prev_length);

			_tr_tally_dist(s, s->strstart -1 - s->prev_match,
						   s->prev_length - MIN_MATCH, bflush);

			/* Insert in hash table all strings up to the end of the match.
			 * strstart-1 and strstart are already inserted. If there is not
			 * enough lookahead, the last two strings are not inserted in
			 * the hash table.
			 */
			s->lookahead -= s->prev_length-1;
			s->prev_length -= 2;
			do {
				if (++s->strstart <= max_insert) {
					INSERT_STRING(s, s->strstart, hash_head);
				}
			} while (--s->prev_length != 0);
			s->match_available = 0;
			s->match_length = MIN_MATCH-1;
			s->strstart++;

			if (bflush) FLUSH_BLOCK(s, 0);

		} else if (s->match_available) {
			/* If there was no match at the previous position, output a
			 * single literal. If there was a match but the current match
			 * is longer, truncate the previous match to a single literal.
			 */
			Tracevv((stderr,"%c", s->window[s->strstart-1]));
			_tr_tally_lit(s, s->window[s->strstart-1], bflush);
			if (bflush) {
				FLUSH_BLOCK_ONLY(s, 0);
			}
			s->strstart++;
			s->lookahead--;
			if (s->strm->avail_out == 0) return need_more;
		} else {
			/* There is no previous match to compare with, wait for
			 * the next step to decide.
			 */
			s->match_available = 1;
			s->strstart++;
			s->lookahead--;
		}
	}
	Assert (flush != Z_NO_FLUSH, "no flush?");
	if (s->match_available) {
		Tracevv((stderr,"%c", s->window[s->strstart-1]));
		_tr_tally_lit(s, s->window[s->strstart-1], bflush);
		s->match_available = 0;
	}
	FLUSH_BLOCK(s, flush == Z_FINISH);
	return flush == Z_FINISH ? finish_done : block_done;
}
#endif /* FASTEST */

#if 0
/* ===========================================================================
 * For Z_RLE, simply look for runs of bytes, generate matches only of distance
 * one.  Do not maintain a hash table.  (It will be regenerated if this run of
 * deflate switches away from Z_RLE.)
 */
local block_state deflate_rle(s, flush)
	deflate_state *s;
	int flush;
{
	int bflush;         /* set if current block must be flushed */
	uInt run;           /* length of run */
	uInt max;           /* maximum length of run */
	uInt prev;          /* byte at distance one to match */
	Bytef *scan;        /* scan for end of run */

	for (;;) {
		/* Make sure that we always have enough lookahead, except
		 * at the end of the input file. We need MAX_MATCH bytes
		 * for the longest encodable run.
		 */
		if (s->lookahead < MAX_MATCH) {
			fill_window(s);
			if (s->lookahead < MAX_MATCH && flush == Z_NO_FLUSH) {
				return need_more;
			}
			if (s->lookahead == 0) break; /* flush the current block */
		}

		/* See how many times the previous byte repeats */
		run = 0;
		if (s->strstart > 0) {      /* if there is a previous byte, that is */
			max = s->lookahead < MAX_MATCH ? s->lookahead : MAX_MATCH;
			scan = s->window + s->strstart - 1;
			prev = *scan++;
			do {
				if (*scan++ != prev)
					break;
			} while (++run < max);
		}

		/* Emit match if have run of MIN_MATCH or longer, else emit literal */
		if (run >= MIN_MATCH) {
			check_match(s, s->strstart, s->strstart - 1, run);
			_tr_tally_dist(s, 1, run - MIN_MATCH, bflush);
			s->lookahead -= run;
			s->strstart += run;
		} else {
			/* No match, output a literal byte */
			Tracevv((stderr,"%c", s->window[s->strstart]));
			_tr_tally_lit (s, s->window[s->strstart], bflush);
			s->lookahead--;
			s->strstart++;
		}
		if (bflush) FLUSH_BLOCK(s, 0);
	}
	FLUSH_BLOCK(s, flush == Z_FINISH);
	return flush == Z_FINISH ? finish_done : block_done;
}
#endif

/*** End of inlined file: deflate.c ***/


/*** Start of inlined file: inffast.c ***/

/*** Start of inlined file: inftrees.h ***/
/* WARNING: this file should *not* be used by applications. It is
   part of the implementation of the compression library and is
   subject to change. Applications should only use zlib.h.
 */

#ifndef _INFTREES_H_
#define _INFTREES_H_

/* Structure for decoding tables.  Each entry provides either the
   information needed to do the operation requested by the code that
   indexed that table entry, or it provides a pointer to another
   table that indexes more bits of the code.  op indicates whether
   the entry is a pointer to another table, a literal, a length or
   distance, an end-of-block, or an invalid code.  For a table
   pointer, the low four bits of op is the number of index bits of
   that table.  For a length or distance, the low four bits of op
   is the number of extra bits to get after the code.  bits is
   the number of bits in this code or part of the code to drop off
   of the bit buffer.  val is the actual byte to output in the case
   of a literal, the base length or distance, or the offset from
   the current table to the next table.  Each entry is four bytes. */
typedef struct {
	unsigned char op;           /* operation, extra bits, table bits */
	unsigned char bits;         /* bits in this part of the code */
	unsigned short val;         /* offset in table or code value */
} code;

/* op values as set by inflate_table():
	00000000 - literal
	0000tttt - table link, tttt != 0 is the number of table index bits
	0001eeee - length or distance, eeee is the number of extra bits
	01100000 - end of block
	01000000 - invalid code
 */

/* Maximum size of dynamic tree.  The maximum found in a long but non-
   exhaustive search was 1444 code structures (852 for length/literals
   and 592 for distances, the latter actually the result of an
   exhaustive search).  The true maximum is not known, but the value
   below is more than safe. */
#define ENOUGH 2048
#define MAXD 592

/* Type of code to build for inftable() */
typedef enum {
	CODES,
	LENS,
	DISTS
} codetype;

extern int inflate_table OF((codetype type, unsigned short FAR *lens,
							 unsigned codes, code FAR * FAR *table,
							 unsigned FAR *bits, unsigned short FAR *work));

#endif

/*** End of inlined file: inftrees.h ***/


/*** Start of inlined file: inflate.h ***/
/* WARNING: this file should *not* be used by applications. It is
   part of the implementation of the compression library and is
   subject to change. Applications should only use zlib.h.
 */

#ifndef _INFLATE_H_
#define _INFLATE_H_

/* define NO_GZIP when compiling if you want to disable gzip header and
   trailer decoding by inflate().  NO_GZIP would be used to avoid linking in
   the crc code when it is not needed.  For shared libraries, gzip decoding
   should be left enabled. */
#ifndef NO_GZIP
#  define GUNZIP
#endif

/* Possible inflate modes between inflate() calls */
typedef enum {
	HEAD,       /* i: waiting for magic header */
	FLAGS,      /* i: waiting for method and flags (gzip) */
	TIME,       /* i: waiting for modification time (gzip) */
	OS,         /* i: waiting for extra flags and operating system (gzip) */
	EXLEN,      /* i: waiting for extra length (gzip) */
	EXTRA,      /* i: waiting for extra bytes (gzip) */
	NAME,       /* i: waiting for end of file name (gzip) */
	COMMENT,    /* i: waiting for end of comment (gzip) */
	HCRC,       /* i: waiting for header crc (gzip) */
	DICTID,     /* i: waiting for dictionary check value */
	DICT,       /* waiting for inflateSetDictionary() call */
		TYPE,       /* i: waiting for type bits, including last-flag bit */
		TYPEDO,     /* i: same, but skip check to exit inflate on new block */
		STORED,     /* i: waiting for stored size (length and complement) */
		COPY,       /* i/o: waiting for input or output to copy stored block */
		TABLE,      /* i: waiting for dynamic block table lengths */
		LENLENS,    /* i: waiting for code length code lengths */
		CODELENS,   /* i: waiting for length/lit and distance code lengths */
			LEN,        /* i: waiting for length/lit code */
			LENEXT,     /* i: waiting for length extra bits */
			DIST,       /* i: waiting for distance code */
			DISTEXT,    /* i: waiting for distance extra bits */
			MATCH,      /* o: waiting for output space to copy string */
			LIT,        /* o: waiting for output space to write literal */
	CHECK,      /* i: waiting for 32-bit check value */
	LENGTH,     /* i: waiting for 32-bit length (gzip) */
	DONE,       /* finished check, done -- remain here until reset */
	BAD,        /* got a data error -- remain here until reset */
	MEM,        /* got an inflate() memory error -- remain here until reset */
	SYNC        /* looking for synchronization bytes to restart inflate() */
} inflate_mode;

/*
	State transitions between above modes -

	(most modes can go to the BAD or MEM mode -- not shown for clarity)

	Process header:
		HEAD -> (gzip) or (zlib)
		(gzip) -> FLAGS -> TIME -> OS -> EXLEN -> EXTRA -> NAME
		NAME -> COMMENT -> HCRC -> TYPE
		(zlib) -> DICTID or TYPE
		DICTID -> DICT -> TYPE
	Read deflate blocks:
			TYPE -> STORED or TABLE or LEN or CHECK
			STORED -> COPY -> TYPE
			TABLE -> LENLENS -> CODELENS -> LEN
	Read deflate codes:
				LEN -> LENEXT or LIT or TYPE
				LENEXT -> DIST -> DISTEXT -> MATCH -> LEN
				LIT -> LEN
	Process trailer:
		CHECK -> LENGTH -> DONE
 */

/* state maintained between inflate() calls.  Approximately 7K bytes. */
struct inflate_state {
	inflate_mode mode;          /* current inflate mode */
	int last;                   /* true if processing last block */
	int wrap;                   /* bit 0 true for zlib, bit 1 true for gzip */
	int havedict;               /* true if dictionary provided */
	int flags;                  /* gzip header method and flags (0 if zlib) */
	unsigned dmax;              /* zlib header max distance (INFLATE_STRICT) */
	unsigned long check;        /* protected copy of check value */
	unsigned long total;        /* protected copy of output count */
	gz_headerp head;            /* where to save gzip header information */
		/* sliding window */
	unsigned wbits;             /* log base 2 of requested window size */
	unsigned wsize;             /* window size or zero if not using window */
	unsigned whave;             /* valid bytes in the window */
	unsigned write;             /* window write index */
	unsigned char FAR *window;  /* allocated sliding window, if needed */
		/* bit accumulator */
	unsigned long hold;         /* input bit accumulator */
	unsigned bits;              /* number of bits in "in" */
		/* for string and stored block copying */
	unsigned length;            /* literal or length of data to copy */
	unsigned offset;            /* distance back to copy string from */
		/* for table and code decoding */
	unsigned extra;             /* extra bits needed */
		/* fixed and dynamic code tables */
	code const FAR *lencode;    /* starting table for length/literal codes */
	code const FAR *distcode;   /* starting table for distance codes */
	unsigned lenbits;           /* index bits for lencode */
	unsigned distbits;          /* index bits for distcode */
		/* dynamic table building */
	unsigned ncode;             /* number of code length code lengths */
	unsigned nlen;              /* number of length code lengths */
	unsigned ndist;             /* number of distance code lengths */
	unsigned have;              /* number of code lengths in lens[] */
	code FAR *next;             /* next available space in codes[] */
	unsigned short lens[320];   /* temporary storage for code lengths */
	unsigned short work[288];   /* work area for code table building */
	code codes[ENOUGH];         /* space for code tables */
};

#endif

/*** End of inlined file: inflate.h ***/


/*** Start of inlined file: inffast.h ***/
/* WARNING: this file should *not* be used by applications. It is
   part of the implementation of the compression library and is
   subject to change. Applications should only use zlib.h.
 */

void inflate_fast OF((z_streamp strm, unsigned start));

/*** End of inlined file: inffast.h ***/

#ifndef ASMINF

/* Allow machine dependent optimization for post-increment or pre-increment.
   Based on testing to date,
   Pre-increment preferred for:
   - PowerPC G3 (Adler)
   - MIPS R5000 (Randers-Pehrson)
   Post-increment preferred for:
   - none
   No measurable difference:
   - Pentium III (Anderson)
   - M68060 (Nikl)
 */
#ifdef POSTINC
#  define OFF 0
#  define PUP(a) *(a)++
#else
#  define OFF 1
#  define PUP(a) *++(a)
#endif

/*
   Decode literal, length, and distance codes and write out the resulting
   literal and match bytes until either not enough input or output is
   available, an end-of-block is encountered, or a data error is encountered.
   When large enough input and output buffers are supplied to inflate(), for
   example, a 16K input buffer and a 64K output buffer, more than 95% of the
   inflate execution time is spent in this routine.

   Entry assumptions:

		state->mode == LEN
		strm->avail_in >= 6
		strm->avail_out >= 258
		start >= strm->avail_out
		state->bits < 8

   On return, state->mode is one of:

		LEN -- ran out of enough output space or enough available input
		TYPE -- reached end of block code, inflate() to interpret next block
		BAD -- error in block data

   Notes:

	- The maximum input bits used by a length/distance pair is 15 bits for the
	  length code, 5 bits for the length extra, 15 bits for the distance code,
	  and 13 bits for the distance extra.  This totals 48 bits, or six bytes.
	  Therefore if strm->avail_in >= 6, then there is enough input to avoid
	  checking for available input while decoding.

	- The maximum bytes that a single length/distance pair can output is 258
	  bytes, which is the maximum length that can be coded.  inflate_fast()
	  requires strm->avail_out >= 258 for each loop to avoid checking for
	  output space.
 */
void inflate_fast (z_streamp strm, unsigned start)
{
	struct inflate_state FAR *state;
	unsigned char FAR *in;      /* local strm->next_in */
	unsigned char FAR *last;    /* while in < last, enough input available */
	unsigned char FAR *out;     /* local strm->next_out */
	unsigned char FAR *beg;     /* inflate()'s initial strm->next_out */
	unsigned char FAR *end;     /* while out < end, enough space available */
#ifdef INFLATE_STRICT
	unsigned dmax;              /* maximum distance from zlib header */
#endif
	unsigned wsize;             /* window size or zero if not using window */
	unsigned whave;             /* valid bytes in the window */
	unsigned write;             /* window write index */
	unsigned char FAR *window;  /* allocated sliding window, if wsize != 0 */
	unsigned long hold;         /* local strm->hold */
	unsigned bits;              /* local strm->bits */
	code const FAR *lcode;      /* local strm->lencode */
	code const FAR *dcode;      /* local strm->distcode */
	unsigned lmask;             /* mask for first level of length codes */
	unsigned dmask;             /* mask for first level of distance codes */
	code thisx;                  /* retrieved table entry */
	unsigned op;                /* code bits, operation, extra bits, or */
								/*  window position, window bytes to copy */
	unsigned len;               /* match length, unused bytes */
	unsigned dist;              /* match distance */
	unsigned char FAR *from;    /* where to copy match from */

	/* copy state to local variables */
	state = (struct inflate_state FAR *)strm->state;
	in = strm->next_in - OFF;
	last = in + (strm->avail_in - 5);
	out = strm->next_out - OFF;
	beg = out - (start - strm->avail_out);
	end = out + (strm->avail_out - 257);
#ifdef INFLATE_STRICT
	dmax = state->dmax;
#endif
	wsize = state->wsize;
	whave = state->whave;
	write = state->write;
	window = state->window;
	hold = state->hold;
	bits = state->bits;
	lcode = state->lencode;
	dcode = state->distcode;
	lmask = (1U << state->lenbits) - 1;
	dmask = (1U << state->distbits) - 1;

	/* decode literals and length/distances until end-of-block or not enough
	   input data or output space */
	do {
		if (bits < 15) {
			hold += (unsigned long)(PUP(in)) << bits;
			bits += 8;
			hold += (unsigned long)(PUP(in)) << bits;
			bits += 8;
		}
		thisx = lcode[hold & lmask];
	  dolen:
		op = (unsigned)(thisx.bits);
		hold >>= op;
		bits -= op;
		op = (unsigned)(thisx.op);
		if (op == 0) {                          /* literal */
			Tracevv((stderr, thisx.val >= 0x20 && thisx.val < 0x7f ?
					"inflate:         literal '%c'\n" :
					"inflate:         literal 0x%02x\n", thisx.val));
			PUP(out) = (unsigned char)(thisx.val);
		}
		else if (op & 16) {                     /* length base */
			len = (unsigned)(thisx.val);
			op &= 15;                           /* number of extra bits */
			if (op) {
				if (bits < op) {
					hold += (unsigned long)(PUP(in)) << bits;
					bits += 8;
				}
				len += (unsigned)hold & ((1U << op) - 1);
				hold >>= op;
				bits -= op;
			}
			Tracevv((stderr, "inflate:         length %u\n", len));
			if (bits < 15) {
				hold += (unsigned long)(PUP(in)) << bits;
				bits += 8;
				hold += (unsigned long)(PUP(in)) << bits;
				bits += 8;
			}
			thisx = dcode[hold & dmask];
		  dodist:
			op = (unsigned)(thisx.bits);
			hold >>= op;
			bits -= op;
			op = (unsigned)(thisx.op);
			if (op & 16) {                      /* distance base */
				dist = (unsigned)(thisx.val);
				op &= 15;                       /* number of extra bits */
				if (bits < op) {
					hold += (unsigned long)(PUP(in)) << bits;
					bits += 8;
					if (bits < op) {
						hold += (unsigned long)(PUP(in)) << bits;
						bits += 8;
					}
				}
				dist += (unsigned)hold & ((1U << op) - 1);
#ifdef INFLATE_STRICT
				if (dist > dmax) {
					strm->msg = (char *)"invalid distance too far back";
					state->mode = BAD;
					break;
				}
#endif
				hold >>= op;
				bits -= op;
				Tracevv((stderr, "inflate:         distance %u\n", dist));
				op = (unsigned)(out - beg);     /* max distance in output */
				if (dist > op) {                /* see if copy from window */
					op = dist - op;             /* distance back in window */
					if (op > whave) {
						strm->msg = (char *)"invalid distance too far back";
						state->mode = BAD;
						break;
					}
					from = window - OFF;
					if (write == 0) {           /* very common case */
						from += wsize - op;
						if (op < len) {         /* some from window */
							len -= op;
							do {
								PUP(out) = PUP(from);
							} while (--op);
							from = out - dist;  /* rest from output */
						}
					}
					else if (write < op) {      /* wrap around window */
						from += wsize + write - op;
						op -= write;
						if (op < len) {         /* some from end of window */
							len -= op;
							do {
								PUP(out) = PUP(from);
							} while (--op);
							from = window - OFF;
							if (write < len) {  /* some from start of window */
								op = write;
								len -= op;
								do {
									PUP(out) = PUP(from);
								} while (--op);
								from = out - dist;      /* rest from output */
							}
						}
					}
					else {                      /* contiguous in window */
						from += write - op;
						if (op < len) {         /* some from window */
							len -= op;
							do {
								PUP(out) = PUP(from);
							} while (--op);
							from = out - dist;  /* rest from output */
						}
					}
					while (len > 2) {
						PUP(out) = PUP(from);
						PUP(out) = PUP(from);
						PUP(out) = PUP(from);
						len -= 3;
					}
					if (len) {
						PUP(out) = PUP(from);
						if (len > 1)
							PUP(out) = PUP(from);
					}
				}
				else {
					from = out - dist;          /* copy direct from output */
					do {                        /* minimum length is three */
						PUP(out) = PUP(from);
						PUP(out) = PUP(from);
						PUP(out) = PUP(from);
						len -= 3;
					} while (len > 2);
					if (len) {
						PUP(out) = PUP(from);
						if (len > 1)
							PUP(out) = PUP(from);
					}
				}
			}
			else if ((op & 64) == 0) {          /* 2nd level distance code */
				thisx = dcode[thisx.val + (hold & ((1U << op) - 1))];
				goto dodist;
			}
			else {
				strm->msg = (char *)"invalid distance code";
				state->mode = BAD;
				break;
			}
		}
		else if ((op & 64) == 0) {              /* 2nd level length code */
			thisx = lcode[thisx.val + (hold & ((1U << op) - 1))];
			goto dolen;
		}
		else if (op & 32) {                     /* end-of-block */
			Tracevv((stderr, "inflate:         end of block\n"));
			state->mode = TYPE;
			break;
		}
		else {
			strm->msg = (char *)"invalid literal/length code";
			state->mode = BAD;
			break;
		}
	} while (in < last && out < end);

	/* return unused bytes (on entry, bits < 8, so in won't go too far back) */
	len = bits >> 3;
	in -= len;
	bits -= len << 3;
	hold &= (1U << bits) - 1;

	/* update state and return */
	strm->next_in = in + OFF;
	strm->next_out = out + OFF;
	strm->avail_in = (unsigned)(in < last ? 5 + (last - in) : 5 - (in - last));
	strm->avail_out = (unsigned)(out < end ?
								 257 + (end - out) : 257 - (out - end));
	state->hold = hold;
	state->bits = bits;
	return;
}

/*
   inflate_fast() speedups that turned out slower (on a PowerPC G3 750CXe):
   - Using bit fields for code structure
   - Different op definition to avoid & for extra bits (do & for table bits)
   - Three separate decoding do-loops for direct, window, and write == 0
   - Special case for distance > 1 copies to do overlapped load and store copy
   - Explicit branch predictions (based on measured branch probabilities)
   - Deferring match copy and interspersed it with decoding subsequent codes
   - Swapping literal/length else
   - Swapping window/direct else
   - Larger unrolled copy loops (three is about right)
   - Moving len -= 3 statement into middle of loop
 */

#endif /* !ASMINF */

/*** End of inlined file: inffast.c ***/

  #undef PULLBYTE
  #undef LOAD
  #undef RESTORE
  #undef INITBITS
  #undef NEEDBITS
  #undef DROPBITS
  #undef BYTEBITS

/*** Start of inlined file: inflate.c ***/
/*
 * Change history:
 *
 * 1.2.beta0    24 Nov 2002
 * - First version -- complete rewrite of inflate to simplify code, avoid
 *   creation of window when not needed, minimize use of window when it is
 *   needed, make inffast.c even faster, implement gzip decoding, and to
 *   improve code readability and style over the previous zlib inflate code
 *
 * 1.2.beta1    25 Nov 2002
 * - Use pointers for available input and output checking in inffast.c
 * - Remove input and output counters in inffast.c
 * - Change inffast.c entry and loop from avail_in >= 7 to >= 6
 * - Remove unnecessary second byte pull from length extra in inffast.c
 * - Unroll direct copy to three copies per loop in inffast.c
 *
 * 1.2.beta2    4 Dec 2002
 * - Change external routine names to reduce potential conflicts
 * - Correct filename to inffixed.h for fixed tables in inflate.c
 * - Make hbuf[] unsigned char to match parameter type in inflate.c
 * - Change strm->next_out[-state->offset] to *(strm->next_out - state->offset)
 *   to avoid negation problem on Alphas (64 bit) in inflate.c
 *
 * 1.2.beta3    22 Dec 2002
 * - Add comments on state->bits assertion in inffast.c
 * - Add comments on op field in inftrees.h
 * - Fix bug in reuse of allocated window after inflateReset()
 * - Remove bit fields--back to byte structure for speed
 * - Remove distance extra == 0 check in inflate_fast()--only helps for lengths
 * - Change post-increments to pre-increments in inflate_fast(), PPC biased?
 * - Add compile time option, POSTINC, to use post-increments instead (Intel?)
 * - Make MATCH copy in inflate() much faster for when inflate_fast() not used
 * - Use local copies of stream next and avail values, as well as local bit
 *   buffer and bit count in inflate()--for speed when inflate_fast() not used
 *
 * 1.2.beta4    1 Jan 2003
 * - Split ptr - 257 statements in inflate_table() to avoid compiler warnings
 * - Move a comment on output buffer sizes from inffast.c to inflate.c
 * - Add comments in inffast.c to introduce the inflate_fast() routine
 * - Rearrange window copies in inflate_fast() for speed and simplification
 * - Unroll last copy for window match in inflate_fast()
 * - Use local copies of window variables in inflate_fast() for speed
 * - Pull out common write == 0 case for speed in inflate_fast()
 * - Make op and len in inflate_fast() unsigned for consistency
 * - Add FAR to lcode and dcode declarations in inflate_fast()
 * - Simplified bad distance check in inflate_fast()
 * - Added inflateBackInit(), inflateBack(), and inflateBackEnd() in new
 *   source file infback.c to provide a call-back interface to inflate for
 *   programs like gzip and unzip -- uses window as output buffer to avoid
 *   window copying
 *
 * 1.2.beta5    1 Jan 2003
 * - Improved inflateBack() interface to allow the caller to provide initial
 *   input in strm.
 * - Fixed stored blocks bug in inflateBack()
 *
 * 1.2.beta6    4 Jan 2003
 * - Added comments in inffast.c on effectiveness of POSTINC
 * - Typecasting all around to reduce compiler warnings
 * - Changed loops from while (1) or do {} while (1) to for (;;), again to
 *   make compilers happy
 * - Changed type of window in inflateBackInit() to unsigned char *
 *
 * 1.2.beta7    27 Jan 2003
 * - Changed many types to unsigned or unsigned short to avoid warnings
 * - Added inflateCopy() function
 *
 * 1.2.0        9 Mar 2003
 * - Changed inflateBack() interface to provide separate opaque descriptors
 *   for the in() and out() functions
 * - Changed inflateBack() argument and in_func typedef to swap the length
 *   and buffer address return values for the input function
 * - Check next_in and next_out for Z_NULL on entry to inflate()
 *
 * The history for versions after 1.2.0 are in ChangeLog in zlib distribution.
 */


/*** Start of inlined file: inffast.h ***/
/* WARNING: this file should *not* be used by applications. It is
   part of the implementation of the compression library and is
   subject to change. Applications should only use zlib.h.
 */

void inflate_fast OF((z_streamp strm, unsigned start));

/*** End of inlined file: inffast.h ***/

#ifdef MAKEFIXED
#  ifndef BUILDFIXED
#    define BUILDFIXED
#  endif
#endif

/* function prototypes */
local void fixedtables OF((struct inflate_state FAR *state));
local int updatewindow OF((z_streamp strm, unsigned out));
#ifdef BUILDFIXED
   void makefixed OF((void));
#endif
local unsigned syncsearch OF((unsigned FAR *have, unsigned char FAR *buf,
							  unsigned len));

int ZEXPORT inflateReset (z_streamp strm)
{
	struct inflate_state FAR *state;

	if (strm == Z_NULL || strm->state == Z_NULL) return Z_STREAM_ERROR;
	state = (struct inflate_state FAR *)strm->state;
	strm->total_in = strm->total_out = state->total = 0;
	strm->msg = Z_NULL;
	strm->adler = 1;        /* to support ill-conceived Java test suite */
	state->mode = HEAD;
	state->last = 0;
	state->havedict = 0;
	state->dmax = 32768U;
	state->head = Z_NULL;
	state->wsize = 0;
	state->whave = 0;
	state->write = 0;
	state->hold = 0;
	state->bits = 0;
	state->lencode = state->distcode = state->next = state->codes;
	Tracev((stderr, "inflate: reset\n"));
	return Z_OK;
}

int ZEXPORT inflatePrime (z_streamp strm, int bits, int value)
{
	struct inflate_state FAR *state;

	if (strm == Z_NULL || strm->state == Z_NULL) return Z_STREAM_ERROR;
	state = (struct inflate_state FAR *)strm->state;
	if (bits > 16 || state->bits + bits > 32) return Z_STREAM_ERROR;
	value &= (1L << bits) - 1;
	state->hold += value << state->bits;
	state->bits += bits;
	return Z_OK;
}

int ZEXPORT inflateInit2_(z_streamp strm, int windowBits, const char *version, int stream_size)
{
	struct inflate_state FAR *state;

	if (version == Z_NULL || version[0] != ZLIB_VERSION[0] ||
		stream_size != (int)(sizeof(z_stream)))
		return Z_VERSION_ERROR;
	if (strm == Z_NULL) return Z_STREAM_ERROR;
	strm->msg = Z_NULL;                 /* in case we return an error */
	if (strm->zalloc == (alloc_func)0) {
		strm->zalloc = zcalloc;
		strm->opaque = (voidpf)0;
	}
	if (strm->zfree == (free_func)0) strm->zfree = zcfree;
	state = (struct inflate_state FAR *)
			ZALLOC(strm, 1, sizeof(struct inflate_state));
	if (state == Z_NULL) return Z_MEM_ERROR;
	Tracev((stderr, "inflate: allocated\n"));
	strm->state = (struct internal_state FAR *)state;
	if (windowBits < 0) {
		state->wrap = 0;
		windowBits = -windowBits;
	}
	else {
		state->wrap = (windowBits >> 4) + 1;
#ifdef GUNZIP
		if (windowBits < 48) windowBits &= 15;
#endif
	}
	if (windowBits < 8 || windowBits > 15) {
		ZFREE(strm, state);
		strm->state = Z_NULL;
		return Z_STREAM_ERROR;
	}
	state->wbits = (unsigned)windowBits;
	state->window = Z_NULL;
	return inflateReset(strm);
}

int ZEXPORT inflateInit_ (z_streamp strm, const char *version, int stream_size)
{
	return inflateInit2_(strm, DEF_WBITS, version, stream_size);
}

/*
   Return state with length and distance decoding tables and index sizes set to
   fixed code decoding.  Normally this returns fixed tables from inffixed.h.
   If BUILDFIXED is defined, then instead this routine builds the tables the
   first time it's called, and returns those tables the first time and
   thereafter.  This reduces the size of the code by about 2K bytes, in
   exchange for a little execution time.  However, BUILDFIXED should not be
   used for threaded applications, since the rewriting of the tables and virgin
   may not be thread-safe.
 */
local void fixedtables (struct inflate_state FAR *state)
{
#ifdef BUILDFIXED
	static int virgin = 1;
	static code *lenfix, *distfix;
	static code fixed[544];

	/* build fixed huffman tables if first call (may not be thread safe) */
	if (virgin) {
		unsigned sym, bits;
		static code *next;

		/* literal/length table */
		sym = 0;
		while (sym < 144) state->lens[sym++] = 8;
		while (sym < 256) state->lens[sym++] = 9;
		while (sym < 280) state->lens[sym++] = 7;
		while (sym < 288) state->lens[sym++] = 8;
		next = fixed;
		lenfix = next;
		bits = 9;
		inflate_table(LENS, state->lens, 288, &(next), &(bits), state->work);

		/* distance table */
		sym = 0;
		while (sym < 32) state->lens[sym++] = 5;
		distfix = next;
		bits = 5;
		inflate_table(DISTS, state->lens, 32, &(next), &(bits), state->work);

		/* do this just once */
		virgin = 0;
	}
#else /* !BUILDFIXED */

/*** Start of inlined file: inffixed.h ***/
	/* WARNING: this file should *not* be used by applications. It
	   is part of the implementation of the compression library and
	   is subject to change. Applications should only use zlib.h.
	 */

	static const code lenfix[512] = {
		{96,7,0},{0,8,80},{0,8,16},{20,8,115},{18,7,31},{0,8,112},{0,8,48},
		{0,9,192},{16,7,10},{0,8,96},{0,8,32},{0,9,160},{0,8,0},{0,8,128},
		{0,8,64},{0,9,224},{16,7,6},{0,8,88},{0,8,24},{0,9,144},{19,7,59},
		{0,8,120},{0,8,56},{0,9,208},{17,7,17},{0,8,104},{0,8,40},{0,9,176},
		{0,8,8},{0,8,136},{0,8,72},{0,9,240},{16,7,4},{0,8,84},{0,8,20},
		{21,8,227},{19,7,43},{0,8,116},{0,8,52},{0,9,200},{17,7,13},{0,8,100},
		{0,8,36},{0,9,168},{0,8,4},{0,8,132},{0,8,68},{0,9,232},{16,7,8},
		{0,8,92},{0,8,28},{0,9,152},{20,7,83},{0,8,124},{0,8,60},{0,9,216},
		{18,7,23},{0,8,108},{0,8,44},{0,9,184},{0,8,12},{0,8,140},{0,8,76},
		{0,9,248},{16,7,3},{0,8,82},{0,8,18},{21,8,163},{19,7,35},{0,8,114},
		{0,8,50},{0,9,196},{17,7,11},{0,8,98},{0,8,34},{0,9,164},{0,8,2},
		{0,8,130},{0,8,66},{0,9,228},{16,7,7},{0,8,90},{0,8,26},{0,9,148},
		{20,7,67},{0,8,122},{0,8,58},{0,9,212},{18,7,19},{0,8,106},{0,8,42},
		{0,9,180},{0,8,10},{0,8,138},{0,8,74},{0,9,244},{16,7,5},{0,8,86},
		{0,8,22},{64,8,0},{19,7,51},{0,8,118},{0,8,54},{0,9,204},{17,7,15},
		{0,8,102},{0,8,38},{0,9,172},{0,8,6},{0,8,134},{0,8,70},{0,9,236},
		{16,7,9},{0,8,94},{0,8,30},{0,9,156},{20,7,99},{0,8,126},{0,8,62},
		{0,9,220},{18,7,27},{0,8,110},{0,8,46},{0,9,188},{0,8,14},{0,8,142},
		{0,8,78},{0,9,252},{96,7,0},{0,8,81},{0,8,17},{21,8,131},{18,7,31},
		{0,8,113},{0,8,49},{0,9,194},{16,7,10},{0,8,97},{0,8,33},{0,9,162},
		{0,8,1},{0,8,129},{0,8,65},{0,9,226},{16,7,6},{0,8,89},{0,8,25},
		{0,9,146},{19,7,59},{0,8,121},{0,8,57},{0,9,210},{17,7,17},{0,8,105},
		{0,8,41},{0,9,178},{0,8,9},{0,8,137},{0,8,73},{0,9,242},{16,7,4},
		{0,8,85},{0,8,21},{16,8,258},{19,7,43},{0,8,117},{0,8,53},{0,9,202},
		{17,7,13},{0,8,101},{0,8,37},{0,9,170},{0,8,5},{0,8,133},{0,8,69},
		{0,9,234},{16,7,8},{0,8,93},{0,8,29},{0,9,154},{20,7,83},{0,8,125},
		{0,8,61},{0,9,218},{18,7,23},{0,8,109},{0,8,45},{0,9,186},{0,8,13},
		{0,8,141},{0,8,77},{0,9,250},{16,7,3},{0,8,83},{0,8,19},{21,8,195},
		{19,7,35},{0,8,115},{0,8,51},{0,9,198},{17,7,11},{0,8,99},{0,8,35},
		{0,9,166},{0,8,3},{0,8,131},{0,8,67},{0,9,230},{16,7,7},{0,8,91},
		{0,8,27},{0,9,150},{20,7,67},{0,8,123},{0,8,59},{0,9,214},{18,7,19},
		{0,8,107},{0,8,43},{0,9,182},{0,8,11},{0,8,139},{0,8,75},{0,9,246},
		{16,7,5},{0,8,87},{0,8,23},{64,8,0},{19,7,51},{0,8,119},{0,8,55},
		{0,9,206},{17,7,15},{0,8,103},{0,8,39},{0,9,174},{0,8,7},{0,8,135},
		{0,8,71},{0,9,238},{16,7,9},{0,8,95},{0,8,31},{0,9,158},{20,7,99},
		{0,8,127},{0,8,63},{0,9,222},{18,7,27},{0,8,111},{0,8,47},{0,9,190},
		{0,8,15},{0,8,143},{0,8,79},{0,9,254},{96,7,0},{0,8,80},{0,8,16},
		{20,8,115},{18,7,31},{0,8,112},{0,8,48},{0,9,193},{16,7,10},{0,8,96},
		{0,8,32},{0,9,161},{0,8,0},{0,8,128},{0,8,64},{0,9,225},{16,7,6},
		{0,8,88},{0,8,24},{0,9,145},{19,7,59},{0,8,120},{0,8,56},{0,9,209},
		{17,7,17},{0,8,104},{0,8,40},{0,9,177},{0,8,8},{0,8,136},{0,8,72},
		{0,9,241},{16,7,4},{0,8,84},{0,8,20},{21,8,227},{19,7,43},{0,8,116},
		{0,8,52},{0,9,201},{17,7,13},{0,8,100},{0,8,36},{0,9,169},{0,8,4},
		{0,8,132},{0,8,68},{0,9,233},{16,7,8},{0,8,92},{0,8,28},{0,9,153},
		{20,7,83},{0,8,124},{0,8,60},{0,9,217},{18,7,23},{0,8,108},{0,8,44},
		{0,9,185},{0,8,12},{0,8,140},{0,8,76},{0,9,249},{16,7,3},{0,8,82},
		{0,8,18},{21,8,163},{19,7,35},{0,8,114},{0,8,50},{0,9,197},{17,7,11},
		{0,8,98},{0,8,34},{0,9,165},{0,8,2},{0,8,130},{0,8,66},{0,9,229},
		{16,7,7},{0,8,90},{0,8,26},{0,9,149},{20,7,67},{0,8,122},{0,8,58},
		{0,9,213},{18,7,19},{0,8,106},{0,8,42},{0,9,181},{0,8,10},{0,8,138},
		{0,8,74},{0,9,245},{16,7,5},{0,8,86},{0,8,22},{64,8,0},{19,7,51},
		{0,8,118},{0,8,54},{0,9,205},{17,7,15},{0,8,102},{0,8,38},{0,9,173},
		{0,8,6},{0,8,134},{0,8,70},{0,9,237},{16,7,9},{0,8,94},{0,8,30},
		{0,9,157},{20,7,99},{0,8,126},{0,8,62},{0,9,221},{18,7,27},{0,8,110},
		{0,8,46},{0,9,189},{0,8,14},{0,8,142},{0,8,78},{0,9,253},{96,7,0},
		{0,8,81},{0,8,17},{21,8,131},{18,7,31},{0,8,113},{0,8,49},{0,9,195},
		{16,7,10},{0,8,97},{0,8,33},{0,9,163},{0,8,1},{0,8,129},{0,8,65},
		{0,9,227},{16,7,6},{0,8,89},{0,8,25},{0,9,147},{19,7,59},{0,8,121},
		{0,8,57},{0,9,211},{17,7,17},{0,8,105},{0,8,41},{0,9,179},{0,8,9},
		{0,8,137},{0,8,73},{0,9,243},{16,7,4},{0,8,85},{0,8,21},{16,8,258},
		{19,7,43},{0,8,117},{0,8,53},{0,9,203},{17,7,13},{0,8,101},{0,8,37},
		{0,9,171},{0,8,5},{0,8,133},{0,8,69},{0,9,235},{16,7,8},{0,8,93},
		{0,8,29},{0,9,155},{20,7,83},{0,8,125},{0,8,61},{0,9,219},{18,7,23},
		{0,8,109},{0,8,45},{0,9,187},{0,8,13},{0,8,141},{0,8,77},{0,9,251},
		{16,7,3},{0,8,83},{0,8,19},{21,8,195},{19,7,35},{0,8,115},{0,8,51},
		{0,9,199},{17,7,11},{0,8,99},{0,8,35},{0,9,167},{0,8,3},{0,8,131},
		{0,8,67},{0,9,231},{16,7,7},{0,8,91},{0,8,27},{0,9,151},{20,7,67},
		{0,8,123},{0,8,59},{0,9,215},{18,7,19},{0,8,107},{0,8,43},{0,9,183},
		{0,8,11},{0,8,139},{0,8,75},{0,9,247},{16,7,5},{0,8,87},{0,8,23},
		{64,8,0},{19,7,51},{0,8,119},{0,8,55},{0,9,207},{17,7,15},{0,8,103},
		{0,8,39},{0,9,175},{0,8,7},{0,8,135},{0,8,71},{0,9,239},{16,7,9},
		{0,8,95},{0,8,31},{0,9,159},{20,7,99},{0,8,127},{0,8,63},{0,9,223},
		{18,7,27},{0,8,111},{0,8,47},{0,9,191},{0,8,15},{0,8,143},{0,8,79},
		{0,9,255}
	};

	static const code distfix[32] = {
		{16,5,1},{23,5,257},{19,5,17},{27,5,4097},{17,5,5},{25,5,1025},
		{21,5,65},{29,5,16385},{16,5,3},{24,5,513},{20,5,33},{28,5,8193},
		{18,5,9},{26,5,2049},{22,5,129},{64,5,0},{16,5,2},{23,5,385},
		{19,5,25},{27,5,6145},{17,5,7},{25,5,1537},{21,5,97},{29,5,24577},
		{16,5,4},{24,5,769},{20,5,49},{28,5,12289},{18,5,13},{26,5,3073},
		{22,5,193},{64,5,0}
	};

/*** End of inlined file: inffixed.h ***/


#endif /* BUILDFIXED */
	state->lencode = lenfix;
	state->lenbits = 9;
	state->distcode = distfix;
	state->distbits = 5;
}

#ifdef MAKEFIXED
#include <stdio.h>

/*
   Write out the inffixed.h that is #include'd above.  Defining MAKEFIXED also
   defines BUILDFIXED, so the tables are built on the fly.  makefixed() writes
   those tables to stdout, which would be piped to inffixed.h.  A small program
   can simply call makefixed to do this:

	void makefixed(void);

	int main(void)
	{
		makefixed();
		return 0;
	}

   Then that can be linked with zlib built with MAKEFIXED defined and run:

	a.out > inffixed.h
 */
void makefixed()
{
	unsigned low, size;
	struct inflate_state state;

	fixedtables(&state);
	puts("    /* inffixed.h -- table for decoding fixed codes");
	puts("     * Generated automatically by makefixed().");
	puts("     */");
	puts("");
	puts("    /* WARNING: this file should *not* be used by applications.");
	puts("       It is part of the implementation of this library and is");
	puts("       subject to change. Applications should only use zlib.h.");
	puts("     */");
	puts("");
	size = 1U << 9;
	printf("    static const code lenfix[%u] = {", size);
	low = 0;
	for (;;) {
		if ((low % 7) == 0) printf("\n        ");
		printf("{%u,%u,%d}", state.lencode[low].op, state.lencode[low].bits,
			   state.lencode[low].val);
		if (++low == size) break;
		putchar(',');
	}
	puts("\n    };");
	size = 1U << 5;
	printf("\n    static const code distfix[%u] = {", size);
	low = 0;
	for (;;) {
		if ((low % 6) == 0) printf("\n        ");
		printf("{%u,%u,%d}", state.distcode[low].op, state.distcode[low].bits,
			   state.distcode[low].val);
		if (++low == size) break;
		putchar(',');
	}
	puts("\n    };");
}
#endif /* MAKEFIXED */

/*
   Update the window with the last wsize (normally 32K) bytes written before
   returning.  If window does not exist yet, create it.  This is only called
   when a window is already in use, or when output has been written during this
   inflate call, but the end of the deflate stream has not been reached yet.
   It is also called to create a window for dictionary data when a dictionary
   is loaded.

   Providing output buffers larger than 32K to inflate() should provide a speed
   advantage, since only the last 32K of output is copied to the sliding window
   upon return from inflate(), and since all distances after the first 32K of
   output will fall in the output data, making match copies simpler and faster.
   The advantage may be dependent on the size of the processor's data caches.
 */
local int updatewindow (z_streamp strm, unsigned out)
{
	struct inflate_state FAR *state;
	unsigned copy, dist;

	state = (struct inflate_state FAR *)strm->state;

	/* if it hasn't been done already, allocate space for the window */
	if (state->window == Z_NULL) {
		state->window = (unsigned char FAR *)
						ZALLOC(strm, 1U << state->wbits,
							   sizeof(unsigned char));
		if (state->window == Z_NULL) return 1;
	}

	/* if window not in use yet, initialize */
	if (state->wsize == 0) {
		state->wsize = 1U << state->wbits;
		state->write = 0;
		state->whave = 0;
	}

	/* copy state->wsize or less output bytes into the circular window */
	copy = out - strm->avail_out;
	if (copy >= state->wsize) {
		zmemcpy(state->window, strm->next_out - state->wsize, state->wsize);
		state->write = 0;
		state->whave = state->wsize;
	}
	else {
		dist = state->wsize - state->write;
		if (dist > copy) dist = copy;
		zmemcpy(state->window + state->write, strm->next_out - copy, dist);
		copy -= dist;
		if (copy) {
			zmemcpy(state->window, strm->next_out - copy, copy);
			state->write = copy;
			state->whave = state->wsize;
		}
		else {
			state->write += dist;
			if (state->write == state->wsize) state->write = 0;
			if (state->whave < state->wsize) state->whave += dist;
		}
	}
	return 0;
}

/* Macros for inflate(): */

/* check function to use adler32() for zlib or crc32() for gzip */
#ifdef GUNZIP
#  define UPDATE(check, buf, len) \
	(state->flags ? crc32(check, buf, len) : adler32(check, buf, len))
#else
#  define UPDATE(check, buf, len) adler32(check, buf, len)
#endif

/* check macros for header crc */
#ifdef GUNZIP
#  define CRC2(check, word) \
	do { \
		hbuf[0] = (unsigned char)(word); \
		hbuf[1] = (unsigned char)((word) >> 8); \
		check = crc32(check, hbuf, 2); \
	} while (0)

#  define CRC4(check, word) \
	do { \
		hbuf[0] = (unsigned char)(word); \
		hbuf[1] = (unsigned char)((word) >> 8); \
		hbuf[2] = (unsigned char)((word) >> 16); \
		hbuf[3] = (unsigned char)((word) >> 24); \
		check = crc32(check, hbuf, 4); \
	} while (0)
#endif

/* Load registers with state in inflate() for speed */
#define LOAD() \
	do { \
		put = strm->next_out; \
		left = strm->avail_out; \
		next = strm->next_in; \
		have = strm->avail_in; \
		hold = state->hold; \
		bits = state->bits; \
	} while (0)

/* Restore state from registers in inflate() */
#define RESTORE() \
	do { \
		strm->next_out = put; \
		strm->avail_out = left; \
		strm->next_in = next; \
		strm->avail_in = have; \
		state->hold = hold; \
		state->bits = bits; \
	} while (0)

/* Clear the input bit accumulator */
#define INITBITS() \
	do { \
		hold = 0; \
		bits = 0; \
	} while (0)

/* Get a byte of input into the bit accumulator, or return from inflate()
   if there is no input available. */
#define PULLBYTE() \
	do { \
		if (have == 0) goto inf_leave; \
		have--; \
		hold += (unsigned long)(*next++) << bits; \
		bits += 8; \
	} while (0)

/* Assure that there are at least n bits in the bit accumulator.  If there is
   not enough available input to do that, then return from inflate(). */
#define NEEDBITS(n) \
	do { \
		while (bits < (unsigned)(n)) \
			PULLBYTE(); \
	} while (0)

/* Return the low n bits of the bit accumulator (n < 16) */
#define BITS(n) \
	((unsigned)hold & ((1U << (n)) - 1))

/* Remove n bits from the bit accumulator */
#define DROPBITS(n) \
	do { \
		hold >>= (n); \
		bits -= (unsigned)(n); \
	} while (0)

/* Remove zero to seven bits as needed to go to a byte boundary */
#define BYTEBITS() \
	do { \
		hold >>= bits & 7; \
		bits -= bits & 7; \
	} while (0)

/* Reverse the bytes in a 32-bit value */
#define REVERSE(q) \
	((((q) >> 24) & 0xff) + (((q) >> 8) & 0xff00) + \
	 (((q) & 0xff00) << 8) + (((q) & 0xff) << 24))

/*
   inflate() uses a state machine to process as much input data and generate as
   much output data as possible before returning.  The state machine is
   structured roughly as follows:

	for (;;) switch (state) {
	...
	case STATEn:
		if (not enough input data or output space to make progress)
			return;
		... make progress ...
		state = STATEm;
		break;
	...
	}

   so when inflate() is called again, the same case is attempted again, and
   if the appropriate resources are provided, the machine proceeds to the
   next state.  The NEEDBITS() macro is usually the way the state evaluates
   whether it can proceed or should return.  NEEDBITS() does the return if
   the requested bits are not available.  The typical use of the BITS macros
   is:

		NEEDBITS(n);
		... do something with BITS(n) ...
		DROPBITS(n);

   where NEEDBITS(n) either returns from inflate() if there isn't enough
   input left to load n bits into the accumulator, or it continues.  BITS(n)
   gives the low n bits in the accumulator.  When done, DROPBITS(n) drops
   the low n bits off the accumulator.  INITBITS() clears the accumulator
   and sets the number of available bits to zero.  BYTEBITS() discards just
   enough bits to put the accumulator on a byte boundary.  After BYTEBITS()
   and a NEEDBITS(8), then BITS(8) would return the next byte in the stream.

   NEEDBITS(n) uses PULLBYTE() to get an available byte of input, or to return
   if there is no input available.  The decoding of variable length codes uses
   PULLBYTE() directly in order to pull just enough bytes to decode the next
   code, and no more.

   Some states loop until they get enough input, making sure that enough
   state information is maintained to continue the loop where it left off
   if NEEDBITS() returns in the loop.  For example, want, need, and keep
   would all have to actually be part of the saved state in case NEEDBITS()
   returns:

	case STATEw:
		while (want < need) {
			NEEDBITS(n);
			keep[want++] = BITS(n);
			DROPBITS(n);
		}
		state = STATEx;
	case STATEx:

   As shown above, if the next state is also the next case, then the break
   is omitted.

   A state may also return if there is not enough output space available to
   complete that state.  Those states are copying stored data, writing a
   literal byte, and copying a matching string.

   When returning, a "goto inf_leave" is used to update the total counters,
   update the check value, and determine whether any progress has been made
   during that inflate() call in order to return the proper return code.
   Progress is defined as a change in either strm->avail_in or strm->avail_out.
   When there is a window, goto inf_leave will update the window with the last
   output written.  If a goto inf_leave occurs in the middle of decompression
   and there is no window currently, goto inf_leave will create one and copy
   output to the window for the next call of inflate().

   In this implementation, the flush parameter of inflate() only affects the
   return code (per zlib.h).  inflate() always writes as much as possible to
   strm->next_out, given the space available and the provided input--the effect
   documented in zlib.h of Z_SYNC_FLUSH.  Furthermore, inflate() always defers
   the allocation of and copying into a sliding window until necessary, which
   provides the effect documented in zlib.h for Z_FINISH when the entire input
   stream available.  So the only thing the flush parameter actually does is:
   when flush is set to Z_FINISH, inflate() cannot return Z_OK.  Instead it
   will return Z_BUF_ERROR if it has not reached the end of the stream.
 */

int ZEXPORT inflate (z_streamp strm, int flush)
{
	struct inflate_state FAR *state;
	unsigned char FAR *next;    /* next input */
	unsigned char FAR *put;     /* next output */
	unsigned have, left;        /* available input and output */
	unsigned long hold;         /* bit buffer */
	unsigned bits;              /* bits in bit buffer */
	unsigned in, out;           /* save starting available input and output */
	unsigned copy;              /* number of stored or match bytes to copy */
	unsigned char FAR *from;    /* where to copy match bytes from */
	code thisx;                  /* current decoding table entry */
	code last;                  /* parent table entry */
	unsigned len;               /* length to copy for repeats, bits to drop */
	int ret;                    /* return code */
#ifdef GUNZIP
	unsigned char hbuf[4];      /* buffer for gzip header crc calculation */
#endif
	static const unsigned short order[19] = /* permutation of code lengths */
		{16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};

	if (strm == Z_NULL || strm->state == Z_NULL || strm->next_out == Z_NULL ||
		(strm->next_in == Z_NULL && strm->avail_in != 0))
		return Z_STREAM_ERROR;

	state = (struct inflate_state FAR *)strm->state;
	if (state->mode == TYPE) state->mode = TYPEDO;      /* skip check */
	LOAD();
	in = have;
	out = left;
	ret = Z_OK;
	for (;;)
		switch (state->mode) {
		case HEAD:
			if (state->wrap == 0) {
				state->mode = TYPEDO;
				break;
			}
			NEEDBITS(16);
#ifdef GUNZIP
			if ((state->wrap & 2) && hold == 0x8b1f) {  /* gzip header */
				state->check = crc32(0L, Z_NULL, 0);
				CRC2(state->check, hold);
				INITBITS();
				state->mode = FLAGS;
				break;
			}
			state->flags = 0;           /* expect zlib header */
			if (state->head != Z_NULL)
				state->head->done = -1;
			if (!(state->wrap & 1) ||   /* check if zlib header allowed */
#else
			if (
#endif
				((BITS(8) << 8) + (hold >> 8)) % 31) {
				strm->msg = (char *)"incorrect header check";
				state->mode = BAD;
				break;
			}
			if (BITS(4) != Z_DEFLATED) {
				strm->msg = (char *)"unknown compression method";
				state->mode = BAD;
				break;
			}
			DROPBITS(4);
			len = BITS(4) + 8;
			if (len > state->wbits) {
				strm->msg = (char *)"invalid window size";
				state->mode = BAD;
				break;
			}
			state->dmax = 1U << len;
			Tracev((stderr, "inflate:   zlib header ok\n"));
			strm->adler = state->check = adler32(0L, Z_NULL, 0);
			state->mode = hold & 0x200 ? DICTID : TYPE;
			INITBITS();
			break;
#ifdef GUNZIP
		case FLAGS:
			NEEDBITS(16);
			state->flags = (int)(hold);
			if ((state->flags & 0xff) != Z_DEFLATED) {
				strm->msg = (char *)"unknown compression method";
				state->mode = BAD;
				break;
			}
			if (state->flags & 0xe000) {
				strm->msg = (char *)"unknown header flags set";
				state->mode = BAD;
				break;
			}
			if (state->head != Z_NULL)
				state->head->text = (int)((hold >> 8) & 1);
			if (state->flags & 0x0200) CRC2(state->check, hold);
			INITBITS();
			state->mode = TIME;
		case TIME:
			NEEDBITS(32);
			if (state->head != Z_NULL)
				state->head->time = hold;
			if (state->flags & 0x0200) CRC4(state->check, hold);
			INITBITS();
			state->mode = OS;
		case OS:
			NEEDBITS(16);
			if (state->head != Z_NULL) {
				state->head->xflags = (int)(hold & 0xff);
				state->head->os = (int)(hold >> 8);
			}
			if (state->flags & 0x0200) CRC2(state->check, hold);
			INITBITS();
			state->mode = EXLEN;
		case EXLEN:
			if (state->flags & 0x0400) {
				NEEDBITS(16);
				state->length = (unsigned)(hold);
				if (state->head != Z_NULL)
					state->head->extra_len = (unsigned)hold;
				if (state->flags & 0x0200) CRC2(state->check, hold);
				INITBITS();
			}
			else if (state->head != Z_NULL)
				state->head->extra = Z_NULL;
			state->mode = EXTRA;
		case EXTRA:
			if (state->flags & 0x0400) {
				copy = state->length;
				if (copy > have) copy = have;
				if (copy) {
					if (state->head != Z_NULL &&
						state->head->extra != Z_NULL) {
						len = state->head->extra_len - state->length;
						zmemcpy(state->head->extra + len, next,
								len + copy > state->head->extra_max ?
								state->head->extra_max - len : copy);
					}
					if (state->flags & 0x0200)
						state->check = crc32(state->check, next, copy);
					have -= copy;
					next += copy;
					state->length -= copy;
				}
				if (state->length) goto inf_leave;
			}
			state->length = 0;
			state->mode = NAME;
		case NAME:
			if (state->flags & 0x0800) {
				if (have == 0) goto inf_leave;
				copy = 0;
				do {
					len = (unsigned)(next[copy++]);
					if (state->head != Z_NULL &&
							state->head->name != Z_NULL &&
							state->length < state->head->name_max)
						state->head->name[state->length++] = len;
				} while (len && copy < have);
				if (state->flags & 0x0200)
					state->check = crc32(state->check, next, copy);
				have -= copy;
				next += copy;
				if (len) goto inf_leave;
			}
			else if (state->head != Z_NULL)
				state->head->name = Z_NULL;
			state->length = 0;
			state->mode = COMMENT;
		case COMMENT:
			if (state->flags & 0x1000) {
				if (have == 0) goto inf_leave;
				copy = 0;
				do {
					len = (unsigned)(next[copy++]);
					if (state->head != Z_NULL &&
							state->head->comment != Z_NULL &&
							state->length < state->head->comm_max)
						state->head->comment[state->length++] = len;
				} while (len && copy < have);
				if (state->flags & 0x0200)
					state->check = crc32(state->check, next, copy);
				have -= copy;
				next += copy;
				if (len) goto inf_leave;
			}
			else if (state->head != Z_NULL)
				state->head->comment = Z_NULL;
			state->mode = HCRC;
		case HCRC:
			if (state->flags & 0x0200) {
				NEEDBITS(16);
				if (hold != (state->check & 0xffff)) {
					strm->msg = (char *)"header crc mismatch";
					state->mode = BAD;
					break;
				}
				INITBITS();
			}
			if (state->head != Z_NULL) {
				state->head->hcrc = (int)((state->flags >> 9) & 1);
				state->head->done = 1;
			}
			strm->adler = state->check = crc32(0L, Z_NULL, 0);
			state->mode = TYPE;
			break;
#endif
		case DICTID:
			NEEDBITS(32);
			strm->adler = state->check = REVERSE(hold);
			INITBITS();
			state->mode = DICT;
		case DICT:
			if (state->havedict == 0) {
				RESTORE();
				return Z_NEED_DICT;
			}
			strm->adler = state->check = adler32(0L, Z_NULL, 0);
			state->mode = TYPE;
		case TYPE:
			if (flush == Z_BLOCK) goto inf_leave;
		case TYPEDO:
			if (state->last) {
				BYTEBITS();
				state->mode = CHECK;
				break;
			}
			NEEDBITS(3);
			state->last = BITS(1);
			DROPBITS(1);
			switch (BITS(2)) {
			case 0:                             /* stored block */
				Tracev((stderr, "inflate:     stored block%s\n",
						state->last ? " (last)" : ""));
				state->mode = STORED;
				break;
			case 1:                             /* fixed block */
				fixedtables(state);
				Tracev((stderr, "inflate:     fixed codes block%s\n",
						state->last ? " (last)" : ""));
				state->mode = LEN;              /* decode codes */
				break;
			case 2:                             /* dynamic block */
				Tracev((stderr, "inflate:     dynamic codes block%s\n",
						state->last ? " (last)" : ""));
				state->mode = TABLE;
				break;
			case 3:
				strm->msg = (char *)"invalid block type";
				state->mode = BAD;
			}
			DROPBITS(2);
			break;
		case STORED:
			BYTEBITS();                         /* go to byte boundary */
			NEEDBITS(32);
			if ((hold & 0xffff) != ((hold >> 16) ^ 0xffff)) {
				strm->msg = (char *)"invalid stored block lengths";
				state->mode = BAD;
				break;
			}
			state->length = (unsigned)hold & 0xffff;
			Tracev((stderr, "inflate:       stored length %u\n",
					state->length));
			INITBITS();
			state->mode = COPY;
		case COPY:
			copy = state->length;
			if (copy) {
				if (copy > have) copy = have;
				if (copy > left) copy = left;
				if (copy == 0) goto inf_leave;
				zmemcpy(put, next, copy);
				have -= copy;
				next += copy;
				left -= copy;
				put += copy;
				state->length -= copy;
				break;
			}
			Tracev((stderr, "inflate:       stored end\n"));
			state->mode = TYPE;
			break;
		case TABLE:
			NEEDBITS(14);
			state->nlen = BITS(5) + 257;
			DROPBITS(5);
			state->ndist = BITS(5) + 1;
			DROPBITS(5);
			state->ncode = BITS(4) + 4;
			DROPBITS(4);
#ifndef PKZIP_BUG_WORKAROUND
			if (state->nlen > 286 || state->ndist > 30) {
				strm->msg = (char *)"too many length or distance symbols";
				state->mode = BAD;
				break;
			}
#endif
			Tracev((stderr, "inflate:       table sizes ok\n"));
			state->have = 0;
			state->mode = LENLENS;
		case LENLENS:
			while (state->have < state->ncode) {
				NEEDBITS(3);
				state->lens[order[state->have++]] = (unsigned short)BITS(3);
				DROPBITS(3);
			}
			while (state->have < 19)
				state->lens[order[state->have++]] = 0;
			state->next = state->codes;
			state->lencode = (code const FAR *)(state->next);
			state->lenbits = 7;
			ret = inflate_table(CODES, state->lens, 19, &(state->next),
								&(state->lenbits), state->work);
			if (ret) {
				strm->msg = (char *)"invalid code lengths set";
				state->mode = BAD;
				break;
			}
			Tracev((stderr, "inflate:       code lengths ok\n"));
			state->have = 0;
			state->mode = CODELENS;
		case CODELENS:
			while (state->have < state->nlen + state->ndist) {
				for (;;) {
					thisx = state->lencode[BITS(state->lenbits)];
					if ((unsigned)(thisx.bits) <= bits) break;
					PULLBYTE();
				}
				if (thisx.val < 16) {
					NEEDBITS(thisx.bits);
					DROPBITS(thisx.bits);
					state->lens[state->have++] = thisx.val;
				}
				else {
					if (thisx.val == 16) {
						NEEDBITS(thisx.bits + 2);
						DROPBITS(thisx.bits);
						if (state->have == 0) {
							strm->msg = (char *)"invalid bit length repeat";
							state->mode = BAD;
							break;
						}
						len = state->lens[state->have - 1];
						copy = 3 + BITS(2);
						DROPBITS(2);
					}
					else if (thisx.val == 17) {
						NEEDBITS(thisx.bits + 3);
						DROPBITS(thisx.bits);
						len = 0;
						copy = 3 + BITS(3);
						DROPBITS(3);
					}
					else {
						NEEDBITS(thisx.bits + 7);
						DROPBITS(thisx.bits);
						len = 0;
						copy = 11 + BITS(7);
						DROPBITS(7);
					}
					if (state->have + copy > state->nlen + state->ndist) {
						strm->msg = (char *)"invalid bit length repeat";
						state->mode = BAD;
						break;
					}
					while (copy--)
						state->lens[state->have++] = (unsigned short)len;
				}
			}

			/* handle error breaks in while */
			if (state->mode == BAD) break;

			/* build code tables */
			state->next = state->codes;
			state->lencode = (code const FAR *)(state->next);
			state->lenbits = 9;
			ret = inflate_table(LENS, state->lens, state->nlen, &(state->next),
								&(state->lenbits), state->work);
			if (ret) {
				strm->msg = (char *)"invalid literal/lengths set";
				state->mode = BAD;
				break;
			}
			state->distcode = (code const FAR *)(state->next);
			state->distbits = 6;
			ret = inflate_table(DISTS, state->lens + state->nlen, state->ndist,
							&(state->next), &(state->distbits), state->work);
			if (ret) {
				strm->msg = (char *)"invalid distances set";
				state->mode = BAD;
				break;
			}
			Tracev((stderr, "inflate:       codes ok\n"));
			state->mode = LEN;
		case LEN:
			if (have >= 6 && left >= 258) {
				RESTORE();
				inflate_fast(strm, out);
				LOAD();
				break;
			}
			for (;;) {
				thisx = state->lencode[BITS(state->lenbits)];
				if ((unsigned)(thisx.bits) <= bits) break;
				PULLBYTE();
			}
			if (thisx.op && (thisx.op & 0xf0) == 0) {
				last = thisx;
				for (;;) {
					thisx = state->lencode[last.val +
							(BITS(last.bits + last.op) >> last.bits)];
					if ((unsigned)(last.bits + thisx.bits) <= bits) break;
					PULLBYTE();
				}
				DROPBITS(last.bits);
			}
			DROPBITS(thisx.bits);
			state->length = (unsigned)thisx.val;
			if ((int)(thisx.op) == 0) {
				Tracevv((stderr, thisx.val >= 0x20 && thisx.val < 0x7f ?
						"inflate:         literal '%c'\n" :
						"inflate:         literal 0x%02x\n", thisx.val));
				state->mode = LIT;
				break;
			}
			if (thisx.op & 32) {
				Tracevv((stderr, "inflate:         end of block\n"));
				state->mode = TYPE;
				break;
			}
			if (thisx.op & 64) {
				strm->msg = (char *)"invalid literal/length code";
				state->mode = BAD;
				break;
			}
			state->extra = (unsigned)(thisx.op) & 15;
			state->mode = LENEXT;
		case LENEXT:
			if (state->extra) {
				NEEDBITS(state->extra);
				state->length += BITS(state->extra);
				DROPBITS(state->extra);
			}
			Tracevv((stderr, "inflate:         length %u\n", state->length));
			state->mode = DIST;
		case DIST:
			for (;;) {
				thisx = state->distcode[BITS(state->distbits)];
				if ((unsigned)(thisx.bits) <= bits) break;
				PULLBYTE();
			}
			if ((thisx.op & 0xf0) == 0) {
				last = thisx;
				for (;;) {
					thisx = state->distcode[last.val +
							(BITS(last.bits + last.op) >> last.bits)];
					if ((unsigned)(last.bits + thisx.bits) <= bits) break;
					PULLBYTE();
				}
				DROPBITS(last.bits);
			}
			DROPBITS(thisx.bits);
			if (thisx.op & 64) {
				strm->msg = (char *)"invalid distance code";
				state->mode = BAD;
				break;
			}
			state->offset = (unsigned)thisx.val;
			state->extra = (unsigned)(thisx.op) & 15;
			state->mode = DISTEXT;
		case DISTEXT:
			if (state->extra) {
				NEEDBITS(state->extra);
				state->offset += BITS(state->extra);
				DROPBITS(state->extra);
			}
#ifdef INFLATE_STRICT
			if (state->offset > state->dmax) {
				strm->msg = (char *)"invalid distance too far back";
				state->mode = BAD;
				break;
			}
#endif
			if (state->offset > state->whave + out - left) {
				strm->msg = (char *)"invalid distance too far back";
				state->mode = BAD;
				break;
			}
			Tracevv((stderr, "inflate:         distance %u\n", state->offset));
			state->mode = MATCH;
		case MATCH:
			if (left == 0) goto inf_leave;
			copy = out - left;
			if (state->offset > copy) {         /* copy from window */
				copy = state->offset - copy;
				if (copy > state->write) {
					copy -= state->write;
					from = state->window + (state->wsize - copy);
				}
				else
					from = state->window + (state->write - copy);
				if (copy > state->length) copy = state->length;
			}
			else {                              /* copy from output */
				from = put - state->offset;
				copy = state->length;
			}
			if (copy > left) copy = left;
			left -= copy;
			state->length -= copy;
			do {
				*put++ = *from++;
			} while (--copy);
			if (state->length == 0) state->mode = LEN;
			break;
		case LIT:
			if (left == 0) goto inf_leave;
			*put++ = (unsigned char)(state->length);
			left--;
			state->mode = LEN;
			break;
		case CHECK:
			if (state->wrap) {
				NEEDBITS(32);
				out -= left;
				strm->total_out += out;
				state->total += out;
				if (out)
					strm->adler = state->check =
						UPDATE(state->check, put - out, out);
				out = left;
				if ((
#ifdef GUNZIP
					 state->flags ? hold :
#endif
					 REVERSE(hold)) != state->check) {
					strm->msg = (char *)"incorrect data check";
					state->mode = BAD;
					break;
				}
				INITBITS();
				Tracev((stderr, "inflate:   check matches trailer\n"));
			}
#ifdef GUNZIP
			state->mode = LENGTH;
		case LENGTH:
			if (state->wrap && state->flags) {
				NEEDBITS(32);
				if (hold != (state->total & 0xffffffffUL)) {
					strm->msg = (char *)"incorrect length check";
					state->mode = BAD;
					break;
				}
				INITBITS();
				Tracev((stderr, "inflate:   length matches trailer\n"));
			}
#endif
			state->mode = DONE;
		case DONE:
			ret = Z_STREAM_END;
			goto inf_leave;
		case BAD:
			ret = Z_DATA_ERROR;
			goto inf_leave;
		case MEM:
			return Z_MEM_ERROR;
		case SYNC:
		default:
			return Z_STREAM_ERROR;
		}

	/*
	   Return from inflate(), updating the total counts and the check value.
	   If there was no progress during the inflate() call, return a buffer
	   error.  Call updatewindow() to create and/or update the window state.
	   Note: a memory error from inflate() is non-recoverable.
	 */
  inf_leave:
	RESTORE();
	if (state->wsize || (state->mode < CHECK && out != strm->avail_out))
		if (updatewindow(strm, out)) {
			state->mode = MEM;
			return Z_MEM_ERROR;
		}
	in -= strm->avail_in;
	out -= strm->avail_out;
	strm->total_in += in;
	strm->total_out += out;
	state->total += out;
	if (state->wrap && out)
		strm->adler = state->check =
			UPDATE(state->check, strm->next_out - out, out);
	strm->data_type = state->bits + (state->last ? 64 : 0) +
					  (state->mode == TYPE ? 128 : 0);
	if (((in == 0 && out == 0) || flush == Z_FINISH) && ret == Z_OK)
		ret = Z_BUF_ERROR;
	return ret;
}

int ZEXPORT inflateEnd (z_streamp strm)
{
	struct inflate_state FAR *state;
	if (strm == Z_NULL || strm->state == Z_NULL || strm->zfree == (free_func)0)
		return Z_STREAM_ERROR;
	state = (struct inflate_state FAR *)strm->state;
	if (state->window != Z_NULL) ZFREE(strm, state->window);
	ZFREE(strm, strm->state);
	strm->state = Z_NULL;
	Tracev((stderr, "inflate: end\n"));
	return Z_OK;
}

int ZEXPORT inflateSetDictionary (z_streamp strm, const Bytef *dictionary, uInt dictLength)
{
	struct inflate_state FAR *state;
	unsigned long id_;

	/* check state */
	if (strm == Z_NULL || strm->state == Z_NULL) return Z_STREAM_ERROR;
	state = (struct inflate_state FAR *)strm->state;
	if (state->wrap != 0 && state->mode != DICT)
		return Z_STREAM_ERROR;

	/* check for correct dictionary id */
	if (state->mode == DICT) {
		id_ = adler32(0L, Z_NULL, 0);
		id_ = adler32(id_, dictionary, dictLength);
		if (id_ != state->check)
			return Z_DATA_ERROR;
	}

	/* copy dictionary to window */
	if (updatewindow(strm, strm->avail_out)) {
		state->mode = MEM;
		return Z_MEM_ERROR;
	}
	if (dictLength > state->wsize) {
		zmemcpy(state->window, dictionary + dictLength - state->wsize,
				state->wsize);
		state->whave = state->wsize;
	}
	else {
		zmemcpy(state->window + state->wsize - dictLength, dictionary,
				dictLength);
		state->whave = dictLength;
	}
	state->havedict = 1;
	Tracev((stderr, "inflate:   dictionary set\n"));
	return Z_OK;
}

int ZEXPORT inflateGetHeader (z_streamp strm, gz_headerp head)
{
	struct inflate_state FAR *state;

	/* check state */
	if (strm == Z_NULL || strm->state == Z_NULL) return Z_STREAM_ERROR;
	state = (struct inflate_state FAR *)strm->state;
	if ((state->wrap & 2) == 0) return Z_STREAM_ERROR;

	/* save header structure */
	state->head = head;
	head->done = 0;
	return Z_OK;
}

/*
   Search buf[0..len-1] for the pattern: 0, 0, 0xff, 0xff.  Return when found
   or when out of input.  When called, *have is the number of pattern bytes
   found in order so far, in 0..3.  On return *have is updated to the new
   state.  If on return *have equals four, then the pattern was found and the
   return value is how many bytes were read including the last byte of the
   pattern.  If *have is less than four, then the pattern has not been found
   yet and the return value is len.  In the latter case, syncsearch() can be
   called again with more data and the *have state.  *have is initialized to
   zero for the first call.
 */
local unsigned syncsearch (unsigned FAR *have, unsigned char FAR *buf, unsigned len)
{
	unsigned got;
	unsigned next;

	got = *have;
	next = 0;
	while (next < len && got < 4) {
		if ((int)(buf[next]) == (got < 2 ? 0 : 0xff))
			got++;
		else if (buf[next])
			got = 0;
		else
			got = 4 - got;
		next++;
	}
	*have = got;
	return next;
}

int ZEXPORT inflateSync (z_streamp strm)
{
	unsigned len;               /* number of bytes to look at or looked at */
	unsigned long in, out;      /* temporary to save total_in and total_out */
	unsigned char buf[4];       /* to restore bit buffer to byte string */
	struct inflate_state FAR *state;

	/* check parameters */
	if (strm == Z_NULL || strm->state == Z_NULL) return Z_STREAM_ERROR;
	state = (struct inflate_state FAR *)strm->state;
	if (strm->avail_in == 0 && state->bits < 8) return Z_BUF_ERROR;

	/* if first time, start search in bit buffer */
	if (state->mode != SYNC) {
		state->mode = SYNC;
		state->hold <<= state->bits & 7;
		state->bits -= state->bits & 7;
		len = 0;
		while (state->bits >= 8) {
			buf[len++] = (unsigned char)(state->hold);
			state->hold >>= 8;
			state->bits -= 8;
		}
		state->have = 0;
		syncsearch(&(state->have), buf, len);
	}

	/* search available input */
	len = syncsearch(&(state->have), strm->next_in, strm->avail_in);
	strm->avail_in -= len;
	strm->next_in += len;
	strm->total_in += len;

	/* return no joy or set up to restart inflate() on a new block */
	if (state->have != 4) return Z_DATA_ERROR;
	in = strm->total_in;  out = strm->total_out;
	inflateReset(strm);
	strm->total_in = in;  strm->total_out = out;
	state->mode = TYPE;
	return Z_OK;
}

/*
   Returns true if inflate is currently at the end of a block generated by
   Z_SYNC_FLUSH or Z_FULL_FLUSH. This function is used by one PPP
   implementation to provide an additional safety check. PPP uses
   Z_SYNC_FLUSH but removes the length bytes of the resulting empty stored
   block. When decompressing, PPP checks that at the end of input packet,
   inflate is waiting for these length bytes.
 */
int ZEXPORT inflateSyncPoint (z_streamp strm)
{
	struct inflate_state FAR *state;

	if (strm == Z_NULL || strm->state == Z_NULL) return Z_STREAM_ERROR;
	state = (struct inflate_state FAR *)strm->state;
	return state->mode == STORED && state->bits == 0;
}

int ZEXPORT inflateCopy(z_streamp dest, z_streamp source)
{
	struct inflate_state FAR *state;
	struct inflate_state FAR *copy;
	unsigned char FAR *window;
	unsigned wsize;

	/* check input */
	if (dest == Z_NULL || source == Z_NULL || source->state == Z_NULL ||
		source->zalloc == (alloc_func)0 || source->zfree == (free_func)0)
		return Z_STREAM_ERROR;
	state = (struct inflate_state FAR *)source->state;

	/* allocate space */
	copy = (struct inflate_state FAR *)
		   ZALLOC(source, 1, sizeof(struct inflate_state));
	if (copy == Z_NULL) return Z_MEM_ERROR;
	window = Z_NULL;
	if (state->window != Z_NULL) {
		window = (unsigned char FAR *)
				 ZALLOC(source, 1U << state->wbits, sizeof(unsigned char));
		if (window == Z_NULL) {
			ZFREE(source, copy);
			return Z_MEM_ERROR;
		}
	}

	/* copy state */
	zmemcpy(dest, source, sizeof(z_stream));
	zmemcpy(copy, state, sizeof(struct inflate_state));
	if (state->lencode >= state->codes &&
		state->lencode <= state->codes + ENOUGH - 1) {
		copy->lencode = copy->codes + (state->lencode - state->codes);
		copy->distcode = copy->codes + (state->distcode - state->codes);
	}
	copy->next = copy->codes + (state->next - state->codes);
	if (window != Z_NULL) {
		wsize = 1U << state->wbits;
		zmemcpy(window, state->window, wsize);
	}
	copy->window = window;
	dest->state = (struct internal_state FAR *)copy;
	return Z_OK;
}

/*** End of inlined file: inflate.c ***/



/*** Start of inlined file: inftrees.c ***/
#define MAXBITS 15

const char inflate_copyright[] =
   " inflate 1.2.3 Copyright 1995-2005 Mark Adler ";
/*
  If you use the zlib library in a product, an acknowledgment is welcome
  in the documentation of your product. If for some reason you cannot
  include such an acknowledgment, I would appreciate that you keep this
  copyright string in the executable of your product.
 */

/*
   Build a set of tables to decode the provided canonical Huffman code.
   The code lengths are lens[0..codes-1].  The result starts at *table,
   whose indices are 0..2^bits-1.  work is a writable array of at least
   lens shorts, which is used as a work area.  type is the type of code
   to be generated, CODES, LENS, or DISTS.  On return, zero is success,
   -1 is an invalid code, and +1 means that ENOUGH isn't enough.  table
   on return points to the next available entry's address.  bits is the
   requested root table index bits, and on return it is the actual root
   table index bits.  It will differ if the request is greater than the
   longest code or if it is less than the shortest code.
 */
int inflate_table (codetype type,
				   unsigned short FAR *lens,
				   unsigned codes,
				   code FAR * FAR *table,
				   unsigned FAR *bits,
				   unsigned short FAR *work)
{
	unsigned len;               /* a code's length in bits */
	unsigned sym;               /* index of code symbols */
	unsigned min, max;          /* minimum and maximum code lengths */
	unsigned root;              /* number of index bits for root table */
	unsigned curr;              /* number of index bits for current table */
	unsigned drop;              /* code bits to drop for sub-table */
	int left;                   /* number of prefix codes available */
	unsigned used;              /* code entries in table used */
	unsigned huff;              /* Huffman code */
	unsigned incr;              /* for incrementing code, index */
	unsigned fill;              /* index for replicating entries */
	unsigned low;               /* low bits for current root entry */
	unsigned mask;              /* mask for low root bits */
	code thisx;                  /* table entry for duplication */
	code FAR *next;             /* next available space in table */
	const unsigned short FAR *base;     /* base value table to use */
	const unsigned short FAR *extra;    /* extra bits table to use */
	int end;                    /* use base and extra for symbol > end */
	unsigned short count[MAXBITS+1];    /* number of codes of each length */
	unsigned short offs[MAXBITS+1];     /* offsets in table for each length */
	static const unsigned short lbase[31] = { /* Length codes 257..285 base */
		3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31,
		35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258, 0, 0};
	static const unsigned short lext[31] = { /* Length codes 257..285 extra */
		16, 16, 16, 16, 16, 16, 16, 16, 17, 17, 17, 17, 18, 18, 18, 18,
		19, 19, 19, 19, 20, 20, 20, 20, 21, 21, 21, 21, 16, 201, 196};
	static const unsigned short dbase[32] = { /* Distance codes 0..29 base */
		1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193,
		257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145,
		8193, 12289, 16385, 24577, 0, 0};
	static const unsigned short dext[32] = { /* Distance codes 0..29 extra */
		16, 16, 16, 16, 17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22,
		23, 23, 24, 24, 25, 25, 26, 26, 27, 27,
		28, 28, 29, 29, 64, 64};

	/*
	   Process a set of code lengths to create a canonical Huffman code.  The
	   code lengths are lens[0..codes-1].  Each length corresponds to the
	   symbols 0..codes-1.  The Huffman code is generated by first sorting the
	   symbols by length from short to long, and retaining the symbol order
	   for codes with equal lengths.  Then the code starts with all zero bits
	   for the first code of the shortest length, and the codes are integer
	   increments for the same length, and zeros are appended as the length
	   increases.  For the deflate format, these bits are stored backwards
	   from their more natural integer increment ordering, and so when the
	   decoding tables are built in the large loop below, the integer codes
	   are incremented backwards.

	   This routine assumes, but does not check, that all of the entries in
	   lens[] are in the range 0..MAXBITS.  The caller must assure this.
	   1..MAXBITS is interpreted as that code length.  zero means that that
	   symbol does not occur in this code.

	   The codes are sorted by computing a count of codes for each length,
	   creating from that a table of starting indices for each length in the
	   sorted table, and then entering the symbols in order in the sorted
	   table.  The sorted table is work[], with that space being provided by
	   the caller.

	   The length counts are used for other purposes as well, i.e. finding
	   the minimum and maximum length codes, determining if there are any
	   codes at all, checking for a valid set of lengths, and looking ahead
	   at length counts to determine sub-table sizes when building the
	   decoding tables.
	 */

	/* accumulate lengths for codes (assumes lens[] all in 0..MAXBITS) */
	for (len = 0; len <= MAXBITS; len++)
		count[len] = 0;
	for (sym = 0; sym < codes; sym++)
		count[lens[sym]]++;

	/* bound code lengths, force root to be within code lengths */
	root = *bits;
	for (max = MAXBITS; max >= 1; max--)
		if (count[max] != 0) break;
	if (root > max) root = max;
	if (max == 0) {                     /* no symbols to code at all */
		thisx.op = (unsigned char)64;    /* invalid code marker */
		thisx.bits = (unsigned char)1;
		thisx.val = (unsigned short)0;
		*(*table)++ = thisx;             /* make a table to force an error */
		*(*table)++ = thisx;
		*bits = 1;
		return 0;     /* no symbols, but wait for decoding to report error */
	}
	for (min = 1; min <= MAXBITS; min++)
		if (count[min] != 0) break;
	if (root < min) root = min;

	/* check for an over-subscribed or incomplete set of lengths */
	left = 1;
	for (len = 1; len <= MAXBITS; len++) {
		left <<= 1;
		left -= count[len];
		if (left < 0) return -1;        /* over-subscribed */
	}
	if (left > 0 && (type == CODES || max != 1))
		return -1;                      /* incomplete set */

	/* generate offsets into symbol table for each length for sorting */
	offs[1] = 0;
	for (len = 1; len < MAXBITS; len++)
		offs[len + 1] = offs[len] + count[len];

	/* sort symbols by length, by symbol order within each length */
	for (sym = 0; sym < codes; sym++)
		if (lens[sym] != 0) work[offs[lens[sym]]++] = (unsigned short)sym;

	/*
	   Create and fill in decoding tables.  In this loop, the table being
	   filled is at next and has curr index bits.  The code being used is huff
	   with length len.  That code is converted to an index by dropping drop
	   bits off of the bottom.  For codes where len is less than drop + curr,
	   those top drop + curr - len bits are incremented through all values to
	   fill the table with replicated entries.

	   root is the number of index bits for the root table.  When len exceeds
	   root, sub-tables are created pointed to by the root entry with an index
	   of the low root bits of huff.  This is saved in low to check for when a
	   new sub-table should be started.  drop is zero when the root table is
	   being filled, and drop is root when sub-tables are being filled.

	   When a new sub-table is needed, it is necessary to look ahead in the
	   code lengths to determine what size sub-table is needed.  The length
	   counts are used for this, and so count[] is decremented as codes are
	   entered in the tables.

	   used keeps track of how many table entries have been allocated from the
	   provided *table space.  It is checked when a LENS table is being made
	   against the space in *table, ENOUGH, minus the maximum space needed by
	   the worst case distance code, MAXD.  This should never happen, but the
	   sufficiency of ENOUGH has not been proven exhaustively, hence the check.
	   This assumes that when type == LENS, bits == 9.

	   sym increments through all symbols, and the loop terminates when
	   all codes of length max, i.e. all codes, have been processed.  This
	   routine permits incomplete codes, so another loop after this one fills
	   in the rest of the decoding tables with invalid code markers.
	 */

	/* set up for code type */
	switch (type) {
	case CODES:
		base = extra = work;    /* dummy value--not used */
		end = 19;
		break;
	case LENS:
		base = lbase;
		base -= 257;
		extra = lext;
		extra -= 257;
		end = 256;
		break;
	default:            /* DISTS */
		base = dbase;
		extra = dext;
		end = -1;
	}

	/* initialize state for loop */
	huff = 0;                   /* starting code */
	sym = 0;                    /* starting code symbol */
	len = min;                  /* starting code length */
	next = *table;              /* current table to fill in */
	curr = root;                /* current table index bits */
	drop = 0;                   /* current bits to drop from code for index */
	low = (unsigned)(-1);       /* trigger new sub-table when len > root */
	used = 1U << root;          /* use root table entries */
	mask = used - 1;            /* mask for comparing low */

	/* check available table space */
	if (type == LENS && used >= ENOUGH - MAXD)
		return 1;

	/* process all codes and make table entries */
	for (;;) {
		/* create table entry */
		thisx.bits = (unsigned char)(len - drop);
		if ((int)(work[sym]) < end) {
			thisx.op = (unsigned char)0;
			thisx.val = work[sym];
		}
		else if ((int)(work[sym]) > end) {
			thisx.op = (unsigned char)(extra[work[sym]]);
			thisx.val = base[work[sym]];
		}
		else {
			thisx.op = (unsigned char)(32 + 64);         /* end of block */
			thisx.val = 0;
		}

		/* replicate for those indices with low len bits equal to huff */
		incr = 1U << (len - drop);
		fill = 1U << curr;
		min = fill;                 /* save offset to next table */
		do {
			fill -= incr;
			next[(huff >> drop) + fill] = thisx;
		} while (fill != 0);

		/* backwards increment the len-bit code huff */
		incr = 1U << (len - 1);
		while (huff & incr)
			incr >>= 1;
		if (incr != 0) {
			huff &= incr - 1;
			huff += incr;
		}
		else
			huff = 0;

		/* go to next symbol, update count, len */
		sym++;
		if (--(count[len]) == 0) {
			if (len == max) break;
			len = lens[work[sym]];
		}

		/* create new sub-table if needed */
		if (len > root && (huff & mask) != low) {
			/* if first time, transition to sub-tables */
			if (drop == 0)
				drop = root;

			/* increment past last table */
			next += min;            /* here min is 1 << curr */

			/* determine length of next table */
			curr = len - drop;
			left = (int)(1 << curr);
			while (curr + drop < max) {
				left -= count[curr + drop];
				if (left <= 0) break;
				curr++;
				left <<= 1;
			}

			/* check for enough space */
			used += 1U << curr;
			if (type == LENS && used >= ENOUGH - MAXD)
				return 1;

			/* point entry in root table to sub-table */
			low = huff & mask;
			(*table)[low].op = (unsigned char)curr;
			(*table)[low].bits = (unsigned char)root;
			(*table)[low].val = (unsigned short)(next - *table);
		}
	}

	/*
	   Fill in rest of table for incomplete codes.  This loop is similar to the
	   loop above in incrementing huff for table indices.  It is assumed that
	   len is equal to curr + drop, so there is no loop needed to increment
	   through high index bits.  When the current sub-table is filled, the loop
	   drops back to the root table to fill in any remaining entries there.
	 */
	thisx.op = (unsigned char)64;                /* invalid code marker */
	thisx.bits = (unsigned char)(len - drop);
	thisx.val = (unsigned short)0;
	while (huff != 0) {
		/* when done with sub-table, drop back to root table */
		if (drop != 0 && (huff & mask) != low) {
			drop = 0;
			len = root;
			next = *table;
			thisx.bits = (unsigned char)len;
		}

		/* put invalid code marker in table */
		next[huff >> drop] = thisx;

		/* backwards increment the len-bit code huff */
		incr = 1U << (len - 1);
		while (huff & incr)
			incr >>= 1;
		if (incr != 0) {
			huff &= incr - 1;
			huff += incr;
		}
		else
			huff = 0;
	}

	/* set return parameters */
	*table += used;
	*bits = root;
	return 0;
}

/*** End of inlined file: inftrees.c ***/


/*** Start of inlined file: trees.c ***/
/*
 *  ALGORITHM
 *
 *      The "deflation" process uses several Huffman trees. The more
 *      common source values are represented by shorter bit sequences.
 *
 *      Each code tree is stored in a compressed form which is itself
 * a Huffman encoding of the lengths of all the code strings (in
 * ascending order by source values).  The actual code strings are
 * reconstructed from the lengths in the inflate process, as described
 * in the deflate specification.
 *
 *  REFERENCES
 *
 *      Deutsch, L.P.,"'Deflate' Compressed Data Format Specification".
 *      Available in ftp.uu.net:/pub/archiving/zip/doc/deflate-1.1.doc
 *
 *      Storer, James A.
 *          Data Compression:  Methods and Theory, pp. 49-50.
 *          Computer Science Press, 1988.  ISBN 0-7167-8156-5.
 *
 *      Sedgewick, R.
 *          Algorithms, p290.
 *          Addison-Wesley, 1983. ISBN 0-201-06672-6.
 */

/* @(#) $Id: trees.c,v 1.1 2007/06/07 17:54:37 jules_rms Exp $ */

/* #define GEN_TREES_H */

#ifdef DEBUG
#  include <ctype.h>
#endif

/* ===========================================================================
 * Constants
 */

#define MAX_BL_BITS 7
/* Bit length codes must not exceed MAX_BL_BITS bits */

#define END_BLOCK 256
/* end of block literal code */

#define REP_3_6      16
/* repeat previous bit length 3-6 times (2 bits of repeat count) */

#define REPZ_3_10    17
/* repeat a zero length 3-10 times  (3 bits of repeat count) */

#define REPZ_11_138  18
/* repeat a zero length 11-138 times  (7 bits of repeat count) */

local const int extra_lbits[LENGTH_CODES] /* extra bits for each length code */
   = {0,0,0,0,0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,5,0};

local const int extra_dbits[D_CODES] /* extra bits for each distance code */
   = {0,0,0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8,9,9,10,10,11,11,12,12,13,13};

local const int extra_blbits[BL_CODES]/* extra bits for each bit length code */
   = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,3,7};

local const uch bl_order[BL_CODES]
   = {16,17,18,0,8,7,9,6,10,5,11,4,12,3,13,2,14,1,15};
/* The lengths of the bit length codes are sent in order of decreasing
 * probability, to avoid transmitting the lengths for unused bit length codes.
 */

#define Buf_size (8 * 2*sizeof(char))
/* Number of bits used within bi_buf. (bi_buf might be implemented on
 * more than 16 bits on some systems.)
 */

/* ===========================================================================
 * Local data. These are initialized only once.
 */

#define DIST_CODE_LEN  512 /* see definition of array dist_code below */

#if defined(GEN_TREES_H) || !defined(STDC)
/* non ANSI compilers may not accept trees.h */

local ct_data static_ltree[L_CODES+2];
/* The static literal tree. Since the bit lengths are imposed, there is no
 * need for the L_CODES extra codes used during heap construction. However
 * The codes 286 and 287 are needed to build a canonical tree (see _tr_init
 * below).
 */

local ct_data static_dtree[D_CODES];
/* The static distance tree. (Actually a trivial tree since all codes use
 * 5 bits.)
 */

uch _dist_code[DIST_CODE_LEN];
/* Distance codes. The first 256 values correspond to the distances
 * 3 .. 258, the last 256 values correspond to the top 8 bits of
 * the 15 bit distances.
 */

uch _length_code[MAX_MATCH-MIN_MATCH+1];
/* length code for each normalized match length (0 == MIN_MATCH) */

local int base_length[LENGTH_CODES];
/* First normalized length for each code (0 = MIN_MATCH) */

local int base_dist[D_CODES];
/* First normalized distance for each code (0 = distance of 1) */

#else

/*** Start of inlined file: trees.h ***/
local const ct_data static_ltree[L_CODES+2] = {
{{ 12},{  8}}, {{140},{  8}}, {{ 76},{  8}}, {{204},{  8}}, {{ 44},{  8}},
{{172},{  8}}, {{108},{  8}}, {{236},{  8}}, {{ 28},{  8}}, {{156},{  8}},
{{ 92},{  8}}, {{220},{  8}}, {{ 60},{  8}}, {{188},{  8}}, {{124},{  8}},
{{252},{  8}}, {{  2},{  8}}, {{130},{  8}}, {{ 66},{  8}}, {{194},{  8}},
{{ 34},{  8}}, {{162},{  8}}, {{ 98},{  8}}, {{226},{  8}}, {{ 18},{  8}},
{{146},{  8}}, {{ 82},{  8}}, {{210},{  8}}, {{ 50},{  8}}, {{178},{  8}},
{{114},{  8}}, {{242},{  8}}, {{ 10},{  8}}, {{138},{  8}}, {{ 74},{  8}},
{{202},{  8}}, {{ 42},{  8}}, {{170},{  8}}, {{106},{  8}}, {{234},{  8}},
{{ 26},{  8}}, {{154},{  8}}, {{ 90},{  8}}, {{218},{  8}}, {{ 58},{  8}},
{{186},{  8}}, {{122},{  8}}, {{250},{  8}}, {{  6},{  8}}, {{134},{  8}},
{{ 70},{  8}}, {{198},{  8}}, {{ 38},{  8}}, {{166},{  8}}, {{102},{  8}},
{{230},{  8}}, {{ 22},{  8}}, {{150},{  8}}, {{ 86},{  8}}, {{214},{  8}},
{{ 54},{  8}}, {{182},{  8}}, {{118},{  8}}, {{246},{  8}}, {{ 14},{  8}},
{{142},{  8}}, {{ 78},{  8}}, {{206},{  8}}, {{ 46},{  8}}, {{174},{  8}},
{{110},{  8}}, {{238},{  8}}, {{ 30},{  8}}, {{158},{  8}}, {{ 94},{  8}},
{{222},{  8}}, {{ 62},{  8}}, {{190},{  8}}, {{126},{  8}}, {{254},{  8}},
{{  1},{  8}}, {{129},{  8}}, {{ 65},{  8}}, {{193},{  8}}, {{ 33},{  8}},
{{161},{  8}}, {{ 97},{  8}}, {{225},{  8}}, {{ 17},{  8}}, {{145},{  8}},
{{ 81},{  8}}, {{209},{  8}}, {{ 49},{  8}}, {{177},{  8}}, {{113},{  8}},
{{241},{  8}}, {{  9},{  8}}, {{137},{  8}}, {{ 73},{  8}}, {{201},{  8}},
{{ 41},{  8}}, {{169},{  8}}, {{105},{  8}}, {{233},{  8}}, {{ 25},{  8}},
{{153},{  8}}, {{ 89},{  8}}, {{217},{  8}}, {{ 57},{  8}}, {{185},{  8}},
{{121},{  8}}, {{249},{  8}}, {{  5},{  8}}, {{133},{  8}}, {{ 69},{  8}},
{{197},{  8}}, {{ 37},{  8}}, {{165},{  8}}, {{101},{  8}}, {{229},{  8}},
{{ 21},{  8}}, {{149},{  8}}, {{ 85},{  8}}, {{213},{  8}}, {{ 53},{  8}},
{{181},{  8}}, {{117},{  8}}, {{245},{  8}}, {{ 13},{  8}}, {{141},{  8}},
{{ 77},{  8}}, {{205},{  8}}, {{ 45},{  8}}, {{173},{  8}}, {{109},{  8}},
{{237},{  8}}, {{ 29},{  8}}, {{157},{  8}}, {{ 93},{  8}}, {{221},{  8}},
{{ 61},{  8}}, {{189},{  8}}, {{125},{  8}}, {{253},{  8}}, {{ 19},{  9}},
{{275},{  9}}, {{147},{  9}}, {{403},{  9}}, {{ 83},{  9}}, {{339},{  9}},
{{211},{  9}}, {{467},{  9}}, {{ 51},{  9}}, {{307},{  9}}, {{179},{  9}},
{{435},{  9}}, {{115},{  9}}, {{371},{  9}}, {{243},{  9}}, {{499},{  9}},
{{ 11},{  9}}, {{267},{  9}}, {{139},{  9}}, {{395},{  9}}, {{ 75},{  9}},
{{331},{  9}}, {{203},{  9}}, {{459},{  9}}, {{ 43},{  9}}, {{299},{  9}},
{{171},{  9}}, {{427},{  9}}, {{107},{  9}}, {{363},{  9}}, {{235},{  9}},
{{491},{  9}}, {{ 27},{  9}}, {{283},{  9}}, {{155},{  9}}, {{411},{  9}},
{{ 91},{  9}}, {{347},{  9}}, {{219},{  9}}, {{475},{  9}}, {{ 59},{  9}},
{{315},{  9}}, {{187},{  9}}, {{443},{  9}}, {{123},{  9}}, {{379},{  9}},
{{251},{  9}}, {{507},{  9}}, {{  7},{  9}}, {{263},{  9}}, {{135},{  9}},
{{391},{  9}}, {{ 71},{  9}}, {{327},{  9}}, {{199},{  9}}, {{455},{  9}},
{{ 39},{  9}}, {{295},{  9}}, {{167},{  9}}, {{423},{  9}}, {{103},{  9}},
{{359},{  9}}, {{231},{  9}}, {{487},{  9}}, {{ 23},{  9}}, {{279},{  9}},
{{151},{  9}}, {{407},{  9}}, {{ 87},{  9}}, {{343},{  9}}, {{215},{  9}},
{{471},{  9}}, {{ 55},{  9}}, {{311},{  9}}, {{183},{  9}}, {{439},{  9}},
{{119},{  9}}, {{375},{  9}}, {{247},{  9}}, {{503},{  9}}, {{ 15},{  9}},
{{271},{  9}}, {{143},{  9}}, {{399},{  9}}, {{ 79},{  9}}, {{335},{  9}},
{{207},{  9}}, {{463},{  9}}, {{ 47},{  9}}, {{303},{  9}}, {{175},{  9}},
{{431},{  9}}, {{111},{  9}}, {{367},{  9}}, {{239},{  9}}, {{495},{  9}},
{{ 31},{  9}}, {{287},{  9}}, {{159},{  9}}, {{415},{  9}}, {{ 95},{  9}},
{{351},{  9}}, {{223},{  9}}, {{479},{  9}}, {{ 63},{  9}}, {{319},{  9}},
{{191},{  9}}, {{447},{  9}}, {{127},{  9}}, {{383},{  9}}, {{255},{  9}},
{{511},{  9}}, {{  0},{  7}}, {{ 64},{  7}}, {{ 32},{  7}}, {{ 96},{  7}},
{{ 16},{  7}}, {{ 80},{  7}}, {{ 48},{  7}}, {{112},{  7}}, {{  8},{  7}},
{{ 72},{  7}}, {{ 40},{  7}}, {{104},{  7}}, {{ 24},{  7}}, {{ 88},{  7}},
{{ 56},{  7}}, {{120},{  7}}, {{  4},{  7}}, {{ 68},{  7}}, {{ 36},{  7}},
{{100},{  7}}, {{ 20},{  7}}, {{ 84},{  7}}, {{ 52},{  7}}, {{116},{  7}},
{{  3},{  8}}, {{131},{  8}}, {{ 67},{  8}}, {{195},{  8}}, {{ 35},{  8}},
{{163},{  8}}, {{ 99},{  8}}, {{227},{  8}}
};

local const ct_data static_dtree[D_CODES] = {
{{ 0},{ 5}}, {{16},{ 5}}, {{ 8},{ 5}}, {{24},{ 5}}, {{ 4},{ 5}},
{{20},{ 5}}, {{12},{ 5}}, {{28},{ 5}}, {{ 2},{ 5}}, {{18},{ 5}},
{{10},{ 5}}, {{26},{ 5}}, {{ 6},{ 5}}, {{22},{ 5}}, {{14},{ 5}},
{{30},{ 5}}, {{ 1},{ 5}}, {{17},{ 5}}, {{ 9},{ 5}}, {{25},{ 5}},
{{ 5},{ 5}}, {{21},{ 5}}, {{13},{ 5}}, {{29},{ 5}}, {{ 3},{ 5}},
{{19},{ 5}}, {{11},{ 5}}, {{27},{ 5}}, {{ 7},{ 5}}, {{23},{ 5}}
};

const uch _dist_code[DIST_CODE_LEN] = {
 0,  1,  2,  3,  4,  4,  5,  5,  6,  6,  6,  6,  7,  7,  7,  7,  8,  8,  8,  8,
 8,  8,  8,  8,  9,  9,  9,  9,  9,  9,  9,  9, 10, 10, 10, 10, 10, 10, 10, 10,
10, 10, 10, 10, 10, 10, 10, 10, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11,
11, 11, 11, 11, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 13, 13, 13, 13,
13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13,
13, 13, 13, 13, 13, 13, 13, 13, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14,
14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14,
14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14,
14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 15, 15, 15, 15, 15, 15, 15, 15,
15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,  0,  0, 16, 17,
18, 18, 19, 19, 20, 20, 20, 20, 21, 21, 21, 21, 22, 22, 22, 22, 22, 22, 22, 22,
23, 23, 23, 23, 23, 23, 23, 23, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24,
24, 24, 24, 24, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25,
26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26,
26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 27, 27, 27, 27, 27, 27, 27, 27,
27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27,
27, 27, 27, 27, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28,
28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28,
28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28,
28, 28, 28, 28, 28, 28, 28, 28, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29,
29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29,
29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29,
29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29
};

const uch _length_code[MAX_MATCH-MIN_MATCH+1]= {
 0,  1,  2,  3,  4,  5,  6,  7,  8,  8,  9,  9, 10, 10, 11, 11, 12, 12, 12, 12,
13, 13, 13, 13, 14, 14, 14, 14, 15, 15, 15, 15, 16, 16, 16, 16, 16, 16, 16, 16,
17, 17, 17, 17, 17, 17, 17, 17, 18, 18, 18, 18, 18, 18, 18, 18, 19, 19, 19, 19,
19, 19, 19, 19, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 22, 22, 22, 22,
22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 23, 23, 23, 23, 23, 23, 23, 23,
23, 23, 23, 23, 23, 23, 23, 23, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24,
24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24,
25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25,
25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 26, 26, 26, 26, 26, 26, 26, 26,
26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26,
26, 26, 26, 26, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27,
27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 28
};

local const int base_length[LENGTH_CODES] = {
0, 1, 2, 3, 4, 5, 6, 7, 8, 10, 12, 14, 16, 20, 24, 28, 32, 40, 48, 56,
64, 80, 96, 112, 128, 160, 192, 224, 0
};

local const int base_dist[D_CODES] = {
	0,     1,     2,     3,     4,     6,     8,    12,    16,    24,
   32,    48,    64,    96,   128,   192,   256,   384,   512,   768,
 1024,  1536,  2048,  3072,  4096,  6144,  8192, 12288, 16384, 24576
};

/*** End of inlined file: trees.h ***/


#endif /* GEN_TREES_H */

struct static_tree_desc_s {
	const ct_data *static_tree;  /* static tree or NULL */
	const intf *extra_bits;      /* extra bits for each code or NULL */
	int     extra_base;          /* base index for extra_bits */
	int     elems;               /* max number of elements in the tree */
	int     max_length;          /* max bit length for the codes */
};

local static_tree_desc  static_l_desc =
{static_ltree, extra_lbits, LITERALS+1, L_CODES, MAX_BITS};

local static_tree_desc  static_d_desc =
{static_dtree, extra_dbits, 0,          D_CODES, MAX_BITS};

local static_tree_desc  static_bl_desc =
{(const ct_data *)0, extra_blbits, 0,   BL_CODES, MAX_BL_BITS};

/* ===========================================================================
 * Local (static) routines in this file.
 */

local void tr_static_init OF((void));
local void init_block     OF((deflate_state *s));
local void pqdownheap     OF((deflate_state *s, ct_data *tree, int k));
local void gen_bitlen     OF((deflate_state *s, tree_desc *desc));
local void gen_codes      OF((ct_data *tree, int max_code, ushf *bl_count));
local void build_tree     OF((deflate_state *s, tree_desc *desc));
local void scan_tree      OF((deflate_state *s, ct_data *tree, int max_code));
local void send_tree      OF((deflate_state *s, ct_data *tree, int max_code));
local int  build_bl_tree  OF((deflate_state *s));
local void send_all_trees OF((deflate_state *s, int lcodes, int dcodes,
							  int blcodes));
local void compress_block OF((deflate_state *s, ct_data *ltree,
							  ct_data *dtree));
local void set_data_type  OF((deflate_state *s));
local unsigned bi_reverse OF((unsigned value, int length));
local void bi_windup      OF((deflate_state *s));
local void bi_flush       OF((deflate_state *s));
local void copy_block     OF((deflate_state *s, charf *buf, unsigned len,
							  int header));

#ifdef GEN_TREES_H
local void gen_trees_header OF((void));
#endif

#ifndef DEBUG
#  define send_code(s, c, tree) send_bits(s, tree[c].Code, tree[c].Len)
   /* Send a code of the given tree. c and tree must not have side effects */

#else /* DEBUG */
#  define send_code(s, c, tree) \
	 { if (z_verbose>2) fprintf(stderr,"\ncd %3d ",(c)); \
	   send_bits(s, tree[c].Code, tree[c].Len); }
#endif

/* ===========================================================================
 * Output a short LSB first on the stream.
 * IN assertion: there is enough room in pendingBuf.
 */
#define put_short(s, w) { \
	put_byte(s, (uch)((w) & 0xff)); \
	put_byte(s, (uch)((ush)(w) >> 8)); \
}

/* ===========================================================================
 * Send a value on a given number of bits.
 * IN assertion: length <= 16 and value fits in length bits.
 */
#ifdef DEBUG
local void send_bits      OF((deflate_state *s, int value, int length));

local void send_bits (deflate_state *s, int value, int length)
{
	Tracevv((stderr," l %2d v %4x ", length, value));
	Assert(length > 0 && length <= 15, "invalid length");
	s->bits_sent += (ulg)length;

	/* If not enough room in bi_buf, use (valid) bits from bi_buf and
	 * (16 - bi_valid) bits from value, leaving (width - (16-bi_valid))
	 * unused bits in value.
	 */
	if (s->bi_valid > (int)Buf_size - length) {
		s->bi_buf |= (value << s->bi_valid);
		put_short(s, s->bi_buf);
		s->bi_buf = (ush)value >> (Buf_size - s->bi_valid);
		s->bi_valid += length - Buf_size;
	} else {
		s->bi_buf |= value << s->bi_valid;
		s->bi_valid += length;
	}
}
#else /* !DEBUG */

#define send_bits(s, value, length) \
{ int len = length;\
  if (s->bi_valid > (int)Buf_size - len) {\
	int val = value;\
	s->bi_buf |= (val << s->bi_valid);\
	put_short(s, s->bi_buf);\
	s->bi_buf = (ush)val >> (Buf_size - s->bi_valid);\
	s->bi_valid += len - Buf_size;\
  } else {\
	s->bi_buf |= (value) << s->bi_valid;\
	s->bi_valid += len;\
  }\
}
#endif /* DEBUG */

/* the arguments must not have side effects */

/* ===========================================================================
 * Initialize the various 'constant' tables.
 */
local void tr_static_init()
{
#if defined(GEN_TREES_H) || !defined(STDC)
	static int static_init_done = 0;
	int n;        /* iterates over tree elements */
	int bits;     /* bit counter */
	int length;   /* length value */
	int code;     /* code value */
	int dist;     /* distance index */
	ush bl_count[MAX_BITS+1];
	/* number of codes at each bit length for an optimal tree */

	if (static_init_done) return;

	/* For some embedded targets, global variables are not initialized: */
	static_l_desc.static_tree = static_ltree;
	static_l_desc.extra_bits = extra_lbits;
	static_d_desc.static_tree = static_dtree;
	static_d_desc.extra_bits = extra_dbits;
	static_bl_desc.extra_bits = extra_blbits;

	/* Initialize the mapping length (0..255) -> length code (0..28) */
	length = 0;
	for (code = 0; code < LENGTH_CODES-1; code++) {
		base_length[code] = length;
		for (n = 0; n < (1<<extra_lbits[code]); n++) {
			_length_code[length++] = (uch)code;
		}
	}
	Assert (length == 256, "tr_static_init: length != 256");
	/* Note that the length 255 (match length 258) can be represented
	 * in two different ways: code 284 + 5 bits or code 285, so we
	 * overwrite length_code[255] to use the best encoding:
	 */
	_length_code[length-1] = (uch)code;

	/* Initialize the mapping dist (0..32K) -> dist code (0..29) */
	dist = 0;
	for (code = 0 ; code < 16; code++) {
		base_dist[code] = dist;
		for (n = 0; n < (1<<extra_dbits[code]); n++) {
			_dist_code[dist++] = (uch)code;
		}
	}
	Assert (dist == 256, "tr_static_init: dist != 256");
	dist >>= 7; /* from now on, all distances are divided by 128 */
	for ( ; code < D_CODES; code++) {
		base_dist[code] = dist << 7;
		for (n = 0; n < (1<<(extra_dbits[code]-7)); n++) {
			_dist_code[256 + dist++] = (uch)code;
		}
	}
	Assert (dist == 256, "tr_static_init: 256+dist != 512");

	/* Construct the codes of the static literal tree */
	for (bits = 0; bits <= MAX_BITS; bits++) bl_count[bits] = 0;
	n = 0;
	while (n <= 143) static_ltree[n++].Len = 8, bl_count[8]++;
	while (n <= 255) static_ltree[n++].Len = 9, bl_count[9]++;
	while (n <= 279) static_ltree[n++].Len = 7, bl_count[7]++;
	while (n <= 287) static_ltree[n++].Len = 8, bl_count[8]++;
	/* Codes 286 and 287 do not exist, but we must include them in the
	 * tree construction to get a canonical Huffman tree (longest code
	 * all ones)
	 */
	gen_codes((ct_data *)static_ltree, L_CODES+1, bl_count);

	/* The static distance tree is trivial: */
	for (n = 0; n < D_CODES; n++) {
		static_dtree[n].Len = 5;
		static_dtree[n].Code = bi_reverse((unsigned)n, 5);
	}
	static_init_done = 1;

#  ifdef GEN_TREES_H
	gen_trees_header();
#  endif
#endif /* defined(GEN_TREES_H) || !defined(STDC) */
}

/* ===========================================================================
 * Genererate the file trees.h describing the static trees.
 */
#ifdef GEN_TREES_H
#  ifndef DEBUG
#    include <stdio.h>
#  endif

#  define SEPARATOR(i, last, width) \
	  ((i) == (last)? "\n};\n\n" :    \
	   ((i) % (width) == (width)-1 ? ",\n" : ", "))

void gen_trees_header()
{
	FILE *header = fopen("trees.h", "w");
	int i;

	Assert (header != NULL, "Can't open trees.h");
	fprintf(header,
			"/* header created automatically with -DGEN_TREES_H */\n\n");

	fprintf(header, "local const ct_data static_ltree[L_CODES+2] = {\n");
	for (i = 0; i < L_CODES+2; i++) {
		fprintf(header, "{{%3u},{%3u}}%s", static_ltree[i].Code,
				static_ltree[i].Len, SEPARATOR(i, L_CODES+1, 5));
	}

	fprintf(header, "local const ct_data static_dtree[D_CODES] = {\n");
	for (i = 0; i < D_CODES; i++) {
		fprintf(header, "{{%2u},{%2u}}%s", static_dtree[i].Code,
				static_dtree[i].Len, SEPARATOR(i, D_CODES-1, 5));
	}

	fprintf(header, "const uch _dist_code[DIST_CODE_LEN] = {\n");
	for (i = 0; i < DIST_CODE_LEN; i++) {
		fprintf(header, "%2u%s", _dist_code[i],
				SEPARATOR(i, DIST_CODE_LEN-1, 20));
	}

	fprintf(header, "const uch _length_code[MAX_MATCH-MIN_MATCH+1]= {\n");
	for (i = 0; i < MAX_MATCH-MIN_MATCH+1; i++) {
		fprintf(header, "%2u%s", _length_code[i],
				SEPARATOR(i, MAX_MATCH-MIN_MATCH, 20));
	}

	fprintf(header, "local const int base_length[LENGTH_CODES] = {\n");
	for (i = 0; i < LENGTH_CODES; i++) {
		fprintf(header, "%1u%s", base_length[i],
				SEPARATOR(i, LENGTH_CODES-1, 20));
	}

	fprintf(header, "local const int base_dist[D_CODES] = {\n");
	for (i = 0; i < D_CODES; i++) {
		fprintf(header, "%5u%s", base_dist[i],
				SEPARATOR(i, D_CODES-1, 10));
	}

	fclose(header);
}
#endif /* GEN_TREES_H */

/* ===========================================================================
 * Initialize the tree data structures for a new zlib stream.
 */
void _tr_init(deflate_state *s)
{
	tr_static_init();

	s->l_desc.dyn_tree = s->dyn_ltree;
	s->l_desc.stat_desc = &static_l_desc;

	s->d_desc.dyn_tree = s->dyn_dtree;
	s->d_desc.stat_desc = &static_d_desc;

	s->bl_desc.dyn_tree = s->bl_tree;
	s->bl_desc.stat_desc = &static_bl_desc;

	s->bi_buf = 0;
	s->bi_valid = 0;
	s->last_eob_len = 8; /* enough lookahead for inflate */
#ifdef DEBUG
	s->compressed_len = 0L;
	s->bits_sent = 0L;
#endif

	/* Initialize the first block of the first file: */
	init_block(s);
}

/* ===========================================================================
 * Initialize a new block.
 */
local void init_block (deflate_state *s)
{
	int n; /* iterates over tree elements */

	/* Initialize the trees. */
	for (n = 0; n < L_CODES;  n++) s->dyn_ltree[n].Freq = 0;
	for (n = 0; n < D_CODES;  n++) s->dyn_dtree[n].Freq = 0;
	for (n = 0; n < BL_CODES; n++) s->bl_tree[n].Freq = 0;

	s->dyn_ltree[END_BLOCK].Freq = 1;
	s->opt_len = s->static_len = 0L;
	s->last_lit = s->matches = 0;
}

#define SMALLEST 1
/* Index within the heap array of least frequent node in the Huffman tree */

/* ===========================================================================
 * Remove the smallest element from the heap and recreate the heap with
 * one less element. Updates heap and heap_len.
 */
#define pqremove(s, tree, top) \
{\
	top = s->heap[SMALLEST]; \
	s->heap[SMALLEST] = s->heap[s->heap_len--]; \
	pqdownheap(s, tree, SMALLEST); \
}

/* ===========================================================================
 * Compares to subtrees, using the tree depth as tie breaker when
 * the subtrees have equal frequency. This minimizes the worst case length.
 */
#define smaller(tree, n, m, depth) \
   (tree[n].Freq < tree[m].Freq || \
   (tree[n].Freq == tree[m].Freq && depth[n] <= depth[m]))

/* ===========================================================================
 * Restore the heap property by moving down the tree starting at node k,
 * exchanging a node with the smallest of its two sons if necessary, stopping
 * when the heap property is re-established (each father smaller than its
 * two sons).
 */
local void pqdownheap (deflate_state *s,
					   ct_data *tree,  /* the tree to restore */
					   int k)               /* node to move down */
{
	int v = s->heap[k];
	int j = k << 1;  /* left son of k */
	while (j <= s->heap_len) {
		/* Set j to the smallest of the two sons: */
		if (j < s->heap_len &&
			smaller(tree, s->heap[j+1], s->heap[j], s->depth)) {
			j++;
		}
		/* Exit if v is smaller than both sons */
		if (smaller(tree, v, s->heap[j], s->depth)) break;

		/* Exchange v with the smallest son */
		s->heap[k] = s->heap[j];  k = j;

		/* And continue down the tree, setting j to the left son of k */
		j <<= 1;
	}
	s->heap[k] = v;
}

/* ===========================================================================
 * Compute the optimal bit lengths for a tree and update the total bit length
 * for the current block.
 * IN assertion: the fields freq and dad are set, heap[heap_max] and
 *    above are the tree nodes sorted by increasing frequency.
 * OUT assertions: the field len is set to the optimal bit length, the
 *     array bl_count contains the frequencies for each bit length.
 *     The length opt_len is updated; static_len is also updated if stree is
 *     not null.
 */
local void gen_bitlen (deflate_state *s, tree_desc *desc)
{
	ct_data *tree        = desc->dyn_tree;
	int max_code         = desc->max_code;
	const ct_data *stree = desc->stat_desc->static_tree;
	const intf *extra    = desc->stat_desc->extra_bits;
	int base             = desc->stat_desc->extra_base;
	int max_length       = desc->stat_desc->max_length;
	int h;              /* heap index */
	int n, m;           /* iterate over the tree elements */
	int bits;           /* bit length */
	int xbits;          /* extra bits */
	ush f;              /* frequency */
	int overflow = 0;   /* number of elements with bit length too large */

	for (bits = 0; bits <= MAX_BITS; bits++) s->bl_count[bits] = 0;

	/* In a first pass, compute the optimal bit lengths (which may
	 * overflow in the case of the bit length tree).
	 */
	tree[s->heap[s->heap_max]].Len = 0; /* root of the heap */

	for (h = s->heap_max+1; h < HEAP_SIZE; h++) {
		n = s->heap[h];
		bits = tree[tree[n].Dad].Len + 1;
		if (bits > max_length) bits = max_length, overflow++;
		tree[n].Len = (ush)bits;
		/* We overwrite tree[n].Dad which is no longer needed */

		if (n > max_code) continue; /* not a leaf node */

		s->bl_count[bits]++;
		xbits = 0;
		if (n >= base) xbits = extra[n-base];
		f = tree[n].Freq;
		s->opt_len += (ulg)f * (bits + xbits);
		if (stree) s->static_len += (ulg)f * (stree[n].Len + xbits);
	}
	if (overflow == 0) return;

	Trace((stderr,"\nbit length overflow\n"));
	/* This happens for example on obj2 and pic of the Calgary corpus */

	/* Find the first bit length which could increase: */
	do {
		bits = max_length-1;
		while (s->bl_count[bits] == 0) bits--;
		s->bl_count[bits]--;      /* move one leaf down the tree */
		s->bl_count[bits+1] += 2; /* move one overflow item as its brother */
		s->bl_count[max_length]--;
		/* The brother of the overflow item also moves one step up,
		 * but this does not affect bl_count[max_length]
		 */
		overflow -= 2;
	} while (overflow > 0);

	/* Now recompute all bit lengths, scanning in increasing frequency.
	 * h is still equal to HEAP_SIZE. (It is simpler to reconstruct all
	 * lengths instead of fixing only the wrong ones. This idea is taken
	 * from 'ar' written by Haruhiko Okumura.)
	 */
	for (bits = max_length; bits != 0; bits--) {
		n = s->bl_count[bits];
		while (n != 0) {
			m = s->heap[--h];
			if (m > max_code) continue;
			if ((unsigned) tree[m].Len != (unsigned) bits) {
				Trace((stderr,"code %d bits %d->%d\n", m, tree[m].Len, bits));
				s->opt_len += ((long)bits - (long)tree[m].Len)
							  *(long)tree[m].Freq;
				tree[m].Len = (ush)bits;
			}
			n--;
		}
	}
}

/* ===========================================================================
 * Generate the codes for a given tree and bit counts (which need not be
 * optimal).
 * IN assertion: the array bl_count contains the bit length statistics for
 * the given tree and the field len is set for all tree elements.
 * OUT assertion: the field code is set for all tree elements of non
 *     zero code length.
 */
local void gen_codes (ct_data *tree,             /* the tree to decorate */
					  int max_code,              /* largest code with non zero frequency */
					  ushf *bl_count)            /* number of codes at each bit length */
{
	ush next_code[MAX_BITS+1]; /* next code value for each bit length */
	ush code = 0;              /* running code value */
	int bits;                  /* bit index */
	int n;                     /* code index */

	/* The distribution counts are first used to generate the code values
	 * without bit reversal.
	 */
	for (bits = 1; bits <= MAX_BITS; bits++) {
		next_code[bits] = code = (code + bl_count[bits-1]) << 1;
	}
	/* Check that the bit counts in bl_count are consistent. The last code
	 * must be all ones.
	 */
	Assert (code + bl_count[MAX_BITS]-1 == (1<<MAX_BITS)-1,
			"inconsistent bit counts");
	Tracev((stderr,"\ngen_codes: max_code %d ", max_code));

	for (n = 0;  n <= max_code; n++) {
		int len = tree[n].Len;
		if (len == 0) continue;
		/* Now reverse the bits */
		tree[n].Code = bi_reverse(next_code[len]++, len);

		Tracecv(tree != static_ltree, (stderr,"\nn %3d %c l %2d c %4x (%x) ",
			 n, (isgraph(n) ? n : ' '), len, tree[n].Code, next_code[len]-1));
	}
}

/* ===========================================================================
 * Construct one Huffman tree and assigns the code bit strings and lengths.
 * Update the total bit length for the current block.
 * IN assertion: the field freq is set for all tree elements.
 * OUT assertions: the fields len and code are set to the optimal bit length
 *     and corresponding code. The length opt_len is updated; static_len is
 *     also updated if stree is not null. The field max_code is set.
 */
local void build_tree (deflate_state *s,
					   tree_desc *desc) /* the tree descriptor */
{
	ct_data *tree         = desc->dyn_tree;
	const ct_data *stree  = desc->stat_desc->static_tree;
	int elems             = desc->stat_desc->elems;
	int n, m;          /* iterate over heap elements */
	int max_code = -1; /* largest code with non zero frequency */
	int node;          /* new node being created */

	/* Construct the initial heap, with least frequent element in
	 * heap[SMALLEST]. The sons of heap[n] are heap[2*n] and heap[2*n+1].
	 * heap[0] is not used.
	 */
	s->heap_len = 0, s->heap_max = HEAP_SIZE;

	for (n = 0; n < elems; n++) {
		if (tree[n].Freq != 0) {
			s->heap[++(s->heap_len)] = max_code = n;
			s->depth[n] = 0;
		} else {
			tree[n].Len = 0;
		}
	}

	/* The pkzip format requires that at least one distance code exists,
	 * and that at least one bit should be sent even if there is only one
	 * possible code. So to avoid special checks later on we force at least
	 * two codes of non zero frequency.
	 */
	while (s->heap_len < 2) {
		node = s->heap[++(s->heap_len)] = (max_code < 2 ? ++max_code : 0);
		tree[node].Freq = 1;
		s->depth[node] = 0;
		s->opt_len--; if (stree) s->static_len -= stree[node].Len;
		/* node is 0 or 1 so it does not have extra bits */
	}
	desc->max_code = max_code;

	/* The elements heap[heap_len/2+1 .. heap_len] are leaves of the tree,
	 * establish sub-heaps of increasing lengths:
	 */
	for (n = s->heap_len/2; n >= 1; n--) pqdownheap(s, tree, n);

	/* Construct the Huffman tree by repeatedly combining the least two
	 * frequent nodes.
	 */
	node = elems;              /* next internal node of the tree */
	do {
		pqremove(s, tree, n);  /* n = node of least frequency */
		m = s->heap[SMALLEST]; /* m = node of next least frequency */

		s->heap[--(s->heap_max)] = n; /* keep the nodes sorted by frequency */
		s->heap[--(s->heap_max)] = m;

		/* Create a new node father of n and m */
		tree[node].Freq = tree[n].Freq + tree[m].Freq;
		s->depth[node] = (uch)((s->depth[n] >= s->depth[m] ?
								s->depth[n] : s->depth[m]) + 1);
		tree[n].Dad = tree[m].Dad = (ush)node;
#ifdef DUMP_BL_TREE
		if (tree == s->bl_tree) {
			fprintf(stderr,"\nnode %d(%d), sons %d(%d) %d(%d)",
					node, tree[node].Freq, n, tree[n].Freq, m, tree[m].Freq);
		}
#endif
		/* and insert the new node in the heap */
		s->heap[SMALLEST] = node++;
		pqdownheap(s, tree, SMALLEST);

	} while (s->heap_len >= 2);

	s->heap[--(s->heap_max)] = s->heap[SMALLEST];

	/* At this point, the fields freq and dad are set. We can now
	 * generate the bit lengths.
	 */
	gen_bitlen(s, (tree_desc *)desc);

	/* The field len is now set, we can generate the bit codes */
	gen_codes ((ct_data *)tree, max_code, s->bl_count);
}

/* ===========================================================================
 * Scan a literal or distance tree to determine the frequencies of the codes
 * in the bit length tree.
 */
local void scan_tree (deflate_state *s,
					  ct_data *tree,   /* the tree to be scanned */
					  int max_code)    /* and its largest code of non zero frequency */
{
	int n;                     /* iterates over all tree elements */
	int prevlen = -1;          /* last emitted length */
	int curlen;                /* length of current code */
	int nextlen = tree[0].Len; /* length of next code */
	int count = 0;             /* repeat count of the current code */
	int max_count = 7;         /* max repeat count */
	int min_count = 4;         /* min repeat count */

	if (nextlen == 0) max_count = 138, min_count = 3;
	tree[max_code+1].Len = (ush)0xffff; /* guard */

	for (n = 0; n <= max_code; n++) {
		curlen = nextlen; nextlen = tree[n+1].Len;
		if (++count < max_count && curlen == nextlen) {
			continue;
		} else if (count < min_count) {
			s->bl_tree[curlen].Freq += count;
		} else if (curlen != 0) {
			if (curlen != prevlen) s->bl_tree[curlen].Freq++;
			s->bl_tree[REP_3_6].Freq++;
		} else if (count <= 10) {
			s->bl_tree[REPZ_3_10].Freq++;
		} else {
			s->bl_tree[REPZ_11_138].Freq++;
		}
		count = 0; prevlen = curlen;
		if (nextlen == 0) {
			max_count = 138, min_count = 3;
		} else if (curlen == nextlen) {
			max_count = 6, min_count = 3;
		} else {
			max_count = 7, min_count = 4;
		}
	}
}

/* ===========================================================================
 * Send a literal or distance tree in compressed form, using the codes in
 * bl_tree.
 */
local void send_tree (deflate_state *s,
					  ct_data *tree, /* the tree to be scanned */
					  int max_code)       /* and its largest code of non zero frequency */
{
	int n;                     /* iterates over all tree elements */
	int prevlen = -1;          /* last emitted length */
	int curlen;                /* length of current code */
	int nextlen = tree[0].Len; /* length of next code */
	int count = 0;             /* repeat count of the current code */
	int max_count = 7;         /* max repeat count */
	int min_count = 4;         /* min repeat count */

	/* tree[max_code+1].Len = -1; */  /* guard already set */
	if (nextlen == 0) max_count = 138, min_count = 3;

	for (n = 0; n <= max_code; n++) {
		curlen = nextlen; nextlen = tree[n+1].Len;
		if (++count < max_count && curlen == nextlen) {
			continue;
		} else if (count < min_count) {
			do { send_code(s, curlen, s->bl_tree); } while (--count != 0);

		} else if (curlen != 0) {
			if (curlen != prevlen) {
				send_code(s, curlen, s->bl_tree); count--;
			}
			Assert(count >= 3 && count <= 6, " 3_6?");
			send_code(s, REP_3_6, s->bl_tree); send_bits(s, count-3, 2);

		} else if (count <= 10) {
			send_code(s, REPZ_3_10, s->bl_tree); send_bits(s, count-3, 3);

		} else {
			send_code(s, REPZ_11_138, s->bl_tree); send_bits(s, count-11, 7);
		}
		count = 0; prevlen = curlen;
		if (nextlen == 0) {
			max_count = 138, min_count = 3;
		} else if (curlen == nextlen) {
			max_count = 6, min_count = 3;
		} else {
			max_count = 7, min_count = 4;
		}
	}
}

/* ===========================================================================
 * Construct the Huffman tree for the bit lengths and return the index in
 * bl_order of the last bit length code to send.
 */
local int build_bl_tree (deflate_state *s)
{
	int max_blindex;  /* index of last bit length code of non zero freq */

	/* Determine the bit length frequencies for literal and distance trees */
	scan_tree(s, (ct_data *)s->dyn_ltree, s->l_desc.max_code);
	scan_tree(s, (ct_data *)s->dyn_dtree, s->d_desc.max_code);

	/* Build the bit length tree: */
	build_tree(s, (tree_desc *)(&(s->bl_desc)));
	/* opt_len now includes the length of the tree representations, except
	 * the lengths of the bit lengths codes and the 5+5+4 bits for the counts.
	 */

	/* Determine the number of bit length codes to send. The pkzip format
	 * requires that at least 4 bit length codes be sent. (appnote.txt says
	 * 3 but the actual value used is 4.)
	 */
	for (max_blindex = BL_CODES-1; max_blindex >= 3; max_blindex--) {
		if (s->bl_tree[bl_order[max_blindex]].Len != 0) break;
	}
	/* Update opt_len to include the bit length tree and counts */
	s->opt_len += 3*(max_blindex+1) + 5+5+4;
	Tracev((stderr, "\ndyn trees: dyn %ld, stat %ld",
			s->opt_len, s->static_len));

	return max_blindex;
}

/* ===========================================================================
 * Send the header for a block using dynamic Huffman trees: the counts, the
 * lengths of the bit length codes, the literal tree and the distance tree.
 * IN assertion: lcodes >= 257, dcodes >= 1, blcodes >= 4.
 */
local void send_all_trees (deflate_state *s,
						   int lcodes, int dcodes, int blcodes) /* number of codes for each tree */
{
	int rank;                    /* index in bl_order */

	Assert (lcodes >= 257 && dcodes >= 1 && blcodes >= 4, "not enough codes");
	Assert (lcodes <= L_CODES && dcodes <= D_CODES && blcodes <= BL_CODES,
			"too many codes");
	Tracev((stderr, "\nbl counts: "));
	send_bits(s, lcodes-257, 5); /* not +255 as stated in appnote.txt */
	send_bits(s, dcodes-1,   5);
	send_bits(s, blcodes-4,  4); /* not -3 as stated in appnote.txt */
	for (rank = 0; rank < blcodes; rank++) {
		Tracev((stderr, "\nbl code %2d ", bl_order[rank]));
		send_bits(s, s->bl_tree[bl_order[rank]].Len, 3);
	}
	Tracev((stderr, "\nbl tree: sent %ld", s->bits_sent));

	send_tree(s, (ct_data *)s->dyn_ltree, lcodes-1); /* literal tree */
	Tracev((stderr, "\nlit tree: sent %ld", s->bits_sent));

	send_tree(s, (ct_data *)s->dyn_dtree, dcodes-1); /* distance tree */
	Tracev((stderr, "\ndist tree: sent %ld", s->bits_sent));
}

/* ===========================================================================
 * Send a stored block
 */
void _tr_stored_block (deflate_state *s, charf *buf, ulg stored_len, int eof)
{
	send_bits(s, (STORED_BLOCK<<1)+eof, 3);  /* send block type */
#ifdef DEBUG
	s->compressed_len = (s->compressed_len + 3 + 7) & (ulg)~7L;
	s->compressed_len += (stored_len + 4) << 3;
#endif
	copy_block(s, buf, (unsigned)stored_len, 1); /* with header */
}

/* ===========================================================================
 * Send one empty static block to give enough lookahead for inflate.
 * This takes 10 bits, of which 7 may remain in the bit buffer.
 * The current inflate code requires 9 bits of lookahead. If the
 * last two codes for the previous block (real code plus EOB) were coded
 * on 5 bits or less, inflate may have only 5+3 bits of lookahead to decode
 * the last real code. In this case we send two empty static blocks instead
 * of one. (There are no problems if the previous block is stored or fixed.)
 * To simplify the code, we assume the worst case of last real code encoded
 * on one bit only.
 */
void _tr_align (deflate_state *s)
{
	send_bits(s, STATIC_TREES<<1, 3);
	send_code(s, END_BLOCK, static_ltree);
#ifdef DEBUG
	s->compressed_len += 10L; /* 3 for block type, 7 for EOB */
#endif
	bi_flush(s);
	/* Of the 10 bits for the empty block, we have already sent
	 * (10 - bi_valid) bits. The lookahead for the last real code (before
	 * the EOB of the previous block) was thus at least one plus the length
	 * of the EOB plus what we have just sent of the empty static block.
	 */
	if (1 + s->last_eob_len + 10 - s->bi_valid < 9) {
		send_bits(s, STATIC_TREES<<1, 3);
		send_code(s, END_BLOCK, static_ltree);
#ifdef DEBUG
		s->compressed_len += 10L;
#endif
		bi_flush(s);
	}
	s->last_eob_len = 7;
}

/* ===========================================================================
 * Determine the best encoding for the current block: dynamic trees, static
 * trees or store, and output the encoded block to the zip file.
 */
void _tr_flush_block (deflate_state *s,
					  charf *buf,       /* input block, or NULL if too old */
					  ulg stored_len,   /* length of input block */
					  int eof)          /* true if this is the last block for a file */
{
	ulg opt_lenb, static_lenb; /* opt_len and static_len in bytes */
	int max_blindex = 0;  /* index of last bit length code of non zero freq */

	/* Build the Huffman trees unless a stored block is forced */
	if (s->level > 0) {

		/* Check if the file is binary or text */
		if (stored_len > 0 && s->strm->data_type == Z_UNKNOWN)
			set_data_type(s);

		/* Construct the literal and distance trees */
		build_tree(s, (tree_desc *)(&(s->l_desc)));
		Tracev((stderr, "\nlit data: dyn %ld, stat %ld", s->opt_len,
				s->static_len));

		build_tree(s, (tree_desc *)(&(s->d_desc)));
		Tracev((stderr, "\ndist data: dyn %ld, stat %ld", s->opt_len,
				s->static_len));
		/* At this point, opt_len and static_len are the total bit lengths of
		 * the compressed block data, excluding the tree representations.
		 */

		/* Build the bit length tree for the above two trees, and get the index
		 * in bl_order of the last bit length code to send.
		 */
		max_blindex = build_bl_tree(s);

		/* Determine the best encoding. Compute the block lengths in bytes. */
		opt_lenb = (s->opt_len+3+7)>>3;
		static_lenb = (s->static_len+3+7)>>3;

		Tracev((stderr, "\nopt %lu(%lu) stat %lu(%lu) stored %lu lit %u ",
				opt_lenb, s->opt_len, static_lenb, s->static_len, stored_len,
				s->last_lit));

		if (static_lenb <= opt_lenb) opt_lenb = static_lenb;

	} else {
		Assert(buf != (char*)0, "lost buf");
		opt_lenb = static_lenb = stored_len + 5; /* force a stored block */
	}

#ifdef FORCE_STORED
	if (buf != (char*)0) { /* force stored block */
#else
	if (stored_len+4 <= opt_lenb && buf != (char*)0) {
					   /* 4: two words for the lengths */
#endif
		/* The test buf != NULL is only necessary if LIT_BUFSIZE > WSIZE.
		 * Otherwise we can't have processed more than WSIZE input bytes since
		 * the last block flush, because compression would have been
		 * successful. If LIT_BUFSIZE <= WSIZE, it is never too late to
		 * transform a block into a stored block.
		 */
		_tr_stored_block(s, buf, stored_len, eof);

#ifdef FORCE_STATIC
	} else if (static_lenb >= 0) { /* force static trees */
#else
	} else if (s->strategy == Z_FIXED || static_lenb == opt_lenb) {
#endif
		send_bits(s, (STATIC_TREES<<1)+eof, 3);
		compress_block(s, (ct_data *)static_ltree, (ct_data *)static_dtree);
#ifdef DEBUG
		s->compressed_len += 3 + s->static_len;
#endif
	} else {
		send_bits(s, (DYN_TREES<<1)+eof, 3);
		send_all_trees(s, s->l_desc.max_code+1, s->d_desc.max_code+1,
					   max_blindex+1);
		compress_block(s, (ct_data *)s->dyn_ltree, (ct_data *)s->dyn_dtree);
#ifdef DEBUG
		s->compressed_len += 3 + s->opt_len;
#endif
	}
	Assert (s->compressed_len == s->bits_sent, "bad compressed size");
	/* The above check is made mod 2^32, for files larger than 512 MB
	 * and uLong implemented on 32 bits.
	 */
	init_block(s);

	if (eof) {
		bi_windup(s);
#ifdef DEBUG
		s->compressed_len += 7;  /* align on byte boundary */
#endif
	}
	Tracev((stderr,"\ncomprlen %lu(%lu) ", s->compressed_len>>3,
		   s->compressed_len-7*eof));
}

/* ===========================================================================
 * Save the match info and tally the frequency counts. Return true if
 * the current block must be flushed.
 */
int _tr_tally (deflate_state *s,
			   unsigned dist,  /* distance of matched string */
			   unsigned lc)    /* match length-MIN_MATCH or unmatched char (if dist==0) */
{
	s->d_buf[s->last_lit] = (ush)dist;
	s->l_buf[s->last_lit++] = (uch)lc;
	if (dist == 0) {
		/* lc is the unmatched char */
		s->dyn_ltree[lc].Freq++;
	} else {
		s->matches++;
		/* Here, lc is the match length - MIN_MATCH */
		dist--;             /* dist = match distance - 1 */
		Assert((ush)dist < (ush)MAX_DIST(s) &&
			   (ush)lc <= (ush)(MAX_MATCH-MIN_MATCH) &&
			   (ush)d_code(dist) < (ush)D_CODES,  "_tr_tally: bad match");

		s->dyn_ltree[_length_code[lc]+LITERALS+1].Freq++;
		s->dyn_dtree[d_code(dist)].Freq++;
	}

#ifdef TRUNCATE_BLOCK
	/* Try to guess if it is profitable to stop the current block here */
	if ((s->last_lit & 0x1fff) == 0 && s->level > 2) {
		/* Compute an upper bound for the compressed length */
		ulg out_length = (ulg)s->last_lit*8L;
		ulg in_length = (ulg)((long)s->strstart - s->block_start);
		int dcode;
		for (dcode = 0; dcode < D_CODES; dcode++) {
			out_length += (ulg)s->dyn_dtree[dcode].Freq *
				(5L+extra_dbits[dcode]);
		}
		out_length >>= 3;
		Tracev((stderr,"\nlast_lit %u, in %ld, out ~%ld(%ld%%) ",
			   s->last_lit, in_length, out_length,
			   100L - out_length*100L/in_length));
		if (s->matches < s->last_lit/2 && out_length < in_length/2) return 1;
	}
#endif
	return (s->last_lit == s->lit_bufsize-1);
	/* We avoid equality with lit_bufsize because of wraparound at 64K
	 * on 16 bit machines and because stored blocks are restricted to
	 * 64K-1 bytes.
	 */
}

/* ===========================================================================
 * Send the block data compressed using the given Huffman trees
 */
local void compress_block (deflate_state *s,
						   ct_data *ltree, /* literal tree */
						   ct_data *dtree) /* distance tree */
{
	unsigned dist;      /* distance of matched string */
	int lc;             /* match length or unmatched char (if dist == 0) */
	unsigned lx = 0;    /* running index in l_buf */
	unsigned code;      /* the code to send */
	int extra;          /* number of extra bits to send */

	if (s->last_lit != 0) do {
		dist = s->d_buf[lx];
		lc = s->l_buf[lx++];
		if (dist == 0) {
			send_code(s, lc, ltree); /* send a literal byte */
			Tracecv(isgraph(lc), (stderr," '%c' ", lc));
		} else {
			/* Here, lc is the match length - MIN_MATCH */
			code = _length_code[lc];
			send_code(s, code+LITERALS+1, ltree); /* send the length code */
			extra = extra_lbits[code];
			if (extra != 0) {
				lc -= base_length[code];
				send_bits(s, lc, extra);       /* send the extra length bits */
			}
			dist--; /* dist is now the match distance - 1 */
			code = d_code(dist);
			Assert (code < D_CODES, "bad d_code");

			send_code(s, code, dtree);       /* send the distance code */
			extra = extra_dbits[code];
			if (extra != 0) {
				dist -= base_dist[code];
				send_bits(s, dist, extra);   /* send the extra distance bits */
			}
		} /* literal or match pair ? */

		/* Check that the overlay between pending_buf and d_buf+l_buf is ok: */
		Assert((uInt)(s->pending) < s->lit_bufsize + 2*lx,
			   "pendingBuf overflow");

	} while (lx < s->last_lit);

	send_code(s, END_BLOCK, ltree);
	s->last_eob_len = ltree[END_BLOCK].Len;
}

/* ===========================================================================
 * Set the data type to BINARY or TEXT, using a crude approximation:
 * set it to Z_TEXT if all symbols are either printable characters (33 to 255)
 * or white spaces (9 to 13, or 32); or set it to Z_BINARY otherwise.
 * IN assertion: the fields Freq of dyn_ltree are set.
 */
local void set_data_type (deflate_state *s)
{
	int n;

	for (n = 0; n < 9; n++)
		if (s->dyn_ltree[n].Freq != 0)
			break;
	if (n == 9)
		for (n = 14; n < 32; n++)
			if (s->dyn_ltree[n].Freq != 0)
				break;
	s->strm->data_type = (n == 32) ? Z_TEXT : Z_BINARY;
}

/* ===========================================================================
 * Reverse the first len bits of a code, using straightforward code (a faster
 * method would use a table)
 * IN assertion: 1 <= len <= 15
 */
local unsigned bi_reverse (unsigned code, int len)
{
	register unsigned res = 0;
	do {
		res |= code & 1;
		code >>= 1, res <<= 1;
	} while (--len > 0);
	return res >> 1;
}

/* ===========================================================================
 * Flush the bit buffer, keeping at most 7 bits in it.
 */
local void bi_flush (deflate_state *s)
{
	if (s->bi_valid == 16) {
		put_short(s, s->bi_buf);
		s->bi_buf = 0;
		s->bi_valid = 0;
	} else if (s->bi_valid >= 8) {
		put_byte(s, (Byte)s->bi_buf);
		s->bi_buf >>= 8;
		s->bi_valid -= 8;
	}
}

/* ===========================================================================
 * Flush the bit buffer and align the output on a byte boundary
 */
local void bi_windup (deflate_state *s)
{
	if (s->bi_valid > 8) {
		put_short(s, s->bi_buf);
	} else if (s->bi_valid > 0) {
		put_byte(s, (Byte)s->bi_buf);
	}
	s->bi_buf = 0;
	s->bi_valid = 0;
#ifdef DEBUG
	s->bits_sent = (s->bits_sent+7) & ~7;
#endif
}

/* ===========================================================================
 * Copy a stored block, storing first the length and its
 * one's complement if requested.
 */
local void copy_block(deflate_state *s,
					  charf    *buf,    /* the input data */
					  unsigned len,     /* its length */
					  int      header)  /* true if block header must be written */
{
	bi_windup(s);        /* align on byte boundary */
	s->last_eob_len = 8; /* enough lookahead for inflate */

	if (header) {
		put_short(s, (ush)len);
		put_short(s, (ush)~len);
#ifdef DEBUG
		s->bits_sent += 2*16;
#endif
	}
#ifdef DEBUG
	s->bits_sent += (ulg)len<<3;
#endif
	while (len--) {
		put_byte(s, *buf++);
	}
}

/*** End of inlined file: trees.c ***/


/*** Start of inlined file: zutil.c ***/
/* @(#) $Id: zutil.c,v 1.1 2007/06/07 17:54:37 jules_rms Exp $ */

#ifndef NO_DUMMY_DECL
struct internal_state      {int dummy;}; /* for buggy compilers */
#endif

const char * const z_errmsg[10] = {
"need dictionary",     /* Z_NEED_DICT       2  */
"stream end",          /* Z_STREAM_END      1  */
"",                    /* Z_OK              0  */
"file error",          /* Z_ERRNO         (-1) */
"stream error",        /* Z_STREAM_ERROR  (-2) */
"data error",          /* Z_DATA_ERROR    (-3) */
"insufficient memory", /* Z_MEM_ERROR     (-4) */
"buffer error",        /* Z_BUF_ERROR     (-5) */
"incompatible version",/* Z_VERSION_ERROR (-6) */
""};

/*const char * ZEXPORT zlibVersion()
{
	return ZLIB_VERSION;
}

uLong ZEXPORT zlibCompileFlags()
{
	uLong flags;

	flags = 0;
	switch (sizeof(uInt)) {
	case 2:     break;
	case 4:     flags += 1;     break;
	case 8:     flags += 2;     break;
	default:    flags += 3;
	}
	switch (sizeof(uLong)) {
	case 2:     break;
	case 4:     flags += 1 << 2;        break;
	case 8:     flags += 2 << 2;        break;
	default:    flags += 3 << 2;
	}
	switch (sizeof(voidpf)) {
	case 2:     break;
	case 4:     flags += 1 << 4;        break;
	case 8:     flags += 2 << 4;        break;
	default:    flags += 3 << 4;
	}
	switch (sizeof(z_off_t)) {
	case 2:     break;
	case 4:     flags += 1 << 6;        break;
	case 8:     flags += 2 << 6;        break;
	default:    flags += 3 << 6;
	}
#ifdef DEBUG
	flags += 1 << 8;
#endif
#if defined(ASMV) || defined(ASMINF)
	flags += 1 << 9;
#endif
#ifdef ZLIB_WINAPI
	flags += 1 << 10;
#endif
#ifdef BUILDFIXED
	flags += 1 << 12;
#endif
#ifdef DYNAMIC_CRC_TABLE
	flags += 1 << 13;
#endif
#ifdef NO_GZCOMPRESS
	flags += 1L << 16;
#endif
#ifdef NO_GZIP
	flags += 1L << 17;
#endif
#ifdef PKZIP_BUG_WORKAROUND
	flags += 1L << 20;
#endif
#ifdef FASTEST
	flags += 1L << 21;
#endif
#ifdef STDC
#  ifdef NO_vsnprintf
		flags += 1L << 25;
#    ifdef HAS_vsprintf_void
		flags += 1L << 26;
#    endif
#  else
#    ifdef HAS_vsnprintf_void
		flags += 1L << 26;
#    endif
#  endif
#else
		flags += 1L << 24;
#  ifdef NO_snprintf
		flags += 1L << 25;
#    ifdef HAS_sprintf_void
		flags += 1L << 26;
#    endif
#  else
#    ifdef HAS_snprintf_void
		flags += 1L << 26;
#    endif
#  endif
#endif
	return flags;
}*/

#if 0

#  ifndef verbose
#    define verbose 0
#  endif
int z_verbose = verbose;

void z_error (const char *m)
{
	fprintf(stderr, "%s\n", m);
	exit(1);
}
#endif

/* exported to allow conversion of error code to string for compress() and
 * uncompress()
 */
const char * ZEXPORT zError(int err)
{
	return ERR_MSG(err);
}

#if defined(_WIN32_WCE)
	/* The Microsoft C Run-Time Library for Windows CE doesn't have
	 * errno.  We define it as a global variable to simplify porting.
	 * Its value is always 0 and should not be used.
	 */
	int errno = 0;
#endif

#ifndef HAVE_MEMCPY

void zmemcpy(dest, source, len)
	Bytef* dest;
	const Bytef* source;
	uInt  len;
{
	if (len == 0) return;
	do {
		*dest++ = *source++; /* ??? to be unrolled */
	} while (--len != 0);
}

int zmemcmp(s1, s2, len)
	const Bytef* s1;
	const Bytef* s2;
	uInt  len;
{
	uInt j;

	for (j = 0; j < len; j++) {
		if (s1[j] != s2[j]) return 2*(s1[j] > s2[j])-1;
	}
	return 0;
}

void zmemzero(dest, len)
	Bytef* dest;
	uInt  len;
{
	if (len == 0) return;
	do {
		*dest++ = 0;  /* ??? to be unrolled */
	} while (--len != 0);
}
#endif

#ifdef SYS16BIT

#ifdef __TURBOC__
/* Turbo C in 16-bit mode */

#  define MY_ZCALLOC

/* Turbo C malloc() does not allow dynamic allocation of 64K bytes
 * and farmalloc(64K) returns a pointer with an offset of 8, so we
 * must fix the pointer. Warning: the pointer must be put back to its
 * original form in order to free it, use zcfree().
 */

#define MAX_PTR 10
/* 10*64K = 640K */

local int next_ptr = 0;

typedef struct ptr_table_s {
	voidpf org_ptr;
	voidpf new_ptr;
} ptr_table;

local ptr_table table[MAX_PTR];
/* This table is used to remember the original form of pointers
 * to large buffers (64K). Such pointers are normalized with a zero offset.
 * Since MSDOS is not a preemptive multitasking OS, this table is not
 * protected from concurrent access. This hack doesn't work anyway on
 * a protected system like OS/2. Use Microsoft C instead.
 */

voidpf zcalloc (voidpf opaque, unsigned items, unsigned size)
{
	voidpf buf = opaque; /* just to make some compilers happy */
	ulg bsize = (ulg)items*size;

	/* If we allocate less than 65520 bytes, we assume that farmalloc
	 * will return a usable pointer which doesn't have to be normalized.
	 */
	if (bsize < 65520L) {
		buf = farmalloc(bsize);
		if (*(ush*)&buf != 0) return buf;
	} else {
		buf = farmalloc(bsize + 16L);
	}
	if (buf == NULL || next_ptr >= MAX_PTR) return NULL;
	table[next_ptr].org_ptr = buf;

	/* Normalize the pointer to seg:0 */
	*((ush*)&buf+1) += ((ush)((uch*)buf-0) + 15) >> 4;
	*(ush*)&buf = 0;
	table[next_ptr++].new_ptr = buf;
	return buf;
}

void  zcfree (voidpf opaque, voidpf ptr)
{
	int n;
	if (*(ush*)&ptr != 0) { /* object < 64K */
		farfree(ptr);
		return;
	}
	/* Find the original pointer */
	for (n = 0; n < next_ptr; n++) {
		if (ptr != table[n].new_ptr) continue;

		farfree(table[n].org_ptr);
		while (++n < next_ptr) {
			table[n-1] = table[n];
		}
		next_ptr--;
		return;
	}
	ptr = opaque; /* just to make some compilers happy */
	Assert(0, "zcfree: ptr not found");
}

#endif /* __TURBOC__ */

#ifdef M_I86
/* Microsoft C in 16-bit mode */

#  define MY_ZCALLOC

#if (!defined(_MSC_VER) || (_MSC_VER <= 600))
#  define _halloc  halloc
#  define _hfree   hfree
#endif

voidpf zcalloc (voidpf opaque, unsigned items, unsigned size)
{
	if (opaque) opaque = 0; /* to make compiler happy */
	return _halloc((long)items, size);
}

void  zcfree (voidpf opaque, voidpf ptr)
{
	if (opaque) opaque = 0; /* to make compiler happy */
	_hfree(ptr);
}

#endif /* M_I86 */

#endif /* SYS16BIT */

#ifndef MY_ZCALLOC /* Any system without a special alloc function */

#ifndef STDC
extern voidp  malloc OF((uInt size));
extern voidp  calloc OF((uInt items, uInt size));
extern void   free   OF((voidpf ptr));
#endif

voidpf zcalloc (voidpf opaque, unsigned items, unsigned size)
{
	if (opaque) items += size - size; /* make compiler happy */
	return sizeof(uInt) > 2 ? (voidpf)malloc(items * size) :
							  (voidpf)calloc(items, size);
}

void  zcfree (voidpf opaque, voidpf ptr)
{
	free(ptr);
	if (opaque) return; /* make compiler happy */
}

#endif /* MY_ZCALLOC */

/*** End of inlined file: zutil.c ***/

  #undef Byte
 #else
  #include JUCE_ZLIB_INCLUDE_PATH
 #endif
}

#if JUCE_MSVC
 #pragma warning (pop)
#endif

// internal helper object that holds the zlib structures so they don't have to be
// included publicly.
class GZIPDecompressorInputStream::GZIPDecompressHelper
{
public:
	GZIPDecompressHelper (const bool noWrap)
		: finished (true),
		  needsDictionary (false),
		  error (true),
		  streamIsValid (false),
		  data (nullptr),
		  dataSize (0)
	{
		using namespace zlibNamespace;
		zerostruct (stream);
		streamIsValid = (inflateInit2 (&stream, noWrap ? -MAX_WBITS : MAX_WBITS) == Z_OK);
		finished = error = ! streamIsValid;
	}

	~GZIPDecompressHelper()
	{
		using namespace zlibNamespace;
		if (streamIsValid)
			inflateEnd (&stream);
	}

	bool needsInput() const noexcept        { return dataSize <= 0; }

	void setInput (uint8* const data_, const size_t size) noexcept
	{
		data = data_;
		dataSize = size;
	}

	int doNextBlock (uint8* const dest, const int destSize)
	{
		using namespace zlibNamespace;
		if (streamIsValid && data != nullptr && ! finished)
		{
			stream.next_in  = data;
			stream.next_out = dest;
			stream.avail_in  = (z_uInt) dataSize;
			stream.avail_out = (z_uInt) destSize;

			switch (inflate (&stream, Z_PARTIAL_FLUSH))
			{
			case Z_STREAM_END:
				finished = true;
				// deliberate fall-through
			case Z_OK:
				data += dataSize - stream.avail_in;
				dataSize = (z_uInt) stream.avail_in;
				return (int) (destSize - stream.avail_out);

			case Z_NEED_DICT:
				needsDictionary = true;
				data += dataSize - stream.avail_in;
				dataSize = (size_t) stream.avail_in;
				break;

			case Z_DATA_ERROR:
			case Z_MEM_ERROR:
				error = true;

			default:
				break;
			}
		}

		return 0;
	}

	bool finished, needsDictionary, error, streamIsValid;

	enum { gzipDecompBufferSize = 32768 };

private:
	zlibNamespace::z_stream stream;
	uint8* data;
	size_t dataSize;

	JUCE_DECLARE_NON_COPYABLE (GZIPDecompressHelper);
};

GZIPDecompressorInputStream::GZIPDecompressorInputStream (InputStream* const sourceStream_,
														  const bool deleteSourceWhenDestroyed,
														  const bool noWrap_,
														  const int64 uncompressedStreamLength_)
  : sourceStream (sourceStream_, deleteSourceWhenDestroyed),
	uncompressedStreamLength (uncompressedStreamLength_),
	noWrap (noWrap_),
	isEof (false),
	activeBufferSize (0),
	originalSourcePos (sourceStream_->getPosition()),
	currentPos (0),
	buffer ((size_t) GZIPDecompressHelper::gzipDecompBufferSize),
	helper (new GZIPDecompressHelper (noWrap_))
{
}

GZIPDecompressorInputStream::GZIPDecompressorInputStream (InputStream& sourceStream_)
  : sourceStream (&sourceStream_, false),
	uncompressedStreamLength (-1),
	noWrap (false),
	isEof (false),
	activeBufferSize (0),
	originalSourcePos (sourceStream_.getPosition()),
	currentPos (0),
	buffer ((size_t) GZIPDecompressHelper::gzipDecompBufferSize),
	helper (new GZIPDecompressHelper (false))
{
}

GZIPDecompressorInputStream::~GZIPDecompressorInputStream()
{
}

int64 GZIPDecompressorInputStream::getTotalLength()
{
	return uncompressedStreamLength;
}

int GZIPDecompressorInputStream::read (void* destBuffer, int howMany)
{
	jassert (destBuffer != nullptr && howMany >= 0);

	if (howMany > 0 && ! isEof)
	{
		int numRead = 0;
		uint8* d = static_cast <uint8*> (destBuffer);

		while (! helper->error)
		{
			const int n = helper->doNextBlock (d, howMany);
			currentPos += n;

			if (n == 0)
			{
				if (helper->finished || helper->needsDictionary)
				{
					isEof = true;
					return numRead;
				}

				if (helper->needsInput())
				{
					activeBufferSize = sourceStream->read (buffer, (int) GZIPDecompressHelper::gzipDecompBufferSize);

					if (activeBufferSize > 0)
					{
						helper->setInput (buffer, (size_t) activeBufferSize);
					}
					else
					{
						isEof = true;
						return numRead;
					}
				}
			}
			else
			{
				numRead += n;
				howMany -= n;
				d += n;

				if (howMany <= 0)
					return numRead;
			}
		}
	}

	return 0;
}

bool GZIPDecompressorInputStream::isExhausted()
{
	return helper->error || isEof;
}

int64 GZIPDecompressorInputStream::getPosition()
{
	return currentPos;
}

bool GZIPDecompressorInputStream::setPosition (int64 newPos)
{
	if (newPos < currentPos)
	{
		// to go backwards, reset the stream and start again..
		isEof = false;
		activeBufferSize = 0;
		currentPos = 0;
		helper = new GZIPDecompressHelper (noWrap);

		sourceStream->setPosition (originalSourcePos);
	}

	skipNextBytes (newPos - currentPos);
	return true;
}

// (This is used as a way for the zip file code to use the crc32 function without including zlib)
unsigned long juce_crc32 (unsigned long, const unsigned char*, unsigned);
unsigned long juce_crc32 (unsigned long crc, const unsigned char* buf, unsigned len)
{
	return zlibNamespace::crc32 (crc, buf, len);
}

/*** End of inlined file: juce_GZIPDecompressorInputStream.cpp ***/


/*** Start of inlined file: juce_GZIPCompressorOutputStream.cpp ***/
class GZIPCompressorOutputStream::GZIPCompressorHelper
{
public:
	GZIPCompressorHelper (const int compressionLevel, const int windowBits)
		: compLevel ((compressionLevel < 1 || compressionLevel > 9) ? -1 : compressionLevel),
		  isFirstDeflate (true),
		  streamIsValid (false),
		  finished (false)
	{
		using namespace zlibNamespace;
		zerostruct (stream);

		streamIsValid = (deflateInit2 (&stream, compLevel, Z_DEFLATED,
									   windowBits != 0 ? windowBits : MAX_WBITS,
									   8, strategy) == Z_OK);
	}

	~GZIPCompressorHelper()
	{
		if (streamIsValid)
			zlibNamespace::deflateEnd (&stream);
	}

	bool write (const uint8* data, int dataSize, OutputStream& destStream)
	{
		// When you call flush() on a gzip stream, the stream is closed, and you can
		// no longer continue to write data to it!
		jassert (! finished);

		while (dataSize > 0)
			if (! doNextBlock (data, dataSize, destStream, Z_NO_FLUSH))
				return false;

		return true;
	}

	void finish (OutputStream& destStream)
	{
		const uint8* data = nullptr;
		int dataSize = 0;

		while (! finished)
			doNextBlock (data, dataSize, destStream, Z_FINISH);
	}

private:
	enum { strategy = 0 };

	zlibNamespace::z_stream stream;
	const int compLevel;
	bool isFirstDeflate, streamIsValid, finished;
	zlibNamespace::Bytef buffer[32768];

	bool doNextBlock (const uint8*& data, int& dataSize, OutputStream& destStream, const int flushMode)
	{
		using namespace zlibNamespace;
		if (streamIsValid)
		{
			stream.next_in   = const_cast <uint8*> (data);
			stream.next_out  = buffer;
			stream.avail_in  = (z_uInt) dataSize;
			stream.avail_out = (z_uInt) sizeof (buffer);

			const int result = isFirstDeflate ? deflateParams (&stream, compLevel, strategy)
											  : deflate (&stream, flushMode);
			isFirstDeflate = false;

			switch (result)
			{
				case Z_STREAM_END:
					finished = true;
					// Deliberate fall-through..
				case Z_OK:
				{
					data += dataSize - stream.avail_in;
					dataSize = (int) stream.avail_in;
					const int bytesDone = ((int) sizeof (buffer)) - (int) stream.avail_out;
					return bytesDone <= 0 || destStream.write (buffer, bytesDone);
				}

				default:
					break;
			}
		}

		return false;
	}

	JUCE_DECLARE_NON_COPYABLE (GZIPCompressorHelper);
};

GZIPCompressorOutputStream::GZIPCompressorOutputStream (OutputStream* const destStream_,
														const int compressionLevel,
														const bool deleteDestStream,
														const int windowBits)
	: destStream (destStream_, deleteDestStream),
	  helper (new GZIPCompressorHelper (compressionLevel, windowBits))
{
	jassert (destStream_ != nullptr);
}

GZIPCompressorOutputStream::~GZIPCompressorOutputStream()
{
	flush();
}

void GZIPCompressorOutputStream::flush()
{
	helper->finish (*destStream);
	destStream->flush();
}

bool GZIPCompressorOutputStream::write (const void* destBuffer, int howMany)
{
	jassert (destBuffer != nullptr && howMany >= 0);

	return helper->write (static_cast <const uint8*> (destBuffer), howMany, *destStream);
}

int64 GZIPCompressorOutputStream::getPosition()
{
	return destStream->getPosition();
}

bool GZIPCompressorOutputStream::setPosition (int64 /*newPosition*/)
{
	jassertfalse; // can't do it!
	return false;
}

#if JUCE_UNIT_TESTS

class GZIPTests  : public UnitTest
{
public:
	GZIPTests()   : UnitTest ("GZIP") {}

	void runTest()
	{
		beginTest ("GZIP");
		Random rng;

		for (int i = 100; --i >= 0;)
		{
			MemoryOutputStream original, compressed, uncompressed;

			{
				GZIPCompressorOutputStream zipper (&compressed, rng.nextInt (10), false);

				for (int j = rng.nextInt (100); --j >= 0;)
				{
					MemoryBlock data (rng.nextInt (2000) + 1);

					for (int k = (int) data.getSize(); --k >= 0;)
						data[k] = (char) rng.nextInt (255);

					original.write (data.getData(), (int) data.getSize());
					zipper  .write (data.getData(), (int) data.getSize());
				}
			}

			{
				MemoryInputStream compressedInput (compressed.getData(), compressed.getDataSize(), false);
				GZIPDecompressorInputStream unzipper (compressedInput);

				uncompressed << unzipper;
			}

			expectEquals ((int) uncompressed.getDataSize(),
						  (int) original.getDataSize());

			if (original.getDataSize() == uncompressed.getDataSize())
				expect (memcmp (uncompressed.getData(),
								original.getData(),
								original.getDataSize()) == 0);
		}
	}
};

static GZIPTests gzipTests;

#endif

/*** End of inlined file: juce_GZIPCompressorOutputStream.cpp ***/


/*** Start of inlined file: juce_ZipFile.cpp ***/
class ZipFile::ZipEntryHolder
{
public:
	ZipEntryHolder (const char* const buffer, const int fileNameLen)
	{
		entry.filename = String::fromUTF8 (buffer + 46, fileNameLen);

		const int time = ByteOrder::littleEndianShort (buffer + 12);
		const int date = ByteOrder::littleEndianShort (buffer + 14);
		entry.fileTime = getFileTimeFromRawEncodings (time, date);

		compressed = ByteOrder::littleEndianShort (buffer + 10) != 0;
		compressedSize = (size_t) ByteOrder::littleEndianInt (buffer + 20);
		entry.uncompressedSize = ByteOrder::littleEndianInt (buffer + 24);

		streamOffset = ByteOrder::littleEndianInt (buffer + 42);
	}

	struct FileNameComparator
	{
		static int compareElements (const ZipEntryHolder* first, const ZipEntryHolder* second)
		{
			return first->entry.filename.compare (second->entry.filename);
		}
	};

	ZipEntry entry;
	size_t streamOffset;
	size_t compressedSize;
	bool compressed;

private:
	static Time getFileTimeFromRawEncodings (int time, int date)
	{
		const int year      = 1980 + (date >> 9);
		const int month     = ((date >> 5) & 15) - 1;
		const int day       = date & 31;
		const int hours     = time >> 11;
		const int minutes   = (time >> 5) & 63;
		const int seconds   = (time & 31) << 1;

		return Time (year, month, day, hours, minutes, seconds);
	}
};

namespace
{
	int findEndOfZipEntryTable (InputStream& input, int& numEntries)
	{
		BufferedInputStream in (input, 8192);

		in.setPosition (in.getTotalLength());
		int64 pos = in.getPosition();
		const int64 lowestPos = jmax ((int64) 0, pos - 1024);

		char buffer [32] = { 0 };

		while (pos > lowestPos)
		{
			in.setPosition (pos - 22);
			pos = in.getPosition();
			memcpy (buffer + 22, buffer, 4);

			if (in.read (buffer, 22) != 22)
				return 0;

			for (int i = 0; i < 22; ++i)
			{
				if (ByteOrder::littleEndianInt (buffer + i) == 0x06054b50)
				{
					in.setPosition (pos + i);
					in.read (buffer, 22);
					numEntries = ByteOrder::littleEndianShort (buffer + 10);

					return (int) ByteOrder::littleEndianInt (buffer + 16);
				}
			}
		}

		return 0;
	}
}

class ZipFile::ZipInputStream  : public InputStream
{
public:
	ZipInputStream (ZipFile& file_, ZipFile::ZipEntryHolder& zei)
		: file (file_),
		  zipEntryHolder (zei),
		  pos (0),
		  headerSize (0),
		  inputStream (file_.inputStream)
	{
		if (file_.inputSource != nullptr)
		{
			inputStream = streamToDelete = file.inputSource->createInputStream();
		}
		else
		{
		   #if JUCE_DEBUG
			file_.streamCounter.numOpenStreams++;
		   #endif
		}

		char buffer [30];

		if (inputStream != nullptr
			 && inputStream->setPosition (zei.streamOffset)
			 && inputStream->read (buffer, 30) == 30
			 && ByteOrder::littleEndianInt (buffer) == 0x04034b50)
		{
			headerSize = 30 + ByteOrder::littleEndianShort (buffer + 26)
							+ ByteOrder::littleEndianShort (buffer + 28);
		}
	}

	~ZipInputStream()
	{
	   #if JUCE_DEBUG
		if (inputStream != nullptr && inputStream == file.inputStream)
			file.streamCounter.numOpenStreams--;
	   #endif
	}

	int64 getTotalLength()
	{
		return zipEntryHolder.compressedSize;
	}

	int read (void* buffer, int howMany)
	{
		if (headerSize <= 0)
			return 0;

		howMany = (int) jmin ((int64) howMany, (int64) (zipEntryHolder.compressedSize - pos));

		if (inputStream == nullptr)
			return 0;

		int num;

		if (inputStream == file.inputStream)
		{
			const ScopedLock sl (file.lock);
			inputStream->setPosition (pos + zipEntryHolder.streamOffset + headerSize);
			num = inputStream->read (buffer, howMany);
		}
		else
		{
			inputStream->setPosition (pos + zipEntryHolder.streamOffset + headerSize);
			num = inputStream->read (buffer, howMany);
		}

		pos += num;
		return num;
	}

	bool isExhausted()
	{
		return headerSize <= 0 || pos >= (int64) zipEntryHolder.compressedSize;
	}

	int64 getPosition()
	{
		return pos;
	}

	bool setPosition (int64 newPos)
	{
		pos = jlimit ((int64) 0, (int64) zipEntryHolder.compressedSize, newPos);
		return true;
	}

private:
	ZipFile& file;
	ZipEntryHolder zipEntryHolder;
	int64 pos;
	int headerSize;
	InputStream* inputStream;
	ScopedPointer<InputStream> streamToDelete;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ZipInputStream);
};

ZipFile::ZipFile (InputStream* const stream, const bool deleteStreamWhenDestroyed)
   : inputStream (stream)
{
	if (deleteStreamWhenDestroyed)
		streamToDelete = inputStream;

	init();
}

ZipFile::ZipFile (InputStream& stream)
   : inputStream (&stream)
{
	init();
}

ZipFile::ZipFile (const File& file)
	: inputStream (nullptr),
	  inputSource (new FileInputSource (file))
{
	init();
}

ZipFile::ZipFile (InputSource* const inputSource_)
	: inputStream (nullptr),
	  inputSource (inputSource_)
{
	init();
}

ZipFile::~ZipFile()
{
	entries.clear();
}

#if JUCE_DEBUG
ZipFile::OpenStreamCounter::~OpenStreamCounter()
{
	/* If you hit this assertion, it means you've created a stream to read one of the items in the
	   zipfile, but you've forgotten to delete that stream object before deleting the file..
	   Streams can't be kept open after the file is deleted because they need to share the input
	   stream that is managed by the ZipFile object.
	*/
	jassert (numOpenStreams == 0);
}
#endif

int ZipFile::getNumEntries() const noexcept
{
	return entries.size();
}

const ZipFile::ZipEntry* ZipFile::getEntry (const int index) const noexcept
{
	ZipEntryHolder* const zei = entries [index];
	return zei != nullptr ? &(zei->entry) : nullptr;
}

int ZipFile::getIndexOfFileName (const String& fileName) const noexcept
{
	for (int i = 0; i < entries.size(); ++i)
		if (entries.getUnchecked (i)->entry.filename == fileName)
			return i;

	return -1;
}

const ZipFile::ZipEntry* ZipFile::getEntry (const String& fileName) const noexcept
{
	return getEntry (getIndexOfFileName (fileName));
}

InputStream* ZipFile::createStreamForEntry (const int index)
{
	ZipEntryHolder* const zei = entries[index];
	InputStream* stream = nullptr;

	if (zei != nullptr)
	{
		stream = new ZipInputStream (*this, *zei);

		if (zei->compressed)
		{
			stream = new GZIPDecompressorInputStream (stream, true, true,
													  zei->entry.uncompressedSize);

			// (much faster to unzip in big blocks using a buffer..)
			stream = new BufferedInputStream (stream, 32768, true);
		}
	}

	return stream;
}

void ZipFile::sortEntriesByFilename()
{
	ZipEntryHolder::FileNameComparator sorter;
	entries.sort (sorter);
}

void ZipFile::init()
{
	ScopedPointer <InputStream> toDelete;
	InputStream* in = inputStream;

	if (inputSource != nullptr)
	{
		in = inputSource->createInputStream();
		toDelete = in;
	}

	if (in != nullptr)
	{
		int numEntries = 0;
		int pos = findEndOfZipEntryTable (*in, numEntries);

		if (pos >= 0 && pos < in->getTotalLength())
		{
			const int size = (int) (in->getTotalLength() - pos);

			in->setPosition (pos);
			MemoryBlock headerData;

			if (in->readIntoMemoryBlock (headerData, size) == size)
			{
				pos = 0;

				for (int i = 0; i < numEntries; ++i)
				{
					if (pos + 46 > size)
						break;

					const char* const buffer = static_cast <const char*> (headerData.getData()) + pos;

					const int fileNameLen = ByteOrder::littleEndianShort (buffer + 28);

					if (pos + 46 + fileNameLen > size)
						break;

					entries.add (new ZipEntryHolder (buffer, fileNameLen));

					pos += 46 + fileNameLen
							+ ByteOrder::littleEndianShort (buffer + 30)
							+ ByteOrder::littleEndianShort (buffer + 32);
				}
			}
		}
	}
}

Result ZipFile::uncompressTo (const File& targetDirectory,
							  const bool shouldOverwriteFiles)
{
	for (int i = 0; i < entries.size(); ++i)
	{
		Result result (uncompressEntry (i, targetDirectory, shouldOverwriteFiles));
		if (result.failed())
			return result;
	}

	return Result::ok();
}

Result ZipFile::uncompressEntry (const int index,
								 const File& targetDirectory,
								 bool shouldOverwriteFiles)
{
	const ZipEntryHolder* zei = entries.getUnchecked (index);

	const File targetFile (targetDirectory.getChildFile (zei->entry.filename));

	if (zei->entry.filename.endsWithChar ('/'))
		return targetFile.createDirectory(); // (entry is a directory, not a file)

	ScopedPointer<InputStream> in (createStreamForEntry (index));

	if (in == nullptr)
		return Result::fail ("Failed to open the zip file for reading");

	if (targetFile.exists())
	{
		if (! shouldOverwriteFiles)
			return Result::ok();

		if (! targetFile.deleteFile())
			return Result::fail ("Failed to write to target file: " + targetFile.getFullPathName());
	}

	if (! targetFile.getParentDirectory().createDirectory())
		return Result::fail ("Failed to create target folder: " + targetFile.getParentDirectory().getFullPathName());

	{
		FileOutputStream out (targetFile);

		if (out.failedToOpen())
			return Result::fail ("Failed to write to target file: " + targetFile.getFullPathName());

		out << *in;
	}

	targetFile.setCreationTime (zei->entry.fileTime);
	targetFile.setLastModificationTime (zei->entry.fileTime);
	targetFile.setLastAccessTime (zei->entry.fileTime);

	return Result::ok();
}

extern unsigned long juce_crc32 (unsigned long crc, const unsigned char* buf, unsigned len);

class ZipFile::Builder::Item
{
public:
	Item (const File& file_, const int compressionLevel_, const String& storedPathName_)
		: file (file_),
		  storedPathname (storedPathName_.isEmpty() ? file_.getFileName() : storedPathName_),
		  compressionLevel (compressionLevel_),
		  compressedSize (0),
		  headerStart (0)
	{
	}

	bool writeData (OutputStream& target, const int64 overallStartPosition)
	{
		MemoryOutputStream compressedData;

		if (compressionLevel > 0)
		{
			GZIPCompressorOutputStream compressor (&compressedData, compressionLevel, false,
												   GZIPCompressorOutputStream::windowBitsRaw);
			if (! writeSource (compressor))
				return false;
		}
		else
		{
			if (! writeSource (compressedData))
				return false;
		}

		compressedSize = (int) compressedData.getDataSize();
		headerStart = (int) (target.getPosition() - overallStartPosition);

		target.writeInt (0x04034b50);
		writeFlagsAndSizes (target);
		target << storedPathname
			   << compressedData;

		return true;
	}

	bool writeDirectoryEntry (OutputStream& target)
	{
		target.writeInt (0x02014b50);
		target.writeShort (20); // version written
		writeFlagsAndSizes (target);
		target.writeShort (0); // comment length
		target.writeShort (0); // start disk num
		target.writeShort (0); // internal attributes
		target.writeInt (0); // external attributes
		target.writeInt (headerStart);
		target << storedPathname;

		return true;
	}

private:
	const File file;
	String storedPathname;
	int compressionLevel, compressedSize, headerStart;
	unsigned long checksum;

	void writeTimeAndDate (OutputStream& target) const
	{
		const Time t (file.getLastModificationTime());
		target.writeShort ((short) (t.getSeconds() + (t.getMinutes() << 5) + (t.getHours() << 11)));
		target.writeShort ((short) (t.getDayOfMonth() + ((t.getMonth() + 1) << 5) + ((t.getYear() - 1980) << 9)));
	}

	bool writeSource (OutputStream& target)
	{
		checksum = 0;
		FileInputStream input (file);

		if (input.failedToOpen())
			return false;

		const int bufferSize = 2048;
		HeapBlock<unsigned char> buffer (bufferSize);

		while (! input.isExhausted())
		{
			const int bytesRead = input.read (buffer, bufferSize);

			if (bytesRead < 0)
				return false;

			checksum = juce_crc32 (checksum, buffer, (unsigned int) bytesRead);
			target.write (buffer, bytesRead);
		}

		return true;
	}

	void writeFlagsAndSizes (OutputStream& target) const
	{
		target.writeShort (10); // version needed
		target.writeShort (0); // flags
		target.writeShort (compressionLevel > 0 ? (short) 8 : (short) 0);
		writeTimeAndDate (target);
		target.writeInt ((int) checksum);
		target.writeInt (compressedSize);
		target.writeInt ((int) file.getSize());
		target.writeShort ((short) storedPathname.toUTF8().sizeInBytes() - 1);
		target.writeShort (0); // extra field length
	}

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Item);
};

ZipFile::Builder::Builder() {}
ZipFile::Builder::~Builder() {}

void ZipFile::Builder::addFile (const File& fileToAdd, const int compressionLevel, const String& storedPathName)
{
	items.add (new Item (fileToAdd, compressionLevel, storedPathName));
}

bool ZipFile::Builder::writeToStream (OutputStream& target, double* const progress) const
{
	const int64 fileStart = target.getPosition();

	for (int i = 0; i < items.size(); ++i)
	{
		if (progress != nullptr)
			*progress = (i + 0.5) / items.size();

		if (! items.getUnchecked (i)->writeData (target, fileStart))
			return false;
	}

	const int64 directoryStart = target.getPosition();

	for (int i = 0; i < items.size(); ++i)
		if (! items.getUnchecked (i)->writeDirectoryEntry (target))
			return false;

	const int64 directoryEnd = target.getPosition();

	target.writeInt (0x06054b50);
	target.writeShort (0);
	target.writeShort (0);
	target.writeShort ((short) items.size());
	target.writeShort ((short) items.size());
	target.writeInt ((int) (directoryEnd - directoryStart));
	target.writeInt ((int) (directoryStart - fileStart));
	target.writeShort (0);

	if (progress != nullptr)
		*progress = 1.0;

	return true;
}

/*** End of inlined file: juce_ZipFile.cpp ***/

// END_AUTOINCLUDE

#if JUCE_MAC || JUCE_IOS

/*** Start of inlined file: juce_osx_ObjCHelpers.h ***/
#ifndef __JUCE_OSX_OBJCHELPERS_JUCEHEADER__
#define __JUCE_OSX_OBJCHELPERS_JUCEHEADER__

/* This file contains a few helper functions that are used internally but which
   need to be kept away from the public headers because they use obj-C symbols.
*/
namespace
{

	String nsStringToJuce (NSString* s)
	{
		return CharPointer_UTF8 ([s UTF8String]);
	}

	NSString* juceStringToNS (const String& s)
	{
		return [NSString stringWithUTF8String: s.toUTF8()];
	}

	NSString* nsStringLiteral (const char* const s) noexcept
	{
		return [NSString stringWithUTF8String: s];
	}

	NSString* nsEmptyString() noexcept
	{
		return [NSString string];
	}
}

#endif   // __JUCE_OSX_OBJCHELPERS_JUCEHEADER__

/*** End of inlined file: juce_osx_ObjCHelpers.h ***/



/*** Start of inlined file: juce_mac_ObjCSuffix.h ***/
#ifndef __JUCE_MAC_OBJCSUFFIX_JUCEHEADER__
#define __JUCE_MAC_OBJCSUFFIX_JUCEHEADER__

/** This suffix is used for naming all Obj-C classes that are used inside juce.

	Because of the flat naming structure used by Obj-C, you can get horrible situations where
	two DLLs are loaded into a host, each of which uses classes with the same names, and these get
	cross-linked so that when you make a call to a class that you thought was private, it ends up
	actually calling into a similarly named class in the other module's address space.

	By changing this macro to a unique value, you ensure that all the obj-C classes in your app
	have unique names, and should avoid this problem.

	If you're using the amalgamated version, you can just set this macro to something unique before
	you include juce_amalgamated.cpp.
*/
#ifndef JUCE_ObjCExtraSuffix
 #define JUCE_ObjCExtraSuffix 3
#endif

#ifndef DOXYGEN
 #define appendMacro1(a, b, c, d, e) a ## _ ## b ## _ ## c ## _ ## d ## _ ## e
 #define appendMacro2(a, b, c, d, e) appendMacro1(a, b, c, d, e)
 #define MakeObjCClassName(rootName) appendMacro2 (rootName, JUCE_MAJOR_VERSION, JUCE_MINOR_VERSION, JUCE_BUILDNUMBER, JUCE_ObjCExtraSuffix)
#endif

#endif   // __JUCE_MAC_OBJCSUFFIX_JUCEHEADER__

/*** End of inlined file: juce_mac_ObjCSuffix.h ***/

#endif

#if JUCE_ANDROID

/*** Start of inlined file: juce_android_JNIHelpers.h ***/
#ifndef __JUCE_ANDROID_JNIHELPERS_JUCEHEADER__
#define __JUCE_ANDROID_JNIHELPERS_JUCEHEADER__

#if ! (defined (JUCE_ANDROID_ACTIVITY_CLASSNAME) && defined (JUCE_ANDROID_ACTIVITY_CLASSPATH))
 #error "The JUCE_ANDROID_ACTIVITY_CLASSNAME and JUCE_ANDROID_ACTIVITY_CLASSPATH macros must be set!"
#endif

extern JNIEnv* getEnv() noexcept;

class GlobalRef
{
public:
	inline GlobalRef() noexcept                 : obj (0) {}
	inline explicit GlobalRef (jobject obj_)    : obj (retain (obj_)) {}
	inline GlobalRef (const GlobalRef& other)   : obj (retain (other.obj)) {}
	~GlobalRef()                                { clear(); }

	inline void clear()
	{
		if (obj != 0)
		{
			getEnv()->DeleteGlobalRef (obj);
			obj = 0;
		}
	}

	inline GlobalRef& operator= (const GlobalRef& other)
	{
		jobject newObj = retain (other.obj);
		clear();
		obj = newObj;
		return *this;
	}

	inline operator jobject() const noexcept    { return obj; }
	inline jobject get() const noexcept         { return obj; }

	#define DECLARE_CALL_TYPE_METHOD(returnType, typeName) \
		returnType call##typeName##Method (jmethodID methodID, ... ) const \
		{ \
			va_list args; \
			va_start (args, methodID); \
			returnType result = getEnv()->Call##typeName##MethodV (obj, methodID, args); \
			va_end (args); \
			return result; \
		}

	DECLARE_CALL_TYPE_METHOD (jobject, Object)
	DECLARE_CALL_TYPE_METHOD (jboolean, Boolean)
	DECLARE_CALL_TYPE_METHOD (jbyte, Byte)
	DECLARE_CALL_TYPE_METHOD (jchar, Char)
	DECLARE_CALL_TYPE_METHOD (jshort, Short)
	DECLARE_CALL_TYPE_METHOD (jint, Int)
	DECLARE_CALL_TYPE_METHOD (jlong, Long)
	DECLARE_CALL_TYPE_METHOD (jfloat, Float)
	DECLARE_CALL_TYPE_METHOD (jdouble, Double)
	#undef DECLARE_CALL_TYPE_METHOD

	void callVoidMethod (jmethodID methodID, ... ) const
	{
		va_list args;
		va_start (args, methodID);
		getEnv()->CallVoidMethodV (obj, methodID, args);
		va_end (args);
	}

private:

	jobject obj;

	static inline jobject retain (jobject obj_)
	{
		return obj_ == 0 ? 0 : getEnv()->NewGlobalRef (obj_);
	}
};

template <typename JavaType>
class LocalRef
{
public:
	explicit inline LocalRef (JavaType obj_) noexcept   : obj (obj_) {}
	inline LocalRef (const LocalRef& other) noexcept    : obj (retain (other.obj)) {}
	~LocalRef()                                         { clear(); }

	void clear()
	{
		if (obj != 0)
			getEnv()->DeleteLocalRef (obj);
	}

	LocalRef& operator= (const LocalRef& other)
	{
		jobject newObj = retain (other.obj);
		clear();
		obj = newObj;
		return *this;
	}

	inline operator JavaType() const noexcept   { return obj; }
	inline JavaType get() const noexcept        { return obj; }

private:
	JavaType obj;

	static JavaType retain (JavaType obj_)
	{
		return obj_ == 0 ? 0 : (JavaType) getEnv()->NewLocalRef (obj_);
	}
};

namespace
{
	String juceString (JNIEnv* env, jstring s)
	{
		const char* const utf8 = env->GetStringUTFChars (s, nullptr);
		CharPointer_UTF8 utf8CP (utf8);
		const String result (utf8CP);
		env->ReleaseStringUTFChars (s, utf8);
		return result;
	}

	String juceString (jstring s)
	{
		return juceString (getEnv(), s);
	}

	LocalRef<jstring> javaString (const String& s)
	{
		return LocalRef<jstring> (getEnv()->NewStringUTF (s.toUTF8()));
	}

	LocalRef<jstring> javaStringFromChar (const juce_wchar c)
	{
		char utf8[8] = { 0 };
		CharPointer_UTF8 (utf8).write (c);
		return LocalRef<jstring> (getEnv()->NewStringUTF (utf8));
	}
}

class JNIClassBase
{
public:
	explicit JNIClassBase (const char* classPath_);
	virtual ~JNIClassBase();

	inline operator jclass() const noexcept { return classRef; }

	static void initialiseAllClasses (JNIEnv*);
	static void releaseAllClasses (JNIEnv*);

protected:
	virtual void initialiseFields (JNIEnv*) = 0;

	jmethodID resolveMethod (JNIEnv*, const char* methodName, const char* params);
	jmethodID resolveStaticMethod (JNIEnv*, const char* methodName, const char* params);
	jfieldID resolveField (JNIEnv*, const char* fieldName, const char* signature);
	jfieldID resolveStaticField (JNIEnv*, const char* fieldName, const char* signature);

private:
	const char* const classPath;
	jclass classRef;

	static Array<JNIClassBase*>& getClasses();
	void initialise (JNIEnv*);
	void release (JNIEnv*);

	JUCE_DECLARE_NON_COPYABLE (JNIClassBase);
};

#define CREATE_JNI_METHOD(methodID, stringName, params)         methodID = resolveMethod (env, stringName, params);
#define CREATE_JNI_STATICMETHOD(methodID, stringName, params)   methodID = resolveStaticMethod (env, stringName, params);
#define CREATE_JNI_FIELD(fieldID, stringName, signature)        fieldID  = resolveField (env, stringName, signature);
#define CREATE_JNI_STATICFIELD(fieldID, stringName, signature)  fieldID  = resolveStaticField (env, stringName, signature);
#define DECLARE_JNI_METHOD(methodID, stringName, params)        jmethodID methodID;
#define DECLARE_JNI_FIELD(fieldID, stringName, signature)       jfieldID  fieldID;

#define DECLARE_JNI_CLASS(CppClassName, javaPath) \
	class CppClassName ## _Class   : public JNIClassBase \
	{ \
	public: \
		CppClassName ## _Class() : JNIClassBase (javaPath) {} \
	\
		void initialiseFields (JNIEnv* env) \
		{ \
			JNI_CLASS_MEMBERS (CREATE_JNI_METHOD, CREATE_JNI_STATICMETHOD, CREATE_JNI_FIELD, CREATE_JNI_STATICFIELD); \
		} \
	\
		JNI_CLASS_MEMBERS (DECLARE_JNI_METHOD, DECLARE_JNI_METHOD, DECLARE_JNI_FIELD, DECLARE_JNI_FIELD); \
	}; \
	static CppClassName ## _Class CppClassName;

#define JUCE_JNI_CALLBACK(className, methodName, returnType, params) \
  extern "C" __attribute__ ((visibility("default"))) returnType JUCE_JOIN_MACRO (JUCE_JOIN_MACRO (Java_, className), _ ## methodName) params

class AndroidSystem
{
public:
	AndroidSystem();

	void initialise (JNIEnv*, jobject activity, jstring appFile, jstring appDataDir);
	void shutdown (JNIEnv*);

	GlobalRef activity;
	String appFile, appDataDir;
	int screenWidth, screenHeight;
};

extern AndroidSystem android;

class ThreadLocalJNIEnvHolder
{
public:
	ThreadLocalJNIEnvHolder()
		: jvm (nullptr)
	{
		zeromem (threads, sizeof (threads));
		zeromem (envs, sizeof (envs));
	}

	void initialise (JNIEnv* env)
	{
		// NB: the DLL can be left loaded by the JVM, so the same static
		// objects can end up being reused by subsequent runs of the app
		zeromem (threads, sizeof (threads));
		zeromem (envs, sizeof (envs));

		env->GetJavaVM (&jvm);
		addEnv (env);
	}

	JNIEnv* attach()
	{
		JNIEnv* env = nullptr;
		jvm->AttachCurrentThread (&env, nullptr);

		if (env != nullptr)
			addEnv (env);

		return env;
	}

	void detach()
	{
		jvm->DetachCurrentThread();

		const pthread_t thisThread = pthread_self();

		SpinLock::ScopedLockType sl (addRemoveLock);
		for (int i = 0; i < maxThreads; ++i)
			if (threads[i] == thisThread)
				threads[i] = 0;
	}

	JNIEnv* getOrAttach() noexcept
	{
		JNIEnv* env = get();

		if (env == nullptr)
			env = attach();

		jassert (env != nullptr);
		return env;
	}

	JNIEnv* get() const noexcept
	{
		const pthread_t thisThread = pthread_self();

		for (int i = 0; i < maxThreads; ++i)
			if (threads[i] == thisThread)
				return envs[i];

		return nullptr;
	}

	enum { maxThreads = 32 };

private:
	JavaVM* jvm;
	pthread_t threads [maxThreads];
	JNIEnv* envs [maxThreads];
	SpinLock addRemoveLock;

	void addEnv (JNIEnv* env)
	{
		SpinLock::ScopedLockType sl (addRemoveLock);

		if (get() == nullptr)
		{
			const pthread_t thisThread = pthread_self();

			for (int i = 0; i < maxThreads; ++i)
			{
				if (threads[i] == 0)
				{
					envs[i] = env;
					threads[i] = thisThread;
					return;
				}
			}
		}

		jassertfalse; // too many threads!
	}
};

extern ThreadLocalJNIEnvHolder threadLocalJNIEnvHolder;

#define JNI_CLASS_MEMBERS(METHOD, STATICMETHOD, FIELD, STATICFIELD) \
 METHOD (createNewView,          "createNewView",        "(Z)L" JUCE_ANDROID_ACTIVITY_CLASSPATH "$ComponentPeerView;") \
 METHOD (deleteView,             "deleteView",           "(L" JUCE_ANDROID_ACTIVITY_CLASSPATH "$ComponentPeerView;)V") \
 METHOD (postMessage,            "postMessage",          "(J)V") \
 METHOD (finish,                 "finish",               "()V") \
 METHOD (getClipboardContent,    "getClipboardContent",  "()Ljava/lang/String;") \
 METHOD (setClipboardContent,    "setClipboardContent",  "(Ljava/lang/String;)V") \
 METHOD (excludeClipRegion,      "excludeClipRegion",    "(Landroid/graphics/Canvas;FFFF)V") \
 METHOD (renderGlyph,            "renderGlyph",          "(CLandroid/graphics/Paint;Landroid/graphics/Matrix;Landroid/graphics/Rect;)[I") \
 STATICMETHOD (createHTTPStream, "createHTTPStream",     "(Ljava/lang/String;Z[BLjava/lang/String;ILjava/lang/StringBuffer;)L" JUCE_ANDROID_ACTIVITY_CLASSPATH "$HTTPStream;") \
 METHOD (launchURL,              "launchURL",            "(Ljava/lang/String;)V") \
 METHOD (showMessageBox,         "showMessageBox",       "(Ljava/lang/String;Ljava/lang/String;J)V") \
 METHOD (showOkCancelBox,        "showOkCancelBox",      "(Ljava/lang/String;Ljava/lang/String;J)V") \
 METHOD (showYesNoCancelBox,     "showYesNoCancelBox",   "(Ljava/lang/String;Ljava/lang/String;J)V") \

DECLARE_JNI_CLASS (JuceAppActivity, JUCE_ANDROID_ACTIVITY_CLASSPATH);
#undef JNI_CLASS_MEMBERS

#define JNI_CLASS_MEMBERS(METHOD, STATICMETHOD, FIELD, STATICFIELD) \
 METHOD (constructor,   "<init>",           "(I)V") \
 METHOD (setColor,      "setColor",         "(I)V") \
 METHOD (setAlpha,      "setAlpha",         "(I)V") \
 METHOD (setTypeface,   "setTypeface",      "(Landroid/graphics/Typeface;)Landroid/graphics/Typeface;") \
 METHOD (ascent,        "ascent",           "()F") \
 METHOD (descent,       "descent",          "()F") \
 METHOD (setTextSize,   "setTextSize",      "(F)V") \
 METHOD (getTextWidths, "getTextWidths",    "(Ljava/lang/String;[F)I") \
 METHOD (setTextScaleX, "setTextScaleX",    "(F)V") \
 METHOD (getTextPath,   "getTextPath",      "(Ljava/lang/String;IIFFLandroid/graphics/Path;)V") \
 METHOD (setShader,     "setShader",        "(Landroid/graphics/Shader;)Landroid/graphics/Shader;") \

DECLARE_JNI_CLASS (Paint, "android/graphics/Paint");
#undef JNI_CLASS_MEMBERS

#define JNI_CLASS_MEMBERS(METHOD, STATICMETHOD, FIELD, STATICFIELD) \
 METHOD (constructor,   "<init>",    "()V") \
 METHOD (setValues,     "setValues", "([F)V") \

DECLARE_JNI_CLASS (Matrix, "android/graphics/Matrix");
#undef JNI_CLASS_MEMBERS

#define JNI_CLASS_MEMBERS(METHOD, STATICMETHOD, FIELD, STATICFIELD) \
 METHOD (constructor,   "<init>",   "(IIII)V") \
 FIELD (left,           "left",     "I") \
 FIELD (right,          "right",    "I") \
 FIELD (top,            "top",      "I") \
 FIELD (bottom,         "bottom",   "I") \

DECLARE_JNI_CLASS (RectClass, "android/graphics/Rect");
#undef JNI_CLASS_MEMBERS

#endif   // __JUCE_ANDROID_JNIHELPERS_JUCEHEADER__

/*** End of inlined file: juce_android_JNIHelpers.h ***/


#endif

#if ! JUCE_WINDOWS

/*** Start of inlined file: juce_posix_SharedCode.h ***/
/*
	This file contains posix routines that are common to both the Linux and Mac builds.

	It gets included directly in the cpp files for these platforms.
*/

CriticalSection::CriticalSection() noexcept
{
	pthread_mutexattr_t atts;
	pthread_mutexattr_init (&atts);
	pthread_mutexattr_settype (&atts, PTHREAD_MUTEX_RECURSIVE);
   #if ! JUCE_ANDROID
	pthread_mutexattr_setprotocol (&atts, PTHREAD_PRIO_INHERIT);
   #endif
	pthread_mutex_init (&internal, &atts);
}

CriticalSection::~CriticalSection() noexcept
{
	pthread_mutex_destroy (&internal);
}

void CriticalSection::enter() const noexcept
{
	pthread_mutex_lock (&internal);
}

bool CriticalSection::tryEnter() const noexcept
{
	return pthread_mutex_trylock (&internal) == 0;
}

void CriticalSection::exit() const noexcept
{
	pthread_mutex_unlock (&internal);
}

class WaitableEventImpl
{
public:
	WaitableEventImpl (const bool manualReset_)
		: triggered (false),
		  manualReset (manualReset_)
	{
		pthread_cond_init (&condition, 0);

		pthread_mutexattr_t atts;
		pthread_mutexattr_init (&atts);
	   #if ! JUCE_ANDROID
		pthread_mutexattr_setprotocol (&atts, PTHREAD_PRIO_INHERIT);
	   #endif
		pthread_mutex_init (&mutex, &atts);
	}

	~WaitableEventImpl()
	{
		pthread_cond_destroy (&condition);
		pthread_mutex_destroy (&mutex);
	}

	bool wait (const int timeOutMillisecs) noexcept
	{
		pthread_mutex_lock (&mutex);

		if (! triggered)
		{
			if (timeOutMillisecs < 0)
			{
				do
				{
					pthread_cond_wait (&condition, &mutex);
				}
				while (! triggered);
			}
			else
			{
				struct timeval now;
				gettimeofday (&now, 0);

				struct timespec time;
				time.tv_sec  = now.tv_sec  + (timeOutMillisecs / 1000);
				time.tv_nsec = (now.tv_usec + ((timeOutMillisecs % 1000) * 1000)) * 1000;

				if (time.tv_nsec >= 1000000000)
				{
					time.tv_nsec -= 1000000000;
					time.tv_sec++;
				}

				do
				{
					if (pthread_cond_timedwait (&condition, &mutex, &time) == ETIMEDOUT)
					{
						pthread_mutex_unlock (&mutex);
						return false;
					}
				}
				while (! triggered);
			}
		}

		if (! manualReset)
			triggered = false;

		pthread_mutex_unlock (&mutex);
		return true;
	}

	void signal() noexcept
	{
		pthread_mutex_lock (&mutex);
		triggered = true;
		pthread_cond_broadcast (&condition);
		pthread_mutex_unlock (&mutex);
	}

	void reset() noexcept
	{
		pthread_mutex_lock (&mutex);
		triggered = false;
		pthread_mutex_unlock (&mutex);
	}

private:
	pthread_cond_t condition;
	pthread_mutex_t mutex;
	bool triggered;
	const bool manualReset;

	JUCE_DECLARE_NON_COPYABLE (WaitableEventImpl);
};

WaitableEvent::WaitableEvent (const bool manualReset) noexcept
	: internal (new WaitableEventImpl (manualReset))
{
}

WaitableEvent::~WaitableEvent() noexcept
{
	delete static_cast <WaitableEventImpl*> (internal);
}

bool WaitableEvent::wait (const int timeOutMillisecs) const noexcept
{
	return static_cast <WaitableEventImpl*> (internal)->wait (timeOutMillisecs);
}

void WaitableEvent::signal() const noexcept
{
	static_cast <WaitableEventImpl*> (internal)->signal();
}

void WaitableEvent::reset() const noexcept
{
	static_cast <WaitableEventImpl*> (internal)->reset();
}

void JUCE_CALLTYPE Thread::sleep (int millisecs)
{
	struct timespec time;
	time.tv_sec = millisecs / 1000;
	time.tv_nsec = (millisecs % 1000) * 1000000;
	nanosleep (&time, 0);
}

const juce_wchar File::separator = '/';
const String File::separatorString ("/");

File File::getCurrentWorkingDirectory()
{
	HeapBlock<char> heapBuffer;

	char localBuffer [1024];
	char* cwd = getcwd (localBuffer, sizeof (localBuffer) - 1);
	int bufferSize = 4096;

	while (cwd == nullptr && errno == ERANGE)
	{
		heapBuffer.malloc (bufferSize);
		cwd = getcwd (heapBuffer, bufferSize - 1);
		bufferSize += 1024;
	}

	return File (CharPointer_UTF8 (cwd));
}

bool File::setAsCurrentWorkingDirectory() const
{
	return chdir (getFullPathName().toUTF8()) == 0;
}

namespace
{
   #if JUCE_LINUX || (JUCE_IOS && ! __DARWIN_ONLY_64_BIT_INO_T) // (this iOS stuff is to avoid a simulator bug)
	typedef struct stat64 juce_statStruct;
	#define JUCE_STAT     stat64
   #else
	typedef struct stat   juce_statStruct;
	#define JUCE_STAT     stat
   #endif

	bool juce_stat (const String& fileName, juce_statStruct& info)
	{
		return fileName.isNotEmpty()
				 && JUCE_STAT (fileName.toUTF8(), &info) == 0;
	}

	// if this file doesn't exist, find a parent of it that does..
	bool juce_doStatFS (File f, struct statfs& result)
	{
		for (int i = 5; --i >= 0;)
		{
			if (f.exists())
				break;

			f = f.getParentDirectory();
		}

		return statfs (f.getFullPathName().toUTF8(), &result) == 0;
	}

	void updateStatInfoForFile (const String& path, bool* const isDir, int64* const fileSize,
								Time* const modTime, Time* const creationTime, bool* const isReadOnly)
	{
		if (isDir != nullptr || fileSize != nullptr || modTime != nullptr || creationTime != nullptr)
		{
			juce_statStruct info;
			const bool statOk = juce_stat (path, info);

			if (isDir != nullptr)         *isDir        = statOk && ((info.st_mode & S_IFDIR) != 0);
			if (fileSize != nullptr)      *fileSize     = statOk ? info.st_size : 0;
			if (modTime != nullptr)       *modTime      = Time (statOk ? (int64) info.st_mtime * 1000 : 0);
			if (creationTime != nullptr)  *creationTime = Time (statOk ? (int64) info.st_ctime * 1000 : 0);
		}

		if (isReadOnly != nullptr)
			*isReadOnly = access (path.toUTF8(), W_OK) != 0;
	}

	Result getResultForErrno()
	{
		return Result::fail (String (strerror (errno)));
	}

	Result getResultForReturnValue (int value)
	{
		return value == -1 ? getResultForErrno() : Result::ok();
	}

	int getFD (void* handle) noexcept
	{
		return (int) (pointer_sized_int) handle;
	}
}

bool File::isDirectory() const
{
	juce_statStruct info;

	return fullPath.isEmpty()
			|| (juce_stat (fullPath, info) && ((info.st_mode & S_IFDIR) != 0));
}

bool File::exists() const
{
	return fullPath.isNotEmpty()
			 && access (fullPath.toUTF8(), F_OK) == 0;
}

bool File::existsAsFile() const
{
	return exists() && ! isDirectory();
}

int64 File::getSize() const
{
	juce_statStruct info;
	return juce_stat (fullPath, info) ? info.st_size : 0;
}

bool File::hasWriteAccess() const
{
	if (exists())
		return access (fullPath.toUTF8(), W_OK) == 0;

	if ((! isDirectory()) && fullPath.containsChar (separator))
		return getParentDirectory().hasWriteAccess();

	return false;
}

bool File::setFileReadOnlyInternal (const bool shouldBeReadOnly) const
{
	juce_statStruct info;
	if (! juce_stat (fullPath, info))
		return false;

	info.st_mode &= 0777;   // Just permissions

	if (shouldBeReadOnly)
		info.st_mode &= ~(S_IWUSR | S_IWGRP | S_IWOTH);
	else
		// Give everybody write permission?
		info.st_mode |= S_IWUSR | S_IWGRP | S_IWOTH;

	return chmod (fullPath.toUTF8(), info.st_mode) == 0;
}

void File::getFileTimesInternal (int64& modificationTime, int64& accessTime, int64& creationTime) const
{
	modificationTime = 0;
	accessTime = 0;
	creationTime = 0;

	juce_statStruct info;
	if (juce_stat (fullPath, info))
	{
		modificationTime = (int64) info.st_mtime * 1000;
		accessTime = (int64) info.st_atime * 1000;
		creationTime = (int64) info.st_ctime * 1000;
	}
}

bool File::setFileTimesInternal (int64 modificationTime, int64 accessTime, int64 /*creationTime*/) const
{
	juce_statStruct info;

	if ((modificationTime != 0 || accessTime != 0) && juce_stat (fullPath, info))
	{
		struct utimbuf times;
		times.actime  = accessTime != 0       ? (time_t) (accessTime / 1000)       : info.st_atime;
		times.modtime = modificationTime != 0 ? (time_t) (modificationTime / 1000) : info.st_mtime;

		return utime (fullPath.toUTF8(), &times) == 0;
	}

	return false;
}

bool File::deleteFile() const
{
	if (! exists())
		return true;

	if (isDirectory())
		return rmdir (fullPath.toUTF8()) == 0;

	return remove (fullPath.toUTF8()) == 0;
}

bool File::moveInternal (const File& dest) const
{
	if (rename (fullPath.toUTF8(), dest.getFullPathName().toUTF8()) == 0)
		return true;

	if (hasWriteAccess() && copyInternal (dest))
	{
		if (deleteFile())
			return true;

		dest.deleteFile();
	}

	return false;
}

Result File::createDirectoryInternal (const String& fileName) const
{
	return getResultForReturnValue (mkdir (fileName.toUTF8(), 0777));
}

int64 juce_fileSetPosition (void* handle, int64 pos)
{
	if (handle != 0 && lseek (getFD (handle), pos, SEEK_SET) == pos)
		return pos;

	return -1;
}

void FileInputStream::openHandle()
{
	totalSize = file.getSize();

	const int f = open (file.getFullPathName().toUTF8(), O_RDONLY, 00644);

	if (f != -1)
		fileHandle = (void*) f;
	else
		status = getResultForErrno();
}

void FileInputStream::closeHandle()
{
	if (fileHandle != 0)
	{
		close (getFD (fileHandle));
		fileHandle = 0;
	}
}

size_t FileInputStream::readInternal (void* const buffer, const size_t numBytes)
{
	ssize_t result = 0;

	if (fileHandle != 0)
	{
		result = ::read (getFD (fileHandle), buffer, numBytes);

		if (result < 0)
		{
			status = getResultForErrno();
			result = 0;
		}
	}

	return (size_t) result;
}

void FileOutputStream::openHandle()
{
	if (file.exists())
	{
		const int f = open (file.getFullPathName().toUTF8(), O_RDWR, 00644);

		if (f != -1)
		{
			currentPosition = lseek (f, 0, SEEK_END);

			if (currentPosition >= 0)
			{
				fileHandle = (void*) f;
			}
			else
			{
				status = getResultForErrno();
				close (f);
			}
		}
		else
		{
			status = getResultForErrno();
		}
	}
	else
	{
		const int f = open (file.getFullPathName().toUTF8(), O_RDWR + O_CREAT, 00644);

		if (f != -1)
			fileHandle = (void*) f;
		else
			status = getResultForErrno();
	}
}

void FileOutputStream::closeHandle()
{
	if (fileHandle != 0)
	{
		close (getFD (fileHandle));
		fileHandle = 0;
	}
}

int FileOutputStream::writeInternal (const void* const data, const int numBytes)
{
	ssize_t result = 0;

	if (fileHandle != 0)
	{
		result = ::write (getFD (fileHandle), data, numBytes);

		if (result == -1)
			status = getResultForErrno();
	}

	return (int) result;
}

void FileOutputStream::flushInternal()
{
	if (fileHandle != 0)
		if (fsync (getFD (fileHandle)) == -1)
			status = getResultForErrno();
}

Result FileOutputStream::truncate()
{
	if (fileHandle == 0)
		return status;

	flush();
	return getResultForReturnValue (ftruncate (getFD (fileHandle), (off_t) currentPosition));
}

MemoryMappedFile::MemoryMappedFile (const File& file, MemoryMappedFile::AccessMode mode)
	: address (nullptr),
	  length (0),
	  fileHandle (0)
{
	jassert (mode == readOnly || mode == readWrite);

	fileHandle = open (file.getFullPathName().toUTF8(),
					   mode == readWrite ? (O_CREAT + O_RDWR) : O_RDONLY, 00644);

	if (fileHandle != -1)
	{
		const int64 fileSize = file.getSize();

		void* m = mmap (0, (size_t) fileSize,
						mode == readWrite ? (PROT_READ | PROT_WRITE) : PROT_READ,
						MAP_SHARED, fileHandle, 0);

		if (m != MAP_FAILED)
		{
			address = m;
			length = (size_t) fileSize;
		}
	}
}

MemoryMappedFile::~MemoryMappedFile()
{
	if (address != nullptr)
		munmap (address, length);

	if (fileHandle != 0)
		close (fileHandle);
}

File juce_getExecutableFile();
File juce_getExecutableFile()
{
   #if JUCE_ANDROID
	return File (android.appFile);
   #else
	struct DLAddrReader
	{
		static String getFilename()
		{
			Dl_info exeInfo;
			dladdr ((void*) juce_getExecutableFile, &exeInfo);
			return CharPointer_UTF8 (exeInfo.dli_fname);
		}
	};

	static String filename (DLAddrReader::getFilename());
	return File::getCurrentWorkingDirectory().getChildFile (filename);
   #endif
}

int64 File::getBytesFreeOnVolume() const
{
	struct statfs buf;
	if (juce_doStatFS (*this, buf))
		return (int64) buf.f_bsize * (int64) buf.f_bavail; // Note: this returns space available to non-super user

	return 0;
}

int64 File::getVolumeTotalSize() const
{
	struct statfs buf;
	if (juce_doStatFS (*this, buf))
		return (int64) buf.f_bsize * (int64) buf.f_blocks;

	return 0;
}

String File::getVolumeLabel() const
{
   #if JUCE_MAC
	struct VolAttrBuf
	{
		u_int32_t       length;
		attrreference_t mountPointRef;
		char            mountPointSpace [MAXPATHLEN];
	} attrBuf;

	struct attrlist attrList = { 0 };
	attrList.bitmapcount = ATTR_BIT_MAP_COUNT;
	attrList.volattr = ATTR_VOL_INFO | ATTR_VOL_NAME;

	File f (*this);

	for (;;)
	{
		if (getattrlist (f.getFullPathName().toUTF8(), &attrList, &attrBuf, sizeof (attrBuf), 0) == 0)
			return String::fromUTF8 (((const char*) &attrBuf.mountPointRef) + attrBuf.mountPointRef.attr_dataoffset,
									 (int) attrBuf.mountPointRef.attr_length);

		const File parent (f.getParentDirectory());

		if (f == parent)
			break;

		f = parent;
	}
   #endif

	return String::empty;
}

int File::getVolumeSerialNumber() const
{
	int result = 0;
/*    int fd = open (getFullPathName().toUTF8(), O_RDONLY | O_NONBLOCK);

	char info [512];

	#ifndef HDIO_GET_IDENTITY
	 #define HDIO_GET_IDENTITY 0x030d
	#endif

	if (ioctl (fd, HDIO_GET_IDENTITY, info) == 0)
	{
		DBG (String (info + 20, 20));
		result = String (info + 20, 20).trim().getIntValue();
	}

	close (fd);*/
	return result;
}

void juce_runSystemCommand (const String&);
void juce_runSystemCommand (const String& command)
{
	int result = system (command.toUTF8());
	(void) result;
}

String juce_getOutputFromCommand (const String&);
String juce_getOutputFromCommand (const String& command)
{
	// slight bodge here, as we just pipe the output into a temp file and read it...
	const File tempFile (File::getSpecialLocation (File::tempDirectory)
						   .getNonexistentChildFile (String::toHexString (Random::getSystemRandom().nextInt()), ".tmp", false));

	juce_runSystemCommand (command + " > " + tempFile.getFullPathName());

	String result (tempFile.loadFileAsString());
	tempFile.deleteFile();
	return result;
}

class InterProcessLock::Pimpl
{
public:
	Pimpl (const String& name, const int timeOutMillisecs)
		: handle (0), refCount (1)
	{
	   #if JUCE_IOS
		handle = 1; // On iOS we can't run multiple apps, so just assume success.
	   #else

		 // Note that we can't get the normal temp folder here, as it might be different for each app.
		#if JUCE_MAC
		 File tempFolder ("~/Library/Caches/com.juce.locks");
		#else
		 File tempFolder ("/var/tmp");

		 if (! tempFolder.isDirectory())
			 tempFolder = "/tmp";
		#endif

		const File temp (tempFolder.getChildFile (name));

		temp.create();
		handle = open (temp.getFullPathName().toUTF8(), O_RDWR);

		if (handle != 0)
		{
			struct flock fl = { 0 };
			fl.l_whence = SEEK_SET;
			fl.l_type = F_WRLCK;

			const int64 endTime = Time::currentTimeMillis() + timeOutMillisecs;

			for (;;)
			{
				const int result = fcntl (handle, F_SETLK, &fl);

				if (result >= 0)
					return;

				if (errno != EINTR)
				{
					if (timeOutMillisecs == 0
						 || (timeOutMillisecs > 0 && Time::currentTimeMillis() >= endTime))
						break;

					Thread::sleep (10);
				}
			}
		}

		closeFile();
	   #endif
	}

	~Pimpl()
	{
		closeFile();
	}

	void closeFile()
	{
	   #if ! JUCE_IOS
		if (handle != 0)
		{
			struct flock fl = { 0 };
			fl.l_whence = SEEK_SET;
			fl.l_type = F_UNLCK;

			while (! (fcntl (handle, F_SETLKW, &fl) >= 0 || errno != EINTR))
			{}

			close (handle);
			handle = 0;
		}
	   #endif
	}

	int handle, refCount;
};

InterProcessLock::InterProcessLock (const String& name_)
	: name (name_)
{
}

InterProcessLock::~InterProcessLock()
{
}

bool InterProcessLock::enter (const int timeOutMillisecs)
{
	const ScopedLock sl (lock);

	if (pimpl == nullptr)
	{
		pimpl = new Pimpl (name, timeOutMillisecs);

		if (pimpl->handle == 0)
			pimpl = nullptr;
	}
	else
	{
		pimpl->refCount++;
	}

	return pimpl != nullptr;
}

void InterProcessLock::exit()
{
	const ScopedLock sl (lock);

	// Trying to release the lock too many times!
	jassert (pimpl != nullptr);

	if (pimpl != nullptr && --(pimpl->refCount) == 0)
		pimpl = nullptr;
}

void JUCE_API juce_threadEntryPoint (void*);

extern "C" void* threadEntryProc (void*);
extern "C" void* threadEntryProc (void* userData)
{
	JUCE_AUTORELEASEPOOL

   #if JUCE_ANDROID
	struct AndroidThreadScope
	{
		AndroidThreadScope()   { threadLocalJNIEnvHolder.attach(); }
		~AndroidThreadScope()  { threadLocalJNIEnvHolder.detach(); }
	};

	const AndroidThreadScope androidEnv;
   #endif

	juce_threadEntryPoint (userData);
	return nullptr;
}

void Thread::launchThread()
{
	threadHandle = 0;
	pthread_t handle = 0;

	if (pthread_create (&handle, 0, threadEntryProc, this) == 0)
	{
		pthread_detach (handle);
		threadHandle = (void*) handle;
		threadId = (ThreadID) threadHandle;
	}
}

void Thread::closeThreadHandle()
{
	threadId = 0;
	threadHandle = 0;
}

void Thread::killThread()
{
	if (threadHandle != 0)
	{
	   #if JUCE_ANDROID
		jassertfalse; // pthread_cancel not available!
	   #else
		pthread_cancel ((pthread_t) threadHandle);
	   #endif
	}
}

void Thread::setCurrentThreadName (const String& name)
{
   #if JUCE_IOS || (JUCE_MAC && defined (MAC_OS_X_VERSION_10_5) && MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_5)
	JUCE_AUTORELEASEPOOL
	[[NSThread currentThread] setName: juceStringToNS (name)];
   #elif JUCE_LINUX
	prctl (PR_SET_NAME, name.toUTF8().getAddress(), 0, 0, 0);
   #endif
}

bool Thread::setThreadPriority (void* handle, int priority)
{
	struct sched_param param;
	int policy;
	priority = jlimit (0, 10, priority);

	if (handle == 0)
		handle = (void*) pthread_self();

	if (pthread_getschedparam ((pthread_t) handle, &policy, &param) != 0)
		return false;

	policy = priority == 0 ? SCHED_OTHER : SCHED_RR;

	const int minPriority = sched_get_priority_min (policy);
	const int maxPriority = sched_get_priority_max (policy);

	param.sched_priority = ((maxPriority - minPriority) * priority) / 10 + minPriority;
	return pthread_setschedparam ((pthread_t) handle, policy, &param) == 0;
}

Thread::ThreadID Thread::getCurrentThreadId()
{
	return (ThreadID) pthread_self();
}

void Thread::yield()
{
	sched_yield();
}

/* Remove this macro if you're having problems compiling the cpu affinity
   calls (the API for these has changed about quite a bit in various Linux
   versions, and a lot of distros seem to ship with obsolete versions)
*/
#if defined (CPU_ISSET) && ! defined (SUPPORT_AFFINITIES)
 #define SUPPORT_AFFINITIES 1
#endif

void Thread::setCurrentThreadAffinityMask (const uint32 affinityMask)
{
   #if SUPPORT_AFFINITIES
	cpu_set_t affinity;
	CPU_ZERO (&affinity);

	for (int i = 0; i < 32; ++i)
		if ((affinityMask & (1 << i)) != 0)
			CPU_SET (i, &affinity);

	/*
	   N.B. If this line causes a compile error, then you've probably not got the latest
	   version of glibc installed.

	   If you don't want to update your copy of glibc and don't care about cpu affinities,
	   then you can just disable all this stuff by setting the SUPPORT_AFFINITIES macro to 0.
	*/
	sched_setaffinity (getpid(), sizeof (cpu_set_t), &affinity);
	sched_yield();

   #else
	/* affinities aren't supported because either the appropriate header files weren't found,
	   or the SUPPORT_AFFINITIES macro was turned off
	*/
	jassertfalse;
	(void) affinityMask;
   #endif
}

bool DynamicLibrary::open (const String& name)
{
	close();
	handle = dlopen (name.toUTF8(), RTLD_LOCAL | RTLD_NOW);
	return handle != 0;
}

void DynamicLibrary::close()
{
	if (handle != nullptr)
	{
		dlclose (handle);
		handle = nullptr;
	}
}

void* DynamicLibrary::getFunction (const String& functionName) noexcept
{
	return handle != nullptr ? dlsym (handle, functionName.toUTF8()) : nullptr;
}

class ChildProcess::ActiveProcess
{
public:
	ActiveProcess (const StringArray& arguments)
		: childPID (0), pipeHandle (0), readHandle (0)
	{
		int pipeHandles[2] = { 0 };

		if (pipe (pipeHandles) == 0)
		{
			const pid_t result = fork();

			if (result < 0)
			{
				close (pipeHandles[0]);
				close (pipeHandles[1]);
			}
			else if (result == 0)
			{
				// we're the child process..
				close (pipeHandles[0]);   // close the read handle
				dup2 (pipeHandles[1], 1); // turns the pipe into stdout
				close (pipeHandles[1]);

				Array<char*> argv;
				for (int i = 0; i < arguments.size(); ++i)
					argv.add (arguments[i].toUTF8().getAddress());

				argv.add (nullptr);

				execvp (argv[0], argv.getRawDataPointer());
				exit (-1);
			}
			else
			{
				// we're the parent process..
				childPID = result;
				pipeHandle = pipeHandles[0];
				close (pipeHandles[1]); // close the write handle
			}
		}
	}

	~ActiveProcess()
	{
		if (readHandle != 0)
			fclose (readHandle);

		if (pipeHandle != 0)
			close (pipeHandle);
	}

	bool isRunning() const
	{
		if (childPID != 0)
		{
			int childState;
			const int pid = waitpid (childPID, &childState, WNOHANG);
			return pid > 0 && ! (WIFEXITED (childState) || WIFSIGNALED (childState));
		}

		return false;
	}

	int read (void* const dest, const int numBytes)
	{
		jassert (dest != nullptr);

		if (readHandle == 0 && childPID != 0)
			readHandle = fdopen (pipeHandle, "r");

		if (readHandle != 0)
			return fread (dest, 1, numBytes, readHandle);

		return 0;
	}

	bool killProcess() const
	{
		return ::kill (childPID, SIGKILL) == 0;
	}

	int childPID;

private:
	int pipeHandle;
	FILE* readHandle;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ActiveProcess);
};

bool ChildProcess::start (const String& command)
{
	StringArray tokens;
	tokens.addTokens (command, true);
	tokens.removeEmptyStrings (true);

	if (tokens.size() == 0)
		return false;

	activeProcess = new ActiveProcess (tokens);

	if (activeProcess->childPID == 0)
		activeProcess = nullptr;

	return activeProcess != nullptr;
}

bool ChildProcess::isRunning() const
{
	return activeProcess != nullptr && activeProcess->isRunning();
}

int ChildProcess::readProcessOutput (void* dest, int numBytes)
{
	return activeProcess != nullptr ? activeProcess->read (dest, numBytes) : 0;
}

bool ChildProcess::kill()
{
	return activeProcess == nullptr || activeProcess->killProcess();
}

/*** End of inlined file: juce_posix_SharedCode.h ***/



/*** Start of inlined file: juce_posix_NamedPipe.cpp ***/
class NamedPipe::Pimpl
{
public:
	Pimpl (const String& pipePath, bool createPipe)
	   : pipeInName  (pipePath + "_in"),
		 pipeOutName (pipePath + "_out"),
		 pipeIn (-1), pipeOut (-1),
		 createdPipe (createPipe),
		 blocked (false), stopReadOperation (false)
	{
		signal (SIGPIPE, signalHandler);
		siginterrupt (SIGPIPE, 1);
	}

	~Pimpl()
	{
		if (pipeIn  != -1)  ::close (pipeIn);
		if (pipeOut != -1)  ::close (pipeOut);

		if (createdPipe)
		{
			unlink (pipeInName.toUTF8());
			unlink (pipeOutName.toUTF8());
		}
	}

	int read (char* destBuffer, int maxBytesToRead)
	{
		int bytesRead = -1;
		blocked = true;

		if (pipeIn == -1)
		{
			pipeIn = ::open ((createdPipe ? pipeInName
										  : pipeOutName).toUTF8(), O_RDWR);

			if (pipeIn == -1)
			{
				blocked = false;
				return -1;
			}
		}

		bytesRead = 0;

		while (bytesRead < maxBytesToRead)
		{
			const int bytesThisTime = maxBytesToRead - bytesRead;
			const int numRead = (int) ::read (pipeIn, destBuffer, bytesThisTime);

			if (numRead <= 0 || stopReadOperation)
			{
				bytesRead = -1;
				break;
			}

			bytesRead += numRead;
			destBuffer += numRead;
		}

		blocked = false;
		return bytesRead;
	}

	int write (const char* sourceBuffer, int numBytesToWrite, int timeOutMilliseconds)
	{
		int bytesWritten = -1;

		if (pipeOut == -1)
		{
			pipeOut = ::open ((createdPipe ? pipeOutName
										   : pipeInName).toUTF8(), O_WRONLY);

			if (pipeOut == -1)
				return -1;
		}

		bytesWritten = 0;
		const uint32 timeOutTime = Time::getMillisecondCounter() + timeOutMilliseconds;

		while (bytesWritten < numBytesToWrite
				&& (timeOutMilliseconds < 0 || Time::getMillisecondCounter() < timeOutTime))
		{
			const int bytesThisTime = numBytesToWrite - bytesWritten;
			const int numWritten = (int) ::write (pipeOut, sourceBuffer, bytesThisTime);

			if (numWritten <= 0)
			{
				bytesWritten = -1;
				break;
			}

			bytesWritten += numWritten;
			sourceBuffer += numWritten;
		}

		return bytesWritten;
	}

	bool createFifos() const
	{
		return (mkfifo (pipeInName .toUTF8(), 0666) == 0 || errno == EEXIST)
			&& (mkfifo (pipeOutName.toUTF8(), 0666) == 0 || errno == EEXIST);
	}

	const String pipeInName, pipeOutName;
	int pipeIn, pipeOut;

	const bool createdPipe;
	bool volatile blocked, stopReadOperation;

private:
	static void signalHandler (int) {}

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Pimpl);
};

NamedPipe::NamedPipe()
{
}

NamedPipe::~NamedPipe()
{
	close();
}

void NamedPipe::cancelPendingReads()
{
	while (pimpl != nullptr && pimpl->blocked)
	{
		pimpl->stopReadOperation = true;

		char buffer[1] = { 0 };
		int bytesWritten = (int) ::write (pimpl->pipeIn, buffer, 1);
		(void) bytesWritten;

		int timeout = 2000;
		while (pimpl->blocked && --timeout >= 0)
			Thread::sleep (2);

		pimpl->stopReadOperation = false;
	}
}

void NamedPipe::close()
{
	cancelPendingReads();
	ScopedPointer<Pimpl> deleter (pimpl); // (clears the pimpl member variable before deleting it)
}

bool NamedPipe::openInternal (const String& pipeName, const bool createPipe)
{
	close();

   #if JUCE_IOS
	pimpl = new Pimpl (File::getSpecialLocation (File::tempDirectory)
						 .getChildFile (File::createLegalFileName (pipeName)).getFullPathName(), createPipe);
   #else
	pimpl = new Pimpl ("/tmp/" + File::createLegalFileName (pipeName), createPipe);
   #endif

	if (createPipe && ! pimpl->createFifos())
	{
		pimpl = nullptr;
		return false;
	}

	return true;
}

int NamedPipe::read (void* destBuffer, int maxBytesToRead, int /*timeOutMilliseconds*/)
{
	return pimpl != nullptr ? pimpl->read (static_cast <char*> (destBuffer), maxBytesToRead) : -1;
}

int NamedPipe::write (const void* sourceBuffer, int numBytesToWrite, int timeOutMilliseconds)
{
	return pimpl != nullptr ? pimpl->write (static_cast <const char*> (sourceBuffer), numBytesToWrite, timeOutMilliseconds) : -1;
}

bool NamedPipe::isOpen() const
{
	return pimpl != nullptr;
}

/*** End of inlined file: juce_posix_NamedPipe.cpp ***/

#endif

#if JUCE_MAC || JUCE_IOS

/*** Start of inlined file: juce_mac_Files.mm ***/
/*
	Note that a lot of methods that you'd expect to find in this file actually
	live in juce_posix_SharedCode.h!
*/

bool File::copyInternal (const File& dest) const
{
	JUCE_AUTORELEASEPOOL
	NSFileManager* fm = [NSFileManager defaultManager];

	return [fm fileExistsAtPath: juceStringToNS (fullPath)]
#if defined (MAC_OS_X_VERSION_10_6) && MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_6
			&& [fm copyItemAtPath: juceStringToNS (fullPath)
						   toPath: juceStringToNS (dest.getFullPathName())
							error: nil];
#else
			&& [fm copyPath: juceStringToNS (fullPath)
					 toPath: juceStringToNS (dest.getFullPathName())
					handler: nil];
#endif
}

void File::findFileSystemRoots (Array<File>& destArray)
{
	destArray.add (File ("/"));
}

namespace FileHelpers
{
	static bool isFileOnDriveType (const File& f, const char* const* types)
	{
		struct statfs buf;

		if (juce_doStatFS (f, buf))
		{
			const String type (buf.f_fstypename);

			while (*types != 0)
				if (type.equalsIgnoreCase (*types++))
					return true;
		}

		return false;
	}

	static bool isHiddenFile (const String& path)
	{
	  #if defined (MAC_OS_X_VERSION_10_6) && MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_6
		JUCE_AUTORELEASEPOOL
		NSNumber* hidden = nil;
		NSError* err = nil;

		return [[NSURL fileURLWithPath: juceStringToNS (path)]
					getResourceValue: &hidden forKey: NSURLIsHiddenKey error: &err]
				&& [hidden boolValue];
	  #elif JUCE_IOS
		return File (path).getFileName().startsWithChar ('.');
	  #else
		FSRef ref;
		LSItemInfoRecord info;

		return FSPathMakeRefWithOptions ((const UInt8*) path.toUTF8().getAddress(), kFSPathMakeRefDoNotFollowLeafSymlink, &ref, 0) == noErr
				 && LSCopyItemInfoForRef (&ref, kLSRequestBasicFlagsOnly, &info) == noErr
				 && (info.flags & kLSItemInfoIsInvisible) != 0;
	  #endif
	}

  #if JUCE_IOS
	String getIOSSystemLocation (NSSearchPathDirectory type)
	{
		return nsStringToJuce ([NSSearchPathForDirectoriesInDomains (type, NSUserDomainMask, YES)
								objectAtIndex: 0]);
	}
  #endif

	static bool launchExecutable (const String& pathAndArguments)
	{
		const char* const argv[4] = { "/bin/sh", "-c", pathAndArguments.toUTF8(), 0 };

		const int cpid = fork();

		if (cpid == 0)
		{
			// Child process
			if (execve (argv[0], (char**) argv, 0) < 0)
				exit (0);
		}
		else
		{
			if (cpid < 0)
				return false;
		}

		return true;
	}
}

bool File::isOnCDRomDrive() const
{
	const char* const cdTypes[] = { "cd9660", "cdfs", "cddafs", "udf", 0 };

	return FileHelpers::isFileOnDriveType (*this, cdTypes);
}

bool File::isOnHardDisk() const
{
	const char* const nonHDTypes[] = { "nfs", "smbfs", "ramfs", 0 };

	return ! (isOnCDRomDrive() || FileHelpers::isFileOnDriveType (*this, nonHDTypes));
}

bool File::isOnRemovableDrive() const
{
   #if JUCE_IOS
	return false; // xxx is this possible?
   #else
	JUCE_AUTORELEASEPOOL
	BOOL removable = false;

	[[NSWorkspace sharedWorkspace]
		   getFileSystemInfoForPath: juceStringToNS (getFullPathName())
						isRemovable: &removable
						 isWritable: nil
					  isUnmountable: nil
						description: nil
							   type: nil];

	return removable;
   #endif
}

bool File::isHidden() const
{
	return FileHelpers::isHiddenFile (getFullPathName());
}

const char* juce_Argv0 = nullptr;  // referenced from juce_Application.cpp

File File::getSpecialLocation (const SpecialLocationType type)
{
	JUCE_AUTORELEASEPOOL
	String resultPath;

	switch (type)
	{
		case userHomeDirectory:                 resultPath = nsStringToJuce (NSHomeDirectory()); break;

	  #if JUCE_IOS
		case userDocumentsDirectory:            resultPath = FileHelpers::getIOSSystemLocation (NSDocumentDirectory); break;
		case userDesktopDirectory:              resultPath = FileHelpers::getIOSSystemLocation (NSDesktopDirectory); break;

		case tempDirectory:
		{
			File tmp (FileHelpers::getIOSSystemLocation (NSCachesDirectory));
			tmp = tmp.getChildFile (juce_getExecutableFile().getFileNameWithoutExtension());
			tmp.createDirectory();
			return tmp.getFullPathName();
		}

	  #else
		case userDocumentsDirectory:            resultPath = "~/Documents"; break;
		case userDesktopDirectory:              resultPath = "~/Desktop"; break;

		case tempDirectory:
		{
			File tmp ("~/Library/Caches/" + juce_getExecutableFile().getFileNameWithoutExtension());
			tmp.createDirectory();
			return tmp.getFullPathName();
		}
	  #endif
		case userMusicDirectory:                resultPath = "~/Music"; break;
		case userMoviesDirectory:               resultPath = "~/Movies"; break;
		case userApplicationDataDirectory:      resultPath = "~/Library"; break;
		case commonApplicationDataDirectory:    resultPath = "/Library"; break;
		case globalApplicationsDirectory:       resultPath = "/Applications"; break;

		case invokedExecutableFile:
			if (juce_Argv0 != 0)
				return File (CharPointer_UTF8 (juce_Argv0));
			// deliberate fall-through...

		case currentExecutableFile:
			return juce_getExecutableFile();

		case currentApplicationFile:
		{
			const File exe (juce_getExecutableFile());
			const File parent (exe.getParentDirectory());

		  #if JUCE_IOS
			return parent;
		  #else
			return parent.getFullPathName().endsWithIgnoreCase ("Contents/MacOS")
					? parent.getParentDirectory().getParentDirectory()
					: exe;
		  #endif
		}

		case hostApplicationPath:
		{
			unsigned int size = 8192;
			HeapBlock<char> buffer;
			buffer.calloc (size + 8);

			_NSGetExecutablePath (buffer.getData(), &size);
			return String::fromUTF8 (buffer, size);
		}

		default:
			jassertfalse; // unknown type?
			break;
	}

	if (resultPath.isNotEmpty())
		return File (resultPath.convertToPrecomposedUnicode());

	return File::nonexistent;
}

String File::getVersion() const
{
	JUCE_AUTORELEASEPOOL
	String result;

	NSBundle* bundle = [NSBundle bundleWithPath: juceStringToNS (getFullPathName())];

	if (bundle != nil)
	{
		NSDictionary* info = [bundle infoDictionary];

		if (info != nil)
		{
			NSString* name = [info valueForKey: nsStringLiteral ("CFBundleShortVersionString")];

			if (name != nil)
				result = nsStringToJuce (name);
		}
	}

	return result;
}

File File::getLinkedTarget() const
{
  #if JUCE_IOS || (defined (MAC_OS_X_VERSION_10_5) && MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_5)
	NSString* dest = [[NSFileManager defaultManager] destinationOfSymbolicLinkAtPath: juceStringToNS (getFullPathName()) error: nil];

  #else
	// (the cast here avoids a deprecation warning)
	NSString* dest = [((id) [NSFileManager defaultManager]) pathContentOfSymbolicLinkAtPath: juceStringToNS (getFullPathName())];
  #endif

	if (dest != nil)
		return File (nsStringToJuce (dest));

	return *this;
}

bool File::moveToTrash() const
{
	if (! exists())
		return true;

  #if JUCE_IOS
	return deleteFile(); //xxx is there a trashcan on the iPhone?
  #else
	JUCE_AUTORELEASEPOOL

	NSString* p = juceStringToNS (getFullPathName());

	return [[NSWorkspace sharedWorkspace]
				performFileOperation: NSWorkspaceRecycleOperation
							  source: [p stringByDeletingLastPathComponent]
						 destination: nsEmptyString()
							   files: [NSArray arrayWithObject: [p lastPathComponent]]
								 tag: nil ];
  #endif
}

class DirectoryIterator::NativeIterator::Pimpl
{
public:
	Pimpl (const File& directory, const String& wildCard_)
		: parentDir (File::addTrailingSeparator (directory.getFullPathName())),
		  wildCard (wildCard_),
		  enumerator (nil)
	{
		JUCE_AUTORELEASEPOOL

		enumerator = [[[NSFileManager defaultManager] enumeratorAtPath: juceStringToNS (directory.getFullPathName())] retain];
	}

	~Pimpl()
	{
		[enumerator release];
	}

	bool next (String& filenameFound,
			   bool* const isDir, bool* const isHidden, int64* const fileSize,
			   Time* const modTime, Time* const creationTime, bool* const isReadOnly)
	{
		JUCE_AUTORELEASEPOOL
		const char* wildcardUTF8 = nullptr;

		for (;;)
		{
			NSString* file;
			if (enumerator == nil || (file = [enumerator nextObject]) == nil)
				return false;

			[enumerator skipDescendents];
			filenameFound = nsStringToJuce (file);

			if (wildcardUTF8 == nullptr)
				wildcardUTF8 = wildCard.toUTF8();

			if (fnmatch (wildcardUTF8, filenameFound.toUTF8(), FNM_CASEFOLD) != 0)
				continue;

			const String path (parentDir + filenameFound);
			updateStatInfoForFile (path, isDir, fileSize, modTime, creationTime, isReadOnly);

			if (isHidden != nullptr)
				*isHidden = FileHelpers::isHiddenFile (path);

			return true;
		}
	}

private:
	String parentDir, wildCard;
	NSDirectoryEnumerator* enumerator;

	JUCE_DECLARE_NON_COPYABLE (Pimpl);
};

DirectoryIterator::NativeIterator::NativeIterator (const File& directory, const String& wildCard)
	: pimpl (new DirectoryIterator::NativeIterator::Pimpl (directory, wildCard))
{
}

DirectoryIterator::NativeIterator::~NativeIterator()
{
}

bool DirectoryIterator::NativeIterator::next (String& filenameFound,
											  bool* const isDir, bool* const isHidden, int64* const fileSize,
											  Time* const modTime, Time* const creationTime, bool* const isReadOnly)
{
	return pimpl->next (filenameFound, isDir, isHidden, fileSize, modTime, creationTime, isReadOnly);
}

bool Process::openDocument (const String& fileName, const String& parameters)
{
  #if JUCE_IOS
	return [[UIApplication sharedApplication] openURL: [NSURL URLWithString: juceStringToNS (fileName)]];
  #else
	JUCE_AUTORELEASEPOOL

	if (parameters.isEmpty())
	{
		return [[NSWorkspace sharedWorkspace] openFile: juceStringToNS (fileName)]
			|| [[NSWorkspace sharedWorkspace] openURL: [NSURL URLWithString: juceStringToNS (fileName)]];
	}

	bool ok = false;
	const File file (fileName);

	if (file.isBundle())
	{
		NSMutableArray* urls = [NSMutableArray array];

		StringArray docs;
		docs.addTokens (parameters, true);
		for (int i = 0; i < docs.size(); ++i)
			[urls addObject: juceStringToNS (docs[i])];

		ok = [[NSWorkspace sharedWorkspace] openURLs: urls
							 withAppBundleIdentifier: [[NSBundle bundleWithPath: juceStringToNS (fileName)] bundleIdentifier]
											 options: 0
					  additionalEventParamDescriptor: nil
								   launchIdentifiers: nil];
	}
	else if (file.exists())
	{
		ok = FileHelpers::launchExecutable ("\"" + fileName + "\" " + parameters);
	}

	return ok;
  #endif
}

void File::revealToUser() const
{
  #if ! JUCE_IOS
	if (exists())
		[[NSWorkspace sharedWorkspace] selectFile: juceStringToNS (getFullPathName()) inFileViewerRootedAtPath: nsEmptyString()];
	else if (getParentDirectory().exists())
		getParentDirectory().revealToUser();
  #endif
}

OSType File::getMacOSType() const
{
	JUCE_AUTORELEASEPOOL

   #if JUCE_IOS || (defined (MAC_OS_X_VERSION_10_5) && MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_5)
	NSDictionary* fileDict = [[NSFileManager defaultManager] attributesOfItemAtPath: juceStringToNS (getFullPathName()) error: nil];
   #else
	// (the cast here avoids a deprecation warning)
	NSDictionary* fileDict = [((id) [NSFileManager defaultManager]) fileAttributesAtPath: juceStringToNS (getFullPathName()) traverseLink: NO];
   #endif

	return [fileDict fileHFSTypeCode];
}

bool File::isBundle() const
{
   #if JUCE_IOS
	return false; // xxx can't find a sensible way to do this without trying to open the bundle..
   #else
	JUCE_AUTORELEASEPOOL
	return [[NSWorkspace sharedWorkspace] isFilePackageAtPath: juceStringToNS (getFullPathName())];
   #endif
}

#if JUCE_MAC
void File::addToDock() const
{
	// check that it's not already there...
	if (! juce_getOutputFromCommand ("defaults read com.apple.dock persistent-apps").containsIgnoreCase (getFullPathName()))
	{
		juce_runSystemCommand ("defaults write com.apple.dock persistent-apps -array-add \"<dict><key>tile-data</key><dict><key>file-data</key><dict><key>_CFURLString</key><string>"
								 + getFullPathName() + "</string><key>_CFURLStringType</key><integer>0</integer></dict></dict></dict>\"");

		juce_runSystemCommand ("osascript -e \"tell application \\\"Dock\\\" to quit\"");
	}
}
#endif

/*** End of inlined file: juce_mac_Files.mm ***/



/*** Start of inlined file: juce_mac_Network.mm ***/
void MACAddress::findAllAddresses (Array<MACAddress>& result)
{
	ifaddrs* addrs = nullptr;

	if (getifaddrs (&addrs) == 0)
	{
		for (const ifaddrs* cursor = addrs; cursor != nullptr; cursor = cursor->ifa_next)
		{
			sockaddr_storage* sto = (sockaddr_storage*) cursor->ifa_addr;
			if (sto->ss_family == AF_LINK)
			{
				const sockaddr_dl* const sadd = (const sockaddr_dl*) cursor->ifa_addr;

				#ifndef IFT_ETHER
				 #define IFT_ETHER 6
				#endif

				if (sadd->sdl_type == IFT_ETHER)
					result.addIfNotAlreadyThere (MACAddress (((const uint8*) sadd->sdl_data) + sadd->sdl_nlen));
			}
		}

		freeifaddrs (addrs);
	}
}

bool Process::openEmailWithAttachments (const String& targetEmailAddress,
										const String& emailSubject,
										const String& bodyText,
										const StringArray& filesToAttach)
{
  #if JUCE_IOS
	//xxx probably need to use MFMailComposeViewController
	jassertfalse;
	return false;
  #else
	JUCE_AUTORELEASEPOOL

	String script;
	script << "tell application \"Mail\"\r\n"
			  "set newMessage to make new outgoing message with properties {subject:\""
		   << emailSubject.replace ("\"", "\\\"")
		   << "\", content:\""
		   << bodyText.replace ("\"", "\\\"")
		   << "\" & return & return}\r\n"
			  "tell newMessage\r\n"
			  "set visible to true\r\n"
			  "set sender to \"sdfsdfsdfewf\"\r\n"
			  "make new to recipient at end of to recipients with properties {address:\""
		   << targetEmailAddress
		   << "\"}\r\n";

	for (int i = 0; i < filesToAttach.size(); ++i)
	{
		script << "tell content\r\n"
				  "make new attachment with properties {file name:\""
			   << filesToAttach[i].replace ("\"", "\\\"")
			   << "\"} at after the last paragraph\r\n"
				  "end tell\r\n";
	}

	script << "end tell\r\n"
			  "end tell\r\n";

	NSAppleScript* s = [[NSAppleScript alloc] initWithSource: juceStringToNS (script)];
	NSDictionary* error = nil;
	const bool ok = [s executeAndReturnError: &error] != nil;
	[s release];

	return ok;
  #endif
}

} // (juce namespace)

using namespace juce;

#define JuceURLConnection MakeObjCClassName(JuceURLConnection)

@interface JuceURLConnection  : NSObject
{
@public
	NSURLRequest* request;
	NSURLConnection* connection;
	NSMutableData* data;
	Thread* runLoopThread;
	bool initialised, hasFailed, hasFinished;
	int position;
	int64 contentLength;
	NSDictionary* headers;
	NSLock* dataLock;
}

- (JuceURLConnection*) initWithRequest: (NSURLRequest*) req withCallback: (URL::OpenStreamProgressCallback*) callback withContext: (void*) context;
- (void) dealloc;
- (void) connection: (NSURLConnection*) connection didReceiveResponse: (NSURLResponse*) response;
- (void) connection: (NSURLConnection*) connection didFailWithError: (NSError*) error;
- (void) connection: (NSURLConnection*) connection didReceiveData: (NSData*) data;
- (void) connectionDidFinishLoading: (NSURLConnection*) connection;

- (BOOL) isOpen;
- (int) read: (char*) dest numBytes: (int) num;
- (int) readPosition;
- (void) stop;
- (void) createConnection;

@end

class JuceURLConnectionMessageThread  : public Thread
{
public:
	JuceURLConnectionMessageThread (JuceURLConnection* owner_)
		: Thread ("http connection"),
		  owner (owner_)
	{
	}

	~JuceURLConnectionMessageThread()
	{
		stopThread (10000);
	}

	void run()
	{
		[owner createConnection];

		while (! threadShouldExit())
		{
			JUCE_AUTORELEASEPOOL
			[[NSRunLoop currentRunLoop] runUntilDate: [NSDate dateWithTimeIntervalSinceNow: 0.01]];
		}
	}

private:
	JuceURLConnection* owner;
};

@implementation JuceURLConnection

- (JuceURLConnection*) initWithRequest: (NSURLRequest*) req
						  withCallback: (URL::OpenStreamProgressCallback*) callback
						   withContext: (void*) context;
{
	[super init];
	request = req;
	[request retain];
	data = [[NSMutableData data] retain];
	dataLock = [[NSLock alloc] init];
	connection = nil;
	initialised = false;
	hasFailed = false;
	hasFinished = false;
	contentLength = -1;
	headers = nil;

	runLoopThread = new JuceURLConnectionMessageThread (self);
	runLoopThread->startThread();

	while (runLoopThread->isThreadRunning() && ! initialised)
	{
		if (callback != nullptr)
			callback (context, -1, (int) [[request HTTPBody] length]);

		Thread::sleep (1);
	}

	return self;
}

- (void) dealloc
{
	[self stop];

	deleteAndZero (runLoopThread);
	[connection release];
	[data release];
	[dataLock release];
	[request release];
	[headers release];
	[super dealloc];
}

- (void) createConnection
{
	NSUInteger oldRetainCount = [self retainCount];
	connection = [[NSURLConnection alloc] initWithRequest: request
												 delegate: self];

	if (oldRetainCount == [self retainCount])
		[self retain]; // newer SDK should already retain this, but there were problems in older versions..

	if (connection == nil)
		runLoopThread->signalThreadShouldExit();
}

- (void) connection: (NSURLConnection*) conn didReceiveResponse: (NSURLResponse*) response
{
	(void) conn;
	[dataLock lock];
	[data setLength: 0];
	[dataLock unlock];
	initialised = true;
	contentLength = [response expectedContentLength];

	[headers release];
	headers = nil;

	if ([response isKindOfClass: [NSHTTPURLResponse class]])
		headers = [[((NSHTTPURLResponse*) response) allHeaderFields] retain];
}

- (void) connection: (NSURLConnection*) conn didFailWithError: (NSError*) error
{
	(void) conn;
	DBG (nsStringToJuce ([error description]));
	hasFailed = true;
	initialised = true;

	if (runLoopThread != nullptr)
		runLoopThread->signalThreadShouldExit();
}

- (void) connection: (NSURLConnection*) conn didReceiveData: (NSData*) newData
{
	(void) conn;
	[dataLock lock];
	[data appendData: newData];
	[dataLock unlock];
	initialised = true;
}

- (void) connectionDidFinishLoading: (NSURLConnection*) conn
{
	(void) conn;
	hasFinished = true;
	initialised = true;

	if (runLoopThread != nullptr)
		runLoopThread->signalThreadShouldExit();
}

- (BOOL) isOpen
{
	return connection != nil && ! hasFailed;
}

- (int) readPosition
{
	return position;
}

- (int) read: (char*) dest numBytes: (int) numNeeded
{
	int numDone = 0;

	while (numNeeded > 0)
	{
		int available = jmin (numNeeded, (int) [data length]);

		if (available > 0)
		{
			[dataLock lock];
			[data getBytes: dest length: available];
			[data replaceBytesInRange: NSMakeRange (0, available) withBytes: nil length: 0];
			[dataLock unlock];

			numDone += available;
			numNeeded -= available;
			dest += available;
		}
		else
		{
			if (hasFailed || hasFinished)
				break;

			Thread::sleep (1);
		}
	}

	position += numDone;
	return numDone;
}

- (void) stop
{
	[connection cancel];

	if (runLoopThread != nullptr)
		runLoopThread->stopThread (10000);
}

@end

namespace juce
{

class WebInputStream  : public InputStream
{
public:

	WebInputStream (const String& address_, bool isPost_, const MemoryBlock& postData_,
					URL::OpenStreamProgressCallback* progressCallback, void* progressCallbackContext,
					const String& headers_, int timeOutMs_, StringPairArray* responseHeaders)
	  : connection (nil),
		address (address_), headers (headers_), postData (postData_), position (0),
		finished (false), isPost (isPost_), timeOutMs (timeOutMs_)
	{
		JUCE_AUTORELEASEPOOL
		connection = createConnection (progressCallback, progressCallbackContext);

		if (responseHeaders != nullptr && connection != nil && connection->headers != nil)
		{
			NSEnumerator* enumerator = [connection->headers keyEnumerator];
			NSString* key;

			while ((key = [enumerator nextObject]) != nil)
				responseHeaders->set (nsStringToJuce (key),
									  nsStringToJuce ((NSString*) [connection->headers objectForKey: key]));
		}
	}

	~WebInputStream()
	{
		close();
	}

	bool isError() const        { return connection == nil; }
	int64 getTotalLength()      { return connection == nil ? -1 : connection->contentLength; }
	bool isExhausted()          { return finished; }
	int64 getPosition()         { return position; }

	int read (void* buffer, int bytesToRead)
	{
		jassert (buffer != nullptr && bytesToRead >= 0);

		if (finished || isError())
		{
			return 0;
		}
		else
		{
			JUCE_AUTORELEASEPOOL
			const int bytesRead = [connection read: static_cast <char*> (buffer) numBytes: bytesToRead];
			position += bytesRead;

			if (bytesRead == 0)
				finished = true;

			return bytesRead;
		}
	}

	bool setPosition (int64 wantedPos)
	{
		if (wantedPos != position)
		{
			finished = false;

			if (wantedPos < position)
			{
				close();
				position = 0;
				connection = createConnection (0, 0);
			}

			skipNextBytes (wantedPos - position);
		}

		return true;
	}

private:
	JuceURLConnection* connection;
	String address, headers;
	MemoryBlock postData;
	int64 position;
	bool finished;
	const bool isPost;
	const int timeOutMs;

	void close()
	{
		[connection stop];
		[connection release];
		connection = nil;
	}

	JuceURLConnection* createConnection (URL::OpenStreamProgressCallback* progressCallback,
										 void* progressCallbackContext)
	{
		NSMutableURLRequest* req = [NSMutableURLRequest  requestWithURL: [NSURL URLWithString: juceStringToNS (address)]
															cachePolicy: NSURLRequestReloadIgnoringLocalCacheData
														timeoutInterval: timeOutMs <= 0 ? 60.0 : (timeOutMs / 1000.0)];

		if (req == nil)
			return nil;

		[req setHTTPMethod: nsStringLiteral (isPost ? "POST" : "GET")];
		//[req setCachePolicy: NSURLRequestReloadIgnoringLocalAndRemoteCacheData];

		StringArray headerLines;
		headerLines.addLines (headers);
		headerLines.removeEmptyStrings (true);

		for (int i = 0; i < headerLines.size(); ++i)
		{
			const String key (headerLines[i].upToFirstOccurrenceOf (":", false, false).trim());
			const String value (headerLines[i].fromFirstOccurrenceOf (":", false, false).trim());

			if (key.isNotEmpty() && value.isNotEmpty())
				[req addValue: juceStringToNS (value) forHTTPHeaderField: juceStringToNS (key)];
		}

		if (isPost && postData.getSize() > 0)
			[req setHTTPBody: [NSData dataWithBytes: postData.getData()
											 length: postData.getSize()]];

		JuceURLConnection* const s = [[JuceURLConnection alloc] initWithRequest: req
																   withCallback: progressCallback
																	withContext: progressCallbackContext];

		if ([s isOpen])
			return s;

		[s release];
		return nil;
	}

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WebInputStream);
};

InputStream* URL::createNativeStream (const String& address, bool isPost, const MemoryBlock& postData,
									  OpenStreamProgressCallback* progressCallback, void* progressCallbackContext,
									  const String& headers, const int timeOutMs, StringPairArray* responseHeaders)
{
	ScopedPointer <WebInputStream> wi (new WebInputStream (address, isPost, postData,
														   progressCallback, progressCallbackContext,
														   headers, timeOutMs, responseHeaders));

	return wi->isError() ? nullptr : wi.release();
}

/*** End of inlined file: juce_mac_Network.mm ***/


/*** Start of inlined file: juce_mac_Strings.mm ***/
String String::fromCFString (CFStringRef cfString)
{
	if (cfString == 0)
		return String::empty;

	CFRange range = { 0, CFStringGetLength (cfString) };
	HeapBlock <UniChar> u (range.length + 1);
	CFStringGetCharacters (cfString, range, u);
	u[range.length] = 0;

	return String (CharPointer_UTF16 ((const CharPointer_UTF16::CharType*) u.getData()));
}

CFStringRef String::toCFString() const
{
	CharPointer_UTF16 utf16 (toUTF16());
	return CFStringCreateWithCharacters (kCFAllocatorDefault, (const UniChar*) utf16.getAddress(), utf16.length());
}

String String::convertToPrecomposedUnicode() const
{
   #if JUCE_IOS
	JUCE_AUTORELEASEPOOL
	return nsStringToJuce ([juceStringToNS (*this) precomposedStringWithCanonicalMapping]);
   #else
	UnicodeMapping map;

	map.unicodeEncoding = CreateTextEncoding (kTextEncodingUnicodeDefault,
											  kUnicodeNoSubset,
											  kTextEncodingDefaultFormat);

	map.otherEncoding = CreateTextEncoding (kTextEncodingUnicodeDefault,
											kUnicodeCanonicalCompVariant,
											kTextEncodingDefaultFormat);

	map.mappingVersion = kUnicodeUseLatestMapping;

	UnicodeToTextInfo conversionInfo = 0;
	String result;

	if (CreateUnicodeToTextInfo (&map, &conversionInfo) == noErr)
	{
		const int bytesNeeded = CharPointer_UTF16::getBytesRequiredFor (getCharPointer());

		HeapBlock <char> tempOut;
		tempOut.calloc (bytesNeeded + 4);

		ByteCount bytesRead = 0;
		ByteCount outputBufferSize = 0;

		if (ConvertFromUnicodeToText (conversionInfo,
									  bytesNeeded, (ConstUniCharArrayPtr) toUTF16().getAddress(),
									  kUnicodeDefaultDirectionMask,
									  0, 0, 0, 0,
									  bytesNeeded, &bytesRead,
									  &outputBufferSize, tempOut) == noErr)
		{
			result = String (CharPointer_UTF16 ((CharPointer_UTF16::CharType*) tempOut.getData()));
		}

		DisposeUnicodeToTextInfo (&conversionInfo);
	}

	return result;
   #endif
}

/*** End of inlined file: juce_mac_Strings.mm ***/


/*** Start of inlined file: juce_mac_SystemStats.mm ***/
ScopedAutoReleasePool::ScopedAutoReleasePool()
{
	pool = [[NSAutoreleasePool alloc] init];
}

ScopedAutoReleasePool::~ScopedAutoReleasePool()
{
	[((NSAutoreleasePool*) pool) release];
}

void Logger::outputDebugString (const String& text)
{
	std::cerr << text << std::endl;
}

namespace SystemStatsHelpers
{
   #if JUCE_INTEL
	static void doCPUID (uint32& a, uint32& b, uint32& c, uint32& d, uint32 type)
	{
		uint32 la = a, lb = b, lc = c, ld = d;

		asm ("mov %%ebx, %%esi \n\t"
			 "cpuid \n\t"
			 "xchg %%esi, %%ebx"
			   : "=a" (la), "=S" (lb), "=c" (lc), "=d" (ld) : "a" (type)
		   #if JUCE_64BIT
				  , "b" (lb), "c" (lc), "d" (ld)
		   #endif
		);

		a = la; b = lb; c = lc; d = ld;
	}
   #endif
}

SystemStats::CPUFlags::CPUFlags()
{
   #if JUCE_INTEL
	uint32 familyModel = 0, extFeatures = 0, features = 0, dummy = 0;
	SystemStatsHelpers::doCPUID (familyModel, extFeatures, dummy, features, 1);

	hasMMX   = (features & (1 << 23)) != 0;
	hasSSE   = (features & (1 << 25)) != 0;
	hasSSE2  = (features & (1 << 26)) != 0;
	has3DNow = (extFeatures & (1 << 31)) != 0;
   #else
	hasMMX = false;
	hasSSE = false;
	hasSSE2 = false;
	has3DNow = false;
   #endif

   #if JUCE_IOS || (MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_5)
	numCpus = (int) [[NSProcessInfo processInfo] activeProcessorCount];
   #else
	numCpus = (int) MPProcessors();
   #endif
}

#if JUCE_MAC
struct RLimitInitialiser
{
	RLimitInitialiser()
	{
		rlimit lim;
		getrlimit (RLIMIT_NOFILE, &lim);
		lim.rlim_cur = lim.rlim_max = RLIM_INFINITY;
		setrlimit (RLIMIT_NOFILE, &lim);
	}
};

static RLimitInitialiser rLimitInitialiser;
#endif

SystemStats::OperatingSystemType SystemStats::getOperatingSystemType()
{
	return MacOSX;
}

String SystemStats::getOperatingSystemName()
{
   #if JUCE_IOS
	return "iOS " + nsStringToJuce ([[UIDevice currentDevice] systemVersion]);
   #else
	SInt32 major, minor;
	Gestalt (gestaltSystemVersionMajor, &major);
	Gestalt (gestaltSystemVersionMinor, &minor);

	String s ("Mac OSX ");
	s << (int) major << '.' << (int) minor;
	return s;
   #endif
}

#if ! JUCE_IOS
int SystemStats::getOSXMinorVersionNumber()
{
	SInt32 versionMinor = 0;
	OSErr err = Gestalt (gestaltSystemVersionMinor, &versionMinor);
	(void) err;
	jassert (err == noErr);
	return (int) versionMinor;
}
#endif

bool SystemStats::isOperatingSystem64Bit()
{
   #if JUCE_IOS
	return false;
   #elif JUCE_64BIT
	return true;
   #else
	return getOSXMinorVersionNumber() >= 6;
   #endif
}

int SystemStats::getMemorySizeInMegabytes()
{
	uint64 mem = 0;
	size_t memSize = sizeof (mem);
	int mib[] = { CTL_HW, HW_MEMSIZE };
	sysctl (mib, 2, &mem, &memSize, 0, 0);
	return (int) (mem / (1024 * 1024));
}

String SystemStats::getCpuVendor()
{
   #if JUCE_INTEL
	uint32 dummy = 0;
	uint32 vendor[4] = { 0 };

	SystemStatsHelpers::doCPUID (dummy, vendor[0], vendor[2], vendor[1], 0);

	return String (reinterpret_cast <const char*> (vendor), 12);
   #else
	return String::empty;
   #endif
}

int SystemStats::getCpuSpeedInMegaherz()
{
	uint64 speedHz = 0;
	size_t speedSize = sizeof (speedHz);
	int mib[] = { CTL_HW, HW_CPU_FREQ };
	sysctl (mib, 2, &speedHz, &speedSize, 0, 0);

   #if JUCE_BIG_ENDIAN
	if (speedSize == 4)
		speedHz >>= 32;
   #endif

	return (int) (speedHz / 1000000);
}

String SystemStats::getLogonName()
{
	return nsStringToJuce (NSUserName());
}

String SystemStats::getFullUserName()
{
	return nsStringToJuce (NSFullUserName());
}

String SystemStats::getComputerName()
{
	char name [256] = { 0 };
	if (gethostname (name, sizeof (name) - 1) == 0)
		return String (name).upToLastOccurrenceOf (".local", false, true);

	return String::empty;
}

class HiResCounterHandler
{
public:
	HiResCounterHandler()
	{
		mach_timebase_info_data_t timebase;
		(void) mach_timebase_info (&timebase);

		if (timebase.numer % 1000000 == 0)
		{
			numerator = timebase.numer / 1000000;
			denominator = timebase.denom;
		}
		else
		{
			numerator = timebase.numer;
			denominator = timebase.denom * (int64) 1000000;
		}

		highResTimerFrequency = (timebase.denom * (int64) 1000000000) / timebase.numer;
		highResTimerToMillisecRatio = numerator / (double) denominator;
	}

	inline uint32 millisecondsSinceStartup() const noexcept
	{
		return (uint32) ((mach_absolute_time() * numerator) / denominator);
	}

	inline double getMillisecondCounterHiRes() const noexcept
	{
		return mach_absolute_time() * highResTimerToMillisecRatio;
	}

	int64 highResTimerFrequency;

private:
	int64 numerator, denominator;
	double highResTimerToMillisecRatio;
};

static HiResCounterHandler hiResCounterHandler;

uint32 juce_millisecondsSinceStartup() noexcept         { return hiResCounterHandler.millisecondsSinceStartup(); }
double Time::getMillisecondCounterHiRes() noexcept      { return hiResCounterHandler.getMillisecondCounterHiRes(); }
int64  Time::getHighResolutionTicksPerSecond() noexcept { return hiResCounterHandler.highResTimerFrequency; }
int64  Time::getHighResolutionTicks() noexcept          { return (int64) mach_absolute_time(); }

bool Time::setSystemTimeToThisTime() const
{
	jassertfalse;
	return false;
}

int SystemStats::getPageSize()
{
	return (int) NSPageSize();
}

/*** End of inlined file: juce_mac_SystemStats.mm ***/


/*** Start of inlined file: juce_mac_Threads.mm ***/
/*
	Note that a lot of methods that you'd expect to find in this file actually
	live in juce_posix_SharedCode.h!
*/

bool Process::isForegroundProcess()
{
   #if JUCE_MAC
	return [NSApp isActive];
   #else
	return true; // xxx change this if more than one app is ever possible on the iPhone!
   #endif
}

void Process::raisePrivilege()
{
	jassertfalse;
}

void Process::lowerPrivilege()
{
	jassertfalse;
}

void Process::terminate()
{
	exit (0);
}

void Process::setPriority (ProcessPriority)
{
	// xxx
}

JUCE_API bool JUCE_CALLTYPE juce_isRunningUnderDebugger()
{
	static char testResult = 0;

	if (testResult == 0)
	{
		struct kinfo_proc info;
		int m[] = { CTL_KERN, KERN_PROC, KERN_PROC_PID, getpid() };
		size_t sz = sizeof (info);
		sysctl (m, 4, &info, &sz, 0, 0);
		testResult = ((info.kp_proc.p_flag & P_TRACED) != 0) ? 1 : -1;
	}

	return testResult > 0;
}

JUCE_API bool JUCE_CALLTYPE Process::isRunningUnderDebugger()
{
	return juce_isRunningUnderDebugger();
}

/*** End of inlined file: juce_mac_Threads.mm ***/

#elif JUCE_WINDOWS

/*** Start of inlined file: juce_win32_ComSmartPtr.h ***/
#ifndef __JUCE_WIN32_COMSMARTPTR_JUCEHEADER__
#define __JUCE_WIN32_COMSMARTPTR_JUCEHEADER__

/** A simple COM smart pointer.
*/
template <class ComClass>
class ComSmartPtr
{
public:
	ComSmartPtr() throw() : p (0)                               {}
	ComSmartPtr (ComClass* const p_) : p (p_)                   { if (p_ != 0) p_->AddRef(); }
	ComSmartPtr (const ComSmartPtr<ComClass>& p_) : p (p_.p)    { if (p  != 0) p ->AddRef(); }
	~ComSmartPtr()                                              { release(); }

	operator ComClass*() const throw()     { return p; }
	ComClass& operator*() const throw()    { return *p; }
	ComClass* operator->() const throw()   { return p; }

	ComSmartPtr& operator= (ComClass* const newP)
	{
		if (newP != 0)  newP->AddRef();
		release();
		p = newP;
		return *this;
	}

	ComSmartPtr& operator= (const ComSmartPtr<ComClass>& newP)  { return operator= (newP.p); }

	// Releases and nullifies this pointer and returns its address
	ComClass** resetAndGetPointerAddress()
	{
		release();
		p = 0;
		return &p;
	}

	HRESULT CoCreateInstance (REFCLSID classUUID, DWORD dwClsContext = CLSCTX_INPROC_SERVER)
	{
	   #if ! JUCE_MINGW
		return ::CoCreateInstance (classUUID, 0, dwClsContext, __uuidof (ComClass), (void**) resetAndGetPointerAddress());
	   #else
		jassertfalse; // need to find a mingw equivalent of __uuidof to make this possible
		return E_NOTIMPL;
	   #endif
	}

	template <class OtherComClass>
	HRESULT QueryInterface (REFCLSID classUUID, ComSmartPtr<OtherComClass>& destObject) const
	{
		if (p == 0)
			return E_POINTER;

		return p->QueryInterface (classUUID, (void**) destObject.resetAndGetPointerAddress());
	}

	template <class OtherComClass>
	HRESULT QueryInterface (ComSmartPtr<OtherComClass>& destObject) const
	{
	   #if ! JUCE_MINGW
		return this->QueryInterface (__uuidof (OtherComClass), destObject);
	   #else
		jassertfalse; // need to find a mingw equivalent of __uuidof to make this possible
		return E_NOTIMPL;
	   #endif
	}

private:
	ComClass* p;

	void release()  { if (p != 0) p->Release(); }

	ComClass** operator&() throw(); // private to avoid it being used accidentally
};

#define JUCE_COMRESULT  HRESULT __stdcall

/** Handy base class for writing COM objects, providing ref-counting and a basic QueryInterface method.
*/
template <class ComClass>
class ComBaseClassHelper   : public ComClass
{
public:
	ComBaseClassHelper()  : refCount (1) {}
	virtual ~ComBaseClassHelper() {}

	JUCE_COMRESULT QueryInterface (REFIID refId, void** result)
	{
	   #if ! JUCE_MINGW
		if (refId == __uuidof (ComClass))   { AddRef(); *result = dynamic_cast <ComClass*> (this); return S_OK; }
	   #else
		jassertfalse; // need to find a mingw equivalent of __uuidof to make this possible
	   #endif

		if (refId == IID_IUnknown)          { AddRef(); *result = dynamic_cast <IUnknown*> (this); return S_OK; }

		*result = 0;
		return E_NOINTERFACE;
	}

	ULONG __stdcall AddRef()    { return ++refCount; }
	ULONG __stdcall Release()   { const ULONG r = --refCount; if (r == 0) delete this; return r; }

	void resetReferenceCount() noexcept     { refCount = 0; }

protected:
	ULONG refCount;
};

#endif   // __JUCE_WIN32_COMSMARTPTR_JUCEHEADER__

/*** End of inlined file: juce_win32_ComSmartPtr.h ***/



/*** Start of inlined file: juce_win32_Files.cpp ***/
#ifndef INVALID_FILE_ATTRIBUTES
 #define INVALID_FILE_ATTRIBUTES ((DWORD) -1)
#endif

namespace WindowsFileHelpers
{
	DWORD getAtts (const String& path)
	{
		return GetFileAttributes (path.toWideCharPointer());
	}

	int64 fileTimeToTime (const FILETIME* const ft)
	{
		static_jassert (sizeof (ULARGE_INTEGER) == sizeof (FILETIME)); // tell me if this fails!

		return (int64) ((reinterpret_cast<const ULARGE_INTEGER*> (ft)->QuadPart - literal64bit (116444736000000000)) / 10000);
	}

	FILETIME* timeToFileTime (const int64 time, FILETIME* const ft) noexcept
	{
		if (time <= 0)
			return nullptr;

		reinterpret_cast<ULARGE_INTEGER*> (ft)->QuadPart = (ULONGLONG) (time * 10000 + literal64bit (116444736000000000));
		return ft;
	}

	String getDriveFromPath (String path)
	{
		if (path.isNotEmpty() && path[1] == ':' && path[2] == 0)
			path << '\\';

		const size_t numBytes = CharPointer_UTF16::getBytesRequiredFor (path.getCharPointer()) + 4;
		HeapBlock<WCHAR> pathCopy;
		pathCopy.calloc (numBytes, 1);
		path.copyToUTF16 (pathCopy, (int) numBytes);

		if (PathStripToRoot (pathCopy))
			path = static_cast <const WCHAR*> (pathCopy);

		return path;
	}

	int64 getDiskSpaceInfo (const String& path, const bool total)
	{
		ULARGE_INTEGER spc, tot, totFree;

		if (GetDiskFreeSpaceEx (getDriveFromPath (path).toWideCharPointer(), &spc, &tot, &totFree))
			return total ? (int64) tot.QuadPart
						 : (int64) spc.QuadPart;

		return 0;
	}

	unsigned int getWindowsDriveType (const String& path)
	{
		return GetDriveType (getDriveFromPath (path).toWideCharPointer());
	}

	File getSpecialFolderPath (int type)
	{
		WCHAR path [MAX_PATH + 256];

		if (SHGetSpecialFolderPath (0, path, type, FALSE))
			return File (String (path));

		return File::nonexistent;
	}

	File getModuleFileName (HINSTANCE moduleHandle)
	{
		WCHAR dest [MAX_PATH + 256];
		dest[0] = 0;
		GetModuleFileName (moduleHandle, dest, (DWORD) numElementsInArray (dest));
		return File (String (dest));
	}

	Result getResultForLastError()
	{
		TCHAR messageBuffer [256] = { 0 };

		FormatMessage (FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
					   nullptr, GetLastError(), MAKELANGID (LANG_NEUTRAL, SUBLANG_DEFAULT),
					   messageBuffer, (DWORD) numElementsInArray (messageBuffer) - 1, nullptr);

		return Result::fail (String (messageBuffer));
	}
}

const juce_wchar File::separator = '\\';
const String File::separatorString ("\\");

bool File::exists() const
{
	return fullPath.isNotEmpty()
			&& WindowsFileHelpers::getAtts (fullPath) != INVALID_FILE_ATTRIBUTES;
}

bool File::existsAsFile() const
{
	return fullPath.isNotEmpty()
			&& (WindowsFileHelpers::getAtts (fullPath) & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

bool File::isDirectory() const
{
	const DWORD attr = WindowsFileHelpers::getAtts (fullPath);
	return ((attr & FILE_ATTRIBUTE_DIRECTORY) != 0) && (attr != INVALID_FILE_ATTRIBUTES);
}

bool File::hasWriteAccess() const
{
	if (exists())
		return (WindowsFileHelpers::getAtts (fullPath) & FILE_ATTRIBUTE_READONLY) == 0;

	// on windows, it seems that even read-only directories can still be written into,
	// so checking the parent directory's permissions would return the wrong result..
	return true;
}

bool File::setFileReadOnlyInternal (const bool shouldBeReadOnly) const
{
	const DWORD oldAtts = WindowsFileHelpers::getAtts (fullPath);

	if (oldAtts == INVALID_FILE_ATTRIBUTES)
		return false;

	const DWORD newAtts = shouldBeReadOnly ? (oldAtts |  FILE_ATTRIBUTE_READONLY)
										   : (oldAtts & ~FILE_ATTRIBUTE_READONLY);
	return newAtts == oldAtts
			|| SetFileAttributes (fullPath.toWideCharPointer(), newAtts) != FALSE;
}

bool File::isHidden() const
{
	return (WindowsFileHelpers::getAtts (fullPath) & FILE_ATTRIBUTE_HIDDEN) != 0;
}

bool File::deleteFile() const
{
	if (! exists())
		return true;

	return isDirectory() ? RemoveDirectory (fullPath.toWideCharPointer()) != 0
						 : DeleteFile (fullPath.toWideCharPointer()) != 0;
}

bool File::moveToTrash() const
{
	if (! exists())
		return true;

	// The string we pass in must be double null terminated..
	const size_t numBytes = CharPointer_UTF16::getBytesRequiredFor (fullPath.getCharPointer()) + 8;
	HeapBlock<WCHAR> doubleNullTermPath;
	doubleNullTermPath.calloc (numBytes, 1);
	fullPath.copyToUTF16 (doubleNullTermPath, (int) numBytes);

	SHFILEOPSTRUCT fos = { 0 };
	fos.wFunc = FO_DELETE;
	fos.pFrom = doubleNullTermPath;
	fos.fFlags = FOF_ALLOWUNDO | FOF_NOERRORUI | FOF_SILENT | FOF_NOCONFIRMATION
				   | FOF_NOCONFIRMMKDIR | FOF_RENAMEONCOLLISION;

	return SHFileOperation (&fos) == 0;
}

bool File::copyInternal (const File& dest) const
{
	return CopyFile (fullPath.toWideCharPointer(), dest.getFullPathName().toWideCharPointer(), false) != 0;
}

bool File::moveInternal (const File& dest) const
{
	return MoveFile (fullPath.toWideCharPointer(), dest.getFullPathName().toWideCharPointer()) != 0;
}

Result File::createDirectoryInternal (const String& fileName) const
{
	return CreateDirectory (fileName.toWideCharPointer(), 0) ? Result::ok()
															 : WindowsFileHelpers::getResultForLastError();
}

int64 juce_fileSetPosition (void* handle, int64 pos)
{
	LARGE_INTEGER li;
	li.QuadPart = pos;
	li.LowPart = SetFilePointer ((HANDLE) handle, (LONG) li.LowPart, &li.HighPart, FILE_BEGIN);  // (returns -1 if it fails)
	return li.QuadPart;
}

void FileInputStream::openHandle()
{
	totalSize = file.getSize();

	HANDLE h = CreateFile (file.getFullPathName().toWideCharPointer(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, 0,
						   OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, 0);

	if (h != INVALID_HANDLE_VALUE)
		fileHandle = (void*) h;
	else
		status = WindowsFileHelpers::getResultForLastError();
}

void FileInputStream::closeHandle()
{
	CloseHandle ((HANDLE) fileHandle);
}

size_t FileInputStream::readInternal (void* buffer, size_t numBytes)
{
	if (fileHandle != 0)
	{
		DWORD actualNum = 0;
		if (! ReadFile ((HANDLE) fileHandle, buffer, (DWORD) numBytes, &actualNum, 0))
			status = WindowsFileHelpers::getResultForLastError();

		return (size_t) actualNum;
	}

	return 0;
}

void FileOutputStream::openHandle()
{
	HANDLE h = CreateFile (file.getFullPathName().toWideCharPointer(), GENERIC_WRITE, FILE_SHARE_READ, 0,
						   OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);

	if (h != INVALID_HANDLE_VALUE)
	{
		LARGE_INTEGER li;
		li.QuadPart = 0;
		li.LowPart = SetFilePointer (h, 0, &li.HighPart, FILE_END);

		if (li.LowPart != INVALID_SET_FILE_POINTER)
		{
			fileHandle = (void*) h;
			currentPosition = li.QuadPart;
			return;
		}
	}

	status = WindowsFileHelpers::getResultForLastError();
}

void FileOutputStream::closeHandle()
{
	CloseHandle ((HANDLE) fileHandle);
}

int FileOutputStream::writeInternal (const void* buffer, int numBytes)
{
	if (fileHandle != nullptr)
	{
		DWORD actualNum = 0;
		if (! WriteFile ((HANDLE) fileHandle, buffer, (DWORD) numBytes, &actualNum, 0))
			status = WindowsFileHelpers::getResultForLastError();

		return (int) actualNum;
	}

	return 0;
}

void FileOutputStream::flushInternal()
{
	if (fileHandle != nullptr)
		if (! FlushFileBuffers ((HANDLE) fileHandle))
			status = WindowsFileHelpers::getResultForLastError();
}

Result FileOutputStream::truncate()
{
	if (fileHandle == nullptr)
		return status;

	flush();
	return SetEndOfFile ((HANDLE) fileHandle) ? Result::ok()
											  : WindowsFileHelpers::getResultForLastError();
}

MemoryMappedFile::MemoryMappedFile (const File& file, MemoryMappedFile::AccessMode mode)
	: address (nullptr),
	  length (0),
	  fileHandle (nullptr)
{
	jassert (mode == readOnly || mode == readWrite);

	DWORD accessMode = GENERIC_READ, createType = OPEN_EXISTING;
	DWORD protect = PAGE_READONLY, access = FILE_MAP_READ;

	if (mode == readWrite)
	{
		accessMode = GENERIC_READ | GENERIC_WRITE;
		createType = OPEN_ALWAYS;
		protect = PAGE_READWRITE;
		access = FILE_MAP_ALL_ACCESS;
	}

	HANDLE h = CreateFile (file.getFullPathName().toWideCharPointer(), accessMode, FILE_SHARE_READ, 0,
						   createType, FILE_ATTRIBUTE_NORMAL, 0);

	if (h != INVALID_HANDLE_VALUE)
	{
		fileHandle = (void*) h;
		const int64 fileSize = file.getSize();

		HANDLE mappingHandle = CreateFileMapping (h, 0, protect, (DWORD) (fileSize >> 32), (DWORD) fileSize, 0);
		if (mappingHandle != 0)
		{
			address = MapViewOfFile (mappingHandle, access, 0, 0, (SIZE_T) fileSize);

			if (address != nullptr)
				length = (size_t) fileSize;

			CloseHandle (mappingHandle);
		}
	}
}

MemoryMappedFile::~MemoryMappedFile()
{
	if (address != nullptr)
		UnmapViewOfFile (address);

	if (fileHandle != nullptr)
		CloseHandle ((HANDLE) fileHandle);
}

int64 File::getSize() const
{
	WIN32_FILE_ATTRIBUTE_DATA attributes;

	if (GetFileAttributesEx (fullPath.toWideCharPointer(), GetFileExInfoStandard, &attributes))
		return (((int64) attributes.nFileSizeHigh) << 32) | attributes.nFileSizeLow;

	return 0;
}

void File::getFileTimesInternal (int64& modificationTime, int64& accessTime, int64& creationTime) const
{
	using namespace WindowsFileHelpers;
	WIN32_FILE_ATTRIBUTE_DATA attributes;

	if (GetFileAttributesEx (fullPath.toWideCharPointer(), GetFileExInfoStandard, &attributes))
	{
		modificationTime = fileTimeToTime (&attributes.ftLastWriteTime);
		creationTime     = fileTimeToTime (&attributes.ftCreationTime);
		accessTime       = fileTimeToTime (&attributes.ftLastAccessTime);
	}
	else
	{
		creationTime = accessTime = modificationTime = 0;
	}
}

bool File::setFileTimesInternal (int64 modificationTime, int64 accessTime, int64 creationTime) const
{
	using namespace WindowsFileHelpers;

	bool ok = false;
	HANDLE h = CreateFile (fullPath.toWideCharPointer(), GENERIC_WRITE, FILE_SHARE_READ, 0,
						   OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);

	if (h != INVALID_HANDLE_VALUE)
	{
		FILETIME m, a, c;

		ok = SetFileTime (h,
						  timeToFileTime (creationTime, &c),
						  timeToFileTime (accessTime, &a),
						  timeToFileTime (modificationTime, &m)) != 0;

		CloseHandle (h);
	}

	return ok;
}

void File::findFileSystemRoots (Array<File>& destArray)
{
	TCHAR buffer [2048] = { 0 };
	GetLogicalDriveStrings (2048, buffer);

	const TCHAR* n = buffer;
	StringArray roots;

	while (*n != 0)
	{
		roots.add (String (n));

		while (*n++ != 0)
		{}
	}

	roots.sort (true);

	for (int i = 0; i < roots.size(); ++i)
		destArray.add (roots [i]);
}

String File::getVolumeLabel() const
{
	TCHAR dest[64];
	if (! GetVolumeInformation (WindowsFileHelpers::getDriveFromPath (getFullPathName()).toWideCharPointer(), dest,
								(DWORD) numElementsInArray (dest), 0, 0, 0, 0, 0))
		dest[0] = 0;

	return dest;
}

int File::getVolumeSerialNumber() const
{
	TCHAR dest[64];
	DWORD serialNum;

	if (! GetVolumeInformation (WindowsFileHelpers::getDriveFromPath (getFullPathName()).toWideCharPointer(), dest,
								(DWORD) numElementsInArray (dest), &serialNum, 0, 0, 0, 0))
		return 0;

	return (int) serialNum;
}

int64 File::getBytesFreeOnVolume() const
{
	return WindowsFileHelpers::getDiskSpaceInfo (getFullPathName(), false);
}

int64 File::getVolumeTotalSize() const
{
	return WindowsFileHelpers::getDiskSpaceInfo (getFullPathName(), true);
}

bool File::isOnCDRomDrive() const
{
	return WindowsFileHelpers::getWindowsDriveType (getFullPathName()) == DRIVE_CDROM;
}

bool File::isOnHardDisk() const
{
	if (fullPath.isEmpty())
		return false;

	const unsigned int n = WindowsFileHelpers::getWindowsDriveType (getFullPathName());

	if (fullPath.toLowerCase()[0] <= 'b' && fullPath[1] == ':')
		return n != DRIVE_REMOVABLE;
	else
		return n != DRIVE_CDROM && n != DRIVE_REMOTE;
}

bool File::isOnRemovableDrive() const
{
	if (fullPath.isEmpty())
		return false;

	const unsigned int n = WindowsFileHelpers::getWindowsDriveType (getFullPathName());

	return n == DRIVE_CDROM
		|| n == DRIVE_REMOTE
		|| n == DRIVE_REMOVABLE
		|| n == DRIVE_RAMDISK;
}

File JUCE_CALLTYPE File::getSpecialLocation (const SpecialLocationType type)
{
	int csidlType = 0;

	switch (type)
	{
		case userHomeDirectory:                 csidlType = CSIDL_PROFILE; break;
		case userDocumentsDirectory:            csidlType = CSIDL_PERSONAL; break;
		case userDesktopDirectory:              csidlType = CSIDL_DESKTOP; break;
		case userApplicationDataDirectory:      csidlType = CSIDL_APPDATA; break;
		case commonApplicationDataDirectory:    csidlType = CSIDL_COMMON_APPDATA; break;
		case globalApplicationsDirectory:       csidlType = CSIDL_PROGRAM_FILES; break;
		case userMusicDirectory:                csidlType = 0x0d /*CSIDL_MYMUSIC*/; break;
		case userMoviesDirectory:               csidlType = 0x0e /*CSIDL_MYVIDEO*/; break;

		case tempDirectory:
		{
			WCHAR dest [2048];
			dest[0] = 0;
			GetTempPath ((DWORD) numElementsInArray (dest), dest);
			return File (String (dest));
		}

		case invokedExecutableFile:
		case currentExecutableFile:
		case currentApplicationFile:
			return WindowsFileHelpers::getModuleFileName ((HINSTANCE) Process::getCurrentModuleInstanceHandle());

		case hostApplicationPath:
			return WindowsFileHelpers::getModuleFileName (0);

		default:
			jassertfalse; // unknown type?
			return File::nonexistent;
	}

	return WindowsFileHelpers::getSpecialFolderPath (csidlType);
}

File File::getCurrentWorkingDirectory()
{
	WCHAR dest [MAX_PATH + 256];
	dest[0] = 0;
	GetCurrentDirectory ((DWORD) numElementsInArray (dest), dest);
	return File (String (dest));
}

bool File::setAsCurrentWorkingDirectory() const
{
	return SetCurrentDirectory (getFullPathName().toWideCharPointer()) != FALSE;
}

String File::getVersion() const
{
	String result;

	DWORD handle = 0;
	DWORD bufferSize = GetFileVersionInfoSize (getFullPathName().toWideCharPointer(), &handle);
	HeapBlock<char> buffer;
	buffer.calloc (bufferSize);

	if (GetFileVersionInfo (getFullPathName().toWideCharPointer(), 0, bufferSize, buffer))
	{
		VS_FIXEDFILEINFO* vffi;
		UINT len = 0;

		if (VerQueryValue (buffer, (LPTSTR) _T("\\"), (LPVOID*) &vffi, &len))
		{
			result << (int) HIWORD (vffi->dwFileVersionMS) << '.'
				   << (int) LOWORD (vffi->dwFileVersionMS) << '.'
				   << (int) HIWORD (vffi->dwFileVersionLS) << '.'
				   << (int) LOWORD (vffi->dwFileVersionLS);
		}
	}

	return result;
}

File File::getLinkedTarget() const
{
	File result (*this);
	String p (getFullPathName());

	if (! exists())
		p += ".lnk";
	else if (getFileExtension() != ".lnk")
		return result;

	ComSmartPtr <IShellLink> shellLink;
	if (SUCCEEDED (shellLink.CoCreateInstance (CLSID_ShellLink)))
	{
		ComSmartPtr <IPersistFile> persistFile;
		if (SUCCEEDED (shellLink.QueryInterface (persistFile)))
		{
			if (SUCCEEDED (persistFile->Load (p.toWideCharPointer(), STGM_READ))
				 && SUCCEEDED (shellLink->Resolve (0, SLR_ANY_MATCH | SLR_NO_UI)))
			{
				WIN32_FIND_DATA winFindData;
				WCHAR resolvedPath [MAX_PATH];

				if (SUCCEEDED (shellLink->GetPath (resolvedPath, MAX_PATH, &winFindData, SLGP_UNCPRIORITY)))
					result = File (resolvedPath);
			}
		}
	}

	return result;
}

class DirectoryIterator::NativeIterator::Pimpl
{
public:
	Pimpl (const File& directory, const String& wildCard)
		: directoryWithWildCard (File::addTrailingSeparator (directory.getFullPathName()) + wildCard),
		  handle (INVALID_HANDLE_VALUE)
	{
	}

	~Pimpl()
	{
		if (handle != INVALID_HANDLE_VALUE)
			FindClose (handle);
	}

	bool next (String& filenameFound,
			   bool* const isDir, bool* const isHidden, int64* const fileSize,
			   Time* const modTime, Time* const creationTime, bool* const isReadOnly)
	{
		using namespace WindowsFileHelpers;
		WIN32_FIND_DATA findData;

		if (handle == INVALID_HANDLE_VALUE)
		{
			handle = FindFirstFile (directoryWithWildCard.toWideCharPointer(), &findData);

			if (handle == INVALID_HANDLE_VALUE)
				return false;
		}
		else
		{
			if (FindNextFile (handle, &findData) == 0)
				return false;
		}

		filenameFound = findData.cFileName;

		if (isDir != nullptr)         *isDir        = ((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0);
		if (isHidden != nullptr)      *isHidden     = ((findData.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) != 0);
		if (isReadOnly != nullptr)    *isReadOnly   = ((findData.dwFileAttributes & FILE_ATTRIBUTE_READONLY) != 0);
		if (fileSize != nullptr)      *fileSize     = findData.nFileSizeLow + (((int64) findData.nFileSizeHigh) << 32);
		if (modTime != nullptr)       *modTime      = Time (fileTimeToTime (&findData.ftLastWriteTime));
		if (creationTime != nullptr)  *creationTime = Time (fileTimeToTime (&findData.ftCreationTime));

		return true;
	}

private:
	const String directoryWithWildCard;
	HANDLE handle;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Pimpl);
};

DirectoryIterator::NativeIterator::NativeIterator (const File& directory, const String& wildCard)
	: pimpl (new DirectoryIterator::NativeIterator::Pimpl (directory, wildCard))
{
}

DirectoryIterator::NativeIterator::~NativeIterator()
{
}

bool DirectoryIterator::NativeIterator::next (String& filenameFound,
											  bool* const isDir, bool* const isHidden, int64* const fileSize,
											  Time* const modTime, Time* const creationTime, bool* const isReadOnly)
{
	return pimpl->next (filenameFound, isDir, isHidden, fileSize, modTime, creationTime, isReadOnly);
}

bool Process::openDocument (const String& fileName, const String& parameters)
{
	HINSTANCE hInstance = 0;

	JUCE_TRY
	{
		hInstance = ShellExecute (0, 0, fileName.toWideCharPointer(), parameters.toWideCharPointer(), 0, SW_SHOWDEFAULT);
	}
	JUCE_CATCH_ALL

	return hInstance > (HINSTANCE) 32;
}

void File::revealToUser() const
{
   #if JUCE_MINGW
	jassertfalse; // not supported in MinGW..
   #else
	#pragma warning (push)
	#pragma warning (disable: 4090) // (alignment warning)
	ITEMIDLIST* const itemIDList = ILCreateFromPath (fullPath.toWideCharPointer());
	#pragma warning (pop)

	if (itemIDList != nullptr)
	{
		SHOpenFolderAndSelectItems (itemIDList, 0, nullptr, 0);
		ILFree (itemIDList);
	}
   #endif
}

class NamedPipe::Pimpl
{
public:
	Pimpl (const String& file, const bool isPipe_)
		: pipeH (0),
		  cancelEvent (CreateEvent (0, FALSE, FALSE, 0)),
		  connected (false),
		  isPipe (isPipe_)
	{
		pipeH = isPipe ? CreateNamedPipe (file.toWideCharPointer(),
										  PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED, 0,
										  PIPE_UNLIMITED_INSTANCES, 4096, 4096, 0, 0)
					   : CreateFile (file.toWideCharPointer(),
									 GENERIC_READ | GENERIC_WRITE, 0, 0,
									 OPEN_EXISTING, FILE_FLAG_OVERLAPPED, 0);
	}

	~Pimpl()
	{
		disconnectPipe();

		if (pipeH != 0)
			CloseHandle (pipeH);

		CloseHandle (cancelEvent);
	}

	bool connect (const int timeOutMs)
	{
		if (! isPipe)
			return true;

		if (! connected)
		{
			OVERLAPPED over = { 0 };
			over.hEvent = CreateEvent (0, TRUE, FALSE, 0);

			if (ConnectNamedPipe (pipeH, &over))
			{
				connected = false;  // yes, you read that right. In overlapped mode it should always return 0.
			}
			else
			{
				const DWORD err = GetLastError();

				if (err == ERROR_IO_PENDING || err == ERROR_PIPE_LISTENING)
				{
					HANDLE handles[] = { over.hEvent, cancelEvent };

					if (WaitForMultipleObjects (2, handles, FALSE,
												timeOutMs >= 0 ? timeOutMs : INFINITE) == WAIT_OBJECT_0)
						connected = true;
				}
				else if (err == ERROR_PIPE_CONNECTED)
				{
					connected = true;
				}
			}

			CloseHandle (over.hEvent);
		}

		return connected;
	}

	void disconnectPipe()
	{
		if (connected)
		{
			DisconnectNamedPipe (pipeH);
			connected = false;
		}
	}

	bool isConnected() const noexcept  { return connected; }

	HANDLE pipeH, cancelEvent;
	bool connected, isPipe;
};

NamedPipe::NamedPipe()
{
}

NamedPipe::~NamedPipe()
{
	close();
}

bool NamedPipe::isOpen() const
{
	return pimpl != nullptr && pimpl->connected;
}

void NamedPipe::cancelPendingReads()
{
	if (pimpl != nullptr)
		SetEvent (pimpl->cancelEvent);
}

void NamedPipe::close()
{
	cancelPendingReads();

	const ScopedLock sl (lock);
	ScopedPointer<Pimpl> deleter (pimpl); // (clears the pimpl member variable before deleting it)
}

bool NamedPipe::openInternal (const String& pipeName, const bool createPipe)
{
	close();

	pimpl = new Pimpl ("\\\\.\\pipe\\" + File::createLegalFileName (pipeName), createPipe);

	if (pimpl->pipeH != INVALID_HANDLE_VALUE)
		return true;

	pimpl = nullptr;
	return false;
}

int NamedPipe::read (void* destBuffer, const int maxBytesToRead, const int timeOutMilliseconds)
{
	const ScopedLock sl (lock);
	int bytesRead = -1;
	bool waitAgain = true;

	while (waitAgain && pimpl != nullptr)
	{
		waitAgain = false;

		if (! pimpl->connect (timeOutMilliseconds))
			break;

		if (maxBytesToRead <= 0)
			return 0;

		OVERLAPPED over = { 0 };
		over.hEvent = CreateEvent (0, TRUE, FALSE, 0);

		unsigned long numRead;

		if (ReadFile (pimpl->pipeH, destBuffer, (DWORD) maxBytesToRead, &numRead, &over))
		{
			bytesRead = (int) numRead;
		}
		else
		{
			const DWORD lastError = GetLastError();

			if (lastError == ERROR_IO_PENDING)
			{
				HANDLE handles[] = { over.hEvent, pimpl->cancelEvent };
				DWORD waitResult = WaitForMultipleObjects (2, handles, FALSE,
														   timeOutMilliseconds >= 0 ? timeOutMilliseconds
																					: INFINITE);
				if (waitResult != WAIT_OBJECT_0)
				{
					// if the operation timed out, let's cancel it...
					CancelIo (pimpl->pipeH);
					WaitForSingleObject (over.hEvent, INFINITE);  // makes sure cancel is complete
				}

				if (GetOverlappedResult (pimpl->pipeH, &over, &numRead, FALSE))
				{
					bytesRead = (int) numRead;
				}
				else if ((GetLastError() == ERROR_BROKEN_PIPE || GetLastError() == ERROR_PIPE_NOT_CONNECTED) && pimpl->isPipe)
				{
					pimpl->disconnectPipe();
					waitAgain = true;
				}
			}
			else if (pimpl->isPipe)
			{
				waitAgain = true;

				if (lastError == ERROR_BROKEN_PIPE || lastError == ERROR_PIPE_NOT_CONNECTED)
					pimpl->disconnectPipe();
				else
					Sleep (5);
			}
		}

		CloseHandle (over.hEvent);
	}

	return bytesRead;
}

int NamedPipe::write (const void* sourceBuffer, int numBytesToWrite, int timeOutMilliseconds)
{
	int bytesWritten = -1;

	if (pimpl != nullptr && pimpl->connect (timeOutMilliseconds))
	{
		if (numBytesToWrite <= 0)
			return 0;

		OVERLAPPED over = { 0 };
		over.hEvent = CreateEvent (0, TRUE, FALSE, 0);

		unsigned long numWritten;

		if (WriteFile (pimpl->pipeH, sourceBuffer, (DWORD) numBytesToWrite, &numWritten, &over))
		{
			bytesWritten = (int) numWritten;
		}
		else if (GetLastError() == ERROR_IO_PENDING)
		{
			HANDLE handles[] = { over.hEvent, pimpl->cancelEvent };
			DWORD waitResult;

			waitResult = WaitForMultipleObjects (2, handles, FALSE,
												 timeOutMilliseconds >= 0 ? timeOutMilliseconds
																		  : INFINITE);

			if (waitResult != WAIT_OBJECT_0)
			{
				CancelIo (pimpl->pipeH);
				WaitForSingleObject (over.hEvent, INFINITE);
			}

			if (GetOverlappedResult (pimpl->pipeH, &over, &numWritten, FALSE))
			{
				bytesWritten = (int) numWritten;
			}
			else if (GetLastError() == ERROR_BROKEN_PIPE && pimpl->isPipe)
			{
				pimpl->disconnectPipe();
			}
		}

		CloseHandle (over.hEvent);
	}

	return bytesWritten;
}

/*** End of inlined file: juce_win32_Files.cpp ***/


/*** Start of inlined file: juce_win32_Network.cpp ***/
#ifndef INTERNET_FLAG_NEED_FILE
 #define INTERNET_FLAG_NEED_FILE 0x00000010
#endif

#ifndef INTERNET_OPTION_DISABLE_AUTODIAL
 #define INTERNET_OPTION_DISABLE_AUTODIAL 70
#endif

#ifndef WORKAROUND_TIMEOUT_BUG
 //#define WORKAROUND_TIMEOUT_BUG 1
#endif

#if WORKAROUND_TIMEOUT_BUG
// Required because of a Microsoft bug in setting a timeout
class InternetConnectThread  : public Thread
{
public:
	InternetConnectThread (URL_COMPONENTS& uc_, HINTERNET sessionHandle_, HINTERNET& connection_, const bool isFtp_)
		: Thread ("Internet"), uc (uc_), sessionHandle (sessionHandle_), connection (connection_), isFtp (isFtp_)
	{
		startThread();
	}

	~InternetConnectThread()
	{
		stopThread (60000);
	}

	void run()
	{
		connection = InternetConnect (sessionHandle, uc.lpszHostName,
									  uc.nPort, _T(""), _T(""),
									  isFtp ? INTERNET_SERVICE_FTP
											: INTERNET_SERVICE_HTTP,
									  0, 0);
		notify();
	}

private:
	URL_COMPONENTS& uc;
	HINTERNET sessionHandle;
	HINTERNET& connection;
	const bool isFtp;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (InternetConnectThread);
};
#endif

class WebInputStream  : public InputStream
{
public:
	WebInputStream (const String& address_, bool isPost_, const MemoryBlock& postData_,
					URL::OpenStreamProgressCallback* progressCallback, void* progressCallbackContext,
					const String& headers_, int timeOutMs_, StringPairArray* responseHeaders)
	  : connection (0), request (0),
		address (address_), headers (headers_), postData (postData_), position (0),
		finished (false), isPost (isPost_), timeOutMs (timeOutMs_)
	{
		createConnection (progressCallback, progressCallbackContext);

		if (responseHeaders != nullptr && ! isError())
		{
			DWORD bufferSizeBytes = 4096;

			for (;;)
			{
				HeapBlock<char> buffer ((size_t) bufferSizeBytes);

				if (HttpQueryInfo (request, HTTP_QUERY_RAW_HEADERS_CRLF, buffer.getData(), &bufferSizeBytes, 0))
				{
					StringArray headersArray;
					headersArray.addLines (reinterpret_cast <const WCHAR*> (buffer.getData()));

					for (int i = 0; i < headersArray.size(); ++i)
					{
						const String& header = headersArray[i];
						const String key (header.upToFirstOccurrenceOf (": ", false, false));
						const String value (header.fromFirstOccurrenceOf (": ", false, false));
						const String previousValue ((*responseHeaders) [key]);

						responseHeaders->set (key, previousValue.isEmpty() ? value : (previousValue + "," + value));
					}

					break;
				}

				if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
					break;
			}

		}
	}

	~WebInputStream()
	{
		close();
	}

	bool isError() const        { return request == 0; }
	bool isExhausted()          { return finished; }
	int64 getPosition()         { return position; }

	int64 getTotalLength()
	{
		if (! isError())
		{
			DWORD index = 0, result = 0, size = sizeof (result);

			if (HttpQueryInfo (request, HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER, &result, &size, &index))
				return (int64) result;
		}

		return -1;
	}

	int read (void* buffer, int bytesToRead)
	{
		jassert (buffer != nullptr && bytesToRead >= 0);
		DWORD bytesRead = 0;

		if (! (finished || isError()))
		{
			InternetReadFile (request, buffer, (DWORD) bytesToRead, &bytesRead);
			position += bytesRead;

			if (bytesRead == 0)
				finished = true;
		}

		return (int) bytesRead;
	}

	bool setPosition (int64 wantedPos)
	{
		if (isError())
			return false;

		if (wantedPos != position)
		{
			finished = false;
			position = (int64) InternetSetFilePointer (request, (LONG) wantedPos, 0, FILE_BEGIN, 0);

			if (position == wantedPos)
				return true;

			if (wantedPos < position)
			{
				close();
				position = 0;
				createConnection (0, 0);
			}

			skipNextBytes (wantedPos - position);
		}

		return true;
	}

private:

	HINTERNET connection, request;
	String address, headers;
	MemoryBlock postData;
	int64 position;
	bool finished;
	const bool isPost;
	int timeOutMs;

	void close()
	{
		if (request != 0)
		{
			InternetCloseHandle (request);
			request = 0;
		}

		if (connection != 0)
		{
			InternetCloseHandle (connection);
			connection = 0;
		}
	}

	void createConnection (URL::OpenStreamProgressCallback* progressCallback,
						   void* progressCallbackContext)
	{
		static HINTERNET sessionHandle = InternetOpen (_T("juce"), INTERNET_OPEN_TYPE_PRECONFIG, 0, 0, 0);

		close();

		if (sessionHandle != 0)
		{
			// break up the url..
			const int fileNumChars = 65536;
			const int serverNumChars = 2048;
			const int usernameNumChars = 1024;
			const int passwordNumChars = 1024;
			HeapBlock<TCHAR> file (fileNumChars), server (serverNumChars),
							 username (usernameNumChars), password (passwordNumChars);

			URL_COMPONENTS uc = { 0 };
			uc.dwStructSize = sizeof (uc);
			uc.lpszUrlPath = file;
			uc.dwUrlPathLength = fileNumChars;
			uc.lpszHostName = server;
			uc.dwHostNameLength = serverNumChars;
			uc.lpszUserName = username;
			uc.dwUserNameLength = usernameNumChars;
			uc.lpszPassword = password;
			uc.dwPasswordLength = passwordNumChars;

			if (InternetCrackUrl (address.toWideCharPointer(), 0, 0, &uc))
				openConnection (uc, sessionHandle, progressCallback, progressCallbackContext);
		}
	}

	void openConnection (URL_COMPONENTS& uc, HINTERNET sessionHandle,
						 URL::OpenStreamProgressCallback* progressCallback,
						 void* progressCallbackContext)
	{
		int disable = 1;
		InternetSetOption (sessionHandle, INTERNET_OPTION_DISABLE_AUTODIAL, &disable, sizeof (disable));

		if (timeOutMs == 0)
			timeOutMs = 30000;
		else if (timeOutMs < 0)
			timeOutMs = -1;

		InternetSetOption (sessionHandle, INTERNET_OPTION_CONNECT_TIMEOUT, &timeOutMs, sizeof (timeOutMs));

		const bool isFtp = address.startsWithIgnoreCase ("ftp:");

	  #if WORKAROUND_TIMEOUT_BUG
		connection = 0;

		{
			InternetConnectThread connectThread (uc, sessionHandle, connection, isFtp);
			connectThread.wait (timeOutMs);

			if (connection == 0)
			{
				InternetCloseHandle (sessionHandle);
				sessionHandle = 0;
			}
		}
	  #else
		connection = InternetConnect (sessionHandle, uc.lpszHostName, uc.nPort,
									  uc.lpszUserName, uc.lpszPassword,
									  isFtp ? (DWORD) INTERNET_SERVICE_FTP
											: (DWORD) INTERNET_SERVICE_HTTP,
									  0, 0);
	  #endif

		if (connection != 0)
		{
			if (isFtp)
				request = FtpOpenFile (connection, uc.lpszUrlPath, GENERIC_READ,
									   FTP_TRANSFER_TYPE_BINARY | INTERNET_FLAG_NEED_FILE, 0);
			else
				openHTTPConnection (uc, progressCallback, progressCallbackContext);
		}
	}

	void openHTTPConnection (URL_COMPONENTS& uc, URL::OpenStreamProgressCallback* progressCallback,
							 void* progressCallbackContext)
	{
		const TCHAR* mimeTypes[] = { _T("*/*"), 0 };

		DWORD flags = INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_NO_COOKIES;

		if (address.startsWithIgnoreCase ("https:"))
			flags |= INTERNET_FLAG_SECURE;  // (this flag only seems necessary if the OS is running IE6 -
											//  IE7 seems to automatically work out when it's https)

		request = HttpOpenRequest (connection, isPost ? _T("POST") : _T("GET"),
								   uc.lpszUrlPath, 0, 0, mimeTypes, flags, 0);

		if (request != 0)
		{
			INTERNET_BUFFERS buffers = { 0 };
			buffers.dwStructSize = sizeof (INTERNET_BUFFERS);
			buffers.lpcszHeader = headers.toWideCharPointer();
			buffers.dwHeadersLength = (DWORD) headers.length();
			buffers.dwBufferTotal = (DWORD) postData.getSize();

			if (HttpSendRequestEx (request, &buffers, 0, HSR_INITIATE, 0))
			{
				int bytesSent = 0;

				for (;;)
				{
					const int bytesToDo = jmin (1024, (int) postData.getSize() - bytesSent);
					DWORD bytesDone = 0;

					if (bytesToDo > 0
						 && ! InternetWriteFile (request,
												 static_cast <const char*> (postData.getData()) + bytesSent,
												 (DWORD) bytesToDo, &bytesDone))
					{
						break;
					}

					if (bytesToDo == 0 || (int) bytesDone < bytesToDo)
					{
						if (HttpEndRequest (request, 0, 0, 0))
							return;

						break;
					}

					bytesSent += bytesDone;

					if (progressCallback != nullptr
						  && ! progressCallback (progressCallbackContext, bytesSent, (int) postData.getSize()))
						break;
				}
			}
		}

		close();
	}

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WebInputStream);
};

InputStream* URL::createNativeStream (const String& address, bool isPost, const MemoryBlock& postData,
									  OpenStreamProgressCallback* progressCallback, void* progressCallbackContext,
									  const String& headers, const int timeOutMs, StringPairArray* responseHeaders)
{
	ScopedPointer <WebInputStream> wi (new WebInputStream (address, isPost, postData,
														   progressCallback, progressCallbackContext,
														   headers, timeOutMs, responseHeaders));

	return wi->isError() ? nullptr : wi.release();
}

namespace MACAddressHelpers
{
	void getViaGetAdaptersInfo (Array<MACAddress>& result)
	{
		DynamicLibrary dll ("iphlpapi.dll");
		JUCE_DLL_FUNCTION (GetAdaptersInfo, getAdaptersInfo, DWORD, dll, (PIP_ADAPTER_INFO, PULONG))

		if (getAdaptersInfo != nullptr)
		{
			ULONG len = sizeof (IP_ADAPTER_INFO);
			HeapBlock<IP_ADAPTER_INFO> adapterInfo (1);

			if (getAdaptersInfo (adapterInfo, &len) == ERROR_BUFFER_OVERFLOW)
				adapterInfo.malloc (len, 1);

			if (getAdaptersInfo (adapterInfo, &len) == NO_ERROR)
			{
				for (PIP_ADAPTER_INFO adapter = adapterInfo; adapter != nullptr; adapter = adapter->Next)
					if (adapter->AddressLength >= 6)
						result.addIfNotAlreadyThere (MACAddress (adapter->Address));
			}
		}
	}

	void getViaNetBios (Array<MACAddress>& result)
	{
		DynamicLibrary dll ("netapi32.dll");
		JUCE_DLL_FUNCTION (Netbios, NetbiosCall, UCHAR, dll, (PNCB))

		if (NetbiosCall != 0)
		{
			LANA_ENUM enums = { 0 };

			{
				NCB ncb = { 0 };
				ncb.ncb_command = NCBENUM;
				ncb.ncb_buffer = (unsigned char*) &enums;
				ncb.ncb_length = sizeof (LANA_ENUM);
				NetbiosCall (&ncb);
			}

			for (int i = 0; i < enums.length; ++i)
			{
				NCB ncb2 = { 0 };
				ncb2.ncb_command = NCBRESET;
				ncb2.ncb_lana_num = enums.lana[i];

				if (NetbiosCall (&ncb2) == 0)
				{
					NCB ncb = { 0 };
					memcpy (ncb.ncb_callname, "*                   ", NCBNAMSZ);
					ncb.ncb_command = NCBASTAT;
					ncb.ncb_lana_num = enums.lana[i];

					struct ASTAT
					{
						ADAPTER_STATUS adapt;
						NAME_BUFFER    NameBuff [30];
					};

					ASTAT astat = { 0 };
					ncb.ncb_buffer = (unsigned char*) &astat;
					ncb.ncb_length = sizeof (ASTAT);

					if (NetbiosCall (&ncb) == 0 && astat.adapt.adapter_type == 0xfe)
						result.addIfNotAlreadyThere (MACAddress (astat.adapt.adapter_address));
				}
			}
		}
	}
}

void MACAddress::findAllAddresses (Array<MACAddress>& result)
{
	MACAddressHelpers::getViaGetAdaptersInfo (result);
	MACAddressHelpers::getViaNetBios (result);
}

bool Process::openEmailWithAttachments (const String& targetEmailAddress,
										const String& emailSubject,
										const String& bodyText,
										const StringArray& filesToAttach)
{
	typedef ULONG (WINAPI *MAPISendMailType) (LHANDLE, ULONG, lpMapiMessage, ::FLAGS, ULONG);

	DynamicLibrary mapiLib ("MAPI32.dll");
	MAPISendMailType mapiSendMail = (MAPISendMailType) mapiLib.getFunction ("MAPISendMail");

	if (mapiSendMail == nullptr)
		return false;

	MapiMessage message = { 0 };
	message.lpszSubject = (LPSTR) emailSubject.toUTF8().getAddress();
	message.lpszNoteText = (LPSTR) bodyText.toUTF8().getAddress();

	MapiRecipDesc recip = { 0 };
	recip.ulRecipClass = MAPI_TO;
	String targetEmailAddress_ (targetEmailAddress);
	if (targetEmailAddress_.isEmpty())
		targetEmailAddress_ = " "; // (Windows Mail can't deal with a blank address)
	recip.lpszName = (LPSTR) targetEmailAddress_.toUTF8().getAddress();
	message.nRecipCount = 1;
	message.lpRecips = &recip;

	HeapBlock <MapiFileDesc> files;
	files.calloc ((size_t) filesToAttach.size());

	message.nFileCount = (ULONG) filesToAttach.size();
	message.lpFiles = files;

	for (int i = 0; i < filesToAttach.size(); ++i)
	{
		files[i].nPosition = (ULONG) -1;
		files[i].lpszPathName = (LPSTR) filesToAttach[i].toUTF8().getAddress();
	}

	return mapiSendMail (0, 0, &message, MAPI_DIALOG | MAPI_LOGON_UI, 0) == SUCCESS_SUCCESS;
}

/*** End of inlined file: juce_win32_Network.cpp ***/


/*** Start of inlined file: juce_win32_Registry.cpp ***/
struct RegistryKeyWrapper
{
	RegistryKeyWrapper (String name, const bool createForWriting, const DWORD wow64Flags)
		: key (0), wideCharValueName (nullptr)
	{
		HKEY rootKey = 0;

		if (name.startsWithIgnoreCase ("HKEY_CURRENT_USER\\"))        rootKey = HKEY_CURRENT_USER;
		else if (name.startsWithIgnoreCase ("HKEY_LOCAL_MACHINE\\"))  rootKey = HKEY_LOCAL_MACHINE;
		else if (name.startsWithIgnoreCase ("HKEY_CLASSES_ROOT\\"))   rootKey = HKEY_CLASSES_ROOT;

		if (rootKey != 0)
		{
			name = name.substring (name.indexOfChar ('\\') + 1);

			const int lastSlash = name.lastIndexOfChar ('\\');
			valueName = name.substring (lastSlash + 1);
			wideCharValueName = valueName.toWideCharPointer();

			name = name.substring (0, lastSlash);
			const wchar_t* const wideCharName = name.toWideCharPointer();
			DWORD result;

			if (createForWriting)
				RegCreateKeyEx (rootKey, wideCharName, 0, 0, REG_OPTION_NON_VOLATILE,
								KEY_WRITE | KEY_QUERY_VALUE | wow64Flags, 0, &key, &result);
			else
				RegOpenKeyEx (rootKey, wideCharName, 0, KEY_READ | wow64Flags, &key);
		}
	}

	~RegistryKeyWrapper()
	{
		if (key != 0)
			RegCloseKey (key);
	}

	static bool setValue (const String& regValuePath, const DWORD type,
						  const void* data, size_t dataSize)
	{
		const RegistryKeyWrapper key (regValuePath, true, 0);

		return key.key != 0
				&& RegSetValueEx (key.key, key.wideCharValueName, 0, type,
								  reinterpret_cast <const BYTE*> (data),
								  (DWORD) dataSize) == ERROR_SUCCESS;
	}

	static uint32 getBinaryValue (const String& regValuePath, MemoryBlock& result, DWORD wow64Flags)
	{
		const RegistryKeyWrapper key (regValuePath, false, wow64Flags);

		if (key.key != 0)
		{
			for (unsigned long bufferSize = 1024; ; bufferSize *= 2)
			{
				result.setSize (bufferSize, false);
				DWORD type = REG_NONE;

				const LONG err = RegQueryValueEx (key.key, key.wideCharValueName, 0, &type,
												  (LPBYTE) result.getData(), &bufferSize);

				if (err == ERROR_SUCCESS)
				{
					result.setSize (bufferSize, false);
					return type;
				}

				if (err != ERROR_MORE_DATA)
					break;
			}
		}

		return REG_NONE;
	}

	static String getValue (const String& regValuePath, const String& defaultValue, DWORD wow64Flags)
	{
		MemoryBlock buffer;
		switch (getBinaryValue (regValuePath, buffer, wow64Flags))
		{
			case REG_SZ:    return static_cast <const WCHAR*> (buffer.getData());
			case REG_DWORD: return String ((int) *reinterpret_cast<const DWORD*> (buffer.getData()));
			default:        break;
		}

		return defaultValue;
	}

	HKEY key;
	const wchar_t* wideCharValueName;
	String valueName;

	JUCE_DECLARE_NON_COPYABLE (RegistryKeyWrapper);
};

uint32 WindowsRegistry::getBinaryValue (const String& regValuePath, MemoryBlock& result)
{
	return RegistryKeyWrapper::getBinaryValue (regValuePath, result, 0);
}

String WindowsRegistry::getValue (const String& regValuePath, const String& defaultValue)
{
	return RegistryKeyWrapper::getValue (regValuePath, defaultValue, 0);
}

String WindowsRegistry::getValueWow64 (const String& regValuePath, const String& defaultValue)
{
	return RegistryKeyWrapper::getValue (regValuePath, defaultValue, KEY_WOW64_64KEY);
}

bool WindowsRegistry::setValue (const String& regValuePath, const String& value)
{
	return RegistryKeyWrapper::setValue (regValuePath, REG_SZ, value.toWideCharPointer(),
										 CharPointer_UTF16::getBytesRequiredFor (value.getCharPointer()));
}

bool WindowsRegistry::setValue (const String& regValuePath, const uint32 value)
{
	return RegistryKeyWrapper::setValue (regValuePath, REG_DWORD, &value, sizeof (value));
}

bool WindowsRegistry::setValue (const String& regValuePath, const uint64 value)
{
	return RegistryKeyWrapper::setValue (regValuePath, REG_QWORD, &value, sizeof (value));
}

bool WindowsRegistry::setValue (const String& regValuePath, const MemoryBlock& value)
{
	return RegistryKeyWrapper::setValue (regValuePath, REG_BINARY, value.getData(), value.getSize());
}

bool WindowsRegistry::valueExists (const String& regValuePath)
{
	const RegistryKeyWrapper key (regValuePath, false, 0);

	if (key.key == 0)
		return false;

	unsigned char buffer [512];
	unsigned long bufferSize = sizeof (buffer);
	DWORD type = 0;

	const LONG result = RegQueryValueEx (key.key, key.wideCharValueName,
										 0, &type, buffer, &bufferSize);

	return result == ERROR_SUCCESS || result == ERROR_MORE_DATA;
}

void WindowsRegistry::deleteValue (const String& regValuePath)
{
	const RegistryKeyWrapper key (regValuePath, true, 0);

	if (key.key != 0)
		RegDeleteValue (key.key, key.wideCharValueName);
}

void WindowsRegistry::deleteKey (const String& regKeyPath)
{
	const RegistryKeyWrapper key (regKeyPath, true, 0);

	if (key.key != 0)
		RegDeleteKey (key.key, key.wideCharValueName);
}

bool WindowsRegistry::registerFileAssociation (const String& fileExtension,
											   const String& symbolicDescription,
											   const String& fullDescription,
											   const File& targetExecutable,
											   const int iconResourceNumber,
											   const bool registerForCurrentUserOnly)
{
	const char* const root = registerForCurrentUserOnly ? "HKEY_CURRENT_USER\\Software\\Classes\\"
														: "HKEY_CLASSES_ROOT\\";
	const String key (root + symbolicDescription);

	return setValue (root + fileExtension + "\\", symbolicDescription)
		&& setValue (key + "\\", fullDescription)
		&& setValue (key + "\\shell\\open\\command\\", targetExecutable.getFullPathName() + " \"%1\"")
		&& (iconResourceNumber == 0
			  || setValue (key + "\\DefaultIcon\\",
						   targetExecutable.getFullPathName() + "," + String (-iconResourceNumber)));
}

/*** End of inlined file: juce_win32_Registry.cpp ***/


/*** Start of inlined file: juce_win32_SystemStats.cpp ***/
void Logger::outputDebugString (const String& text)
{
	OutputDebugString ((text + "\n").toWideCharPointer());
}

#ifdef JUCE_DLL
 JUCE_API void* juceDLL_malloc (size_t sz)    { return std::malloc (sz); }
 JUCE_API void  juceDLL_free (void* block)    { std::free (block); }
#endif

#if JUCE_USE_INTRINSICS

// CPU info functions using intrinsics...

#pragma intrinsic (__cpuid)
#pragma intrinsic (__rdtsc)

String SystemStats::getCpuVendor()
{
	int info [4];
	__cpuid (info, 0);

	char v [12];
	memcpy (v, info + 1, 4);
	memcpy (v + 4, info + 3, 4);
	memcpy (v + 8, info + 2, 4);

	return String (v, 12);
}

#else

// CPU info functions using old fashioned inline asm...

static void juce_getCpuVendor (char* const v)
{
	int vendor[4] = { 0 };

   #if ! JUCE_MINGW
	__try
   #endif
	{
	   #if JUCE_GCC
		unsigned int dummy = 0;
		__asm__ ("cpuid" : "=a" (dummy), "=b" (vendor[0]), "=c" (vendor[2]),"=d" (vendor[1]) : "a" (0));
	   #else
		__asm
		{
			mov eax, 0
			cpuid
			mov [vendor], ebx
			mov [vendor + 4], edx
			mov [vendor + 8], ecx
		}
	   #endif
	}
   #if ! JUCE_MINGW
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		*v = 0;
	}
   #endif

	memcpy (v, vendor, 16);
}

String SystemStats::getCpuVendor()
{
	char v [16];
	juce_getCpuVendor (v);
	return String (v, 16);
}
#endif

SystemStats::CPUFlags::CPUFlags()
{
	hasMMX   = IsProcessorFeaturePresent (PF_MMX_INSTRUCTIONS_AVAILABLE) != 0;
	hasSSE   = IsProcessorFeaturePresent (PF_XMMI_INSTRUCTIONS_AVAILABLE) != 0;
	hasSSE2  = IsProcessorFeaturePresent (PF_XMMI64_INSTRUCTIONS_AVAILABLE) != 0;
   #ifdef PF_AMD3D_INSTRUCTIONS_AVAILABLE
	has3DNow = IsProcessorFeaturePresent (PF_AMD3D_INSTRUCTIONS_AVAILABLE) != 0;
   #else
	has3DNow = IsProcessorFeaturePresent (PF_3DNOW_INSTRUCTIONS_AVAILABLE) != 0;
   #endif

	SYSTEM_INFO systemInfo;
	GetNativeSystemInfo (&systemInfo);
	numCpus = (int) systemInfo.dwNumberOfProcessors;
}

#if JUCE_MSVC && JUCE_CHECK_MEMORY_LEAKS
struct DebugFlagsInitialiser
{
	DebugFlagsInitialiser()
	{
		_CrtSetDbgFlag (_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	}
};

static DebugFlagsInitialiser debugFlagsInitialiser;
#endif

SystemStats::OperatingSystemType SystemStats::getOperatingSystemType()
{
	OSVERSIONINFO info;
	info.dwOSVersionInfoSize = sizeof (info);
	GetVersionEx (&info);

	if (info.dwPlatformId == VER_PLATFORM_WIN32_NT)
	{
		switch (info.dwMajorVersion)
		{
			case 5:     return (info.dwMinorVersion == 0) ? Win2000 : WinXP;
			case 6:     return (info.dwMinorVersion == 0) ? WinVista : Windows7;
			default:    jassertfalse; break; // !! not a supported OS!
		}
	}
	else if (info.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
	{
		jassert (info.dwMinorVersion != 0); // !! still running on Windows 95??
		return Win98;
	}

	return UnknownOS;
}

String SystemStats::getOperatingSystemName()
{
	const char* name = "Unknown OS";

	switch (getOperatingSystemType())
	{
		case Windows7:          name = "Windows 7";         break;
		case WinVista:          name = "Windows Vista";     break;
		case WinXP:             name = "Windows XP";        break;
		case Win2000:           name = "Windows 2000";      break;
		case Win98:             name = "Windows 98";        break;
		default:                jassertfalse; break; // !! new type of OS?
	}

	return name;
}

bool SystemStats::isOperatingSystem64Bit()
{
   #ifdef _WIN64
	return true;
   #else
	typedef BOOL (WINAPI* LPFN_ISWOW64PROCESS) (HANDLE, PBOOL);

	LPFN_ISWOW64PROCESS fnIsWow64Process = (LPFN_ISWOW64PROCESS) GetProcAddress (GetModuleHandle (_T("kernel32")), "IsWow64Process");

	BOOL isWow64 = FALSE;

	return fnIsWow64Process != 0
			&& fnIsWow64Process (GetCurrentProcess(), &isWow64)
			&& (isWow64 != FALSE);
   #endif
}

int SystemStats::getMemorySizeInMegabytes()
{
	MEMORYSTATUSEX mem;
	mem.dwLength = sizeof (mem);
	GlobalMemoryStatusEx (&mem);
	return (int) (mem.ullTotalPhys / (1024 * 1024)) + 1;
}

uint32 juce_millisecondsSinceStartup() noexcept
{
	return (uint32) timeGetTime();
}

class HiResCounterHandler
{
public:
	HiResCounterHandler()
		: hiResTicksOffset (0)
	{
		const MMRESULT res = timeBeginPeriod (1);
		(void) res;
		jassert (res == TIMERR_NOERROR);

		LARGE_INTEGER f;
		QueryPerformanceFrequency (&f);
		hiResTicksPerSecond = f.QuadPart;
		hiResTicksScaleFactor = 1000.0 / hiResTicksPerSecond;
	}

	inline int64 getHighResolutionTicks() noexcept
	{
		LARGE_INTEGER ticks;
		QueryPerformanceCounter (&ticks);

		const int64 mainCounterAsHiResTicks = (juce_millisecondsSinceStartup() * hiResTicksPerSecond) / 1000;
		const int64 newOffset = mainCounterAsHiResTicks - ticks.QuadPart;

		// fix for a very obscure PCI hardware bug that can make the counter
		// sometimes jump forwards by a few seconds..
		const int64 offsetDrift = abs64 (newOffset - hiResTicksOffset);

		if (offsetDrift > (hiResTicksPerSecond >> 1))
			hiResTicksOffset = newOffset;

		return ticks.QuadPart + hiResTicksOffset;
	}

	inline double getMillisecondCounterHiRes() noexcept
	{
		return getHighResolutionTicks() * hiResTicksScaleFactor;
	}

	int64 hiResTicksPerSecond, hiResTicksOffset;
	double hiResTicksScaleFactor;
};

static HiResCounterHandler hiResCounterHandler;

int64  Time::getHighResolutionTicksPerSecond() noexcept  { return hiResCounterHandler.hiResTicksPerSecond; }
int64  Time::getHighResolutionTicks() noexcept           { return hiResCounterHandler.getHighResolutionTicks(); }
double Time::getMillisecondCounterHiRes() noexcept       { return hiResCounterHandler.getMillisecondCounterHiRes(); }

static int64 juce_getClockCycleCounter() noexcept
{
   #if JUCE_USE_INTRINSICS
	// MS intrinsics version...
	return (int64) __rdtsc();

   #elif JUCE_GCC
	// GNU inline asm version...
	unsigned int hi = 0, lo = 0;

	__asm__ __volatile__ (
		"xor %%eax, %%eax               \n\
		 xor %%edx, %%edx               \n\
		 rdtsc                          \n\
		 movl %%eax, %[lo]              \n\
		 movl %%edx, %[hi]"
		 :
		 : [hi] "m" (hi),
		   [lo] "m" (lo)
		 : "cc", "eax", "ebx", "ecx", "edx", "memory");

	return (int64) ((((uint64) hi) << 32) | lo);
   #else
	// MSVC inline asm version...
	unsigned int hi = 0, lo = 0;

	__asm
	{
		xor eax, eax
		xor edx, edx
		rdtsc
		mov lo, eax
		mov hi, edx
	}

	return (int64) ((((uint64) hi) << 32) | lo);
   #endif
}

int SystemStats::getCpuSpeedInMegaherz()
{
	const int64 cycles = juce_getClockCycleCounter();
	const uint32 millis = Time::getMillisecondCounter();
	int lastResult = 0;

	for (;;)
	{
		int n = 1000000;
		while (--n > 0) {}

		const uint32 millisElapsed = Time::getMillisecondCounter() - millis;
		const int64 cyclesNow = juce_getClockCycleCounter();

		if (millisElapsed > 80)
		{
			const int newResult = (int) (((cyclesNow - cycles) / millisElapsed) / 1000);

			if (millisElapsed > 500 || (lastResult == newResult && newResult > 100))
				return newResult;

			lastResult = newResult;
		}
	}
}

bool Time::setSystemTimeToThisTime() const
{
	SYSTEMTIME st;

	st.wDayOfWeek = 0;
	st.wYear           = (WORD) getYear();
	st.wMonth          = (WORD) (getMonth() + 1);
	st.wDay            = (WORD) getDayOfMonth();
	st.wHour           = (WORD) getHours();
	st.wMinute         = (WORD) getMinutes();
	st.wSecond         = (WORD) getSeconds();
	st.wMilliseconds   = (WORD) (millisSinceEpoch % 1000);

	// do this twice because of daylight saving conversion problems - the
	// first one sets it up, the second one kicks it in.
	return SetLocalTime (&st) != 0
			&& SetLocalTime (&st) != 0;
}

int SystemStats::getPageSize()
{
	SYSTEM_INFO systemInfo;
	GetNativeSystemInfo (&systemInfo);

	return (int) systemInfo.dwPageSize;
}

String SystemStats::getLogonName()
{
	TCHAR text [256] = { 0 };
	DWORD len = (DWORD) numElementsInArray (text) - 1;
	GetUserName (text, &len);
	return String (text, len);
}

String SystemStats::getFullUserName()
{
	return getLogonName();
}

String SystemStats::getComputerName()
{
	TCHAR text [MAX_COMPUTERNAME_LENGTH + 1] = { 0 };
	DWORD len = (DWORD) numElementsInArray (text) - 1;
	GetComputerName (text, &len);
	return String (text, len);
}

/*** End of inlined file: juce_win32_SystemStats.cpp ***/


/*** Start of inlined file: juce_win32_Threads.cpp ***/
HWND juce_messageWindowHandle = 0;  // (this is used by other parts of the codebase)

#if ! JUCE_USE_INTRINSICS
// In newer compilers, the inline versions of these are used (in juce_Atomic.h), but in
// older ones we have to actually call the ops as win32 functions..
long juce_InterlockedExchange (volatile long* a, long b) noexcept                { return InterlockedExchange (a, b); }
long juce_InterlockedIncrement (volatile long* a) noexcept                       { return InterlockedIncrement (a); }
long juce_InterlockedDecrement (volatile long* a) noexcept                       { return InterlockedDecrement (a); }
long juce_InterlockedExchangeAdd (volatile long* a, long b) noexcept             { return InterlockedExchangeAdd (a, b); }
long juce_InterlockedCompareExchange (volatile long* a, long b, long c) noexcept { return InterlockedCompareExchange (a, b, c); }

__int64 juce_InterlockedCompareExchange64 (volatile __int64* value, __int64 newValue, __int64 valueToCompare) noexcept
{
	jassertfalse; // This operation isn't available in old MS compiler versions!

	__int64 oldValue = *value;
	if (oldValue == valueToCompare)
		*value = newValue;

	return oldValue;
}

#endif

CriticalSection::CriticalSection() noexcept
{
	// (just to check the MS haven't changed this structure and broken things...)
  #if JUCE_VC7_OR_EARLIER
	static_jassert (sizeof (CRITICAL_SECTION) <= 24);
  #else
	static_jassert (sizeof (CRITICAL_SECTION) <= sizeof (internal));
  #endif

	InitializeCriticalSection ((CRITICAL_SECTION*) internal);
}

CriticalSection::~CriticalSection() noexcept
{
	DeleteCriticalSection ((CRITICAL_SECTION*) internal);
}

void CriticalSection::enter() const noexcept
{
	EnterCriticalSection ((CRITICAL_SECTION*) internal);
}

bool CriticalSection::tryEnter() const noexcept
{
	return TryEnterCriticalSection ((CRITICAL_SECTION*) internal) != FALSE;
}

void CriticalSection::exit() const noexcept
{
	LeaveCriticalSection ((CRITICAL_SECTION*) internal);
}

WaitableEvent::WaitableEvent (const bool manualReset) noexcept
	: internal (CreateEvent (0, manualReset ? TRUE : FALSE, FALSE, 0))
{
}

WaitableEvent::~WaitableEvent() noexcept
{
	CloseHandle (internal);
}

bool WaitableEvent::wait (const int timeOutMillisecs) const noexcept
{
	return WaitForSingleObject (internal, (DWORD) timeOutMillisecs) == WAIT_OBJECT_0;
}

void WaitableEvent::signal() const noexcept
{
	SetEvent (internal);
}

void WaitableEvent::reset() const noexcept
{
	ResetEvent (internal);
}

void JUCE_API juce_threadEntryPoint (void*);

static unsigned int __stdcall threadEntryProc (void* userData)
{
	if (juce_messageWindowHandle != 0)
		AttachThreadInput (GetWindowThreadProcessId (juce_messageWindowHandle, 0),
						   GetCurrentThreadId(), TRUE);

	juce_threadEntryPoint (userData);

	_endthreadex (0);
	return 0;
}

void Thread::launchThread()
{
	unsigned int newThreadId;
	threadHandle = (void*) _beginthreadex (0, 0, &threadEntryProc, this, 0, &newThreadId);
	threadId = (ThreadID) newThreadId;
}

void Thread::closeThreadHandle()
{
	CloseHandle ((HANDLE) threadHandle);
	threadId = 0;
	threadHandle = 0;
}

void Thread::killThread()
{
	if (threadHandle != 0)
	{
	   #if JUCE_DEBUG
		OutputDebugStringA ("** Warning - Forced thread termination **\n");
	   #endif
		TerminateThread (threadHandle, 0);
	}
}

void Thread::setCurrentThreadName (const String& name)
{
   #if JUCE_DEBUG && JUCE_MSVC
	struct
	{
		DWORD dwType;
		LPCSTR szName;
		DWORD dwThreadID;
		DWORD dwFlags;
	} info;

	info.dwType = 0x1000;
	info.szName = name.toUTF8();
	info.dwThreadID = GetCurrentThreadId();
	info.dwFlags = 0;

	__try
	{
		RaiseException (0x406d1388 /*MS_VC_EXCEPTION*/, 0, sizeof (info) / sizeof (ULONG_PTR), (ULONG_PTR*) &info);
	}
	__except (EXCEPTION_CONTINUE_EXECUTION)
	{}
   #else
	(void) name;
   #endif
}

Thread::ThreadID Thread::getCurrentThreadId()
{
	return (ThreadID) (pointer_sized_int) GetCurrentThreadId();
}

bool Thread::setThreadPriority (void* handle, int priority)
{
	int pri = THREAD_PRIORITY_TIME_CRITICAL;

	if (priority < 1)       pri = THREAD_PRIORITY_IDLE;
	else if (priority < 2)  pri = THREAD_PRIORITY_LOWEST;
	else if (priority < 5)  pri = THREAD_PRIORITY_BELOW_NORMAL;
	else if (priority < 7)  pri = THREAD_PRIORITY_NORMAL;
	else if (priority < 9)  pri = THREAD_PRIORITY_ABOVE_NORMAL;
	else if (priority < 10) pri = THREAD_PRIORITY_HIGHEST;

	if (handle == 0)
		handle = GetCurrentThread();

	return SetThreadPriority (handle, pri) != FALSE;
}

void Thread::setCurrentThreadAffinityMask (const uint32 affinityMask)
{
	SetThreadAffinityMask (GetCurrentThread(), affinityMask);
}

struct SleepEvent
{
	SleepEvent()
		: handle (CreateEvent (0, 0, 0,
					#if JUCE_DEBUG
					   _T("Juce Sleep Event")))
					#else
					   0))
					#endif
	{
	}

	HANDLE handle;
};

static SleepEvent sleepEvent;

void JUCE_CALLTYPE Thread::sleep (const int millisecs)
{
	if (millisecs >= 10)
	{
		Sleep ((DWORD) millisecs);
	}
	else
	{
		// unlike Sleep() this is guaranteed to return to the current thread after
		// the time expires, so we'll use this for short waits, which are more likely
		// to need to be accurate
		WaitForSingleObject (sleepEvent.handle, (DWORD) millisecs);
	}
}

void Thread::yield()
{
	Sleep (0);
}

static int lastProcessPriority = -1;

// called by WindowDriver because Windows does wierd things to process priority
// when you swap apps, and this forces an update when the app is brought to the front.
void juce_repeatLastProcessPriority()
{
	if (lastProcessPriority >= 0) // (avoid changing this if it's not been explicitly set by the app..)
	{
		DWORD p;

		switch (lastProcessPriority)
		{
			case Process::LowPriority:          p = IDLE_PRIORITY_CLASS; break;
			case Process::NormalPriority:       p = NORMAL_PRIORITY_CLASS; break;
			case Process::HighPriority:         p = HIGH_PRIORITY_CLASS; break;
			case Process::RealtimePriority:     p = REALTIME_PRIORITY_CLASS; break;
			default:                            jassertfalse; return; // bad priority value
		}

		SetPriorityClass (GetCurrentProcess(), p);
	}
}

void Process::setPriority (ProcessPriority prior)
{
	if (lastProcessPriority != (int) prior)
	{
		lastProcessPriority = (int) prior;
		juce_repeatLastProcessPriority();
	}
}

JUCE_API bool JUCE_CALLTYPE juce_isRunningUnderDebugger()
{
	return IsDebuggerPresent() != FALSE;
}

bool JUCE_CALLTYPE Process::isRunningUnderDebugger()
{
	return juce_isRunningUnderDebugger();
}

String JUCE_CALLTYPE Process::getCurrentCommandLineParams()
{
	return CharacterFunctions::findEndOfToken (CharPointer_UTF16 (GetCommandLineW()),
											   CharPointer_UTF16 (L" "),
											   CharPointer_UTF16 (L"\"")).findEndOfWhitespace();
}

static void* currentModuleHandle = nullptr;

void* Process::getCurrentModuleInstanceHandle() noexcept
{
	if (currentModuleHandle == nullptr)
		currentModuleHandle = GetModuleHandle (0);

	return currentModuleHandle;
}

void Process::setCurrentModuleInstanceHandle (void* const newHandle) noexcept
{
	currentModuleHandle = newHandle;
}

void Process::raisePrivilege()
{
	jassertfalse; // xxx not implemented
}

void Process::lowerPrivilege()
{
	jassertfalse; // xxx not implemented
}

void Process::terminate()
{
   #if JUCE_MSVC && JUCE_CHECK_MEMORY_LEAKS
	_CrtDumpMemoryLeaks();
   #endif

	// bullet in the head in case there's a problem shutting down..
	ExitProcess (0);
}

bool juce_IsRunningInWine()
{
	HMODULE ntdll = GetModuleHandle (_T("ntdll.dll"));
	return ntdll != 0 && GetProcAddress (ntdll, "wine_get_version") != 0;
}

bool DynamicLibrary::open (const String& name)
{
	close();

	JUCE_TRY
	{
		handle = LoadLibrary (name.toWideCharPointer());
	}
	JUCE_CATCH_ALL

	return handle != nullptr;
}

void DynamicLibrary::close()
{
	JUCE_TRY
	{
		if (handle != nullptr)
		{
			FreeLibrary ((HMODULE) handle);
			handle = nullptr;
		}
	}
	JUCE_CATCH_ALL
}

void* DynamicLibrary::getFunction (const String& functionName) noexcept
{
	return handle != nullptr ? (void*) GetProcAddress ((HMODULE) handle, functionName.toUTF8()) // (void* cast is required for mingw)
							 : nullptr;
}

class InterProcessLock::Pimpl
{
public:
	Pimpl (String name, const int timeOutMillisecs)
		: handle (0), refCount (1)
	{
		name = name.replaceCharacter ('\\', '/');
		handle = CreateMutexW (0, TRUE, ("Global\\" + name).toWideCharPointer());

		// Not 100% sure why a global mutex sometimes can't be allocated, but if it fails, fall back to
		// a local one. (A local one also sometimes fails on other machines so neither type appears to be
		// universally reliable)
		if (handle == 0)
			handle = CreateMutexW (0, TRUE, ("Local\\" + name).toWideCharPointer());

		if (handle != 0 && GetLastError() == ERROR_ALREADY_EXISTS)
		{
			if (timeOutMillisecs == 0)
			{
				close();
				return;
			}

			switch (WaitForSingleObject (handle, timeOutMillisecs < 0 ? INFINITE : timeOutMillisecs))
			{
				case WAIT_OBJECT_0:
				case WAIT_ABANDONED:
					break;

				case WAIT_TIMEOUT:
				default:
					close();
					break;
			}
		}
	}

	~Pimpl()
	{
		close();
	}

	void close()
	{
		if (handle != 0)
		{
			ReleaseMutex (handle);
			CloseHandle (handle);
			handle = 0;
		}
	}

	HANDLE handle;
	int refCount;
};

InterProcessLock::InterProcessLock (const String& name_)
	: name (name_)
{
}

InterProcessLock::~InterProcessLock()
{
}

bool InterProcessLock::enter (const int timeOutMillisecs)
{
	const ScopedLock sl (lock);

	if (pimpl == nullptr)
	{
		pimpl = new Pimpl (name, timeOutMillisecs);

		if (pimpl->handle == 0)
			pimpl = nullptr;
	}
	else
	{
		pimpl->refCount++;
	}

	return pimpl != nullptr;
}

void InterProcessLock::exit()
{
	const ScopedLock sl (lock);

	// Trying to release the lock too many times!
	jassert (pimpl != nullptr);

	if (pimpl != nullptr && --(pimpl->refCount) == 0)
		pimpl = nullptr;
}

class ChildProcess::ActiveProcess
{
public:
	ActiveProcess (const String& command)
		: ok (false), readPipe (0), writePipe (0)
	{
		SECURITY_ATTRIBUTES securityAtts = { 0 };
		securityAtts.nLength = sizeof (securityAtts);
		securityAtts.bInheritHandle = TRUE;

		if (CreatePipe (&readPipe, &writePipe, &securityAtts, 0)
			 && SetHandleInformation (readPipe, HANDLE_FLAG_INHERIT, 0))
		{
			STARTUPINFOW startupInfo = { 0 };
			startupInfo.cb = sizeof (startupInfo);
			startupInfo.hStdError  = writePipe;
			startupInfo.hStdOutput = writePipe;
			startupInfo.dwFlags = STARTF_USESTDHANDLES;

			ok = CreateProcess (nullptr, const_cast <LPWSTR> (command.toWideCharPointer()),
								nullptr, nullptr, TRUE, CREATE_NO_WINDOW | CREATE_UNICODE_ENVIRONMENT,
								nullptr, nullptr, &startupInfo, &processInfo) != FALSE;
		}
	}

	~ActiveProcess()
	{
		if (ok)
		{
			CloseHandle (processInfo.hThread);
			CloseHandle (processInfo.hProcess);
		}

		if (readPipe != 0)
			CloseHandle (readPipe);

		if (writePipe != 0)
			CloseHandle (writePipe);
	}

	bool isRunning() const
	{
		return WaitForSingleObject (processInfo.hProcess, 0) != WAIT_OBJECT_0;
	}

	int read (void* dest, int numNeeded) const
	{
		int total = 0;

		while (ok && numNeeded > 0)
		{
			DWORD available = 0;

			if (! PeekNamedPipe ((HANDLE) readPipe, nullptr, 0, nullptr, &available, nullptr))
				break;

			const int numToDo = jmin ((int) available, numNeeded);

			if (available == 0)
			{
				if (! isRunning())
					break;

				Thread::yield();
			}
			else
			{
				DWORD numRead = 0;
				if (! ReadFile ((HANDLE) readPipe, dest, numToDo, &numRead, nullptr))
					break;

				total += numRead;
				dest = addBytesToPointer (dest, numRead);
				numNeeded -= numRead;
			}
		}

		return total;
	}

	bool killProcess() const
	{
		return TerminateProcess (processInfo.hProcess, 0) != FALSE;
	}

	bool ok;

private:
	HANDLE readPipe, writePipe;
	PROCESS_INFORMATION processInfo;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ActiveProcess);
};

bool ChildProcess::start (const String& command)
{
	activeProcess = new ActiveProcess (command);

	if (! activeProcess->ok)
		activeProcess = nullptr;

	return activeProcess != nullptr;
}

bool ChildProcess::isRunning() const
{
	return activeProcess != nullptr && activeProcess->isRunning();
}

int ChildProcess::readProcessOutput (void* dest, int numBytes)
{
	return activeProcess != nullptr ? activeProcess->read (dest, numBytes) : 0;
}

bool ChildProcess::kill()
{
	return activeProcess == nullptr || activeProcess->killProcess();
}

/*** End of inlined file: juce_win32_Threads.cpp ***/

#elif JUCE_LINUX

/*** Start of inlined file: juce_linux_Files.cpp ***/
enum
{
	U_ISOFS_SUPER_MAGIC = 0x9660,   // linux/iso_fs.h
	U_MSDOS_SUPER_MAGIC = 0x4d44,   // linux/msdos_fs.h
	U_NFS_SUPER_MAGIC = 0x6969,     // linux/nfs_fs.h
	U_SMB_SUPER_MAGIC = 0x517B      // linux/smb_fs.h
};

bool File::copyInternal (const File& dest) const
{
	FileInputStream in (*this);

	if (dest.deleteFile())
	{
		{
			FileOutputStream out (dest);

			if (out.failedToOpen())
				return false;

			if (out.writeFromInputStream (in, -1) == getSize())
				return true;
		}

		dest.deleteFile();
	}

	return false;
}

void File::findFileSystemRoots (Array<File>& destArray)
{
	destArray.add (File ("/"));
}

bool File::isOnCDRomDrive() const
{
	struct statfs buf;

	return statfs (getFullPathName().toUTF8(), &buf) == 0
			 && buf.f_type == (short) U_ISOFS_SUPER_MAGIC;
}

bool File::isOnHardDisk() const
{
	struct statfs buf;

	if (statfs (getFullPathName().toUTF8(), &buf) == 0)
	{
		switch (buf.f_type)
		{
			case U_ISOFS_SUPER_MAGIC:   // CD-ROM
			case U_MSDOS_SUPER_MAGIC:   // Probably floppy (but could be mounted FAT filesystem)
			case U_NFS_SUPER_MAGIC:     // Network NFS
			case U_SMB_SUPER_MAGIC:     // Network Samba
				return false;

			default:
				// Assume anything else is a hard-disk (but note it could
				// be a RAM disk.  There isn't a good way of determining
				// this for sure)
				return true;
		}
	}

	// Assume so if this fails for some reason
	return true;
}

bool File::isOnRemovableDrive() const
{
	jassertfalse; // xxx not implemented for linux!
	return false;
}

bool File::isHidden() const
{
	return getFileName().startsWithChar ('.');
}

namespace
{
	File juce_readlink (const String& file, const File& defaultFile)
	{
		const int size = 8192;
		HeapBlock<char> buffer;
		buffer.malloc (size + 4);

		const size_t numBytes = readlink (file.toUTF8(), buffer, size);

		if (numBytes > 0 && numBytes <= size)
			return File (file).getSiblingFile (String::fromUTF8 (buffer, (int) numBytes));

		return defaultFile;
	}
}

File File::getLinkedTarget() const
{
	return juce_readlink (getFullPathName().toUTF8(), *this);
}

const char* juce_Argv0 = nullptr;  // referenced from juce_Application.cpp

File File::getSpecialLocation (const SpecialLocationType type)
{
	switch (type)
	{
	case userHomeDirectory:
	{
		const char* homeDir = getenv ("HOME");

		if (homeDir == nullptr)
		{
			struct passwd* const pw = getpwuid (getuid());
			if (pw != nullptr)
				homeDir = pw->pw_dir;
		}

		return File (CharPointer_UTF8 (homeDir));
	}

	case userDocumentsDirectory:
	case userMusicDirectory:
	case userMoviesDirectory:
	case userApplicationDataDirectory:
		return File ("~");

	case userDesktopDirectory:
		return File ("~/Desktop");

	case commonApplicationDataDirectory:
		return File ("/var");

	case globalApplicationsDirectory:
		return File ("/usr");

	case tempDirectory:
	{
		File tmp ("/var/tmp");

		if (! tmp.isDirectory())
		{
			tmp = "/tmp";

			if (! tmp.isDirectory())
				tmp = File::getCurrentWorkingDirectory();
		}

		return tmp;
	}

	case invokedExecutableFile:
		if (juce_Argv0 != nullptr)
			return File (CharPointer_UTF8 (juce_Argv0));
		// deliberate fall-through...

	case currentExecutableFile:
	case currentApplicationFile:
		return juce_getExecutableFile();

	case hostApplicationPath:
		return juce_readlink ("/proc/self/exe", juce_getExecutableFile());

	default:
		jassertfalse; // unknown type?
		break;
	}

	return File::nonexistent;
}

String File::getVersion() const
{
	return String::empty; // xxx not yet implemented
}

bool File::moveToTrash() const
{
	if (! exists())
		return true;

	File trashCan ("~/.Trash");

	if (! trashCan.isDirectory())
		trashCan = "~/.local/share/Trash/files";

	if (! trashCan.isDirectory())
		return false;

	return moveFileTo (trashCan.getNonexistentChildFile (getFileNameWithoutExtension(),
														 getFileExtension()));
}

class DirectoryIterator::NativeIterator::Pimpl
{
public:
	Pimpl (const File& directory, const String& wildCard_)
		: parentDir (File::addTrailingSeparator (directory.getFullPathName())),
		  wildCard (wildCard_),
		  dir (opendir (directory.getFullPathName().toUTF8()))
	{
	}

	~Pimpl()
	{
		if (dir != nullptr)
			closedir (dir);
	}

	bool next (String& filenameFound,
			   bool* const isDir, bool* const isHidden, int64* const fileSize,
			   Time* const modTime, Time* const creationTime, bool* const isReadOnly)
	{
		if (dir != nullptr)
		{
			const char* wildcardUTF8 = nullptr;

			for (;;)
			{
				struct dirent* const de = readdir (dir);

				if (de == nullptr)
					break;

				if (wildcardUTF8 == nullptr)
					wildcardUTF8 = wildCard.toUTF8();

				if (fnmatch (wildcardUTF8, de->d_name, FNM_CASEFOLD) == 0)
				{
					filenameFound = CharPointer_UTF8 (de->d_name);

					updateStatInfoForFile (parentDir + filenameFound, isDir, fileSize, modTime, creationTime, isReadOnly);

					if (isHidden != nullptr)
						*isHidden = filenameFound.startsWithChar ('.');

					return true;
				}
			}
		}

		return false;
	}

private:
	String parentDir, wildCard;
	DIR* dir;

	JUCE_DECLARE_NON_COPYABLE (Pimpl);
};

DirectoryIterator::NativeIterator::NativeIterator (const File& directory, const String& wildCard)
	: pimpl (new DirectoryIterator::NativeIterator::Pimpl (directory, wildCard))
{
}

DirectoryIterator::NativeIterator::~NativeIterator()
{
}

bool DirectoryIterator::NativeIterator::next (String& filenameFound,
											  bool* const isDir, bool* const isHidden, int64* const fileSize,
											  Time* const modTime, Time* const creationTime, bool* const isReadOnly)
{
	return pimpl->next (filenameFound, isDir, isHidden, fileSize, modTime, creationTime, isReadOnly);
}

bool Process::openDocument (const String& fileName, const String& parameters)
{
	String cmdString (fileName.replace (" ", "\\ ",false));
	cmdString << " " << parameters;

	if (URL::isProbablyAWebsiteURL (fileName)
		 || cmdString.startsWithIgnoreCase ("file:")
		 || URL::isProbablyAnEmailAddress (fileName))
	{
		// create a command that tries to launch a bunch of likely browsers
		const char* const browserNames[] = { "xdg-open", "/etc/alternatives/x-www-browser", "firefox", "mozilla", "konqueror", "opera" };

		StringArray cmdLines;

		for (int i = 0; i < numElementsInArray (browserNames); ++i)
			cmdLines.add (String (browserNames[i]) + " " + cmdString.trim().quoted());

		cmdString = cmdLines.joinIntoString (" || ");
	}

	const char* const argv[4] = { "/bin/sh", "-c", cmdString.toUTF8(), 0 };

	const int cpid = fork();

	if (cpid == 0)
	{
		setsid();

		// Child process
		execve (argv[0], (char**) argv, environ);
		exit (0);
	}

	return cpid >= 0;
}

void File::revealToUser() const
{
	if (isDirectory())
		startAsProcess();
	else if (getParentDirectory().exists())
		getParentDirectory().startAsProcess();
}

/*** End of inlined file: juce_linux_Files.cpp ***/



/*** Start of inlined file: juce_linux_Network.cpp ***/
void MACAddress::findAllAddresses (Array<MACAddress>& result)
{
	const int s = socket (AF_INET, SOCK_DGRAM, 0);
	if (s != -1)
	{
		char buf [1024];
		struct ifconf ifc;
		ifc.ifc_len = sizeof (buf);
		ifc.ifc_buf = buf;
		ioctl (s, SIOCGIFCONF, &ifc);

		for (unsigned int i = 0; i < ifc.ifc_len / sizeof (struct ifreq); ++i)
		{
			struct ifreq ifr;
			strcpy (ifr.ifr_name, ifc.ifc_req[i].ifr_name);

			if (ioctl (s, SIOCGIFFLAGS, &ifr) == 0
				 && (ifr.ifr_flags & IFF_LOOPBACK) == 0
				 && ioctl (s, SIOCGIFHWADDR, &ifr) == 0)
			{
				result.addIfNotAlreadyThere (MACAddress ((const uint8*) ifr.ifr_hwaddr.sa_data));
			}
		}

		close (s);
	}
}

bool Process::openEmailWithAttachments (const String& targetEmailAddress,
										const String& emailSubject,
										const String& bodyText,
										const StringArray& filesToAttach)
{
	jassertfalse;    // xxx todo

	return false;
}

class WebInputStream  : public InputStream
{
public:
	WebInputStream (const String& address_, bool isPost_, const MemoryBlock& postData_,
					URL::OpenStreamProgressCallback* progressCallback, void* progressCallbackContext,
					const String& headers_, int timeOutMs_, StringPairArray* responseHeaders)
	  : socketHandle (-1), levelsOfRedirection (0),
		address (address_), headers (headers_), postData (postData_), position (0),
		finished (false), isPost (isPost_), timeOutMs (timeOutMs_)
	{
		createConnection (progressCallback, progressCallbackContext);

		if (responseHeaders != nullptr && ! isError())
		{
			for (int i = 0; i < headerLines.size(); ++i)
			{
				const String& headersEntry = headerLines[i];
				const String key (headersEntry.upToFirstOccurrenceOf (": ", false, false));
				const String value (headersEntry.fromFirstOccurrenceOf (": ", false, false));
				const String previousValue ((*responseHeaders) [key]);
				responseHeaders->set (key, previousValue.isEmpty() ? value : (previousValue + "," + value));
			}
		}
	}

	~WebInputStream()
	{
		closeSocket();
	}

	bool isError() const        { return socketHandle < 0; }
	bool isExhausted()          { return finished; }
	int64 getPosition()         { return position; }

	int64 getTotalLength()
	{
		//xxx to do
		return -1;
	}

	int read (void* buffer, int bytesToRead)
	{
		if (finished || isError())
			return 0;

		fd_set readbits;
		FD_ZERO (&readbits);
		FD_SET (socketHandle, &readbits);

		struct timeval tv;
		tv.tv_sec = jmax (1, timeOutMs / 1000);
		tv.tv_usec = 0;

		if (select (socketHandle + 1, &readbits, 0, 0, &tv) <= 0)
			return 0;   // (timeout)

		const int bytesRead = jmax (0, (int) recv (socketHandle, buffer, bytesToRead, MSG_WAITALL));
		if (bytesRead == 0)
			finished = true;
		position += bytesRead;
		return bytesRead;
	}

	bool setPosition (int64 wantedPos)
	{
		if (isError())
			return false;

		if (wantedPos != position)
		{
			finished = false;

			if (wantedPos < position)
			{
				closeSocket();
				position = 0;
				createConnection (0, 0);
			}

			skipNextBytes (wantedPos - position);
		}

		return true;
	}

private:
	int socketHandle, levelsOfRedirection;
	StringArray headerLines;
	String address, headers;
	MemoryBlock postData;
	int64 position;
	bool finished;
	const bool isPost;
	const int timeOutMs;

	void closeSocket()
	{
		if (socketHandle >= 0)
			close (socketHandle);

		socketHandle = -1;
		levelsOfRedirection = 0;
	}

	void createConnection (URL::OpenStreamProgressCallback* progressCallback, void* progressCallbackContext)
	{
		closeSocket();

		uint32 timeOutTime = Time::getMillisecondCounter();

		if (timeOutMs == 0)
			timeOutTime += 60000;
		else if (timeOutMs < 0)
			timeOutTime = 0xffffffff;
		else
			timeOutTime += timeOutMs;

		String hostName, hostPath;
		int hostPort;
		if (! decomposeURL (address, hostName, hostPath, hostPort))
			return;

		String serverName, proxyName, proxyPath;
		int proxyPort = 0;
		int port = 0;

		const String proxyURL (getenv ("http_proxy"));
		if (proxyURL.startsWithIgnoreCase ("http://"))
		{
			if (! decomposeURL (proxyURL, proxyName, proxyPath, proxyPort))
				return;

			serverName = proxyName;
			port = proxyPort;
		}
		else
		{
			serverName = hostName;
			port = hostPort;
		}

		struct addrinfo hints = { 0 };
		hints.ai_family = AF_UNSPEC;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_flags = AI_NUMERICSERV;

		struct addrinfo* result = nullptr;
		if (getaddrinfo (serverName.toUTF8(), String (port).toUTF8(), &hints, &result) != 0 || result == 0)
			return;

		socketHandle = socket (result->ai_family, result->ai_socktype, 0);

		if (socketHandle == -1)
		{
			freeaddrinfo (result);
			return;
		}

		int receiveBufferSize = 16384;
		setsockopt (socketHandle, SOL_SOCKET, SO_RCVBUF, (char*) &receiveBufferSize, sizeof (receiveBufferSize));
		setsockopt (socketHandle, SOL_SOCKET, SO_KEEPALIVE, 0, 0);

	  #if JUCE_MAC
		setsockopt (socketHandle, SOL_SOCKET, SO_NOSIGPIPE, 0, 0);
	  #endif

		if (connect (socketHandle, result->ai_addr, result->ai_addrlen) == -1)
		{
			closeSocket();
			freeaddrinfo (result);
			return;
		}

		freeaddrinfo (result);

		{
			const MemoryBlock requestHeader (createRequestHeader (hostName, hostPort, proxyName, proxyPort,
																  hostPath, address, headers, postData, isPost));

			if (! sendHeader (socketHandle, requestHeader, timeOutTime, progressCallback, progressCallbackContext))
			{
				closeSocket();
				return;
			}
		}

		String responseHeader (readResponse (socketHandle, timeOutTime));

		if (responseHeader.isNotEmpty())
		{
			headerLines.clear();
			headerLines.addLines (responseHeader);

			const int statusCode = responseHeader.fromFirstOccurrenceOf (" ", false, false)
												 .substring (0, 3).getIntValue();

			//int contentLength = findHeaderItem (lines, "Content-Length:").getIntValue();
			//bool isChunked = findHeaderItem (lines, "Transfer-Encoding:").equalsIgnoreCase ("chunked");

			String location (findHeaderItem (headerLines, "Location:"));

			if (statusCode >= 300 && statusCode < 400 && location.isNotEmpty())
			{
				if (! location.startsWithIgnoreCase ("http://"))
					location = "http://" + location;

				if (++levelsOfRedirection <= 3)
				{
					address = location;
					createConnection (progressCallback, progressCallbackContext);
					return;
				}
			}
			else
			{
				levelsOfRedirection = 0;
				return;
			}
		}

		closeSocket();
	}

	static String readResponse (const int socketHandle, const uint32 timeOutTime)
	{
		int bytesRead = 0, numConsecutiveLFs  = 0;
		MemoryBlock buffer (1024, true);

		while (numConsecutiveLFs < 2 && bytesRead < 32768
				&& Time::getMillisecondCounter() <= timeOutTime)
		{
			fd_set readbits;
			FD_ZERO (&readbits);
			FD_SET (socketHandle, &readbits);

			struct timeval tv;
			tv.tv_sec = jmax (1, (int) (timeOutTime - Time::getMillisecondCounter()) / 1000);
			tv.tv_usec = 0;

			if (select (socketHandle + 1, &readbits, 0, 0, &tv) <= 0)
				return String::empty;  // (timeout)

			buffer.ensureSize (bytesRead + 8, true);
			char* const dest = (char*) buffer.getData() + bytesRead;

			if (recv (socketHandle, dest, 1, 0) == -1)
				return String::empty;

			const char lastByte = *dest;
			++bytesRead;

			if (lastByte == '\n')
				++numConsecutiveLFs;
			else if (lastByte != '\r')
				numConsecutiveLFs = 0;
		}

		const String header (CharPointer_UTF8 ((const char*) buffer.getData()));

		if (header.startsWithIgnoreCase ("HTTP/"))
			return header.trimEnd();

		return String::empty;
	}

	static void writeValueIfNotPresent (MemoryOutputStream& dest, const String& headers, const String& key, const String& value)
	{
		if (! headers.containsIgnoreCase (key))
			dest << "\r\n" << key << ' ' << value;
	}

	static void writeHost (MemoryOutputStream& dest, const bool isPost, const String& path, const String& host, const int port)
	{
		dest << (isPost ? "POST " : "GET ") << path << " HTTP/1.0\r\nHost: " << host;

		if (port > 0)
			dest << ':' << port;
	}

	static MemoryBlock createRequestHeader (const String& hostName, const int hostPort,
											const String& proxyName, const int proxyPort,
											const String& hostPath, const String& originalURL,
											const String& userHeaders, const MemoryBlock& postData,
											const bool isPost)
	{
		MemoryOutputStream header;

		if (proxyName.isEmpty())
			writeHost (header, isPost, hostPath, hostName, hostPort);
		else
			writeHost (header, isPost, originalURL, proxyName, proxyPort);

		writeValueIfNotPresent (header, userHeaders, "User-Agent:", "JUCE/" JUCE_STRINGIFY(JUCE_MAJOR_VERSION)
																		"." JUCE_STRINGIFY(JUCE_MINOR_VERSION)
																		"." JUCE_STRINGIFY(JUCE_BUILDNUMBER));
		writeValueIfNotPresent (header, userHeaders, "Connection:", "Close");

		if (isPost)
			writeValueIfNotPresent (header, userHeaders, "Content-Length:", String ((int) postData.getSize()));

		header << "\r\n" << userHeaders
			   << "\r\n" << postData;

		return header.getMemoryBlock();
	}

	static bool sendHeader (int socketHandle, const MemoryBlock& requestHeader, const uint32 timeOutTime,
							URL::OpenStreamProgressCallback* progressCallback, void* progressCallbackContext)
	{
		size_t totalHeaderSent = 0;

		while (totalHeaderSent < requestHeader.getSize())
		{
			if (Time::getMillisecondCounter() > timeOutTime)
				return false;

			const int numToSend = jmin (1024, (int) (requestHeader.getSize() - totalHeaderSent));

			if (send (socketHandle, static_cast <const char*> (requestHeader.getData()) + totalHeaderSent, numToSend, 0) != numToSend)
				return false;

			totalHeaderSent += numToSend;

			if (progressCallback != nullptr && ! progressCallback (progressCallbackContext, totalHeaderSent, requestHeader.getSize()))
				return false;
		}

		return true;
	}

	static bool decomposeURL (const String& url, String& host, String& path, int& port)
	{
		if (! url.startsWithIgnoreCase ("http://"))
			return false;

		const int nextSlash = url.indexOfChar (7, '/');
		int nextColon = url.indexOfChar (7, ':');
		if (nextColon > nextSlash && nextSlash > 0)
			nextColon = -1;

		if (nextColon >= 0)
		{
			host = url.substring (7, nextColon);

			if (nextSlash >= 0)
				port = url.substring (nextColon + 1, nextSlash).getIntValue();
			else
				port = url.substring (nextColon + 1).getIntValue();
		}
		else
		{
			port = 80;

			if (nextSlash >= 0)
				host = url.substring (7, nextSlash);
			else
				host = url.substring (7);
		}

		if (nextSlash >= 0)
			path = url.substring (nextSlash);
		else
			path = "/";

		return true;
	}

	static String findHeaderItem (const StringArray& lines, const String& itemName)
	{
		for (int i = 0; i < lines.size(); ++i)
			if (lines[i].startsWithIgnoreCase (itemName))
				return lines[i].substring (itemName.length()).trim();

		return String::empty;
	}

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WebInputStream);
};

InputStream* URL::createNativeStream (const String& address, bool isPost, const MemoryBlock& postData,
									  OpenStreamProgressCallback* progressCallback, void* progressCallbackContext,
									  const String& headers, const int timeOutMs, StringPairArray* responseHeaders)
{
	ScopedPointer <WebInputStream> wi (new WebInputStream (address, isPost, postData,
														   progressCallback, progressCallbackContext,
														   headers, timeOutMs, responseHeaders));

	return wi->isError() ? nullptr : wi.release();
}

/*** End of inlined file: juce_linux_Network.cpp ***/


/*** Start of inlined file: juce_linux_SystemStats.cpp ***/
void Logger::outputDebugString (const String& text)
{
	std::cerr << text << std::endl;
}

SystemStats::OperatingSystemType SystemStats::getOperatingSystemType()
{
	return Linux;
}

String SystemStats::getOperatingSystemName()
{
	return "Linux";
}

bool SystemStats::isOperatingSystem64Bit()
{
   #if JUCE_64BIT
	return true;
   #else
	//xxx not sure how to find this out?..
	return false;
   #endif
}

namespace LinuxStatsHelpers
{
	String getCpuInfo (const char* const key)
	{
		StringArray lines;
		File ("/proc/cpuinfo").readLines (lines);

		for (int i = lines.size(); --i >= 0;) // (NB - it's important that this runs in reverse order)
			if (lines[i].startsWithIgnoreCase (key))
				return lines[i].fromFirstOccurrenceOf (":", false, false).trim();

		return String::empty;
	}
}

String SystemStats::getCpuVendor()
{
	return LinuxStatsHelpers::getCpuInfo ("vendor_id");
}

int SystemStats::getCpuSpeedInMegaherz()
{
	return roundToInt (LinuxStatsHelpers::getCpuInfo ("cpu MHz").getFloatValue());
}

int SystemStats::getMemorySizeInMegabytes()
{
	struct sysinfo sysi;

	if (sysinfo (&sysi) == 0)
		return (sysi.totalram * sysi.mem_unit / (1024 * 1024));

	return 0;
}

int SystemStats::getPageSize()
{
	return sysconf (_SC_PAGESIZE);
}

String SystemStats::getLogonName()
{
	const char* user = getenv ("USER");

	if (user == nullptr)
	{
		struct passwd* const pw = getpwuid (getuid());
		if (pw != nullptr)
			user = pw->pw_name;
	}

	return CharPointer_UTF8 (user);
}

String SystemStats::getFullUserName()
{
	return getLogonName();
}

String SystemStats::getComputerName()
{
	char name [256] = { 0 };
	if (gethostname (name, sizeof (name) - 1) == 0)
		return name;

	return String::empty;
}

SystemStats::CPUFlags::CPUFlags()
{
	const String flags (LinuxStatsHelpers::getCpuInfo ("flags"));
	hasMMX   = flags.contains ("mmx");
	hasSSE   = flags.contains ("sse");
	hasSSE2  = flags.contains ("sse2");
	has3DNow = flags.contains ("3dnow");

	numCpus = LinuxStatsHelpers::getCpuInfo ("processor").getIntValue() + 1;
}

uint32 juce_millisecondsSinceStartup() noexcept
{
	timespec t;
	clock_gettime (CLOCK_MONOTONIC, &t);

	return t.tv_sec * 1000 + t.tv_nsec / 1000000;
}

int64 Time::getHighResolutionTicks() noexcept
{
	timespec t;
	clock_gettime (CLOCK_MONOTONIC, &t);

	return (t.tv_sec * (int64) 1000000) + (t.tv_nsec / 1000);
}

int64 Time::getHighResolutionTicksPerSecond() noexcept
{
	return 1000000;  // (microseconds)
}

double Time::getMillisecondCounterHiRes() noexcept
{
	return getHighResolutionTicks() * 0.001;
}

bool Time::setSystemTimeToThisTime() const
{
	timeval t;
	t.tv_sec = millisSinceEpoch / 1000;
	t.tv_usec = (millisSinceEpoch - t.tv_sec * 1000) * 1000;

	return settimeofday (&t, 0) == 0;
}

/*** End of inlined file: juce_linux_SystemStats.cpp ***/


/*** Start of inlined file: juce_linux_Threads.cpp ***/
/*
	Note that a lot of methods that you'd expect to find in this file actually
	live in juce_posix_SharedCode.h!
*/

void Process::setPriority (const ProcessPriority prior)
{
	const int policy = (prior <= NormalPriority) ? SCHED_OTHER : SCHED_RR;
	const int minp = sched_get_priority_min (policy);
	const int maxp = sched_get_priority_max (policy);

	struct sched_param param;

	switch (prior)
	{
		case LowPriority:
		case NormalPriority:    param.sched_priority = 0; break;
		case HighPriority:      param.sched_priority = minp + (maxp - minp) / 4; break;
		case RealtimePriority:  param.sched_priority = minp + (3 * (maxp - minp) / 4); break;
		default:                jassertfalse; break;
	}

	pthread_setschedparam (pthread_self(), policy, &param);
}

void Process::terminate()
{
	exit (0);
}

JUCE_API bool JUCE_CALLTYPE juce_isRunningUnderDebugger()
{
	static char testResult = 0;

	if (testResult == 0)
	{
		testResult = (char) ptrace (PT_TRACE_ME, 0, 0, 0);

		if (testResult >= 0)
		{
			ptrace (PT_DETACH, 0, (caddr_t) 1, 0);
			testResult = 1;
		}
	}

	return testResult < 0;
}

JUCE_API bool JUCE_CALLTYPE Process::isRunningUnderDebugger()
{
	return juce_isRunningUnderDebugger();
}

void Process::raisePrivilege()
{
	// If running suid root, change effective user to root
	if (geteuid() != 0 && getuid() == 0)
	{
		setreuid (geteuid(), getuid());
		setregid (getegid(), getgid());
	}
}

void Process::lowerPrivilege()
{
	// If runing suid root, change effective user back to real user
	if (geteuid() == 0 && getuid() != 0)
	{
		setreuid (geteuid(), getuid());
		setregid (getegid(), getgid());
	}
}

/*** End of inlined file: juce_linux_Threads.cpp ***/

#elif JUCE_ANDROID

/*** Start of inlined file: juce_android_Files.cpp ***/
bool File::copyInternal (const File& dest) const
{
	FileInputStream in (*this);

	if (dest.deleteFile())
	{
		{
			FileOutputStream out (dest);

			if (out.failedToOpen())
				return false;

			if (out.writeFromInputStream (in, -1) == getSize())
				return true;
		}

		dest.deleteFile();
	}

	return false;
}

void File::findFileSystemRoots (Array<File>& destArray)
{
	destArray.add (File ("/"));
}

bool File::isOnCDRomDrive() const
{
	return false;
}

bool File::isOnHardDisk() const
{
	return true;
}

bool File::isOnRemovableDrive() const
{
	return false;
}

bool File::isHidden() const
{
	return getFileName().startsWithChar ('.');
}

namespace
{
	File juce_readlink (const String& file, const File& defaultFile)
	{
		const int size = 8192;
		HeapBlock<char> buffer;
		buffer.malloc (size + 4);

		const size_t numBytes = readlink (file.toUTF8(), buffer, size);

		if (numBytes > 0 && numBytes <= size)
			return File (file).getSiblingFile (String::fromUTF8 (buffer, (int) numBytes));

		return defaultFile;
	}
}

File File::getLinkedTarget() const
{
	return juce_readlink (getFullPathName().toUTF8(), *this);
}

File File::getSpecialLocation (const SpecialLocationType type)
{
	switch (type)
	{
	case userHomeDirectory:
	case userDocumentsDirectory:
	case userMusicDirectory:
	case userMoviesDirectory:
	case userApplicationDataDirectory:
	case userDesktopDirectory:
		return File (android.appDataDir);

	case commonApplicationDataDirectory:
		return File (android.appDataDir);

	case globalApplicationsDirectory:
		return File ("/system/app");

	case tempDirectory:
		//return File (AndroidStatsHelpers::getSystemProperty ("java.io.tmpdir"));
		return File (android.appDataDir).getChildFile (".temp");

	case invokedExecutableFile:
	case currentExecutableFile:
	case currentApplicationFile:
	case hostApplicationPath:
		return juce_getExecutableFile();

	default:
		jassertfalse; // unknown type?
		break;
	}

	return File::nonexistent;
}

String File::getVersion() const
{
	return String::empty;
}

bool File::moveToTrash() const
{
	if (! exists())
		return true;

	// TODO

	return false;
}

class DirectoryIterator::NativeIterator::Pimpl
{
public:
	Pimpl (const File& directory, const String& wildCard_)
		: parentDir (File::addTrailingSeparator (directory.getFullPathName())),
		  wildCard (wildCard_),
		  dir (opendir (directory.getFullPathName().toUTF8()))
	{
	}

	~Pimpl()
	{
		if (dir != 0)
			closedir (dir);
	}

	bool next (String& filenameFound,
			   bool* const isDir, bool* const isHidden, int64* const fileSize,
			   Time* const modTime, Time* const creationTime, bool* const isReadOnly)
	{
		if (dir != 0)
		{
			const char* wildcardUTF8 = nullptr;

			for (;;)
			{
				struct dirent* const de = readdir (dir);

				if (de == nullptr)
					break;

				if (wildcardUTF8 == nullptr)
					wildcardUTF8 = wildCard.toUTF8();

				if (fnmatch (wildcardUTF8, de->d_name, FNM_CASEFOLD) == 0)
				{
					filenameFound = CharPointer_UTF8 (de->d_name);

					updateStatInfoForFile (parentDir + filenameFound, isDir, fileSize, modTime, creationTime, isReadOnly);

					if (isHidden != 0)
						*isHidden = filenameFound.startsWithChar ('.');

					return true;
				}
			}
		}

		return false;
	}

private:
	String parentDir, wildCard;
	DIR* dir;

	JUCE_DECLARE_NON_COPYABLE (Pimpl);
};

DirectoryIterator::NativeIterator::NativeIterator (const File& directory, const String& wildCard)
	: pimpl (new DirectoryIterator::NativeIterator::Pimpl (directory, wildCard))
{
}

DirectoryIterator::NativeIterator::~NativeIterator()
{
}

bool DirectoryIterator::NativeIterator::next (String& filenameFound,
											  bool* const isDir, bool* const isHidden, int64* const fileSize,
											  Time* const modTime, Time* const creationTime, bool* const isReadOnly)
{
	return pimpl->next (filenameFound, isDir, isHidden, fileSize, modTime, creationTime, isReadOnly);
}

bool Process::openDocument (const String& fileName, const String& parameters)
{
	const LocalRef<jstring> t (javaString (fileName));
	android.activity.callVoidMethod (JuceAppActivity.launchURL, t.get());
}

void File::revealToUser() const
{
}

/*** End of inlined file: juce_android_Files.cpp ***/



/*** Start of inlined file: juce_android_Misc.cpp ***/
void Logger::outputDebugString (const String& text)
{
	__android_log_print (ANDROID_LOG_INFO, "JUCE", text.toUTF8());
}

/*** End of inlined file: juce_android_Misc.cpp ***/


/*** Start of inlined file: juce_android_Network.cpp ***/
#define JNI_CLASS_MEMBERS(METHOD, STATICMETHOD, FIELD, STATICFIELD) \
 METHOD (constructor, "<init>", "()V") \
 METHOD (toString, "toString", "()Ljava/lang/String;") \

DECLARE_JNI_CLASS (StringBuffer, "java/lang/StringBuffer");
#undef JNI_CLASS_MEMBERS

#define JNI_CLASS_MEMBERS(METHOD, STATICMETHOD, FIELD, STATICFIELD) \
 METHOD (release, "release", "()V") \
 METHOD (read, "read", "([BI)I") \
 METHOD (getPosition, "getPosition", "()J") \
 METHOD (getTotalLength, "getTotalLength", "()J") \
 METHOD (isExhausted, "isExhausted", "()Z") \
 METHOD (setPosition, "setPosition", "(J)Z") \

DECLARE_JNI_CLASS (HTTPStream, JUCE_ANDROID_ACTIVITY_CLASSPATH "$HTTPStream");
#undef JNI_CLASS_MEMBERS

void MACAddress::findAllAddresses (Array<MACAddress>& result)
{
	// TODO
}

bool Process::openEmailWithAttachments (const String& targetEmailAddress,
										const String& emailSubject,
										const String& bodyText,
										const StringArray& filesToAttach)
{
	// TODO
	return false;
}

class WebInputStream  : public InputStream
{
public:

	WebInputStream (String address, bool isPost, const MemoryBlock& postData,
					URL::OpenStreamProgressCallback* progressCallback, void* progressCallbackContext,
					const String& headers, int timeOutMs, StringPairArray* responseHeaders)
	{
		if (! address.contains ("://"))
			address = "http://" + address;

		JNIEnv* env = getEnv();

		jbyteArray postDataArray = 0;

		if (postData.getSize() > 0)
		{
			postDataArray = env->NewByteArray (postData.getSize());
			env->SetByteArrayRegion (postDataArray, 0, postData.getSize(), (const jbyte*) postData.getData());
		}

		LocalRef<jobject> responseHeaderBuffer (env->NewObject (StringBuffer, StringBuffer.constructor));

		stream = GlobalRef (env->CallStaticObjectMethod (JuceAppActivity,
														 JuceAppActivity.createHTTPStream,
														 javaString (address).get(),
														 (jboolean) isPost,
														 postDataArray,
														 javaString (headers).get(),
														 (jint) timeOutMs,
														 responseHeaderBuffer.get()));

		if (postDataArray != 0)
			env->DeleteLocalRef (postDataArray);

		if (stream != 0)
		{
			StringArray headerLines;

			{
				LocalRef<jstring> headersString ((jstring) env->CallObjectMethod (responseHeaderBuffer.get(),
																				  StringBuffer.toString));
				headerLines.addLines (juceString (env, headersString));
			}

			if (responseHeaders != 0)
			{
				for (int i = 0; i < headerLines.size(); ++i)
				{
					const String& header = headerLines[i];
					const String key (header.upToFirstOccurrenceOf (": ", false, false));
					const String value (header.fromFirstOccurrenceOf (": ", false, false));
					const String previousValue ((*responseHeaders) [key]);

					responseHeaders->set (key, previousValue.isEmpty() ? value : (previousValue + "," + value));
				}
			}
		}
	}

	~WebInputStream()
	{
		if (stream != 0)
			stream.callVoidMethod (HTTPStream.release);
	}

	bool isExhausted()                  { return stream != nullptr && stream.callBooleanMethod (HTTPStream.isExhausted); }
	int64 getTotalLength()              { return stream != nullptr ? stream.callLongMethod (HTTPStream.getTotalLength) : 0; }
	int64 getPosition()                 { return stream != nullptr ? stream.callLongMethod (HTTPStream.getPosition) : 0; }
	bool setPosition (int64 wantedPos)  { return stream != nullptr && stream.callBooleanMethod (HTTPStream.setPosition, (jlong) wantedPos); }

	int read (void* buffer, int bytesToRead)
	{
		jassert (buffer != nullptr && bytesToRead >= 0);

		if (stream == nullptr)
			return 0;

		JNIEnv* env = getEnv();

		jbyteArray javaArray = env->NewByteArray (bytesToRead);

		int numBytes = stream.callIntMethod (HTTPStream.read, javaArray, (jint) bytesToRead);

		if (numBytes > 0)
			env->GetByteArrayRegion (javaArray, 0, numBytes, static_cast <jbyte*> (buffer));

		env->DeleteLocalRef (javaArray);
		return numBytes;
	}

	GlobalRef stream;

private:
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WebInputStream);
};

InputStream* URL::createNativeStream (const String& address, bool isPost, const MemoryBlock& postData,
									  OpenStreamProgressCallback* progressCallback, void* progressCallbackContext,
									  const String& headers, const int timeOutMs, StringPairArray* responseHeaders)
{
	ScopedPointer <WebInputStream> wi (new WebInputStream (address, isPost, postData,
														   progressCallback, progressCallbackContext,
														   headers, timeOutMs, responseHeaders));

	return wi->stream != 0 ? wi.release() : nullptr;
}

/*** End of inlined file: juce_android_Network.cpp ***/


/*** Start of inlined file: juce_android_SystemStats.cpp ***/
JNIClassBase::JNIClassBase (const char* classPath_)
	: classPath (classPath_), classRef (0)
{
	getClasses().add (this);
}

JNIClassBase::~JNIClassBase()
{
	getClasses().removeValue (this);
}

Array<JNIClassBase*>& JNIClassBase::getClasses()
{
	static Array<JNIClassBase*> classes;
	return classes;
}

void JNIClassBase::initialise (JNIEnv* env)
{
	classRef = (jclass) env->NewGlobalRef (env->FindClass (classPath));
	jassert (classRef != 0);

	initialiseFields (env);
}

void JNIClassBase::release (JNIEnv* env)
{
	env->DeleteGlobalRef (classRef);
}

void JNIClassBase::initialiseAllClasses (JNIEnv* env)
{
	const Array<JNIClassBase*>& classes = getClasses();
	for (int i = classes.size(); --i >= 0;)
		classes.getUnchecked(i)->initialise (env);
}

void JNIClassBase::releaseAllClasses (JNIEnv* env)
{
	const Array<JNIClassBase*>& classes = getClasses();
	for (int i = classes.size(); --i >= 0;)
		classes.getUnchecked(i)->release (env);
}

jmethodID JNIClassBase::resolveMethod (JNIEnv* env, const char* methodName, const char* params)
{
	jmethodID m = env->GetMethodID (classRef, methodName, params);
	jassert (m != 0);
	return m;
}

jmethodID JNIClassBase::resolveStaticMethod (JNIEnv* env, const char* methodName, const char* params)
{
	jmethodID m = env->GetStaticMethodID (classRef, methodName, params);
	jassert (m != 0);
	return m;
}

jfieldID JNIClassBase::resolveField (JNIEnv* env, const char* fieldName, const char* signature)
{
	jfieldID f = env->GetFieldID (classRef, fieldName, signature);
	jassert (f != 0);
	return f;
}

jfieldID JNIClassBase::resolveStaticField (JNIEnv* env, const char* fieldName, const char* signature)
{
	jfieldID f = env->GetStaticFieldID (classRef, fieldName, signature);
	jassert (f != 0);
	return f;
}

ThreadLocalJNIEnvHolder threadLocalJNIEnvHolder;

#if JUCE_DEBUG
static bool systemInitialised = false;
#endif

JNIEnv* getEnv() noexcept
{
   #if JUCE_DEBUG
	if (! systemInitialised)
	{
		DBG ("*** Call to getEnv() when system not initialised");
		jassertfalse;
		exit (0);
	}
   #endif

	return threadLocalJNIEnvHolder.getOrAttach();
}

extern "C" jint JNI_OnLoad (JavaVM*, void*)
{
	return JNI_VERSION_1_2;
}

AndroidSystem::AndroidSystem() : screenWidth (0), screenHeight (0)
{
}

void AndroidSystem::initialise (JNIEnv* env, jobject activity_,
								jstring appFile_, jstring appDataDir_)
{
	screenWidth = screenHeight = 0;
	JNIClassBase::initialiseAllClasses (env);

	threadLocalJNIEnvHolder.initialise (env);
   #if JUCE_DEBUG
	systemInitialised = true;
   #endif

	activity = GlobalRef (activity_);
	appFile = juceString (env, appFile_);
	appDataDir = juceString (env, appDataDir_);
}

void AndroidSystem::shutdown (JNIEnv* env)
{
	activity.clear();

   #if JUCE_DEBUG
	systemInitialised = false;
   #endif

	JNIClassBase::releaseAllClasses (env);
}

AndroidSystem android;

namespace AndroidStatsHelpers
{

	#define JNI_CLASS_MEMBERS(METHOD, STATICMETHOD, FIELD, STATICFIELD) \
	 STATICMETHOD (getProperty, "getProperty", "(Ljava/lang/String;)Ljava/lang/String;")

	DECLARE_JNI_CLASS (SystemClass, "java/lang/System");
	#undef JNI_CLASS_MEMBERS

	String getSystemProperty (const String& name)
	{
		return juceString (LocalRef<jstring> ((jstring) getEnv()->CallStaticObjectMethod (SystemClass,
																						  SystemClass.getProperty,
																						  javaString (name).get())));
	}
}

SystemStats::OperatingSystemType SystemStats::getOperatingSystemType()
{
	return Android;
}

String SystemStats::getOperatingSystemName()
{
	return "Android " + AndroidStatsHelpers::getSystemProperty ("os.version");
}

bool SystemStats::isOperatingSystem64Bit()
{
   #if JUCE_64BIT
	return true;
   #else
	return false;
   #endif
}

String SystemStats::getCpuVendor()
{
	return AndroidStatsHelpers::getSystemProperty ("os.arch");
}

int SystemStats::getCpuSpeedInMegaherz()
{
	return 0; // TODO
}

int SystemStats::getMemorySizeInMegabytes()
{
   #if __ANDROID_API__ >= 9
	struct sysinfo sysi;

	if (sysinfo (&sysi) == 0)
		return (sysi.totalram * sysi.mem_unit / (1024 * 1024));
   #endif

	return 0;
}

int SystemStats::getPageSize()
{
	return sysconf (_SC_PAGESIZE);
}

String SystemStats::getLogonName()
{
	const char* user = getenv ("USER");

	if (user == 0)
	{
		struct passwd* const pw = getpwuid (getuid());
		if (pw != 0)
			user = pw->pw_name;
	}

	return CharPointer_UTF8 (user);
}

String SystemStats::getFullUserName()
{
	return getLogonName();
}

String SystemStats::getComputerName()
{
	char name [256] = { 0 };
	if (gethostname (name, sizeof (name) - 1) == 0)
		return name;

	return String::empty;
}

SystemStats::CPUFlags::CPUFlags()
{
	// TODO
	hasMMX = false;
	hasSSE = false;
	hasSSE2 = false;
	has3DNow = false;

	numCpus = jmax (1, sysconf (_SC_NPROCESSORS_ONLN));
}

uint32 juce_millisecondsSinceStartup() noexcept
{
	timespec t;
	clock_gettime (CLOCK_MONOTONIC, &t);

	return t.tv_sec * 1000 + t.tv_nsec / 1000000;
}

int64 Time::getHighResolutionTicks() noexcept
{
	timespec t;
	clock_gettime (CLOCK_MONOTONIC, &t);

	return (t.tv_sec * (int64) 1000000) + (t.tv_nsec / 1000);
}

int64 Time::getHighResolutionTicksPerSecond() noexcept
{
	return 1000000;  // (microseconds)
}

double Time::getMillisecondCounterHiRes() noexcept
{
	return getHighResolutionTicks() * 0.001;
}

bool Time::setSystemTimeToThisTime() const
{
	jassertfalse;
	return false;
}

/*** End of inlined file: juce_android_SystemStats.cpp ***/


/*** Start of inlined file: juce_android_Threads.cpp ***/
/*
	Note that a lot of methods that you'd expect to find in this file actually
	live in juce_posix_SharedCode.h!
*/

// sets the process to 0=low priority, 1=normal, 2=high, 3=realtime
void Process::setPriority (ProcessPriority prior)
{
	// TODO

	struct sched_param param;
	int policy, maxp, minp;

	const int p = (int) prior;

	if (p <= 1)
		policy = SCHED_OTHER;
	else
		policy = SCHED_RR;

	minp = sched_get_priority_min (policy);
	maxp = sched_get_priority_max (policy);

	if (p < 2)
		param.sched_priority = 0;
	else if (p == 2 )
		// Set to middle of lower realtime priority range
		param.sched_priority = minp + (maxp - minp) / 4;
	else
		// Set to middle of higher realtime priority range
		param.sched_priority = minp + (3 * (maxp - minp) / 4);

	pthread_setschedparam (pthread_self(), policy, &param);
}

void Process::terminate()
{
	// TODO
	exit (0);
}

JUCE_API bool JUCE_CALLTYPE juce_isRunningUnderDebugger()
{
	return false;
}

JUCE_API bool JUCE_CALLTYPE Process::isRunningUnderDebugger()
{
	return juce_isRunningUnderDebugger();
}

void Process::raisePrivilege() {}
void Process::lowerPrivilege() {}

/*** End of inlined file: juce_android_Threads.cpp ***/

#endif
}

/*** End of inlined file: juce_core.cpp ***/


