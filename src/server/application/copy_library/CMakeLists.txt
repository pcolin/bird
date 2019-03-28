message("copy all 3rd libraries...")
add_custom_target(copy_libraries ALL
  COMMAND ${CMAKE_COMMAND} -E copy "${3RD_LIBRARY_DIR}/boost/lib/libboost_date_time.so.1.67.0" "${LIBRARY_OUTPUT_PATH}"
  COMMAND ${CMAKE_COMMAND} -E copy "${3RD_LIBRARY_DIR}/nanomsg/lib/libnanomsg.so.5" "${LIBRARY_OUTPUT_PATH}"
  COMMAND ${CMAKE_COMMAND} -E copy "${3RD_LIBRARY_DIR}/protobuf/lib/libprotobuf.so.18" "${LIBRARY_OUTPUT_PATH}"
  COMMAND ${CMAKE_COMMAND} -E copy "${3RD_LIBRARY_DIR}/tbb/lib/libtbb.so.2" "${LIBRARY_OUTPUT_PATH}"
  COMMAND ${CMAKE_COMMAND} -E copy "${3RD_LIBRARY_DIR}/tcmalloc/lib/libtcmalloc.so.4" "${LIBRARY_OUTPUT_PATH}"
  COMMAND ${CMAKE_COMMAND} -E copy "${3RD_LIBRARY_DIR}/ctp/lib/libthostmduserapi.so" "${LIBRARY_OUTPUT_PATH}"
  COMMAND ${CMAKE_COMMAND} -E copy "${3RD_LIBRARY_DIR}/ctp/lib/libthosttraderapi.so" "${LIBRARY_OUTPUT_PATH}"
  )
