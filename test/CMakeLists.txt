cmake_minimum_required(VERSION 3.15)

# set project
project(${COMMUTILS_TARGET_NAME}_unittest)

# get source list
set(unittest_sources_list "")
append_source_files_in_current_dir(unittest_sources_list)

message(STATUS ${unittest_sources_list})
# build target for each source file
foreach(source_file ${unittest_sources_list})
  
  # recognize *.c *.cc *.cpp *.cxx, get test name
  STRING(REGEX REPLACE ".+/(.*)\\.c.+" "\\1" unittest_name ${source_file})

  # add target
  add_executable(${unittest_name} ${source_file})
  
  # 添加 -fno-pie 选项，避免较新的 ubuntu 系统下默认添加 -pie 导致可执行文件被强制标记为共享对象文件
  set_target_properties(${unittest_name} PROPERTIES LINK_FLAGS "-no-pie")

  # add ctest
  add_test(NAME ${unittest_name} COMMAND ${unittest_name})
  set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/unittest)

  # link library
  target_link_libraries(${unittest_name} 
    PRIVATE
      ${COMMUTILS_TARGET_NAME}
      gtest dl pthread
  )
  
  # use sanitize
  if (COMMUTILS_SANITIZE_ADDRESS)
    target_enable_sanitizer(${unittest_name})
  endif()
  
  # install target
  install(TARGETS ${unittest_name}
          RUNTIME DESTINATION test
          )
  
endforeach()
