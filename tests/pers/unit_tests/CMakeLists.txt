# Unit Tests for Pers Graphics Engine
cmake_minimum_required(VERSION 3.20)

# Main test executable
add_executable(pers_unit_tests
    main.cpp
    test_critical_path.cpp
    test_memory_safety.cpp
    test_type_conversion.cpp
    test_resource_management.cpp
    test_error_handling.cpp
)

# JSON test loader executable  
add_executable(pers_json_tests
    json_test_main.cpp
    json_test_loader_rapidjson.cpp
    test_result_writer.cpp
)

# Link with pers library
target_link_libraries(pers_unit_tests PRIVATE
    pers_static
)

# Find rapidjson
find_package(RapidJSON CONFIG REQUIRED)

target_link_libraries(pers_json_tests PRIVATE
    pers_static
)

# Include directories
target_include_directories(pers_unit_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/pers/include
    ${CMAKE_SOURCE_DIR}/third_party/wgpu-native-runtime/include
)

target_include_directories(pers_json_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/pers/include
    ${CMAKE_SOURCE_DIR}/third_party/wgpu-native-runtime/include
    ${RAPIDJSON_INCLUDE_DIRS}
)

# Include the centralized copy function
include(${CMAKE_SOURCE_DIR}/cmake/CopyRuntimeDependencies.cmake)

# Copy runtime dependencies for both test executables
copy_runtime_dependencies(pers_unit_tests)
copy_runtime_dependencies(pers_json_tests)

# Copy JSON test files to output directory
add_custom_command(TARGET pers_json_tests POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_CURRENT_SOURCE_DIR}/test_cases_2025_01_02.json"
    $<TARGET_FILE_DIR:pers_json_tests>
    COMMENT "Copying JSON test cases"
)

# Enable testing
enable_testing()

# Add test
add_test(NAME PersUnitTests COMMAND pers_unit_tests)

# Set working directory for test
set_tests_properties(PersUnitTests PROPERTIES
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin/$<CONFIG>
)