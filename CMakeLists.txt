cmake_minimum_required(VERSION 3.6 FATAL_ERROR)

project(ui LANGUAGES C)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK3 REQUIRED gtk+-3.0 IMPORTED_TARGET)

add_executable(ui main.c)
target_compile_features(ui PRIVATE c_std_17)
target_compile_definitions(ui PRIVATE GTK_DISABLE_DEPRECATED)
target_link_libraries(ui PRIVATE PkgConfig::GTK3)
