cmake_minimum_required(VERSION 3.0.0)
project(oo VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(USE_CMD "build oocmd" ON)
option(USE_HTTPS "use https" OFF)


if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /utf-8") 
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
endif()

# curl build
set(BUILD_CURL_EXE OFF)
set(BUILD_TESTING OFF)
set(CURL_ENABLE_SSL OFF)
set(CURL_USE_OPENSSL OFF)
set(HTTP_ONLY ON)

if(USE_HTTPS)
  set(CURL_ENABLE_SSL ON)
  set(CURL_USE_OPENSSL ON)

  if(CMAKE_SYSTEM_NAME MATCHES "Windows")
    set(OPENSSL_LIBRARIES "${OPENSSL_ROOT_DIR}/lib") 
    set(OPENSSL_INCLUDE_DIR "${OPENSSL_ROOT_DIR}/include") 
  endif()
endif(USE_HTTPS)

set(MY_CURL_DIR_NAME curl-7.86.0)
add_subdirectory(extern/${MY_CURL_DIR_NAME})

# lua script
set(MY_LUA_DIR_NAME lua-5.4.4)
file(GLOB LUA_LIB_SRC extern/${MY_LUA_DIR_NAME}/src/*.c)
list(FILTER LUA_LIB_SRC EXCLUDE REGEX "lua.c$" )

add_library(liblua STATIC ${LUA_LIB_SRC})
set_target_properties(liblua PROPERTIES OUTPUT_NAME "lua")
set_target_properties(liblua PROPERTIES CLEAN_DIRECT_OUTPUT 1)

# oo
set(oo_STATIC liboo)
add_library(${oo_STATIC} STATIC oo.cpp)
set_target_properties(${oo_STATIC} PROPERTIES OUTPUT_NAME "oo")
set_target_properties(${oo_STATIC} PROPERTIES CLEAN_DIRECT_OUTPUT 1)

# include
set(oo_INCLUDES 
  extern/${MY_CURL_DIR_NAME}/include
  extern/${MY_LUA_DIR_NAME}/src
  extern/json-3.11.2/include)
target_include_directories(${oo_STATIC} PUBLIC ${oo_INCLUDES})

# link
set(oo_LINKS libcurl liblua)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
  list(APPEND oo_LINKS stdc++fs)
endif()
target_link_libraries(${oo_STATIC} PUBLIC ${oo_LINKS})

add_custom_command(TARGET ${oo_STATIC} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_BINARY_DIR}/dist/"
  COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:${oo_STATIC}>" "${PROJECT_BINARY_DIR}/dist/"
  COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:liblua>" "${PROJECT_BINARY_DIR}/dist/"
  COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:libcurl>" "${PROJECT_BINARY_DIR}/dist/"
)

# copy openssl dll
if(USE_HTTPS AND CMAKE_SYSTEM_NAME MATCHES "Windows")
   file(GLOB libcrypto_DLL "${OPENSSL_ROOT_DIR}/libcrypto*.dll")
   file(GLOB libssl_DLL "${OPENSSL_ROOT_DIR}/libssl*.dll")

   add_custom_command(TARGET ${oo_STATIC} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_BINARY_DIR}/dist/"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${libcrypto_DLL}" "${PROJECT_BINARY_DIR}/dist/"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${libssl_DLL}" "${PROJECT_BINARY_DIR}/dist/"
  )
endif()

# oocmd exe
if(USE_CMD)
  set(oocmd_PROJECT_NAME "oo")
  add_executable(${oocmd_PROJECT_NAME} oocmd.cpp)
  target_include_directories(${oocmd_PROJECT_NAME} PUBLIC oo.h)

  set(oocmd_LINKS ${oo_STATIC})
  if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    list(APPEND oocmd_LINKS stdc++fs)
  endif()
  target_link_libraries(${oocmd_PROJECT_NAME} PUBLIC ${oocmd_LINKS})

  add_custom_command(TARGET ${oocmd_PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_BINARY_DIR}/dist/"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:${oocmd_PROJECT_NAME}>" "${PROJECT_BINARY_DIR}/dist/"
  )
endif(USE_CMD)

