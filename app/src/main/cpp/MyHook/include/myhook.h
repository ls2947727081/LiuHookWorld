#include <jni.h>
#include <string>
#include <dlfcn.h>
#include <link.h>
#include <android/log.h>
#include <android/dlext.h>
#include "shadowhook.h"


#define LOG_TAG "HOOK1"
#define HOOKNAME "TESTGAME1: "
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, HOOKNAME __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, HOOKNAME __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, HOOKNAME __VA_ARGS__)
