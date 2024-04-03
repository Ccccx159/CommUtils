cmake_minimum_required(VERSION 3.15)

# 添加加密库
set(LICENSE_LIB_PATH "${COMMUTILS_EXTERN_DIR}/lib/license/${platform}/${sdk}")
link_directories(${LICENSE_LIB_PATH})
# 优先链接静态库，没有静态库时链接动态库
file(GLOB LICENSE_LIB_FULL_PATH "${LICENSE_LIB_PATH}/*.a")
list(LENGTH LICENSE_LIB_FULL_PATH LICENSE_STATIC_LIB_NUMS)

if(${LICENSE_STATIC_LIB_NUMS} EQUAL 0)
  # 静态库搜索结果为0，则搜索动态库
  file(GLOB LICENSE_LIB_FULL_PATH "${LICENSE_LIB_PATH}/*.so")
  list(LENGTH LICENSE_LIB_FULL_PATH LICENSE_SHARED_LIB_NUMS)

  if(${LICENSE_SHARED_LIB_NUMS} EQUAL 0)
    # 找不到license相关依赖项，终止
    message(WARNING ${CMAKE_CURRENT_SOURCE_DIR})
    message(WARNING ${LICENSE_LIB_PATH})
    message(FATAL_ERROR "no license depend! please check")
  endif()
endif()

foreach (EACH_LICENSE_LIB ${LICENSE_LIB_FULL_PATH})
  string(REGEX REPLACE ".+/(lib.+\\..*)" "\\1" LICENSE_LIB_NAME ${EACH_LICENSE_LIB})
  list(APPEND LICENSE_DEPEND ${LICENSE_LIB_NAME})
endforeach()
message(STATUS "license lib: "${LICENSE_DEPEND})


# 添加 nnx 依赖库
set(NNX_LIB_PATH "${COMMUTILS_EXTERN_DIR}/lib/NNX/${platform}/${sdk}")
link_directories("${NNX_LIB_PATH}/sdk")
set(CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH};${NNX_LIB_PATH}/moduleConfigs")
file(GLOB NNX_LIB_FULL_PATH "${NNX_LIB_PATH}/moduleConfigs/*.cmake")

foreach (EACH_NNX_LIB ${NNX_LIB_FULL_PATH})
  string(REGEX REPLACE ".+/(.+)Config\\..*" "\\1" NNX_LIB_NAME ${EACH_NNX_LIB})
  if (${NNX_LIB_NAME} STREQUAL "ffmpeg")
    continue()
  endif()
  if (NOT TARGET ${NNX_LIB_NAME})
    find_package(${NNX_LIB_NAME} REQUIRED)
    list(APPEND NNX_DEPEND ${NNX_LIB_NAME})
  endif()
endforeach()
message(STATUS "NNX libs: "${NNX_DEPEND})