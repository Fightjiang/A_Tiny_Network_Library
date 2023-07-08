add_executable(logTest logTest.cc)
target_link_libraries(logTest Tiny_WebServer)
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/src/log/test)