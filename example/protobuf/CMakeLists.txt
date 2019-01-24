cmake_minimum_required(VERSION 3.5)

PROJECT(aic_commu_test)

message(STATUS "#### PROJECT_SOURCE_DIR=" ${PROJECT_SOURCE_DIR})
message(STATUS "#### PROJECT_BINARY_DIR=" ${PROJECT_BINARY_DIR})

add_executable(req_proto req.cpp packet.pb.cc)
add_executable(rep_proto rep.cpp packet.pb.cc)
add_executable(sub_proto sub.cpp packet.pb.cc)
add_executable(pub_proto pub.cpp packet.pb.cc)

if(WIN32)

add_definitions(-DPROTOBUF_USE_DLLS)

set(DEP_LIB libzmq.lib libprotobuf.lib aic_commu.lib)
target_link_libraries(req_proto ${DEP_LIB})
target_link_libraries(rep_proto ${DEP_LIB})
target_link_libraries(sub_proto ${DEP_LIB})
target_link_libraries(pub_proto ${DEP_LIB})

else(_linux_)

set(DEP_LIB libpthread.so libprotobuf.so ${LIBRARY_OUTPUT_PATH}/libaic_commu.so)
target_link_libraries(req_proto ${DEP_LIB})
target_link_libraries(rep_proto ${DEP_LIB})
target_link_libraries(sub_proto ${DEP_LIB})
target_link_libraries(pub_proto ${DEP_LIB})

endif()
