cmake_minimum_required(VERSION 3.15)
project(${COMMUTILS_TARGET_NAME}_3rdparty)

# add fmt
add_subdirectory(fmt EXCLUDE_FROM_ALL)
list(APPEND 3RDPARTY_LIB fmt::fmt-header-only)

# using concurrentqueue
add_subdirectory(concurrentqueue EXCLUDE_FROM_ALL)
list(APPEND 3RDPARTY_LIB concurrentqueue)

# using nlohmann::json
add_subdirectory(nlohmann_json EXCLUDE_FROM_ALL)
list(APPEND 3RDPARTY_LIB nlohmann_json::nlohmann_json)

# using spdlog and set external fmt
if (COMMUTILS_WITH_SPDLOG)
  set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
  set(SPDLOG_FMT_EXTERNAL_HO ON)
  add_subdirectory(spdlog EXCLUDE_FROM_ALL)
  list(APPEND 3RDPARTY_LIB spdlog::spdlog_header_only)
endif()

# using gtest
if (COMMUTILS_BUILD_TEST_HO)
  set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
  add_subdirectory(googletest)
endif()

set(3RDPARTY_LIB ${3RDPARTY_LIB} PARENT_SCOPE)
set(3RDPARTY_INC ${3RDPARTY_INC} PARENT_SCOPE)
set(3RDPARTY_DEF ${3RDPARTY_DEF} PARENT_SCOPE)