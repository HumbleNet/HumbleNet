#include "humblenet.h"
#include "humblenet_p2p.h"

#include <string>

// globally exclude this as we won't be using the one in the loaded humblenet (it is blank anyway)
#define HUMBLENET_SKIP_humblenet_loader_init

extern "C" {
/* Typedefs for function pointers for jump table, and pre-declare default funcs */
/* The DEFAULT funcs will init jump table and then call real function. */
#define LOADER_PROC(rc,fn, params, args, ret) \
	typedef rc (*humblenet_DYNFN_##fn) params; \
	static rc HUMBLENET_CALL fn##_DEFAULT params;
#include  "humblenet_loader_procs.h"
#undef  LOADER_PROC
}

#if HAVE_DLOPEN
#include <dlfcn.h>

static void *LoadObject(const char* sofile)
{
	void *handle = dlopen(sofile, RTLD_NOW|RTLD_LOCAL);
	if (!handle) {
		humblenet_set_error_DEFAULT(dlerror());
	}
	return handle;
}

static void* LoadFunction(void *handle, const char* name)
{
	void *symbol = dlsym(handle, name);
	if (!symbol) {
		humblenet_set_error_DEFAULT(dlerror());
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
	humblenet_set_error_DEFAULT("Could not load humblenet.dll");
	return handle;
}

static void* LoadFunction(void *handle, const char* name)
{
	void *symbol = GetProcAddress((HMODULE)handle, name);
	humblenet_set_error_DEFAULT("Could not load symbol from humblenet.dll");
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


/* The jump table! */
typedef struct {
#define LOADER_PROC(rc,fn, params, args, ret) humblenet_DYNFN_##fn fn;
#include  "humblenet_loader_procs.h"
#undef  LOADER_PROC
} humblenet_DYNF_jump_table;

/* The actual jump table. */
static humblenet_DYNF_jump_table jump_table = {
#define LOADER_PROC(rc,fn,params,args,ret) fn##_DEFAULT,
#include "humblenet_loader_procs.h"
#undef LOADER_PROC
};

static void* dllHandle = nullptr;

static void humblenet_UnloadLibrary()
{
#define LOADER_PROC(rc,fn,params,args,ret) \
	jump_table.fn = fn##_DEFAULT;
#include "humblenet_loader_procs.h"
#undef LOADER_PROC
	UnloadObject(dllHandle);
	dllHandle = nullptr;
}

#ifdef WIN32
#define HUMBLENET_LIBRARY "humblenet.dll"
#elif defined(__linux__)
#define HUMBLENET_LIBRARY "libhumblenet.so"
#elif defined(__APPLE__)
#define HUMBLENET_LIBRARY "libhumblenet.dylib"
#endif

static ha_bool humblenet_LoadLibrary(const char* override)
{
	if (dllHandle) return  1;

	if (override) {
		std::string buff(override);
		auto end = buff.back();
		if (end != '/' && end != '\\') {
#ifdef WIN32
			buff += "\\";
#else
			buff += "/";
#endif
		}
		buff += HUMBLENET_LIBRARY;

		dllHandle = LoadObject(buff.c_str());
	}

	if (!dllHandle) {
		dllHandle = LoadObject(HUMBLENET_LIBRARY);
	}

#ifdef __linux__
	if (!dllHandle) {
		Dl_info dl_info;
		if (dladdr((const void*)"", &dl_info) != 0)  {
			std::string buff(dl_info.dli_fname);
			auto pos = buff.find_last_of('/');
			if (pos != buff.npos) {
				buff.erase(pos);
				buff += "/libhumblenet.so";
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

			size_t pos = buff.find("/humblenet_unity_editor.bundle");
			if (pos != buff.npos) {
				buff.erase(pos);
				buff += "/humblenet.bundle/Contents/MacOS/humblenet";

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
			buff += "\\humblenet.dll";

			dllHandle = LoadObject(buff.c_str());
		}
	}
#endif
	if (dllHandle) {
		void *procHandle = nullptr;
#define LOADER_PROC(rc,fn,params,args,ret) \
procHandle = LoadFunction(dllHandle, #fn); \
if (!procHandle) { \
	humblenet_UnloadLibrary(); \
	humblenet_set_error_DEFAULT("Failed to load symbol: " #fn); \
	return 0; \
} \
jump_table.fn = (humblenet_DYNFN_##fn)procHandle;
#include "humblenet_loader_procs.h"
#undef LOADER_PROC

		humblenet_clear_error_DEFAULT();
		return 1;
	} else {
		humblenet_set_error_DEFAULT("Failed to load humblenet library");
		return 0;
	}
}

/* Default functions init the function table then call right thing. */
#define HUMBLENET_SKIP_humblenet_set_error
#define HUMBLENET_SKIP_humblenet_get_error
#define HUMBLENET_SKIP_humblenet_clear_error
#define LOADER_PROC(rc,fn,params,args,ret) \
	static rc HUMBLENET_CALL fn##_DEFAULT params { \
		humblenet_LoadLibrary(nullptr); \
		if (dllHandle) ret jump_table.fn args; \
		ret (rc)0; \
	}
#include "humblenet_loader_procs.h"
#undef LOADER_PROC
#undef HUMBLENET_SKIP_humblenet_set_error
#undef HUMBLENET_SKIP_humblenet_get_error
#undef HUMBLENET_SKIP_humblenet_clear_error

/* Public API functions to jump into the jump table. */
#define HUMBLENET_SKIP_humblenet_shutdown
#define LOADER_PROC(rc,fn,params,args,ret) \
	rc HUMBLENET_CALL fn params { ret jump_table.fn args; }
#include "humblenet_loader_procs.h"
#undef LOADER_PROC
#undef HUMBLENET_SKIP_humblenet_shutdown

// explicit implementations

// Basic error handling when the library is not loaded.
// Used for reporting back library loading errors
#if defined(WIN32)
#define THREAD_LOCAL __declspec(thread)
#else // Everyone else
#define THREAD_LOCAL __thread
#endif

static THREAD_LOCAL const char * _error = nullptr;

const char * HUMBLENET_CALL humblenet_get_error_DEFAULT()
{
	return _error;
}

void HUMBLENET_CALL humblenet_set_error_DEFAULT(const char *error) {
	_error = error;
}

void HUMBLENET_CALL humblenet_clear_error_DEFAULT() {
	_error = nullptr;
}

ha_bool HUMBLENET_CALL humblenet_loader_init(const char* path) {
	return humblenet_LoadLibrary(path);
}

// Shutdown unloads the real library.

void HUMBLENET_CALL humblenet_shutdown() {
	jump_table.humblenet_shutdown();
	humblenet_UnloadLibrary();
}
