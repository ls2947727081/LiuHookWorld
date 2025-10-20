#include "myhook.h"


extern "C" JNIEXPORT jstring JNICALL
Java_com_hookeasy_liuhookworld_MainActivity_stringFromJNI(
        JNIEnv* env,
        jobject /* this */) {
    std::string hello = "libil2cpp.so";
    return env->NewStringUTF(hello.c_str());
}

