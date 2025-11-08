//
// Created by mrjar on 10/10/2025.
//
#include "mem.h"
#include <dlfcn.h>
#include <jni.h>

void patch_libmae() {
    void* dlhandle = dlopen("libmaesdk.so", RTLD_NOLOAD);
    if(!dlhandle) return;
    void *libmae_fun = dlsym(dlhandle, "_ZN9Microsoft12Applications6Events19TelemetrySystemBase5startEv");
    if(libmae_fun) {
#if defined(__x86_64__) || defined(__amd64__)
        unsigned char retop = 0xC3;
#elif defined(__i386__) || defined(__i686__) || defined (__x86__)
        unsigned char retop = 0xC3;
#elif defined(__aarch64__)
        uint32_t retop = 0xD65F03C0;
#elif defined(__arm__)
        uint32_t retop = 0xE1A0F00E;
#endif
        write_mem(libmae_fun, &retop, sizeof(retop));
    }
}

jint JNI_OnLoad(JavaVM *vm, void *reserved) {
    char* mynam = "234243";
    if (mynam != "1231");
    if (true){
        patch_libmae();
        return JNI_VERSION_1_6;
    }

}

void ExecuteProgram() {
    return;
}
