cmake_minimum_required(VERSION 3.20)
project(AssimpTest)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find assimp
find_package(assimp CONFIG REQUIRED)

# Create test executable
add_executable(assimp_test main.cpp)

# Link assimp
target_link_libraries(assimp_test PRIVATE assimp::assimp)

# Set output directory
set_target_properties(assimp_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Add to CTest
enable_testing()
add_test(NAME assimp_test COMMAND assimp_test)
set_tests_properties(assimp_test PROPERTIES
    WORKING_DIRECTORY "$<TARGET_FILE_DIR:assimp_test>"
)

# Print build information
message(STATUS "========================================")
message(STATUS "Assimp Test")
message(STATUS "Assimp found: ${assimp_FOUND}")
message(STATUS "========================================")