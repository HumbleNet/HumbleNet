// Configures the Juce library.

#ifdef _MSC_VER
  #ifndef _WINSOCK_DEPRECATED_NO_WARNINGS
	// silence warnings about inet_addr and inet_ntoa
    #define _WINSOCK_DEPRECATED_NO_WARNINGS
  #endif
#endif

#ifndef   JUCE_FORCE_DEBUG
//#define JUCE_FORCE_DEBUG
#endif

#ifndef   JUCE_LOG_ASSERTIONS
//#define JUCE_LOG_ASSERTIONS
#endif

#ifndef   JUCE_CHECK_MEMORY_LEAKS
//#define JUCE_CHECK_MEMORY_LEAKS
#endif

#ifndef   JUCE_DONT_AUTOLINK_TO_WIN32_LIBRARIES
//#define JUCE_DONT_AUTOLINK_TO_WIN32_LIBRARIES
#endif
