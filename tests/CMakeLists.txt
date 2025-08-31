cmake_minimum_required(VERSION 3.20)

# Test for third-party integration
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/pers)
    add_subdirectory(pers)
endif()

# Test for WebGPU samples  
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/third_party)
    add_subdirectory(third_party)
endif()