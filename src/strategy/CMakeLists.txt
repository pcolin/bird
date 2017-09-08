ADD_LIBRARY(strategy ClusterManager.cpp DeviceManager.cpp StrategyDevice.cpp TestStrategy.cpp MarketMonitor.cpp)
TARGET_LINK_LIBRARIES(strategy model logger disruptor)
