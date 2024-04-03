
cmake_minimum_required(VERSION 3.2)

# set interface lib
add_library(${COMMUTILS_TARGET_NAME} INTERFACE)

# set include dir
target_include_directories(${COMMUTILS_TARGET_NAME}
  INTERFACE
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

# add 3rdparty option
target_add_3rdparty(${COMMUTILS_TARGET_NAME})