cmake_minimum_required(VERSION 3.12)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_compile_options("-g")
add_compile_options("-std=c++17")
add_compile_options("-Wall")
add_compile_options("-MD")

include_directories(/usr/local/include
                    /usr/local/opt/openssl/include)
add_compile_definitions(CNX_TCP_MAX=1005)
add_compile_definitions(CNX_ADM=5)
add_compile_definitions(FD_SET_SIZE=1010)

link_directories(/usr/local/opt/openssl/lib
                 /opt/local/lib)
add_link_options("-lstdc++")
add_link_options("-pthread")
add_link_options("-lcrypto")


file(GLOB SOURCE_FILES
    ../src/*.cpp
)

file(GLOB HEADER_FILES
    ../src/*.h
)

add_executable(comserv ${HEADER_FILES} ${SOURCE_FILES})