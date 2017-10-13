ADD_EXECUTABLE(proxy ClientProxy.cpp)
#TARGET_LINK_LIBRARIES(proxy nanomsg logger protobuf config protomsg)
TARGET_LINK_LIBRARIES(proxy nanomsg protomsg logger config)
