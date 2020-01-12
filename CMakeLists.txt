cmake_minimum_required(VERSION 3.10)
project(ZOSFS)

set(CMAKE_CXX_STANDARD 17)

add_executable(ZOSFS main.cpp zosfsstruct.h incp.cpp incp.h inode.cpp inode.h directory.cpp directory.h datablock.cpp datablock.h cat.cpp cat.h outcp.cpp outcp.h rm.cpp rm.h info.cpp info.h pwd.cpp pwd.h cd.cpp cd.h mkdir.cpp mkdir.h mv.cpp mv.h ls.cpp ls.h)

target_link_libraries(ZOSFS stdc++fs)