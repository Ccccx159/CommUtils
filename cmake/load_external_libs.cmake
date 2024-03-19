cmake_minimum_required(VERSION 3.15)

if(WITH_LICENSE)
  # 添加加密库
  set(LICENSE_LIB_PATH "${EXTERNAL_DEPEND_DIR}/lib/license/${platform}/${sdk}")
  link_directories(${LICENSE_LIB_PATH})
  # 优先链接动态库，没有动态库时链接静态库
  file(GLOB LICENSE_LIB_FULL_PATH "${LICENSE_LIB_PATH}/*.so")
  list(LENGTH LICENSE_LIB_FULL_PATH LICENSE_SHARED_LIB_NUMS)

  if(${LICENSE_SHARED_LIB_NUMS} EQUAL 0)
    # 动态库搜索结果为0，则搜索静态库
    file(GLOB LICENSE_LIB_FULL_PATH "${LICENSE_LIB_PATH}/*.a")
    list(LENGTH LICENSE_LIB_FULL_PATH LICENSE_STATIC_LIB_NUMS)

    if(${LICENSE_STATIC_LIB_NUMS} EQUAL 0)
      # 找不到license相关依赖项，终止
      message(FATAL_ERROR "no license depend! please check")
    endif(${LICENSE_STATIC_LIB_NUMS} EQUAL 0)

  endif(${LICENSE_SHARED_LIB_NUMS} EQUAL 0)

  foreach (EACH_LICENSE_LIB ${LICENSE_LIB_FULL_PATH})
    string(REGEX REPLACE ".+/lib(.+)\\..*" "\\1" LICENSE_LIB_NAME ${EACH_LICENSE_LIB})
    list(APPEND LICENSE_DEPEND ${LICENSE_LIB_NAME})
  endforeach()

  # if(${LICENSE_STATIC_LIB_NUMS})
  #   set(LICENSE_DEPEND "-Wl,-Bstatic ${LICENSE_DEPEND} -Wl,-Bdynamic")
  # endif()

  if(${LICENSE_SHARED_LIB_NUMS})
    install(DIRECTORY ${LICENSE_LIB_PATH}/ 
      DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
      FILES_MATCHING
      PATTERN "*.so"
      PATTERN "*.so.+"
      PATTERN "*.a" EXCLUDE
    )
  endif()

endif(WITH_LICENSE)


# 添加 nnx 依赖库
set(NNX_LIB_PATH "${EXTERNAL_DEPEND_DIR}/lib/NNX/${PLATFORM}/${SDK}")
link_directories("${NNX_LIB_PATH}/sdk")
set(CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH};${NNX_LIB_PATH}/moduleConfigs")
file(GLOB NNX_LIB_FULL_PATH "${NNX_LIB_PATH}/moduleConfigs/*.cmake")

foreach (EACH_NNX_LIB ${NNX_LIB_FULL_PATH})
  string(REGEX REPLACE ".+/(.+)Config\\..*" "\\1" NNX_LIB_NAME ${EACH_NNX_LIB})
  if (${NNX_LIB_NAME} STREQUAL "ffmpeg")
    continue()
  endif()
  find_package(${NNX_LIB_NAME} REQUIRED)
  list(APPEND NNX_DEPEND ${NNX_LIB_NAME})
endforeach()

if(USE_CUDA)
  include(cuda)
endif()

# 增加硬解选项
if(WITH_VDEC)
  list(APPEND VDEC_DEPEND vdec_general)
  add_definitions(-DWITH_VDEC)
endif()