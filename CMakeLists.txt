cmake_minimum_required(VERSION 3.16.3)
project(Jit)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenSSL REQUIRED)
find_package(ZLIB REQUIRED)

add_executable(Jit main.cpp
        DirectoryManagent/DirManager.cpp
        DirectoryManagent/DirManager.h
        ChangesManagement/ChangesManager.cpp
        ChangesManagement/ChangesManager.h
        ChangesManagement/IndexFileParser.cpp
        ChangesManagement/IndexFileParser.h
        ChangesManagement/data.h
        ChangesManagement/JitActions.cpp
        ChangesManagement/JitActions.h
        JitUtility/jit_utility.h
        JitUtility/jit_utility.cpp
        ChangesManagement/JitActionsBranchMerge.cpp
)

target_link_libraries(Jit OpenSSL::SSL OpenSSL::Crypto ZLIB::ZLIB pthread)
