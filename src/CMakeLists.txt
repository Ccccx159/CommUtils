
cmake_minimum_required(VERSION 3.15)

find_package(Threads REQUIRED)

# set library
add_library(${COMMUTILS_TARGET_NAME}
  ring_buffer.c
)

# set include dir
target_include_directories(${COMMUTILS_TARGET_NAME}
  PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/3rdparty/>
    $<INSTALL_INTERFACE:include>
)

# Public headers still include 3rdparty headers, so keep these dependencies
# visible to consumers even though commutils now has compiled sources.
target_link_libraries(${COMMUTILS_TARGET_NAME}
  PUBLIC
    ${3RDPARTY_LIB}
    Threads::Threads
)

target_compile_definitions(${COMMUTILS_TARGET_NAME}
  PUBLIC
    ${3RDPARTY_DEF}
)
