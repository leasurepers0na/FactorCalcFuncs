cmake_minimum_required(VERSION 3.10)
project(FactorCalcFuncs CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# 因子计算算子库：Rolling*Helper / sn_* 算子 / 公共函数
# 算子源码只存在于本工程；FactorReplay 与因子 DLL 链接本库。
file(GLOB OPS_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)

add_library(FactorCalcFuncs SHARED ${OPS_SRC})

# include/        对外头文件（随库安装）
# include_private/ 仅本库编译使用（ABI 头由使用方提供，避免与平台副本冲突）
target_include_directories(FactorCalcFuncs
  PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/include
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include_private
)
target_compile_options(FactorCalcFuncs PRIVATE -O3 -fPIC -w)

install(TARGETS FactorCalcFuncs DESTINATION lib)
install(DIRECTORY include/ DESTINATION include)
