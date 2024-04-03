
cmake_minimum_required(VERSION 3.15)

# set interface lib
add_library(${COMMUTILS_TARGET_NAME} INTERFACE)

# set include dir
target_include_directories(${COMMUTILS_TARGET_NAME}
  INTERFACE
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/3rdparty/>
    $<INSTALL_INTERFACE:include>
)

# add 3rdparty option
target_add_3rdparty(${COMMUTILS_TARGET_NAME})