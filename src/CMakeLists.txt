cmake_minimum_required(VERSION 3.5)

project(aic_commu_lib)

message(STATUS "#### PROJECT_SOURCE_DIR=" ${PROJECT_SOURCE_DIR})
message(STATUS "#### PROJECT_BINARY_DIR=" ${PROJECT_BINARY_DIR})

add_definitions(-DAIC_COMMU_BUILD)
add_definitions(-DPROTOBUF_USE_DLLS)

if(WIN32)

file(GLOB SRC_FILES "../include/*.h" "*.cc" "*.cpp" "*.h" "*.hpp")
add_library(aic_commu SHARED ${SRC_FILES})
target_link_libraries(
    aic_commu
    libzmq.lib
    libprotobuf.lib)

else(_linux_)

file(GLOB SRC_FILES "*.cc" "*.cpp" "*.hpp")
add_library(aic_commu SHARED ${SRC_FILES})
target_link_libraries(
    aic_commu
    libzmq.a
#如果提示undefined reference to 'google::protobuf...'，请确认libprotobuf.so是否在LD_LIBRARY_PATH环境变量设定的目录中，且libprotobuf版本为3.5.1以上，一般在/usr/lib /usr/lib/x86_64-linux-gnu目录下会有同名的低版本文件
    libprotobuf.so)

install(TARGETS aic_commu DESTINATION /usr/lib)
install(FILES ${PROJECT_SOURCE_DIR}/aic_commu.h DESTINATION /usr/include)


install(FILES ${CMAKE_BINARY_DIR}/libprotobuf.so DESTINATION /usr/lib)
install(FILES ${CMAKE_BINARY_DIR}/libprotobuf.so.15 DESTINATION /usr/lib)
install(FILES ${LIBRARY_THIRD_PARTY_PATH}/libprotobuf.so.15.0.1 DESTINATION /usr/lib)

endif()
