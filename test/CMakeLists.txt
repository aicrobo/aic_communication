cmake_minimum_required(VERSION 3.5)

PROJECT(aic_commu_test)

message(STATUS "#### PROJECT_SOURCE_DIR=" ${PROJECT_SOURCE_DIR})
message(STATUS "#### PROJECT_BINARY_DIR=" ${PROJECT_BINARY_DIR})

add_definitions(-DPROTOBUF_USE_DLLS)

add_executable(req req.cpp)
add_executable(rep rep.cpp)
add_executable(sub sub.cpp)
add_executable(pub pub.cpp)

if(WIN32)

set(DEP_LIB libzmq.lib libprotobuf.lib aic_commu.lib)
target_link_libraries(req ${DEP_LIB})
target_link_libraries(rep ${DEP_LIB})
target_link_libraries(sub ${DEP_LIB})
target_link_libraries(pub ${DEP_LIB})

else(_linux_)

set(DEP_LIB libpthread.so ${LIBRARY_THIRD_PARTY_PATH}/libzmq.a ${LIBRARY_THIRD_PARTY_PATH}/libprotobuf.so.15.0.1 ${LIBRARY_OUTPUT_PATH}/libaic_commu.so)
target_link_libraries(req ${DEP_LIB})
target_link_libraries(rep ${DEP_LIB})
target_link_libraries(sub ${DEP_LIB})
target_link_libraries(pub ${DEP_LIB})

endif()
