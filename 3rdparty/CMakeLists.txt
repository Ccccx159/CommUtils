cmake_minimum_required(VERSION 3.2)
project(3rdparty)

# using gtest
if (WITH_UNIT_TESTS)
  set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
  add_subdirectory(googletest)
endif()

# using fmt
add_subdirectory(fmt EXCLUDE_FROM_ALL)
install(DIRECTORY fmt/include/ DESTINATION include)

# using spdlog and set external fmt
if (WITH_SPDLOG)
  set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
  set(SPDLOG_FMT_EXTERNAL_HO ON)
  add_subdirectory(spdlog EXCLUDE_FROM_ALL)
  install(DIRECTORY spdlog/include/ DESTINATION include)
endif()

# using concurrentqueue
add_subdirectory(concurrentqueue EXCLUDE_FROM_ALL)
install(FILES concurrentqueue/concurrentqueue.h DESTINATION include)

# using nlohmann::json
add_subdirectory(nlohmann_json EXCLUDE_FROM_ALL)
install(FILES nlohmann_json/single_include/nlohmann/json.hpp DESTINATION include/nlohmann)