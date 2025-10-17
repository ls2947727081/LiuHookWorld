#include "myhook.h"

extern "C" JNIEXPORT jstring JNICALL
Java_com_hookeasy_liuhookworld_MainActivity_stringFromJNI(
        JNIEnv* env,
        jobject /* this */) {
    std::string hello = "Hello from C++";
    if (shadowhook_init(SHADOWHOOK_MODE_UNIQUE, false) != 0) {
        LOGE("shadowhook_init failed: %d", shadowhook_get_errno());
    }
    return env->NewStringUTF(hello.c_str());
}