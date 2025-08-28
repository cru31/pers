cmake_minimum_required(VERSION 3.21)

set(TARGET_NAME webgpu_instance_test)

# Add executable
add_executable(${TARGET_NAME}
    main.cpp
)

# Find GLFW (required for surface creation)
find_package(glfw3 CONFIG REQUIRED)

# Link with pers library
target_link_libraries(${TARGET_NAME} PRIVATE
    pers_static
    glfw
)

# Set output directory
set_target_properties(${TARGET_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Set C++ standard
target_compile_features(${TARGET_NAME} PRIVATE cxx_std_17)

# Add as a test
add_test(NAME ${TARGET_NAME} COMMAND ${TARGET_NAME})

# Include the centralized copy runtime dependencies function
include(${CMAKE_CURRENT_LIST_DIR}/../../../cmake/CopyRuntimeDependencies.cmake)

# Copy runtime dependencies
copy_runtime_dependencies(${TARGET_NAME})