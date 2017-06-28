include_directories(../include)

add_executable(
  bssl

  args.cc
  ciphers.cc
  client.cc
  const.cc
  digest.cc
  generate_ed25519.cc
  genrsa.cc
  pkcs12.cc
  rand.cc
  server.cc
  speed.cc
  tool.cc
  transport_common.cc
)

set(REQUIRED_LIBS ssl crypto)

if (NOT (APPLE OR WIN32 OR ANDROID) )
  find_library(LIBRT_LIBRARY rt)
  if (LIBRT_LIBRARY)
    list(APPEND REQUIRED_LIBS ${LIBRT_LIBRARY})
  endif()
endif()

target_link_libraries(bssl ${REQUIRED_LIBS})
