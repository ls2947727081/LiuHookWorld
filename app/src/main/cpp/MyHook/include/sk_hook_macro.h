// sk_hook_macro.h
#pragma once
#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef SK_LOG
#define SK_LOG(fmt, ...) printf("[SK_HOOK] " fmt "\n", ##__VA_ARGS__)
#endif

typedef void* sk_address;
typedef void* (*sk_hook_register_fn)(const char* lib, const char* sym, void* new_func, void** old_func);

// ShadowHook hook 注册函数，用户需实现
extern void* shadowhook_hook_sym_name(const char* lib, const char* sym, void* new_func, void** old_func);

#ifdef __cplusplus
}
#endif

// 拼接辅助
#define SK_PASTE2(a,b) a##b
#define SK_PASTE(a,b) SK_PASTE2(a,b)
#if defined(__COUNTER__)
#define SK_UNIQUE_ID(prefix) SK_PASTE(prefix,__COUNTER__)
#else
#define SK_UNIQUE_ID(prefix) SK_PASTE(prefix,__LINE__)
#endif

// =========== 宏库核心 ===========

// 修改点：
// 1. 原函数指针和 hook 函数都不再 static
// 2. 支持跨 cpp 文件使用
// 3. auto_reg = 1 表示 constructor 自动注册，0 表示手动注册
#define SK_HOOK_FUNC(lib_str, sym_str, name, ret, args, auto_reg)           \
    ret (*name##_implement) args = NULL;                                    \
    ret name args;                                                          \
    void name##_register(void) {                                            \
        if (shadowhook_hook_sym_name(lib_str, sym_str, (void*)name, (void**)&name##_implement) == NULL) { \
            SK_LOG("Hook %s failed", #name);                                \
        } else {                                                            \
            SK_LOG("Hook %s success", #name);                               \
        }                                                                   \
    }                                                                       \
    /* 自动注册 constructor */                                               \
    __attribute__((constructor)) void SK_PASTE(name,_auto_register)(void) { \
        if(auto_reg) name##_register();                                     \
    }                                                                       \
    ret name args

// ---------- 调用原函数 ----------
#define SK_CALL_ORIG(name, ...) name##_implement(__VA_ARGS__)
