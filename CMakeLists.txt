cmake_minimum_required(VERSION 3.0.0)
project(oo VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(USE_GUI "build oogui" ON)
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

# lua script
set(MY_LUA_DIR_NAME lua-5.4.4)
file(GLOB LUA_SRC extern/${MY_LUA_DIR_NAME}/src/*.c)
list(FILTER LUA_SRC EXCLUDE REGEX "luac.c$")
list(FILTER LUA_SRC EXCLUDE REGEX "lua.c$")

# oocmd exe
if(USE_CMD)
  set(oocmd_PROJECT_NAME "oo")
  add_executable(${oocmd_PROJECT_NAME} oocmd.cpp oo.cpp ${LUA_SRC})
endif(USE_CMD)

# oocmd include
target_include_directories(${oocmd_PROJECT_NAME} PUBLIC 
  extern/${MY_CURL_DIR_NAME}/include
  extern/${MY_LUA_DIR_NAME}/src
  extern/json-3.11.2/include
)

set(link_LIBS libcurl)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
	list(APPEND link_LIBS stdc++fs)
endif()
target_link_libraries(${oocmd_PROJECT_NAME} PUBLIC ${link_LIBS})



# copy exe to build/dist
add_custom_command(TARGET ${oocmd_PROJECT_NAME} POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_BINARY_DIR}/dist/"
	COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:${oocmd_PROJECT_NAME}>" "${PROJECT_BINARY_DIR}/dist/"
  COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:libcurl>" "${PROJECT_BINARY_DIR}/dist/"
)



if(USE_GUI)

  if(CMAKE_SYSTEM_NAME MATCHES "Windows")
    set(WXWIN "D:/program/wxWidgets-3.2.1") 
    set(wx_LIB_DIR "${WXWIN}/lib/vc_x64_dll") 
  endif()

  set(oogui_PROJECT_NAME "oogui")
  add_executable(${oogui_PROJECT_NAME} WIN32 oogui.cpp)

  # include
  set(oogui_INCLUDE ${WXWIN}/include)
  if(CMAKE_SYSTEM_NAME MATCHES "Windows")
    # if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    #   list(APPEND oogui_INCLUDE ${WXWIN}/include/msvc)
    # endif()
    list(APPEND oogui_INCLUDE ${wx_LIB_DIR}/mswu)
  endif()

  target_include_directories(${oogui_PROJECT_NAME} PUBLIC ${oogui_INCLUDE})


  # link
  if(CMAKE_SYSTEM_NAME MATCHES "Windows")
    file(GLOB wxlibs ${wx_LIB_DIR}/wx*.lib)
    target_link_libraries(${oogui_PROJECT_NAME} ${wxlibs})
  endif()

  # copy oogui exe to dist
  add_custom_command(TARGET ${oogui_PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_BINARY_DIR}/dist/"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:${oogui_PROJECT_NAME}>" "${PROJECT_BINARY_DIR}/dist/"
  )

  # copy wx dlls to dist
  if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    add_custom_command(TARGET ${oocmd_PROJECT_NAME} POST_BUILD
      COMMAND cp "${wx_LIB_DIR}/*.dll" "${PROJECT_BINARY_DIR}/dist/"
    )
  endif()

endif(USE_GUI)