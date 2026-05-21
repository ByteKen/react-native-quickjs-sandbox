cmake_minimum_required(VERSION 3.10)

project(quickjs-sandbox-jsi)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -Wall -fexceptions -frtti -O3")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -O3")

add_compile_definitions(CONFIG_VERSION="2024-01-13")

# Resolve the real physical path of the source directory (resolves Windows junctions/symlinks)
get_filename_component(REAL_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}" REALPATH)

# Define absolute paths for cpp files and quickjs directory
set(CPP_DIR "${REAL_SOURCE_DIR}/../cpp")
set(QUICKJS_DIR "${REAL_SOURCE_DIR}/../cpp/quickjs")

add_library(quickjs-sandbox-jsi SHARED
        "${CPP_DIR}/quickjs-sandbox-jsi.cpp"
        "${CPP_DIR}/quickjs-sandbox-jni.cpp"
        "${QUICKJS_DIR}/quickjs.c"
        "${QUICKJS_DIR}/cutils.c"
        "${QUICKJS_DIR}/dtoa.c"
        "${QUICKJS_DIR}/libregexp.c"
        "${QUICKJS_DIR}/libunicode.c"
)

target_include_directories(quickjs-sandbox-jsi PRIVATE
        "${CPP_DIR}"
        "${QUICKJS_DIR}"
)

find_package(ReactAndroid REQUIRED)

target_link_libraries(quickjs-sandbox-jsi
        android
        log
        ReactAndroid::jsi
)
