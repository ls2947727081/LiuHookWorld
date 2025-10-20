/*
    About Author(s):
    Name: shuke
    Mail: shukebeita1126@gmail.com, yingsang0504@gmail.com
    File: sk_adapter.h
    Data: 2024-08-30 14:34
*/

#ifndef FRAMEWORK_SK_HOOK_ADAPTER_SK_ADAPTER_H
#define FRAMEWORK_SK_HOOK_ADAPTER_SK_ADAPTER_H

#include "sk_def/sk_def.h"

class skHookAdapter;
skHookAdapter * sk_get_hook_adapter();
class skHookAdapter : skNonCopyable
{
public:
    bool inline_hook_func_address(skAddress address, void * cb, void ** ori_cb, bool ignore_dft = false);

private:
    friend skHookAdapter * sk_get_hook_adapter();
    skHookAdapter() = default;
    ~skHookAdapter() = default;
    static skHookAdapter * ins;
};

#define _LH_REPLACE_FUNC       _SK_REPLACE_FUNC
#define _LH_REPLACE_BUILD_FUNC _SK_REPLACE_BUILD_FUNC

#ifdef SK_DEUBG
#define _SK_REPLACE_FUNC(fn_n)                                                                                                  \
if(!sk_get_hook_adapter()->inline_hook_func_address(fn_n##_address, (void *)fn_n##_replace, (void **)&fn_n##_implement, false)) \
{                                                                                                                               \
    LOGE("Hook " #fn_n " Failed!");                                                                                             \
}
#else
#define _SK_REPLACE_FUNC(fn_n) sk_get_hook_adapter()->inline_hook_func_address(fn_n##_address, (void *)fn_n##_replace, (void **)&fn_n##_implement, false);
#endif

#define _SK_REPLACE_BUILD_FUNC(fn_n, return_type, ...)                                                                          \
SK_LOCAL return_type (*fn_n##_implement)(__VA_ARGS__);                                                                          \
SK_LOCAL return_type fn_n##_replace(__VA_ARGS__)

#ifdef SK_DEUBG
#define _SK_REPLACE_BYTE_HOOK_FUNC(lib_n, so_n, fn_n)                                                                                                                                 \
do {                                                                                                                                                                                  \
    bytehook_stub_t so_n##fn_n##_stub = bytehook_hook_single(#lib_n, NULL, #fn_n, (void *)so_n##_##fn_n##_replace, so_n##_##fn_n##_byte_hooked, NULL);                                \
    LOGD("byte hook stub: %lx", (skAddress)so_n##fn_n##_stub);                                                                                                                        \
} while(0)
#else
#define _SK_REPLACE_BYTE_HOOK_FUNC(lib_n, so_n, fn_n) bytehook_hook_single(#lib_n, NULL, #fn_n, (void *)so_n##_##fn_n##_replace, so_n##_##fn_n##_byte_hooked, NULL);
#endif

#ifdef SK_DEBUG
#define _SK_REPLACE_BUILD_BYTE_HOOK_FUNC(so_n, fn_n, return_type, ...)                                                                                                                \
SK_LOCAL return_type so_n##_##fn_n##_replace(__VA_ARGS__);                                                                                                                            \
SK_LOCAL return_type (*so_n##_##fn_n##_implement)(__VA_ARGS__);                                                                                                                       \
SK_LOCAL void so_n##_##fn_n##_byte_hooked(bytehook_stub_t task_stub, int status_code, const char *caller_path_name, const char *sym_name, void *new_func, void *prev_func, void *arg) \
{                                                                                                                                                                                     \
    if(status_code == BYTEHOOK_STATUS_CODE_ORIG_ADDR)                                                                                                                                 \
    {                                                                                                                                                                                 \
        so_n##_##fn_n##_implement = decltype(so_n##_##fn_n##_implement)(prev_func);                                                                                                   \
    }                                                                                                                                                                                 \
    else if(status_code != BYTEHOOK_STATUS_CODE_OK)                                                                                                                                   \
    {                                                                                                                                                                                 \
        LOGE(#so_n " " #fn_n " byte hooked failed, stub: 0x%lx, status_code: %d, caller_path_name: %s, sym_name: %s, new_func: 0x%lx, prev_func: 0x%lx, arg: 0x%lx",                  \
             (skAddress)task_stub, status_code, caller_path_name, sym_name, (skAddress)new_func, (skAddress)prev_func, (skAddress)arg);                                               \
    }                                                                                                                                                                                 \
}                                                                                                                                                                                     \
SK_LOCAL return_type so_n##_##fn_n##_replace(__VA_ARGS__)
#else
#define _SK_REPLACE_BUILD_BYTE_HOOK_FUNC(so_n, fn_n, return_type, ...)                                                                                                                \
SK_LOCAL return_type so_n##_##fn_n##_replace(__VA_ARGS__);                                                                                                                            \
SK_LOCAL return_type (*so_n##_##fn_n##_implement)(__VA_ARGS__);                                                                                                                       \
SK_LOCAL void so_n##_##fn_n##_byte_hooked(bytehook_stub_t task_stub, int status_code, const char *caller_path_name, const char *sym_name, void *new_func, void *prev_func, void *arg) \
{                                                                                                                                                                                     \
    if(status_code == BYTEHOOK_STATUS_CODE_ORIG_ADDR)                                                                                                                                 \
    {                                                                                                                                                                                 \
        so_n##_##fn_n##_implement = decltype(so_n##_##fn_n##_implement)(prev_func);                                                                                                   \
    }                                                                                                                                                                                 \
}                                                                                                                                                                                     \
SK_LOCAL return_type so_n##_##fn_n##_replace(__VA_ARGS__)
#endif

// 新版 只能干 Unity-IL2Cpp
// 参数1: 命名空间
// 参数2: 类名
// 参数3: 函数名
// 参数4: 参数个数(不计算 this)
// 参数5: 32位备用函数偏移, 可传 0
// 参数6: 64为备用函数偏移, 可传 0
#define _SK_REPLACE_IL2CPP_FUNC(nz,clz_n,fn_n,arg_sz,off_32,off_64)                              \
{                                                                                                \
Il2CppClass * klass=sk_unity_get_class(NULL,#nz,ECStr(#clz_n));                                  \
skAddress clz_n##_##fn_n##_address=sk_unity_class_get_method_address(klass,ECStr(#fn_n),arg_sz); \
if(clz_n##_##fn_n##_address==0&&off_32!=0&&sizeof(skPtr)==4)                                     \
{clz_n##_##fn_n##_address=sk_il2cpp->base_addr+off_32;}                                          \
if(clz_n##_##fn_n##_address==0&&off_64!=0&&sizeof(skPtr)==8)                                     \
{clz_n##_##fn_n##_address=sk_il2cpp->base_addr+off_64;}                                          \
if(clz_n##_##fn_n##_address==0||!sk_get_hook_adapter()->inline_hook_func_address(                \
clz_n##_##fn_n##_address,(void *)clz_n##_##fn_n##_##arg_sz##_replace,                            \
(void **)&clz_n##_##fn_n##_##arg_sz##_implement,false))                                          \
{LOGE("Hook " #nz "." #clz_n "." #fn_n "(" #arg_sz ") Failed!");}                                \
}

// 参数1: 类名
// 参数2: 函数名
// 参数3: 参数个数(不计算 this)
// 参数4: 返回值
// 参数5: 参数 ...
#define _SK_REPLACE_BUILD_IL2CPP_FUNC(clz_n,fn_n,arg_sz,r_type, ...)                             \
SK_LOCAL r_type (*clz_n##_##fn_n##_##arg_sz##_implement)(__VA_ARGS__);                           \
SK_LOCAL r_type clz_n##_##fn_n##_##arg_sz##_replace(__VA_ARGS__)

#endif // FRAMEWORK_SK_HOOK_ADAPTER_SK_ADAPTER_H