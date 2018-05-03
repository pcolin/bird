ADD_LIBRARY(simulation STATIC SimulationTraderApi.cpp SimulationMdApi.cpp)
TARGET_LINK_LIBRARIES(simulation logger config model strategy exchangebase)
