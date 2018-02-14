ADD_EXECUTABLE(databased DatabaseD.cpp sqlite3.c DbBase.cpp UserDB.cpp)
TARGET_LINK_LIBRARIES(databased dl nanomsg protomsg logger config)
