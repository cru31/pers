cmake_minimum_required(VERSION 3.20)
project(pers VERSION 0.1.0 LANGUAGES CXX)

# C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# GLM is required for pers engine
find_package(glm CONFIG REQUIRED)
message(STATUS "Found GLM")

# GLFW is NOT needed for pers library itself
# It will be needed for tests/samples

# Pers Engine Library Sources
file(GLOB_RECURSE PERS_SOURCES 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cc
)

file(GLOB_RECURSE PERS_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp
)

# Create static library
add_library(pers_static STATIC ${PERS_SOURCES} ${PERS_HEADERS})
target_include_directories(pers_static PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(pers_static PUBLIC glm::glm)
set_target_properties(pers_static PROPERTIES OUTPUT_NAME "pers_static")

# Create shared library
add_library(pers_shared SHARED ${PERS_SOURCES} ${PERS_HEADERS})
target_include_directories(pers_shared PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(pers_shared PUBLIC glm::glm)
set_target_properties(pers_shared PROPERTIES 
    OUTPUT_NAME "pers"
    WINDOWS_EXPORT_ALL_SYMBOLS ON
)
if(WIN32)
    target_compile_definitions(pers_shared PRIVATE PERS_BUILD_DLL)
    target_compile_definitions(pers_shared INTERFACE PERS_USE_DLL)
endif()

# Export configuration
message(STATUS "===========================================")
message(STATUS "Pers Library Configuration")
message(STATUS "===========================================")
message(STATUS "Version: ${PROJECT_VERSION}")
message(STATUS "GLM: Required (math library)")
message(STATUS "===========================================")