#ADD_LIBRARY(strategy ClusterManager.cpp DeviceManager.cpp StrategyDevice.cpp TestStrategy.cpp MarketMonitor.cpp Pricer.cpp)
file(GLOB STRATEGY_SRC_FILES ${PROJECT_SOURCE_DIR}/strategy/ "*.cpp")
ADD_LIBRARY(strategy ${STRATEGY_SRC_FILES})
TARGET_LINK_LIBRARIES(strategy protomsg model calculator logger disruptor boost_date_time)
