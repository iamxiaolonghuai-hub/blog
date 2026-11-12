cmake_minimum_required(VERSION 3.16)
project(blog_server LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

find_package(Drogon CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)

# 递归收集所有实现文件
file(GLOB_RECURSE APP_SOURCES CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cc")

add_executable(blog_server ${APP_SOURCES})

target_include_directories(blog_server PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/src")

target_link_libraries(blog_server PRIVATE
  Drogon::Drogon
  OpenSSL::SSL
  OpenSSL::Crypto
  crypt)
