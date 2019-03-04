ADD_LIBRARY(strategy
  base/Strategy.cpp
  base/StrategyDevice.cpp
  base/DeviceManager.cpp
  base/ClusterManager.cpp
  base/MarketMonitor.cpp
  pricer/TheoCalculator.cpp
  pricer/Pricer.cpp
  quoter/Quoter.cpp
  hitter/Hitter.cpp
  dimer/Dimer.cpp
  hedger/Hedger.cpp
  test/TestStrategy.cpp)
#file(GLOB STRATEGY_SRC_FILES ${PROJECT_SOURCE_DIR}/strategy/ "*.cpp")
#ADD_LIBRARY(strategy ${STRATEGY_SRC_FILES})
TARGET_LINK_LIBRARIES(strategy protomsg model calculator logger disruptor boost_date_time tbb)
