#include "sk_def/sk_def.h"
#include "sk_log/sk_log.h"
#include "shadowhook/include/shadowhook.h"

#define SK_HIDDEN_CODE SK##_IMPL##_ADAPTER##_FUNC

SK_HIDDEN_CODE(void *, get_handle, const s8 * n) { return shadowhook_dlopen(n); }
SK_HIDDEN_CODE(skAddress, find_import, void * h, const s8 * n) { return 0; }
SK_HIDDEN_CODE(skAddress, find_export, void * h, const s8 * n) { return 0; }
SK_HIDDEN_CODE(skAddress, find_symbol, void * h, const s8 * n) { return (skAddress)shadowhook_dlsym(h, n); }
//SK_HIDDEN_CODE(void, enum_import, void * h, void * cb, void * user_data) { }
//SK_HIDDEN_CODE(void, enum_export, void * h, void * cb, void * user_data) { }
//SK_HIDDEN_CODE(void, enum_symbol, void * h, void * cb, void * user_data) { }
SK_HIDDEN_CODE(void, delete_module, void * h) { if(sk_likely(h != NULL)) { shadowhook_dlclose(h); } }

// by shuke 2025.01.13 更新到 v1.1.1 以后, shadowhook 多了很多操作, 无法解决, 强行 return true
//SK_HIDDEN_CODE(bool, hook_engine_init) { return (shadowhook_init(SHADOWHOOK_MODE_UNIQUE, false) == SHADOWHOOK_ERRNO_OK); }
SK_HIDDEN_CODE(bool, hook_engine_init)
{
    int ret = shadowhook_init(SHADOWHOOK_MODE_UNIQUE, false);
    if(ret != SHADOWHOOK_ERRNO_OK)
    {
        LOGE_NO_LINE("ShadowHook 初始化出现错误(%d), 已被忽略", ret);
    }
    return true;
}

SK_HIDDEN_CODE(bool, inline_hook, skAddress address, void * cb, void ** ori_cb, unsigned long data)
{
    void * stub = NULL;
    if(sk_unlikely(data)) { stub = shadowhook_hook_sym_addr((void *)address, cb, ori_cb); }
    else { stub = shadowhook_hook_func_addr((void *)address, cb, ori_cb); }
    return stub;
}