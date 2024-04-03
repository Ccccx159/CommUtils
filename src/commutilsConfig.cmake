cmake_minimum_required(VERSION 3.15)
include(CMakeFindDependencyMacro)

find_dependency(Threads REQUIRED)
include("${CMAKE_CURRENT_LIST_DIR}/commutilsTargets.cmake")