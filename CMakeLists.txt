cmake_minimum_required(VERSION 3.19)
project(pawn-sockets LANGUAGES C CXX VERSION 0.0.1)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(sdk)

include_directories(
	.
	pawn/source
	pawn/source/linux
	pawn-natives
)

add_definitions(
	-DHAVE_STDINT_H=1
	-DPAWN_CELL_SIZE=32
)

add_library(${PROJECT_NAME} SHARED 
src/main.cpp
src/SocketManager.cpp
src/Socket.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE
		OMP-SDK
)

