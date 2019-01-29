ADD_LIBRARY(exchangemanager STATIC TraderApi.cpp ExchangeManager.cpp)
TARGET_LINK_LIBRARIES(exchangemanager logger simulation ctp)

ADD_LIBRARY(exchangebase STATIC TraderApi.cpp MarketDataMonitor.cpp WashTradeProtector.cpp FlowController.cpp)
TARGET_LINK_LIBRARIES(exchangebase logger config strategy)
