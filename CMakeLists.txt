cmake_minimum_required(VERSION 3.16)
project(crsf_lib VERSION 1.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

add_compile_options(-Wall -Wextra -pedantic)

# Библиотека
add_library(crsf_lib STATIC
    src/crsf_core.c
    src/crsf_crc.c
    src/crsf_parse.c
    src/crsf_make.c
)

target_include_directories(crsf_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Тесты
option(CRSF_BUILD_TESTS "Build unit tests" ON)
if(CRSF_BUILD_TESTS)
    enable_testing()
    
    add_executable(test_crc tests/test_crc.c)
    target_link_libraries(test_crc crsf_lib)
    add_test(NAME crc COMMAND test_crc)
    
    add_executable(test_ringbuf tests/test_ringbuf.c)
    target_link_libraries(test_ringbuf crsf_lib)
    add_test(NAME ringbuf COMMAND test_ringbuf)
    
    add_executable(test_ctx tests/test_ctx.c)
    target_link_libraries(test_ctx crsf_lib)
    add_test(NAME ctx COMMAND test_ctx)
    
    add_executable(test_parse tests/test_parse.c)
    target_link_libraries(test_parse crsf_lib)
    add_test(NAME parse COMMAND test_parse)
    
    add_executable(test_make tests/test_make.c)
    target_link_libraries(test_make crsf_lib)
    add_test(NAME make COMMAND test_make)
    
endif()

# Установка
install(TARGETS crsf_lib
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)

install(DIRECTORY include/ DESTINATION include)