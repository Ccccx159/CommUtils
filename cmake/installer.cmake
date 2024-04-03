cmake_minimum_required(VERSION 3.15)

install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/ 
  DESTINATION include
  FILES_MATCHING
  PATTERN "*.h"
  PATTERN "*.hpp"
  PATTERN ".gitkeep" EXCLUDE
)

install(FILES src/commutilsConfig.cmake DESTINATION lib/cmake/${PROJECT_NAME})

install(DIRECTORY 3rdparty/fmt/include/ DESTINATION include)

install(FILES 3rdparty/concurrentqueue/concurrentqueue.h DESTINATION include/concurrentqueue)

install(FILES 3rdparty/nlohmann_json/single_include/nlohmann/json.hpp DESTINATION include/nlohmann)

if (COMMUTILS_WITH_SPDLOG)
  install(DIRECTORY 3rdparty/spdlog/include/ DESTINATION include)
endif()

install(TARGETS ${COMMUTILS_TARGET_NAME} EXPORT ${COMMUTILS_TARGET_NAME}Targets
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION bin
  INCLUDES DESTINATION include
)

# 安装依赖目标
install(TARGETS nlohmann_json
        EXPORT ${COMMUTILS_TARGET_NAME}Targets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include)

install(TARGETS fmt-header-only
        EXPORT ${COMMUTILS_TARGET_NAME}Targets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include)

install(TARGETS spdlog_header_only
        EXPORT ${COMMUTILS_TARGET_NAME}Targets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include)

install(TARGETS concurrentqueue
        EXPORT ${COMMUTILS_TARGET_NAME}Targets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include)


install(EXPORT ${COMMUTILS_TARGET_NAME}Targets
  FILE ${COMMUTILS_TARGET_NAME}Targets.cmake
  NAMESPACE ${PROJECT_NAME}::
  DESTINATION lib/cmake/${PROJECT_NAME}
)

set(PROJECT_VERSION 1.0.0)
include(CMakePackageConfigHelpers)
write_basic_package_version_file("commutilsConfigVersion.cmake"
                                 VERSION ${PROJECT_VERSION}
                                 COMPATIBILITY SameMajorVersion)

# 安装配置文件
install(FILES src/commutilsConfig.cmake ${CMAKE_CURRENT_BINARY_DIR}/commutilsConfigVersion.cmake
        DESTINATION lib/cmake/commutils)