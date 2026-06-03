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
  get_filename_component(unittest_name ${source_file} NAME_WE)

  # add target
  add_executable(${unittest_name} ${source_file})
  
  # 添加 -fno-pie 选项，避免较新的 ubuntu 系统下默认添加 -pie 导致可执行文件被强制标记为共享对象文件
  set_target_properties(${unittest_name} PROPERTIES LINK_FLAGS "-no-pie")

  # add ctest
  add_test(NAME ${unittest_name} COMMAND ${unittest_name})
  if (COMMUTILS_SANITIZE_ADDRESS)
    if (COMMUTILS_SANITIZE_LEAK)
      set(commutils_asan_detect_leaks 1)
    else()
      set(commutils_asan_detect_leaks 0)
    endif()
    set_tests_properties(${unittest_name} PROPERTIES
      ENVIRONMENT "ASAN_OPTIONS=detect_leaks=${commutils_asan_detect_leaks}:halt_on_error=1:abort_on_error=1;LSAN_OPTIONS=exitcode=23;UBSAN_OPTIONS=halt_on_error=1:abort_on_error=1"
    )
  endif()
  set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/unittest)

  # link library
  target_link_libraries(${unittest_name} 
    PRIVATE
      ${COMMUTILS_TARGET_NAME}
      gtest
      dl
      pthread
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
