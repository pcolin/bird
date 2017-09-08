ADD_LIBRARY(ctp STATIC CtpTraderApi.cpp CtpTraderSpi.cpp CtpMdApi.cpp CtpMdSpi.cpp)
TARGET_LINK_LIBRARIES(ctp thosttraderapi thostmduserapi logger config model strategy)
