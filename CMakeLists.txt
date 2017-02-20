cmake_minimum_required(VERSION 2.8)
project(someProject)

set(CMAKE_CXX_STANDARD 11)

find_package(Boost REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})

find_package(OpenMP REQUIRED)
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")

include_directories(include)

################################
# Testing
################################
add_subdirectory(googletest)

enable_testing()

include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})

SET(MAIN_HEADER_FILES
	src/main.h        
        )


set(SOURCE_FILE 
        src/main.cc
	)

set(TEST_FILES
        test/test.cc 
	)


add_executable(app ${SOURCE_FILE} ${MAIN_HEADER_FILES})
target_link_libraries(app ${CMAKE_CURRENT_SOURCE_DIR}/lib/libhiredis.a)

add_executable(runAllTests ${TEST_FILES} ${MAIN_HEADER_FILES})
target_link_libraries(runAllTests gtest gtest_main)

# Options. Turn on with 'cmake -Dmyvarname=ON'.


set(CMAKE_CXX_FLAGS "-std=c++11 -fopenmp -g -Wall")


