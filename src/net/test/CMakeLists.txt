add_executable(serverTest serverTest.cc)
target_link_libraries(serverTest Tiny_WebServer)
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/src/net/test)