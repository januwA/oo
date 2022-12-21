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
set(CURL_ENABLE_SSL ON)
set(CURL_USE_OPENSSL ON)
set(HTTP_ONLY ON)

if(CMAKE_SYSTEM_NAME MATCHES "Windows")
   set(OPENSSL_ROOT_DIR "D:\\program\\OpenSSL-Win64") 
   set(OPENSSL_LIBRARIES "D:\\program\\OpenSSL-Win64\\lib") 
   set(OPENSSL_INCLUDE_DIR "D:\\program\\OpenSSL-Win64\\include") 
endif()

set(MY_CURL_DIR_NAME curl-7.86.0)
add_subdirectory(extern/${MY_CURL_DIR_NAME})

set(MY_LUA_DIR_NAME lua-5.4.4)
file(GLOB LUA_SRC extern/${MY_LUA_DIR_NAME}/src/*.c)
list(FILTER LUA_SRC EXCLUDE REGEX "luac.c$")
list(FILTER LUA_SRC EXCLUDE REGEX "lua.c$")

add_executable(${PROJECT_NAME} main.cpp oo.cpp ${LUA_SRC})

target_include_directories(${PROJECT_NAME} PUBLIC 
  extern/${MY_CURL_DIR_NAME}/include
  extern/${MY_LUA_DIR_NAME}/src
  extern/json-3.11.2/include
)

set(link_LIBS libcurl)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
	list(APPEND link_LIBS stdc++fs)
endif()
target_link_libraries(${PROJECT_NAME} PUBLIC ${link_LIBS})

# add_custom_target(asd COMMAND ${CMAKE_COMMAND} -E echo "$<TARGET_FILE:libcurl>")

# copy exe to build/dist
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_BINARY_DIR}/dist/"
	COMMAND ${CMAKE_COMMAND} -E copy_if_different
	"$<TARGET_FILE:${PROJECT_NAME}>" 
	"${PROJECT_BINARY_DIR}/dist/"
)

# copy curl dll to build/dist
if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
  add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
		"$<TARGET_FILE:libcurl>"
		"${PROJECT_BINARY_DIR}/dist/"
  )
endif()


