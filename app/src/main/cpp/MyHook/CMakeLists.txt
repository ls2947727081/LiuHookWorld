cmake_minimum_required(VERSION 3.22.1)
project(MyHook LANGUAGES C CXX)

# 使用 CMake 推荐方式设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 可通过 -DARCH_LINK_FLAGS="..." 在 cmake 调用时传入架构相关的额外 link flags（可选）
if(NOT DEFINED ARCH_LINK_FLAGS)
    set(ARCH_LINK_FLAGS "")
endif()

# 源文件
set(MYHOOK_SRC
        ${CMAKE_CURRENT_SOURCE_DIR}/myhook.cpp
)
set(UNITYDUMPER_SRC
        ${CMAKE_CURRENT_SOURCE_DIR}/UnityDumper.cpp
)

# 创建两个共享库
add_library(myhook SHARED ${MYHOOK_SRC})
add_library(UnityDumper SHARED ${UNITYDUMPER_SRC})

# 公共 include 目录（对两个 target 都生效）
set(COMMON_INCLUDES
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/libil2cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../ShadowHook/include
)

target_include_directories(myhook PUBLIC ${COMMON_INCLUDES})
target_include_directories(UnityDumper PUBLIC ${COMMON_INCLUDES})

# 链接库：假设 shadowhook 是一个已存在的 CMake target（例如你在别处 add_library(shadowhook ...)）
# 如果 shadowhook 不是 CMake target，见下面的“注意（shadowhook）”部分。
target_link_libraries(myhook PRIVATE shadowhook log)
target_link_libraries(UnityDumper PRIVATE shadowhook log)

# 如果需要额外的 link options（例如某些架构需要特定 flags），逐个 target 添加
if(ARCH_LINK_FLAGS)
    # target_link_options 在 CMake 3.13+ 可用
    target_link_options(myhook PRIVATE ${ARCH_LINK_FLAGS})
    target_link_options(UnityDumper PRIVATE ${ARCH_LINK_FLAGS})
endif()

# 便于调试：导出符号表（可选，根据需要打开/注释）
# set_target_properties(myhook PROPERTIES LINK_FLAGS "-Wl,--export-dynamic")
# set_target_properties(UnityDumper PROPERTIES LINK_FLAGS "-Wl,--export-dynamic")
