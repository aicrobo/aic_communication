cmake_minimum_required(VERSION 3.5)

PROJECT(aic_commu_test)

message(STATUS "#### PROJECT_SOURCE_DIR=" ${PROJECT_SOURCE_DIR})
message(STATUS "#### PROJECT_BINARY_DIR=" ${PROJECT_BINARY_DIR})

add_executable(req_json req.cpp)
add_executable(rep_json rep.cpp)
add_executable(sub_json sub.cpp)
add_executable(pub_json pub.cpp)

if(WIN32)

set(DEP_LIB libzmq.lib aic_commu.lib)
target_link_libraries(req_json ${DEP_LIB})
target_link_libraries(rep_json ${DEP_LIB})
target_link_libraries(sub_json ${DEP_LIB})
target_link_libraries(pub_json ${DEP_LIB})

else(_linux_)

set(DEP_LIB libpthread.so ${LIBRARY_OUTPUT_PATH}/libaic_commu.so)
target_link_libraries(req_json ${DEP_LIB})
target_link_libraries(rep_json ${DEP_LIB})
target_link_libraries(sub_json ${DEP_LIB})
target_link_libraries(pub_json ${DEP_LIB})

endif()
