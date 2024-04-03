cmake_minimum_required(VERSION 3.2)

# 添加外部依赖公共头文件路径
include_directories(
  ${COMMUTILS_EXTERN_DIR}/include/public
  ${COMMUTILS_EXTERN_DIR}/include/public/common
  ${COMMUTILS_EXTERN_DIR}/include/public/cbir
  ${COMMUTILS_EXTERN_DIR}/include/alg
  ${COMMUTILS_EXTERN_DIR}/include/alg/imgpreproc
  ${COMMUTILS_EXTERN_DIR}/include/alg/sdk
  ${COMMUTILS_EXTERN_DIR}/include/alg/sdk/osa
  ${COMMUTILS_EXTERN_DIR}/include/alg/sdk/utils
  ${COMMUTILS_EXTERN_DIR}/include/alg/sdk/sdk
  ${COMMUTILS_EXTERN_DIR}/include/alg/platform
  ${COMMUTILS_EXTERN_DIR}/include/license
  ${COMMUTILS_EXTERN_DIR}/include/pal
)