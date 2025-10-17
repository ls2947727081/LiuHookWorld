cmake_minimum_required(VERSION 3.22.1)


#######################          警告          #######################
####################### 以下内容一般是不需要修改的 #######################
####################### 除非你清楚地知道你在做什么 #######################
set(shadowhook_module.module_name shadowhook)
if ("${ANDROID_ABI}" STREQUAL "armeabi-v7a")
    set(arch "arm")
    set(shadowhook_module.source_arch
        ${CMAKE_CURRENT_SOURCE_DIR}/arch/${arch}/sh_a32.c
        ${CMAKE_CURRENT_SOURCE_DIR}/arch/${arch}/sh_inst.c
        ${CMAKE_CURRENT_SOURCE_DIR}/arch/${arch}/sh_t16.c
        ${CMAKE_CURRENT_SOURCE_DIR}/arch/${arch}/sh_t32.c
        ${CMAKE_CURRENT_SOURCE_DIR}/arch/${arch}/sh_txx.c)
elseif("${ANDROID_ABI}" STREQUAL "arm64-v8a")
    set(arch "arm64")
    set(shadowhook_module.source_arch
        ${CMAKE_CURRENT_SOURCE_DIR}/arch/${arch}/sh_a64.c
        ${CMAKE_CURRENT_SOURCE_DIR}/arch/${arch}/sh_inst.c)
else()
    set(arch "fku")
    set(shadowhook_module.source_arch)
endif()
set(shadowhook_module.source_c
        ${CMAKE_CURRENT_SOURCE_DIR}/sh_enter.c
        ${CMAKE_CURRENT_SOURCE_DIR}/sh_exit.c
        ${CMAKE_CURRENT_SOURCE_DIR}/sh_hub.c
        ${CMAKE_CURRENT_SOURCE_DIR}/sh_linker.c
        ${CMAKE_CURRENT_SOURCE_DIR}/sh_recorder.c
        ${CMAKE_CURRENT_SOURCE_DIR}/sh_safe.c
        ${CMAKE_CURRENT_SOURCE_DIR}/sh_switch.c
        ${CMAKE_CURRENT_SOURCE_DIR}/sh_task.c
        ${CMAKE_CURRENT_SOURCE_DIR}/shadowhook.c
        ${CMAKE_CURRENT_SOURCE_DIR}/common/bytesig.c
        ${CMAKE_CURRENT_SOURCE_DIR}/common/sh_errno.c
        ${CMAKE_CURRENT_SOURCE_DIR}/common/sh_log.c
        ${CMAKE_CURRENT_SOURCE_DIR}/common/sh_trampo.c
        ${CMAKE_CURRENT_SOURCE_DIR}/common/sh_util.c
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/xdl/xdl.c
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/xdl/xdl_iterate.c
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/xdl/xdl_linker.c
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/xdl/xdl_lzma.c
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/xdl/xdl_util.c)
add_library(${shadowhook_module.module_name} STATIC ${shadowhook_module.source_c} ${shadowhook_module.source_arch})
target_compile_options(${shadowhook_module.module_name} PRIVATE $<$<COMPILE_LANGUAGE:C>: -std=c17>)# PRIVATE -Weverything)
target_include_directories(${shadowhook_module.module_name} PRIVATE . include arch/${arch} common third_party/xdl third_party/bsd third_party/lss)
target_link_libraries(${shadowhook_module.module_name} -Wl,--exclude-libs=ALL log)