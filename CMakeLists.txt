
# -----------------------------------------------------------------------------
# download external depend
# -----------------------------------------------------------------------------
if (NOT sdk)
  execute_process(COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/scripts/build_pre.py -p ${platform} RESULT_VARIABLE result)
else()
  execute_process(COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/scripts/build_pre.py -p ${platform} -s ${sdk} RESULT_VARIABLE result)
endif()
if (NOT ${result} EQUAL 0)
  message("Error: python3 ${CMAKE_CURRENT_SOURCE_DIR}/scripts/build_pre.py -p ${platform} -s ${sdk} Failed!")
  message(FATAL_ERROR "Stopping configuration!")
endif()

# -----------------------------------------------------------------------------
# cmake minimum
# -----------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.15)

# -----------------------------------------------------------------------------
# Start project
# -----------------------------------------------------------------------------
set(COMMUTILS_EXTERN_DIR "${CMAKE_CURRENT_SOURCE_DIR}/.depend" CACHE STRING "external depend download dir")

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake ${COMMUTILS_EXTERN_DIR}/cmake_module)

include(utils)

set(COMMUTILS_TARGET_NAME "commutils")
project(${COMMUTILS_TARGET_NAME})

# -----------------------------------------------------------------------------
# Set default build to release
# -----------------------------------------------------------------------------
if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose Release or Debug" FORCE)
endif()

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
  add_compile_options(-g -O0 -fno-omit-frame-pointer)
endif()

# -----------------------------------------------------------------------------
# special platform configuration
# -----------------------------------------------------------------------------
# 特殊平台配置
if("teslaT4" STREQUAL ${platform})
  set(USE_CUDA "ON")
  set(HAVE_OPENCV "ON")
  include(cuda)
endif()

# -----------------------------------------------------------------------------
# Compiler config
# -----------------------------------------------------------------------------
if (NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 11)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif()

# -----------------------------------------------------------------------------
# include module
# -----------------------------------------------------------------------------
include(GNUInstallDirs)
include(custom_utils)
# include(load_external_header)
# include(load_external_libs)

# -----------------------------------------------------------------------------
# Set COMMUTILS_MASTER_PROJECT to ON if we are building spdlog
# -----------------------------------------------------------------------------
# Check if commutils is being used directly or via add_subdirectory, but allow overriding
if (NOT DEFINED COMMUTILS_MASTER_PROJECT)
  if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    set(COMMUTILS_MASTER_PROJECT ON)
  else()
    set(COMMUTILS_MASTER_PROJECT OFF)
  endif()
endif()

if (COMMUTILS_MASTER_PROJECT)
  # 设置默认安装目录必须在project之后，否则不生效
  if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_SOURCE_DIR}/artifacts/${CMAKE_BUILD_TYPE}/${COMMUTILS_TARGET_NAME}" CACHE PATH "default install prefix" FORCE)
  endif()

  # 强制导出 compile_command.json, 用于 clangd code intelligent completion
  set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
  if(CMAKE_EXPORT_COMPILE_COMMANDS)
    # 使用交叉编译器时 c++ 标准头文件无法追踪，使用以下语句导出交叉编译器对应的标准头文件
    set(CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES ${CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES})
  endif()
endif()

# unit test option
option(COMMUTILS_BUILD_TEST_HO "Build unit tests with gtest" OFF)

# if not master project, do not install
option(COMMUTILS_INSTALL "Install commutils" ${COMMUTILS_MASTER_PROJECT})

# build with spdlog option
option(COMMUTILS_WITH_SPDLOG "Build with spdlog" ON)

# sanitizer option
option(COMMUTILS_SANITIZE_ADDRESS "Eable address sanitizer in tests" OFF)

# custon target option
option(COMMUTILS_CUSTOM_TARGET "Custom target" OFF)

# -----------------------------------------------------------------------------
# load 3rdparty
# -----------------------------------------------------------------------------
include(load_3rdparty)

# -----------------------------------------------------------------------------
# add src subdirectory
# -----------------------------------------------------------------------------
add_subdirectory(src)

# -----------------------------------------------------------------------------
# add unit test
# -----------------------------------------------------------------------------
if (COMMUTILS_BUILD_TEST_HO)
  enable_testing()
  add_subdirectory(test)
endif()


# -----------------------------------------------------------------------------
# add custom target
# -----------------------------------------------------------------------------
# This script is used to modify the compile_commands.json file to help the 
# clang-tidy tool to recognize the GNU compiler version.
if (COMMUTILS_CUSTOM_TARGET)
  add_custom_target(commutils_py_script ALL
    COMMAND ${CMAKE_COMMAND} -E echo "Running commutils Python script..."
    COMMAND python3 ${CMAKE_SOURCE_DIR}/scripts/build_post.py
    COMMENT "Running commutils Python script..."
  )
endif()

# -----------------------------------------------------------------------------
# install
# -----------------------------------------------------------------------------
if (COMMUTILS_INSTALL)
  include(installer)
endif()





