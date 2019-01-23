cmake_minimum_required(VERSION 3.5)

project(aic_commu_lib)

message(STATUS "#### PROJECT_SOURCE_DIR=" ${PROJECT_SOURCE_DIR})
message(STATUS "#### PROJECT_BINARY_DIR=" ${PROJECT_BINARY_DIR})

add_definitions(-DAIC_COMMU_BUILD)
add_definitions(-DPROTOBUF_USE_DLLS)

if(WIN32)

file(GLOB SRC_FILES "../include/*.h" "*.cpp" "*.h" "*.hpp")
add_library(aic_commu SHARED ${SRC_FILES})
target_link_libraries(
    aic_commu
    libzmq.lib
  )

else(_linux_)

file(GLOB SRC_FILES "*.cc" "*.cpp" "*.hpp")
add_library(aic_commu SHARED ${SRC_FILES})
target_link_libraries(
    aic_commu
    libzmq.a
  )

install(TARGETS aic_commu DESTINATION /usr/lib)
install(FILES ${PROJECT_SOURCE_DIR}/aic_commu.h DESTINATION /usr/include)

endif()
