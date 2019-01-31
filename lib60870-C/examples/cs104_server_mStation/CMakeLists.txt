include_directories(
   .
)

set(example_SRCS
   simple_server_mStation.c  #simple_server
)

IF(WIN32)
set_source_files_properties(${example_SRCS}
                                       PROPERTIES LANGUAGE CXX)
ENDIF(WIN32)

add_executable(cs104_server_mStation
  ${example_SRCS}
)

target_link_libraries(cs104_server_mStation
    lib60870
)
