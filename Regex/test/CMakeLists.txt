cmake_minimum_required(VERSION 3.0)
project(nedit-regex-test CXX)

add_executable(nedit-regex-test
	Test.cpp
)

target_link_libraries(nedit-regex-test
	Regex
)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR})

set_property(TARGET nedit-regex-test PROPERTY CXX_STANDARD 14)

add_test("nedit-regex-test" "nedit-regex-test")
