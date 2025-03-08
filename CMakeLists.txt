cmake_minimum_required(VERSION 3.10)
project(protozero)

add_library(protozero INTERFACE)
target_include_directories(protozero INTERFACE include)
target_compile_definitions(protozero INTERFACE PROTOZERO_USE_VIEW=std::string_view)
