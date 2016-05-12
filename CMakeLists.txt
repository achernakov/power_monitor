cmake_minimum_required(VERSION 3.0)
project(power_monitor)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(SOURCE_FILES AgilentOscope.cpp PowerMonitor.cpp)

add_library(power_monitor SHARED ${SOURCE_FILES})

include_directories(
        /usr/include/python2.7
        ${Boost_INCLUDE_DIR}
        /usr/include/jsoncpp
        /usr/local/include/mongo
        /usr/include/octave-3.8.2/octave
        /usr/include/octave-3.8.2
        /usr/include/hdf5/serial
)
