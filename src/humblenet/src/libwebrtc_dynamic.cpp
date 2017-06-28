#if !defined(EMSCRIPTEN) && defined(HUMBLENET_LOAD_WEBRTC)

#include "libwebrtc.h"

#include <string>

#define LOG printf

#define INTERFACE( X )  \
	X( struct libwebrtc_context* ,      libwebrtc_create_context,               ( lwrtc_callback_function cb ),                                         (cb) )                  \
	X( void,                            libwebrtc_destroy_context,              ( struct libwebrtc_context* ctx),                                       (ctx) )                 \
	X( void,                            libwebrtc_set_stun_servers,             ( struct libwebrtc_context* ctx, const char** servers, int count),      (ctx, servers, count) ) \
	X( struct libwebrtc_connection*,    libwebrtc_create_connection_extended,   ( struct libwebrtc_context* ctx, void* user_data ),                     (ctx, user_data) )      \
	X( struct libwebrtc_data_channel*,  libwebrtc_create_channel,               ( struct libwebrtc_connection* conn, const char* name ),                (conn,name) )           \
	X( int,                             libwebrtc_create_offer,                 ( struct libwebrtc_connection* conn),                                   (conn) )                \
	X( int,                             libwebrtc_set_offer,                    ( struct libwebrtc_connection* conn, const char* sdp ),                 (conn,sdp) )            \
	X( int,                             libwebrtc_set_answer,                   ( struct libwebrtc_connection* conn, const char* sdp ),                 (conn,sdp) )            \
	X( int,                             libwebrtc_add_ice_candidate,            ( struct libwebrtc_connection* conn, const char* candidate ),           (conn,candidate) )      \
	X( int,                             libwebrtc_write,                        ( struct libwebrtc_data_channel* dc, const void* buf, int len ),        (dc, buf, len) )        \
	X( void,                            libwebrtc_close_channel,                ( struct libwebrtc_data_channel* dc ),                                  (dc) )                  \
	X( void,                            libwebrtc_close_connection,             ( struct libwebrtc_connection* conn ),                                  (conn) )


#define FN_TYPES( rtype, name, ptypes, ... )    \
	typedef rtype (*FN_##name) ptypes;

#define FN_FIELDS( rtype, name, ptypes, ... )   \
	FN_##name name;

#define FN_MICROSTACK( rtype, name, ... )       \
	Microstack::name,

#define FN_MICROSTACK_FWD( rtype, name, ptypes, ... )       \
	rtype name ptypes;

#define FN_LOADER( rtype, name, ... )          \
	&LOADER_##name,

#define FN_LOADER_FWD( rtype, name, ptypes, ... ) \
	static rtype LOADER_##name ptypes;

#define FN_LOADER_IMPL( rtype, name, ptypes, args ) \
	static rtype LOADER_##name ptypes {             \
		webrtc_LoadLibrary();                       \
		return fn_table.name args;                  \
	}

#define FN_STUB_IMPL( rtype, name, ptypes, args )   \
	rtype name ptypes {                             \
		return fn_table.name args;                  \
	}

#define FN_RESOLVE( rtype, name, ... )                      \
	procHandle = LoadFunction(dllHandle, #name);            \
	if (!procHandle) { webrtc_UnloadLibrary(); return; }    \
	fn_table.name = (FN_##name)procHandle;


// flow
//
//  stubs delegate to the currently loaded fn_table.
//  the default fn_table is initalzied with load then delegate routines.
//  once loaded, fn_table is initialized with either the direct intneral, or external implementation.
//

// declare pointers types for all the interface methods.
INTERFACE( FN_TYPES );

// declare the function table type
struct fn_table_t {
	INTERFACE( FN_FIELDS );
};

// forward declare Microstack impl
namespace Microstack {
	INTERFACE( FN_MICROSTACK_FWD )
};

// this is the internal (fallback)
static fn_table_t fn_microstack = {
	INTERFACE( FN_MICROSTACK )
};

// fwd declare the loader functions
INTERFACE( FN_LOADER_FWD );

// initialize the default table to be the loader functions
static fn_table_t fn_table = {
	INTERFACE( FN_LOADER )
};

static void webrtc_LoadLibrary();

// implement the loader functions
INTERFACE( FN_LOADER_IMPL );

// implement the stub functions
INTERFACE( FN_STUB_IMPL );

#if HAVE_DLOPEN
#include <dlfcn.h>

static void *LoadObject(const char* sofile)
{
	void *handle = dlopen(sofile, RTLD_NOW|RTLD_LOCAL);
	if( !handle ) {
	LOG("Could not open '%s': %s\n", sofile, dlerror() );
	}
	return handle;
}

static void* LoadFunction(void *handle, const char* name)
{
	void *symbol = dlsym(handle, name);
	if( !symbol ) {
	LOG("Missing symbol: %s\n", name );
	}
	return symbol;
}

static void UnloadObject(void *handle)
{
	if (handle) {
		dlclose(handle);
	}
}
#elif defined(WIN32)
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

static void *LoadObject(const char* sofile)
{
	void *handle = (void*)LoadLibraryA(sofile);
	return handle;
}

static void* LoadFunction(void *handle, const char* name)
{
	void *symbol = GetProcAddress((HMODULE)handle, name);
	return symbol;
}

static void UnloadObject(void *handle)
{
	if (handle) {
		FreeLibrary((HMODULE) handle);
	}
}

extern "C" IMAGE_DOS_HEADER __ImageBase;

#else
#error No Shared library open support
#endif

static void* dllHandle = nullptr;

static void webrtc_UnloadLibrary()
{
	// set to internal implementation
	fn_table = fn_microstack;

	UnloadObject(dllHandle);
	dllHandle = nullptr;
}

#ifdef WIN32
#define WEBRTC_LIBRARY "webrtc.dll"
#elif defined(__linux__)
#define WEBRTC_LIBRARY "libwebrtc.so"
#elif defined(__APPLE__)
#define WEBRTC_LIBRARY "libwebrtc.dylib"
#endif

static void webrtc_LoadLibrary()
{
	dllHandle = LoadObject(WEBRTC_LIBRARY);
#ifdef __linux__
	if (!dllHandle) {
		Dl_info dl_info;
		if (dladdr((const void*)"", &dl_info) != 0)  {
			std::string buff(dl_info.dli_fname);
			auto pos = buff.find_last_of('/');
			if (pos != buff.npos) {
				buff.erase(pos);
				buff += "/" WEBRTC_LIBRARY;
				dllHandle = LoadObject(buff.c_str());
			}
		}
	}
#endif
#ifdef __APPLE__
	// Special case for OS X. look for a bundle
	if (!dllHandle) {
		Dl_info dl_info;
		if (dladdr((const void*)"", &dl_info) != 0) {
			std::string buff(dl_info.dli_fname);
			auto pos = buff.find_last_of('/');
			if (pos != buff.npos) {
				buff.erase(pos);
				buff += "/" WEBRTC_LIBRARY;
				dllHandle = LoadObject(buff.c_str());
			}
		}
	}
#endif
#ifdef WIN32
	// Special case for Win32
	if (!dllHandle) {
		char modFile[4096];
		GetModuleFileNameA((HMODULE)&__ImageBase, modFile, sizeof(modFile));

		std::string buff(modFile);
		auto pos = buff.find_last_of('\\');
		if (pos != buff.npos) {
			buff.erase(pos);
			buff += "\\" WEBRTC_LIBRARY;

			dllHandle = LoadObject(buff.c_str());
		}
	}
#endif
	if (dllHandle) {
		void *procHandle = nullptr;

		INTERFACE( FN_RESOLVE );

		LOG("external webrtc implementation loaded\n");
	} else {
		fn_table = fn_microstack;
	}
}

#endif
