cmake_minimum_required(VERSION 3.15)

if (WITH_SHARED_LIB)
  set(BUILD_LIB_TYPE "SHARED")
else()
  set(BUILD_LIB_TYPE "STATIC")
endif()

add_library(${TARGET_NAME} INTERFACE)

# 添加头文件依赖，并设置头文件导出
target_include_directories(${TARGET_NAME}
  INTERFACE
    ${PROJECT_SOURCE_DIR}/include
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(${TARGET_NAME} 
  INTERFACE
    fmt::fmt-header-only
    concurrentqueue
    nlohmann_json::nlohmann_json
)

if (WITH_SPDLOG)
  target_compile_definitions(${TARGET_NAME} INTERFACE _WITH_SPDLOG)
  target_link_libraries(${TARGET_NAME} 
    INTERFACE
      spdlog::spdlog_header_only
  )
endif()

include(installer)
