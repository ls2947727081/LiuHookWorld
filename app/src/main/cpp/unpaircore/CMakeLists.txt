cmake_minimum_required(VERSION 3.22.1)

project(pairipcore)

add_library(pairipcore SHARED
        src/library.c
        src/mem.c
        src/proc.c
)

target_link_libraries(pairipcore
        android
        log
)

## 给 C 文件也加
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mllvm -irobf-indbr -mllvm -irobf-icall -mllvm -irobf-indgv -mllvm -irobf-cse -mllvm -irobf-cff -mllvm -irobf-cie -mllvm -irobf-cfe")
## 给 C++ 文件也加
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mllvm -irobf-indbr -mllvm -irobf-icall -mllvm -irobf-indgv -mllvm -irobf-cse -mllvm -irobf-cff -mllvm -irobf-cie -mllvm -irobf-cfe")
