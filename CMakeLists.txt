############################## si446x-programmer CMake ##############################

# Set the minimum cmake version
cmake_minimum_required(VERSION 2.8.4)

########## Compilers ##########

# Set compiler 
set(CMAKE_C_COMPILER 		gcc)
set(CMAKE_CXX_COMPILER 		g++)

set(COMMON_C_FLAGS 			"-std=gnu99 -g -Wall -Wextra")
set(COMMON_CXX_FLAGS 		"-std=c++11 -stdlib=libc++ -g -Wall -Wextra")

set(CMAKE_C_FLAGS_DEBUG 	"${COMMON_C_FLAGS} -g")
set(CMAKE_CXX_FLAGS_DEBUG 	"${COMMON_CXX_FLAGS} -g")

set(CMAKE_C_FLAGS_RELEASE 	"${COMMON_C_FLAGS} -Os")
set(CMAKE_CXX_FLAGS_RELEASE "${COMMON_CXX_FLAGS} -Os")


set(CMAKE_BUILD_TYPE Debug)

# Set flags
add_definitions(-Wall -Wextra -Wconditional-uninitialized -Wno-unused-function -Wno-unused-parameter)

########## Project ##########

# Project name and languages
project(bgm111 C CXX)

# The version number
set(network-simulation_VERSION_MAJOR 0)
set(network-simulation_VERSION_MINOR 2)

########## Files ##########

# Add include locations
set(INCLUDES 
	${PROJECT_SOURCE_DIR}/work/include
)
include_directories(${INCLUDES})

# Add source locations
set(SOURCES
	${PROJECT_SOURCE_DIR}/work/source/main.cpp 
	${PROJECT_SOURCE_DIR}/work/source/uart.c
)

########## Outputs ##########

# Set the project output name
set(PROJECT_OUTPUT ${CMAKE_PROJECT_NAME})

# Add executable output
add_executable(${PROJECT_OUTPUT} ${SOURCES})

# Link the executable
target_link_libraries(${PROJECT_OUTPUT} ${BASE_LIBRARIES} ${OPTIONAL_LIBS} pthread)

########## Custom Targets ##########

########## Post Builds ##########
