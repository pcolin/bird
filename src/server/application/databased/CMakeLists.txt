ADD_EXECUTABLE(databased
  sqlite3.c
  DatabaseD.cpp
  DbBase.cpp
  UserDB.cpp
  InstrumentDB.cpp
  ExchangeParameterDB.cpp
  PositionDB.cpp
  CashLimitDB.cpp
  InterestRateDB.cpp
  SSRateDB.cpp
  VolatilityCurveDB.cpp
  CreditDB.cpp
  PricerDB.cpp
  DestrikerDB.cpp)
TARGET_LINK_LIBRARIES(databased dl nanomsg protomsg logger config boost_date_time)
