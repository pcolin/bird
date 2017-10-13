set(PROTO_FILES
  ../../share/msg/Heartbeat.proto
  ../../share/msg/Login.proto
  ../../share/msg/Server.proto
  ../../share/msg/Cash.proto
  ../../share/msg/Position.proto
  ../../share/msg/Price.proto
  ../../share/msg/Order.proto
  ../../share/msg/Trade.proto
  ../../share/msg/Strategy.proto)

message ("Current Dir: ${CMAKE_CURRENT_SOURCE_DIR}")
set(CC_FILES)
#set(PROTOC_COMMAND "/home/test/bird/server/3rd_library/protobuf/src/protoc")
foreach(proto_file ${PROTO_FILES})
  string(REPLACE ".proto" ".pb.h" abs_file_h ${proto_file})
  string(REPLACE "../../share/msg/" "" file_h ${abs_file_h})
  string(REPLACE ".proto" ".pb.cc" abs_file_cc ${proto_file})
  string(REPLACE "../../share/msg/" "" file_cc ${abs_file_cc})
  list(APPEND CC_FILES ${file_cc})
  add_custom_command(
    OUTPUT ${file_h} ${file_cc}
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/../3rd_library/protobuf/src/protoc
    ARGS -I${CMAKE_CURRENT_SOURCE_DIR}/../3rd_library/protobuf/src --cpp_out . ${CMAKE_CURRENT_SOURCE_DIR}/${proto_file} --proto_path='${CMAKE_CURRENT_SOURCE_DIR}/../../share/msg'
    DEPENDS ${proto_file})
endforeach()

add_library(protomsg ${CC_FILES})
target_link_libraries(protomsg protobuf pthread)
add_custom_target(PROTOC ALL DEPENDS ${PROTO_FILES})
