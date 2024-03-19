cmake_minimum_required(VERSION 3.15)

set(platform "${PLATFORM}")
set(sdk "${SDK}")

include(platform/${PLATFORM})

# 特殊平台配置
if("teslaT4" STREQUAL ${PLATFORM})
  set(USE_CUDA "ON")
  set(HAVE_OPENCV "ON")
endif()
