include_directories(../../include)

add_library(
  hkdf

  OBJECT

  hkdf.c
)

if(BUILD_TESTS)
add_executable(
  hkdf_test

  hkdf_test.c

  $<TARGET_OBJECTS:test_support>
)

target_link_libraries(hkdf_test crypto)
add_dependencies(all_tests hkdf_test)
endif()