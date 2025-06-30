# Minimum version used by CMake
cmake_minimum_required(VERSION 3.10)
# Generate compile_commands.json file
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Project name
project(project_name)

# C++ version used, added compilation parameters
set(CMAKE_CXX_STANDARD 14)

if (CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|x86|i386|i686|amd64")
	#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -g3 -O0 -ggdb -mavx2")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -O3 -mavx2 -ftree-vectorize")
elseif (CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm|armv7l|armv6l|armhf")
	#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -g3 -O0")
	#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -O3 -ftree-vectorize")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -g3 -O3 -ftree-vectorize")
else()
	message(WARNING "Unknown or unsupported architecture: ${CMAKE_SYSTEM_PROCESSOR}")
endif()


# Specify header file directory
include_directories(include)

# Find all source files in the current directory
aux_source_directory(. SOURCES)

# Compile all source code together and link it to an executable file 'a.out'
add_executable(a.out ${SOURCES})


