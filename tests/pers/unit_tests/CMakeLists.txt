# Unit Tests for Pers Graphics Engine
cmake_minimum_required(VERSION 3.20)

# Main test executable
add_executable(pers_unit_tests
    main.cpp
    json_test_loader.cpp
    # Handlers
    handlers/instance_creation_handler.cpp
    handlers/request_adapter_handler.cpp
    handlers/device_creation_handler.cpp
    handlers/swapchain_builder_handler.cpp
    # Buffer handlers
    handlers/buffer/webgpu_buffer_handler.cpp
    handlers/buffer/buffer_data_verification_handler.cpp
)

# Find rapidjson
find_package(RapidJSON CONFIG REQUIRED)

# Find GLFW
find_package(glfw3 CONFIG REQUIRED)

# Link libraries
target_link_libraries(pers_unit_tests PRIVATE
    pers_static
    glfw
)

# Include directories
target_include_directories(pers_unit_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/pers/include
    ${CMAKE_SOURCE_DIR}/third_party/wgpu-native-runtime/include
    ${RAPIDJSON_INCLUDE_DIRS}
)

# Define paths as compile definitions
target_compile_definitions(pers_unit_tests PRIVATE
    WEB_SERVER_DIR="${CMAKE_CURRENT_SOURCE_DIR}/web"
    TEST_CASES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/test_cases/set_buffer_creation_01"
)

# Include the centralized copy function
include(${CMAKE_SOURCE_DIR}/cmake/CopyRuntimeDependencies.cmake)

# Copy runtime dependencies for test executable
copy_runtime_dependencies(pers_unit_tests)

# Copy test cases directory to output directory
add_custom_command(TARGET pers_unit_tests POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_CURRENT_SOURCE_DIR}/test_cases"
    "$<TARGET_FILE_DIR:pers_unit_tests>/test_cases"
    COMMENT "Copying test cases directory"
)

# Copy web resources including node_modules to output directory
add_custom_command(TARGET pers_unit_tests POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
    "$<TARGET_FILE_DIR:pers_unit_tests>/web_resources"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_CURRENT_SOURCE_DIR}/web/node_modules"
    "$<TARGET_FILE_DIR:pers_unit_tests>/web_resources/node_modules"
    COMMENT "Copying web node_modules to output directory"
)

# Enable testing
enable_testing()

# Add test
add_test(NAME PersUnitTests COMMAND pers_unit_tests test_cases.json)

# Set working directory for test
set_tests_properties(PersUnitTests PROPERTIES
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin/$<CONFIG>
)