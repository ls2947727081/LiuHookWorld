//
// Created by admin on 2025/10/17.
//
// HOOK JNI_OnLoad
#include"myhook.h"
#include "sk_hook_macro.h"


// hook do_dlopen
SK_HOOK_FUNC("linker64",
             "__dl__Z9do_dlopenPKciPK17android_dlextinfoPKv",
             hook_do_open,
             void*,
             (const char* name,int flags,const android_dlextinfo* extinfo,const void* caller_addr),
             0) // 0 表示手动注册
{
    LOGE("hook_do_open called: %s", name ? name : "(null)");

    // 调用原函数
    void* result = SK_CALL_ORIG(hook_do_open, name, flags, extinfo, caller_addr);

    if (name && strstr(name, "libil2cpp.so")) {
        LOGI("Find libil2cpp.so loaded, handle = %p", result);
        g_il2cpp_handle = result;
        // 在 dlopen 返回后获取基地址
        il2cppbase = get_base("libil2cpp.so"); // 假设 get_base 函数已实现并返回正确基地址

        // 提取包名
        NameToApkName(name);

        // 确保只 hook 一次 il2cpp_init
        if (!il2cpp_hook_flag && g_il2cpp_handle) {
            LOGI("Hooking il2cpp_init...");

            //hook 的il2api
            hook_il2cpp_api();
        }
    }

    // 可修改返回值
    return result;
}


// 你自己的函数里调用注册
static jboolean nativeHookTestGamecpp() {
    LOGE("123132");
    hook_do_open_register();
    return 1;
}

JNIEXPORT jint JNI_OnLoad(JavaVM* vm, void* reserved) {
    LOGE("START ONLOAD!!!");
    if (vm){
        if (shadowhook_init(SHADOWHOOK_MODE_UNIQUE, false) != 0) {
            LOGE("shadowhook_init failed: %d", shadowhook_get_errno());
            return JNI_ERR;
        }

        JNIEnv* env = nullptr;
        if (vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK || !env) {
            LOGE("GetEnv failed");
            return JNI_ERR;
        }

        // 直接调用 hook
        if (!nativeHookTestGamecpp()) {
            return JNI_ERR;
        }
    }


    return JNI_VERSION_1_6;
}