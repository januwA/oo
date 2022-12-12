cmake_minimum_required(VERSION 3.0.0)
project(oo VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
  # /utf-8 避免中文注释错误
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /utf-8") 
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
  # -pthread undefined reference to symbol 'pthread_create@@GLIBC_2.2.5'
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
endif()

# curl build
set(BUILD_CURL_EXE OFF)
set(BUILD_TESTING OFF)
set(MY_CURL_DIR_NAME curl-7.86.0)
add_subdirectory(extern/${MY_CURL_DIR_NAME})

add_executable(${PROJECT_NAME} main.cpp oo.cpp)

target_include_directories(${PROJECT_NAME} PUBLIC extern/${MY_CURL_DIR_NAME}/include)

target_link_libraries(${PROJECT_NAME} PUBLIC libcurl)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
  # stdc++fs  std::filesystem error
  target_link_libraries(${PROJECT_NAME} PUBLIC stdc++fs)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
  add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${PROJECT_BINARY_DIR}/extern/${MY_CURL_DIR_NAME}/lib/$<CONFIG>/libcurl$<$<CONFIG:Debug>:-d>.dll"
    "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
  )
endif()