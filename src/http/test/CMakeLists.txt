add_executable(httpRequestTest httpRequestTest.cc)
target_link_libraries(httpRequestTest Tiny_WebServer)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/src/http/test)