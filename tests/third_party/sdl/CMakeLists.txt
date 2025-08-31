cmake_minimum_required(VERSION 3.20)
project(SDL3Test)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find SDL3
find_package(SDL3 CONFIG REQUIRED)

# Create test executable
add_executable(sdl3_test main.cpp)

# Link SDL3
target_link_libraries(sdl3_test PRIVATE SDL3::SDL3)

# Set output directory
set_target_properties(sdl3_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Add to CTest
enable_testing()
add_test(NAME sdl3_test COMMAND sdl3_test)
set_tests_properties(sdl3_test PROPERTIES
    WORKING_DIRECTORY "$<TARGET_FILE_DIR:sdl3_test>"
)

# Print build information
message(STATUS "========================================")
message(STATUS "SDL3 Test")
message(STATUS "SDL3 found: ${SDL3_FOUND}")
message(STATUS "========================================")