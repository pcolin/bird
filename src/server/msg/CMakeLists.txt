set(PROTO_FILES
  ../../share/msg/Position.proto)

set(CC_FILES)
foreach(proto_file ${PROTO_FILES})
  string(REPLACE ".proto" ".pb.h" abs_file_h ${proto_file})
  string(REPLACE "../../share/msg/" "" file_h ${abs_file_h})
  string(REPLACE ".proto" ".pb.cc" abs_file_cc ${proto_file})
  string(REPLACE "../../share/msg/" "" file_cc ${abs_file_cc})
  list(APPEND CC_FILES ${file_cc})
  add_custom_command(
    OUTPUT ${file_h} ${file_cc}
    COMMAND protoc
    ARGS --cpp_out . ${CMAKE_CURRENT_SOURCE_DIR}/${proto_file} --proto_path='${CMAKE_CURRENT_SOURCE_DIR}/../../share/msg'
    DEPENDS ${proto_file})
endforeach()

add_library(protomsg ${CC_FILES})
target_link_libraries(protomsg protobuf pthread)
add_custom_target(PROTOC ALL DEPENDS ${PROTO_FILES})
