cmake_minimum_required(VERSION 3.20)
project(mg_wrapper LANGUAGES C CXX)

option (ENABLE_SSL "Enable SSL support" ON)
option (ENABLE_DEBUG "Enable debug"     OFF)

if (ENABLE_DEBUG)
    set(CMAKE_BUILD_TYPE "Debug")
    set(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g2 -ggdb -DDEBUG")
endif ()


add_compile_options(-fPIC)
include_directories (
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

file (GLOB MG_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cc
)

add_library (${PROJECT_NAME} SHARED ${MG_SRCS})
target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
if (ENABLE_SSL)
    find_package(OpenSSL REQUIRED)
    target_compile_definitions(${PROJECT_NAME} PRIVATE MG_TLS=MG_TLS_OPENSSL)
    target_link_libraries(${PROJECT_NAME} PRIVATE OpenSSL::SSL OpenSSL::Crypto)
endif ()

if (ENABLE_TEST)
    add_executable("mgtest" test.cc)
    target_link_libraries("mgtest" ${PROJECT_NAME} gtest)
endif ()
