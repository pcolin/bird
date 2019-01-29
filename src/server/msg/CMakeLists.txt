set(PROTO_FILES
  ../../share/msg/Heartbeat.proto
  ../../share/msg/Request.proto
  ../../share/msg/Reply.proto
  ../../share/msg/Exchange.proto
  ../../share/msg/Login.proto
  ../../share/msg/Server.proto
  ../../share/msg/Currency.proto
  ../../share/msg/Instrument.proto
  ../../share/msg/ProductParameter.proto
  ../../share/msg/Cash.proto
  ../../share/msg/CashLimit.proto
  ../../share/msg/Position.proto
  ../../share/msg/Price.proto
  ../../share/msg/Order.proto
  ../../share/msg/Trade.proto
  ../../share/msg/InterestRate.proto
  ../../share/msg/SSRate.proto
  ../../share/msg/Volatility.proto
  ../../share/msg/Elastic.proto
  ../../share/msg/Destriker.proto
  ../../share/msg/Pricer.proto
  ../../share/msg/Quoter.proto
  ../../share/msg/Hitter.proto
  ../../share/msg/Dimer.proto
  ../../share/msg/Strategy.proto
  ../../share/msg/Credit.proto
  ../../share/msg/RequestForQuote.proto)

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
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/../../share/tools/protobuf/src/protoc
    ARGS -I${CMAKE_CURRENT_SOURCE_DIR}/../../share/tools/protobuf/src --cpp_out . ${CMAKE_CURRENT_SOURCE_DIR}/${proto_file} --proto_path='${CMAKE_CURRENT_SOURCE_DIR}/../../share/msg'
    DEPENDS ${proto_file})
endforeach()

add_library(protomsg ${CC_FILES})
target_link_libraries(protomsg protobuf pthread)
add_custom_target(PROTOC ALL DEPENDS ${PROTO_FILES})
