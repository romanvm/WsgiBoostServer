cmake_minimum_required (VERSION 2.8.8)
set(CMAKE_BUILD_TYPE "Release")
project (WsgiBoostServer)

if(${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -O3 -Wall -Wextra")
    if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.8)
        message(FATAL_ERROR "GCC version >=4.8 required.")
    endif()
elseif (${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -O3 -Wall -Wextra")
    if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 3.3)
        message(FATAL_ERROR "Clang version >=3.3 required.")
    endif()
elseif(MSVC14) #TODO: What about other MSVC versions?
    foreach(flag_var
            CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
            CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
       if(${flag_var} MATCHES "/MD")
          string(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
       endif(${flag_var} MATCHES "/MD")
    endforeach(flag_var)
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /SAFESEH:NO")
else()
    message(FATAL_ERROR "Your compiler (${CMAKE_CXX_COMPILER_ID}) is not supported! Only MSVS 2014, GCC and Clang are supported.")
endif()

include_directories(wsgi_boost)

if(NOT PYTHON_VERSION)
	set(PYTHON_VERSION 2.7)
endif(NOT PYTHON_VERSION)

find_package(PythonLibs ${PYTHON_VERSION} REQUIRED)
include_directories(SYSTEM ${PYTHON_INCLUDE_PATH})

set(Boost_USE_STATIC_LIBS ON)
add_definitions(-DBOOST_PYTHON_STATIC_LIB)
find_package(Boost 1.54.0 COMPONENTS regex system thread coroutine context filesystem date_time iostreams zlib python REQUIRED)
include_directories(SYSTEM ${Boost_INCLUDE_DIR})

file(GLOB SOURCES wsgi_boost/*.cpp)
add_library(wsgi_boost SHARED ${SOURCES})

target_link_libraries(wsgi_boost ${Boost_LIBRARIES} ${PYTHON_LIBRARIES})

if(WIN32)
    set_target_properties(wsgi_boost PROPERTIES SUFFIX ".pyd")
elseif(UNIX)
    set_target_properties(wsgi_boost PROPERTIES PREFIX "")
endif(WIN32)
