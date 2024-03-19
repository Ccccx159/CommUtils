cmake_minimum_required(VERSION 3.15)

macro(append_source_files_in_current_dir)
  aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} CURRENT_DIR_SRCS)
  list(APPEND PROJ_SOURCES_LISTS ${CURRENT_DIR_SRCS})
  set(PROJ_SOURCES_LISTS ${PROJ_SOURCES_LISTS} PARENT_SCOPE)
endmacro(append_source_files_in_current_dir)

add_definitions(${CUSTOM_DEFINE})
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CUSTOM_OPTION}")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CUSTOM_OPTION}")