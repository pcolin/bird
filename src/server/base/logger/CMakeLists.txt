# project(logger)

ADD_LIBRARY(logger SHARED Logging.cpp LogStream.cpp AsyncLogging.cpp LogFile.cpp Timestamp.cpp)
