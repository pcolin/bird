#ADD_LIBRARY(model Message.cpp Price.cpp Order.cpp ProductManager.cpp PositionManager.cpp CashManager.cpp OrderManager.cpp InactiveOrderManager.cpp)
file(GLOB MODEL_SRC_FILES ${PROJECT_SOURCE_DIR}/model/ "*.cpp")
ADD_LIBRARY(model ${MODEL_SRC_FILES})
TARGET_LINK_LIBRARIES(model logger protomsg)
