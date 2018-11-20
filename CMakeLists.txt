cmake_minimum_required(VERSION 3.5)

project(aic_commu)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 11)

set(CMAKE_BUILD_TYPE Release)
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR})
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR})
set(LIBRARY_THIRD_PARTY_PATH ${PROJECT_SOURCE_DIR}/lib)

message(STATUS "#### PROJECT_SOURCE_DIR=" ${PROJECT_SOURCE_DIR})
message(STATUS "#### PROJECT_BINARY_DIR=" ${PROJECT_BINARY_DIR})
message(STATUS "#### LIBRARY_OUTPUT_PATH=" ${LIBRARY_OUTPUT_PATH} )
message(STATUS "#### EXECUTABLE_OUTPUT_PATH=" ${EXECUTABLE_OUTPUT_PATH})

if(_linux_)
include_directories(
  "/usr/local/include/"
  "/usr/include/"
)
else(WIN32)
include_directories(
  "include/"
)
endif()

add_subdirectory(src)
add_subdirectory(test)
