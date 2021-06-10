if(NOT TARGET ThrowTheSwitch::Unity)

  add_library(unity
    src/unity.h
    src/unity_internals.h
    src/unity.c
    
    # The "test fixture" extension
    extras/fixture/src/unity_fixture.c
  )
  set_target_properties(unity PROPERTIES
    FOLDER tests
    C_CLANG_TIDY "" # Don't run clang-tidy on external code
    CXX_CLANG_TIDY ""
  )
  target_include_directories(unity PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/src
    ${CMAKE_CURRENT_LIST_DIR}/extras/fixture/src
  )

  set(UNITY_CONFIG_FILE_DIR "" CACHE STRING "The directory path of the unity_config.h file, if provided.")
  if(UNITY_CONFIG_FILE_DIR)
    target_compile_definitions(unity PUBLIC UNITY_INCLUDE_CONFIG_H)
    target_include_directories(unity PUBLIC ${UNITY_CONFIG_FILE_DIR})
  endif()

  add_library(ThrowTheSwitch::Unity ALIAS unity)
endif()
