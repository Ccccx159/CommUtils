cmake_minimum_required(VERSION 3.15)

option(WITH_SHARED_LIB "" OFF)
option(WITH_UNIT_TESTS "create unit tests from unittest dir" OFF)
option(WITH_ASAN_CHECK "using sanitize to check problems with memleak, etc" OFF)
option(WITH_VDEC "using Alg Integration Department's internal vdec lib to support decoding by hard resources" OFF)
option(WITH_ALG_BASE "using alg_base_func lib to support decoding by soft way" ON)

if (NOT PLATFORM)
  message(FATAL_ERROR "PLATFORM must be set! check your parameters!")
endif(NOT PLATFORM)

if (NOT SDK)
  message(WARNING "now constructing for ${PLATFORM} without SDK!")
endif(NOT SDK)

set(TARGET_NAME "CommUtils" CACHE STRING "target name" FORCE)


# 设置外部依赖根路径
set(EXTERNAL_DEPEND_DIR "${CMAKE_CURRENT_SOURCE_DIR}/.tmp" CACHE STRING "external depend root")
# 导入cmake module path
set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${EXTERNAL_DEPEND_DIR}/cmake_module;${CMAKE_CURRENT_SOURCE_DIR}/cmake")
# 导入cmake module 交叉编译模块必须在project前导入，保证交叉编译器的校验正确
include(toolchain_options)

project(${TARGET_NAME})

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")

# 设置默认编译模式 Debug
if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Choose Release or Debug" FORCE)
endif()

# 打开 asan 时，强制以 Debug 模式编译
if (WITH_ASAN_CHECK)
  set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "must build in Debug way with sanitize" FORCE)
  add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address")
endif()

if (CMAKE_BUILD_TYPE STREQUAL "Debug") 
  add_compile_options(-O0)
endif()
add_compile_options(-Wall -Werror)

# 设置默认安装目录必须在project之后，否则不生效
if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  set(CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_SOURCE_DIR}/artifacts/${CMAKE_BUILD_TYPE}/${TARGET_NAME}" CACHE PATH "default install prefix" FORCE)
endif()

include(load_external_header)
# include(load_external_libs)
include(custom_utils)
include(load_3rdparty)


# 强制导出 compile_command.json, 用于 clangd code intelligent completion
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if(CMAKE_EXPORT_COMPILE_COMMANDS)
  # 使用交叉编译器时 c++ 标准头文件无法追踪，使用以下语句导出交叉编译器对应的标准头文件
  set(CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES ${CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES})
endif()

# 构建目标文件
add_subdirectory(src)

# 是否生成单元测试
if (WITH_UNIT_TESTS)
  enable_testing()
  add_subdirectory(test)
endif()
