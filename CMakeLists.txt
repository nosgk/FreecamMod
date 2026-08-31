cmake_minimum_required(VERSION 3.16)
project(Freecam LANGUAGES CXX ASM_MASM)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE SRC CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h
)

add_subdirectory(extern)

add_library(${PROJECT_NAME} SHARED
    ${SRC}
    src/hook/daytime_update.asm
 )

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

# 源码以 UTF-8 编码（含中文字符串字面量），MSVC 需显式指定；仅作用于 C/C++，避免 MASM 警告
target_compile_options(${PROJECT_NAME} PRIVATE "$<$<COMPILE_LANGUAGE:C,CXX>:/utf-8>")

target_link_libraries(${PROJECT_NAME} PRIVATE
    extern::all
    d3d12
    dxgi
    d3dcompiler
    Shell32
)

# 汉化资源：中文字体与中文映射表，随 DLL 一起输出
set(I18N_ASSETS
    "${CMAKE_CURRENT_SOURCE_DIR}/AlibabaHealthFont2.0CN-85B.ttf"
    "${CMAKE_CURRENT_SOURCE_DIR}/config/zh-CN.json"
)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${I18N_ASSETS} $<TARGET_FILE_DIR:${PROJECT_NAME}>
)

set(DLL_OUTPUT_DIR "" CACHE PATH "Directory to copy the built DLL to (e.g. mods folder)")
if(DLL_OUTPUT_DIR)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:${PROJECT_NAME}>
        "${DLL_OUTPUT_DIR}"
    )
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${I18N_ASSETS} "${DLL_OUTPUT_DIR}"
    )
endif()

add_custom_target(GenerateBuildInfoHeader ALL
    COMMAND ${CMAKE_COMMAND}
        -DSRC_DIR=${CMAKE_SOURCE_DIR}
        -DDST_DIR=${CMAKE_BINARY_DIR}/generated
        -P ${CMAKE_SOURCE_DIR}/cmake/generate_build_info.cmake
    BYPRODUCTS ${CMAKE_BINARY_DIR}/generated/build_info.h
    COMMENT "Generating build_info.h"
)

add_dependencies(${PROJECT_NAME} GenerateBuildInfoHeader)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_BINARY_DIR}/generated)
