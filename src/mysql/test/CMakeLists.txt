add_executable(mysqlPool mysqlPoolTest.cc)
target_link_libraries(mysqlPool Tiny_WebServer)
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/src/mysql/test)