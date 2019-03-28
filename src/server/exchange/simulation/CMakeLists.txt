ADD_LIBRARY(simulation SHARED SimulationTraderApi.cpp SimulationMdApi.cpp)
TARGET_LINK_LIBRARIES(simulation logger config model strategy exchangebase)
