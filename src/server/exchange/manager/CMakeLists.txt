ADD_LIBRARY(exchangemanager SHARED TraderApi.cpp ExchangeManager.cpp)
TARGET_LINK_LIBRARIES(exchangemanager logger simulation ctp)

ADD_LIBRARY(exchangebase SHARED TraderApi.cpp MarketDataMonitor.cpp WashTradeProtector.cpp FlowController.cpp)
TARGET_LINK_LIBRARIES(exchangebase logger config strategy)
