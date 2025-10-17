#include "myhook.h"

extern "C" JNIEXPORT jstring JNICALL
Java_com_hookeasy_liuhookworld_MainActivity_stringFromJNI(
        JNIEnv* env,
        jobject /* this */) {
    std::string hello = "Hello from C++";
    return env->NewStringUTF(hello.c_str());
}

// HOOK JNI_OnLoad
JNIEXPORT jint JNI_OnLoad(JavaVM* vm, void* reserved) {
    if (shadowhook_init(SHADOWHOOK_MODE_UNIQUE, false) != 0) {
        LOGE("shadowhook_init failed: %d", shadowhook_get_errno());
        return JNI_ERR;
    }

    JNIEnv* env = nullptr;
    if (vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK || !env) {
        LOGE("GetEnv failed");
        return JNI_ERR;
    }

    return JNI_VERSION_1_6;
}