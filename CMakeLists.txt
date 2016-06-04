cmake_minimum_required (VERSION 2.8.10)

project (WsgiBoostServer)

set(CMAKE_BUILD_TYPE Release)
set(CMAKE_CONFIGURATION_TYPES Release)
set(CMAKE_VERBOSE_MAKEFILE TRUE)


if(${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
    message(STATUS "CXX version: " ${CMAKE_CXX_COMPILER_VERSION})
    if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 5.0)
        message(FATAL_ERROR "GCC version >=5.0 required.")
    endif()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -O3 -Wall")
elseif(MSVC14)
    foreach(flag_var
            CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
            CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
       if(${flag_var} MATCHES "/MD")
          string(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
       endif()
    endforeach()
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /SAFESEH:NO")
else()
    message(FATAL_ERROR "Your compiler (${CMAKE_CXX_COMPILER_ID}) is not supported! Only MSVS 2014 Update 2+ and GCC 5.0+ are supported.")
endif()

include_directories(wsgi_boost)

file(GLOB SOURCES wsgi_boost/*.cpp)
add_library(wsgi_boost SHARED ${SOURCES})

set_target_properties(wsgi_boost PROPERTIES PREFIX "")

if(NOT PYTHON_VERSION)
    set(PYTHON_VERSION 2.7)
endif()

find_package(PythonLibs ${PYTHON_VERSION} REQUIRED)
include_directories(${PYTHON_INCLUDE_PATH})
target_link_libraries(wsgi_boost ${PYTHON_LIBRARIES})

if(WIN32)
    set(Boost_USE_STATIC_LIBS ON)
    set(Boost_USE_STATIC_RUNTIME ON)
    find_package(Boost 1.55.0 COMPONENTS regex system coroutine context filesystem iostreams python zlib REQUIRED)
    include_directories(${Boost_INCLUDE_DIR})
    add_definitions(-DBOOST_PYTHON_STATIC_LIB)
    target_link_libraries(wsgi_boost ${Boost_LIBRARIES})
    
    set_target_properties(wsgi_boost PROPERTIES SUFFIX ".pyd")
else()
    add_definitions(-DBOOST_ALL_NO_LIB)
    set(Boost_USE_STATIC_LIBS OFF)
    set(Boost_USE_STATIC_RUNTIME OFF)
    find_package(Boost 1.55.0 COMPONENTS coroutine regex system context filesystem iostreams python REQUIRED)
    include_directories(${Boost_INCLUDE_DIR})
    target_link_libraries(wsgi_boost -Wl,-Bdynamic ${Boost_LIBRARIES})

    find_package(ZLIB REQUIRED)    
    include_directories(${ZLIB_INCLUDE_DIRS})
    target_link_libraries(wsgi_boost ${ZLIB_LIBRARIES})
endif()

link_directories(wsgi_boost ${Boost_LIBRARY_DIRS})

