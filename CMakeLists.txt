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

# 源码以 UTF-8 编码（含中文字符串字面量），MSVC 需显式指定
target_compile_options(${PROJECT_NAME} PRIVATE /utf-8)

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

set(GAME_DIR "" CACHE PATH "Path to game folder")
if(GAME_DIR)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:${PROJECT_NAME}>
        "${GAME_DIR}"
    )
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${I18N_ASSETS} "${GAME_DIR}"
    )
endif()