cmake_minimum_required(VERSION 3.22.1)
project(MyHook)
add_library(myhook SHARED
        myhook.cpp
        myhook_onload.cpp
)



target_compile_features(myhook PUBLIC cxx_std_17)
target_include_directories(myhook PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../ShadowHook/include
)

target_link_libraries(myhook shadowhook log)
target_link_options(myhook PUBLIC ${ARCH_LINK_FLAGS})
