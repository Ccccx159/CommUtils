cmake_minimum_required(VERSION 3.15)

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

install(DIRECTORY 3rdparty/fmt/include/ DESTINATION include)

install(FILES 3rdparty/concurrentqueue/concurrentqueue.h DESTINATION include)

install(FILES 3rdparty/nlohmann_json/single_include/nlohmann/json.hpp DESTINATION include/nlohmann)

if (COMMUTILS_WITH_SPDLOG)
  install(DIRECTORY 3rdparty/spdlog/include/ DESTINATION include)
endif()