cmake_minimum_required(VERSION 3.15)

function(append_source_files_in_current_dir source_list)
  aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} CURRENT_DIR_SRCS)
  list(APPEND ${source_list} ${CURRENT_DIR_SRCS})
  set(${source_list} ${${source_list}} PARENT_SCOPE)
endfunction()

# get keyword of target
function(get_keyword_for_target target_name keywork)
  get_target_property(target_type ${target_name} TYPE)
  set(keyword "PRIVATE" PARENT_SCOPE)
  if (${target_type} STREQUAL "INTERFACE_LIBRARY")
    set(keyword  "INTERFACE" PARENT_SCOPE)
  endif()
endfunction()

# Enable address sanitizer (gcc/clang only)
function(target_enable_sanitizer target_name)
    if(NOT CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        message(FATAL_ERROR "Sanitizer supported only for gcc/clang")
    endif()
    get_keyword_for_target(${target_name} keyword)
    message(STATUS "${target_name} - ${keyword}")
    # message(STATUS "Address sanitizer enabled")
    target_compile_options(${target_name} ${keyword} -g -O0)
    target_compile_options(${target_name} ${keyword} -fsanitize=address,undefined)
    target_compile_options(${target_name} ${keyword} -fno-sanitize=signed-integer-overflow)
    target_compile_options(${target_name} ${keyword} -fno-sanitize-recover=all)
    target_compile_options(${target_name} ${keyword} -fno-omit-frame-pointer)
    target_link_libraries(${target_name} ${keyword} -fsanitize=address,undefined -fuse-ld=gold)
endfunction()

# link 3rdparty
function(target_add_3rdparty target_name)
    get_keyword_for_target(${target_name} keyword)
    message(STATUS "${target_name} - ${keyword}")
    target_include_directories(${target_name} ${keyword} ${3RDPARTY_INC})
    target_link_libraries(${target_name} ${keyword} ${3RDPARTY_LIB})
    target_compile_definitions(${target_name} ${keyword} ${3RDPARTY_DEF})
endfunction()

# link common lib
function(target_add_comm_lib target_name) 
    get_keyword_for_target(${target_name} keyword)
    message(STATUS "${target_name} - ${keyword}")
    target_link_libraries(${target_name} ${keyword}
        "-Wl,--start-group"
        ${NNX_DEPEND}
        ${LICENSE_DEPEND}
        "-Wl,--end-group"
        pthread m rt dl
    ) 
endfunction()



add_definitions(${CUSTOM_DEFINE})
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CUSTOM_OPTION}")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CUSTOM_OPTION}")