cmake_minimum_required(VERSION 3.20)

# Add subdirectories for different test categories
add_subdirectory(lib_link_test)
add_subdirectory(triangle)
add_subdirectory(async_stress_test)
add_subdirectory(webgpu_instance_test)
add_subdirectory(unit_tests)