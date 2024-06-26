cmake_minimum_required(VERSION 3.13)

project(Heartbeat)
enable_language(C CXX)

# find and link with LLVM
find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -g -w")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -w")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Link with LLVM
include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})
add_definitions(
  -D__STDC_LIMIT_MACROS
  -D__STDC_CONSTANT_MACROS
)
link_directories(${LLVM_LIBRARY_DIRS})

# prepare the pass to be included in the source tree
list(APPEND CMAKE_MODULE_PATH "${LLVM_CMAKE_DIR}")
include(AddLLVM)

# llvm pass
add_subdirectory(src)
