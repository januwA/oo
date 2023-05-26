cmake_minimum_required(VERSION 3.0.0)
project(oo VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(USE_CMD "build oocmd" ON)

if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /utf-8") 
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
endif()

# curl build
set(BUILD_CURL_EXE OFF)
set(BUILD_TESTING OFF)
set(CURL_ENABLE_SSL ON) # 开启https
set(CURL_USE_OPENSSL ON) # 使用openssl
set(HTTP_ONLY ON)

# windows上设置openssl变量，linux上安装openssl即可
if(CMAKE_SYSTEM_NAME MATCHES "Windows")
   set(OPENSSL_ROOT_DIR "D:\\program\\OpenSSL-Win64") 
   set(OPENSSL_LIBRARIES "D:\\program\\OpenSSL-Win64\\lib") 
   set(OPENSSL_INCLUDE_DIR "D:\\program\\OpenSSL-Win64\\include") 
endif()

set(MY_CURL_DIR_NAME curl-7.86.0)
add_subdirectory(extern/${MY_CURL_DIR_NAME})

# lua script
set(MY_LUA_DIR_NAME lua-5.4.4)
file(GLOB LUA_LIB_SRC extern/${MY_LUA_DIR_NAME}/src/*.c)
list(FILTER LUA_LIB_SRC EXCLUDE REGEX "lua.c$" )

add_library(lua_static STATIC ${LUA_LIB_SRC})
set_target_properties(lua_static PROPERTIES OUTPUT_NAME "lua")
set_target_properties(lua_static PROPERTIES CLEAN_DIRECT_OUTPUT 1)

add_library(lua_shared SHARED  ${LUA_LIB_SRC})
set_target_properties(lua_shared PROPERTIES OUTPUT_NAME "lua")
set_target_properties(lua_shared PROPERTIES CLEAN_DIRECT_OUTPUT 1)

# oo
set(libs_SRC oo.cpp)

# 静态库
set(MY_static_name oo)
add_library(${MY_static_name} STATIC ${libs_SRC})
set_target_properties(${MY_static_name} PROPERTIES OUTPUT_NAME "oo")
set_target_properties(${MY_static_name} PROPERTIES CLEAN_DIRECT_OUTPUT 1)

# 共享库
set(MY_shared_name oo_shared)
add_library(${MY_shared_name} SHARED ${libs_SRC})
set_target_properties(${MY_shared_name} PROPERTIES OUTPUT_NAME "oo")
set_target_properties(${MY_shared_name} PROPERTIES CLEAN_DIRECT_OUTPUT 1)

# include
set(MY_includes 
  extern/${MY_CURL_DIR_NAME}/include
  extern/${MY_LUA_DIR_NAME}/src
  extern/json-3.11.2/include)
target_include_directories(${MY_static_name} PUBLIC ${MY_includes})
target_include_directories(${MY_shared_name} PUBLIC ${MY_includes})

# link
set(link_LIBS libcurl lua_static)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
  list(APPEND link_LIBS stdc++fs)
endif()
target_link_libraries(${MY_static_name} PUBLIC ${link_LIBS})
target_link_libraries(${MY_shared_name} PUBLIC ${link_LIBS})

add_custom_command(TARGET ${MY_shared_name} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_BINARY_DIR}/dist/"
  COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:${MY_static_name}>" "${PROJECT_BINARY_DIR}/dist/"
  COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:${MY_shared_name}>" "${PROJECT_BINARY_DIR}/dist/"
  COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:lua_static>" "${PROJECT_BINARY_DIR}/dist/"
  COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:lua_shared>" "${PROJECT_BINARY_DIR}/dist/"
  COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:libcurl>" "${PROJECT_BINARY_DIR}/dist/"
)

# copy dll
if(CMAKE_SYSTEM_NAME MATCHES "Windows")
   add_custom_command(TARGET ${MY_shared_name} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_BINARY_DIR}/dist/"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${OPENSSL_ROOT_DIR}/libcrypto-1_1-x64.dll" "${PROJECT_BINARY_DIR}/dist/"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${OPENSSL_ROOT_DIR}/libssl-1_1-x64.dll" "${PROJECT_BINARY_DIR}/dist/"
  )
endif()

# oocmd exe
if(USE_CMD)
  set(oocmd_PROJECT_NAME "oocmd")
  add_executable(${oocmd_PROJECT_NAME} oocmd.cpp)

  # oocmd include
  target_include_directories(${oocmd_PROJECT_NAME} PUBLIC oo.h)

  set(link_LIBS oo)
  if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    list(APPEND link_LIBS stdc++fs)
  endif()
  target_link_libraries(${oocmd_PROJECT_NAME} PUBLIC ${link_LIBS})

  # copy exe to build/dist
  add_custom_command(TARGET ${oocmd_PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_BINARY_DIR}/dist/"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:${oocmd_PROJECT_NAME}>" "${PROJECT_BINARY_DIR}/dist/"
  )
endif(USE_CMD)

