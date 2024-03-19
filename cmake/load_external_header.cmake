cmake_minimum_required(VERSION 3.15)

# 添加外部依赖公共头文件路径
include_directories(
  ${EXTERNAL_DEPEND_DIR}/include/public
  ${EXTERNAL_DEPEND_DIR}/include/public/common
  ${EXTERNAL_DEPEND_DIR}/include/public/cbir
  ${EXTERNAL_DEPEND_DIR}/include/alg
  ${EXTERNAL_DEPEND_DIR}/include/alg/imgpreproc
  ${EXTERNAL_DEPEND_DIR}/include/alg/sdk
  ${EXTERNAL_DEPEND_DIR}/include/alg/sdk/osa
  ${EXTERNAL_DEPEND_DIR}/include/alg/sdk/utils
  ${EXTERNAL_DEPEND_DIR}/include/alg/sdk/sdk
  ${EXTERNAL_DEPEND_DIR}/include/alg/platform
  ${EXTERNAL_DEPEND_DIR}/include/license
  ${EXTERNAL_DEPEND_DIR}/include/pal
)