cmake_minimum_required(VERSION 3.15)
include(GNUInstallDirs)

# include(CMakePackageConfigHelpers)
# write_basic_package_version_file(
#   ${PROJECT_BINARY_DIR}/cmake/${TARGET_NAME}ConfigVersion.cmake
#   VERSION 0.0.1
#   COMPATIBILITY SameMajorVersion
# )

# configure_package_config_file(
#   ${PROJECT_SOURCE_DIR}/cmake/${TARGET_NAME}Config.cmake.in
#   ${PROJECT_BINARY_DIR}/cmake/${TARGET_NAME}Config.cmake
#   INSTALL_DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/cmake
# )

# 安装头文件
install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/ 
  DESTINATION include
  FILES_MATCHING
  PATTERN "*.h"
  PATTERN "*.hpp"
  PATTERN ".gitkeep" EXCLUDE
)

install(TARGETS ${TARGET_NAME} EXPORT ${TARGET_NAME}Targets
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION bin
  INCLUDES DESTINATION include
)

# install(EXPORT ${TARGET_NAME}Targets
#   FILE ${TARGET_NAME}Targets.cmake
#   NAMESPACE ${TARGET_NAME}::
#   DESTINATION lib/cmake/${TARGET_NAME}
#   )

# install(
#   FILES
#     ${PROJECT_BINARY_DIR}/cmake/${TARGET_NAME}ConfigVersion.cmake
#     ${PROJECT_BINARY_DIR}/cmake/${TARGET_NAME}Config.cmake
#   DESTINATION
#     ${CMAKE_INSTALL_PREFIX}/lib/cmake
# )