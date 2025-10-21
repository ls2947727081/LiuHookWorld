//
// Created by admin on 2025/7/7.
//
#include <jni.h>
#include <android/log.h>
#include <dlfcn.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <cinttypes>
#include <fcntl.h>
#include <android/dlext.h>
#include <vector>
#include "shadowhook.h"
#include <set>
#include <string>
#include <sys/mman.h>
#include <sys/stat.h>
#include <elf.h>
#include <algorithm>
#include "il2cpp-object-internals.h"
#include "il2cpp-tabledefs.h"

#define LOG_TAG "HOOK1"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)


// --- 全局变量 ---
// libil2cpp.so 库的句柄
void* g_il2cpp_handle = nullptr;
// libil2cpp.so 在内存中的基地址
uintptr_t il2cppbase = 0x0;
// 储存apk的包名，用于日志路径
char* apkname = nullptr;
// Dump 日志文件句柄
FILE* g_dump_fp = nullptr;
// il2cpp_init hook 标志，确保只 hook 一次
bool il2cpp_hook_flag = false;
//il2api集成
struct Il2CppFunctions;


// --- 注册方法 ---
void NameToApkName(const char* name);

uintptr_t get_base(const char* basename);

void DumpIl2CppClassesOnce(void* il2cpp_handle);

bool initialize_il2cpp_functions(void* libHandle, Il2CppFunctions& funcs);

// --- IL2CPP Function Pointer Typedefs ---
typedef void* (*il2cpp_domain_get_t)();
typedef Il2CppAssembly** (*il2cpp_domain_get_assemblies_t)(void* domain, size_t* size);
typedef Il2CppClass* (*il2cpp_image_get_class_t)(Il2CppImage* image, uint32_t index);
typedef const char* (*il2cpp_class_get_namespace_t)(Il2CppClass* klass);
typedef const char* (*il2cpp_class_get_name_t)(Il2CppClass* klass);
typedef uint32_t (*il2cpp_image_get_class_count_t)(Il2CppImage* image);

typedef FieldInfo* (*il2cpp_class_get_fields_t)(Il2CppClass* klass, void** iter);
typedef const char* (*il2cpp_field_get_name_t)(FieldInfo* field);
typedef int32_t (*il2cpp_field_get_offset_t)(FieldInfo* field);
typedef MethodInfo* (*il2cpp_class_get_methods_t)(Il2CppClass* klass, void** iter);
typedef const char* (*il2cpp_method_get_name_t)(const MethodInfo* method);

// *** NEW/UNCOMMENTED TYPEDEFS FOR DETAILED METHOD INFO ***
typedef uint32_t (*il2cpp_method_get_flags_t)(MethodInfo* method, void* p_unused);
typedef const Il2CppType* (*il2cpp_method_get_return_type_t)(MethodInfo* method);
typedef int (*il2cpp_method_get_param_count_t)(MethodInfo* method);
typedef const Il2CppType* (*il2cpp_method_get_param_t)(MethodInfo* method, uint32_t index);
typedef const char* (*il2cpp_type_get_name_t)(const Il2CppType* type);
typedef int (*il2cpp_field_get_flags_t)(FieldInfo *field);
typedef int (*il2cpp_class_get_flags_t)(const Il2CppClass *klass);
typedef Il2CppClass* (*il2cpp_class_get_parent_t)(const Il2CppClass *klass);

typedef void (*il2cpp_field_static_get_value_t)(FieldInfo *field, void *value);
typedef Il2CppClass* (*il2cpp_type_get_class_t)(const  Il2CppType* type);
typedef uint8_t (*il2cpp_type_get_type_t)(const  Il2CppType* type);
typedef bool (*il2cpp_class_is_enum_t)(const Il2CppClass* klass);

struct Il2CppFunctions {
    il2cpp_domain_get_t il2cpp_domain_get_fn = nullptr;
    il2cpp_domain_get_assemblies_t il2cpp_domain_get_assemblies_fn = nullptr;
    il2cpp_image_get_class_t il2cpp_image_get_class_fn = nullptr;
    il2cpp_class_get_namespace_t il2cpp_class_get_namespace_fn = nullptr;
    il2cpp_class_get_name_t il2cpp_class_get_name_fn = nullptr;
    il2cpp_image_get_class_count_t il2cpp_image_get_class_count_fn = nullptr;

    il2cpp_class_get_fields_t il2cpp_class_get_fields_fn = nullptr;
    il2cpp_field_get_name_t il2cpp_field_get_name_fn = nullptr;
    il2cpp_field_get_offset_t il2cpp_field_get_offset_fn = nullptr;
    il2cpp_class_get_methods_t il2cpp_class_get_methods_fn = nullptr;
    il2cpp_method_get_name_t il2cpp_method_get_name_fn = nullptr;

    il2cpp_method_get_flags_t il2cpp_method_get_flags_fn = nullptr;
    il2cpp_method_get_param_count_t il2cpp_method_get_param_count_fn = nullptr;
    il2cpp_method_get_param_t il2cpp_method_get_param_fn = nullptr;
    il2cpp_method_get_return_type_t il2cpp_method_get_return_type_fn = nullptr;
    il2cpp_type_get_name_t il2cpp_type_get_name_fn = nullptr;
    il2cpp_field_get_flags_t il2cpp_field_get_flags_fn = nullptr;
    il2cpp_class_get_flags_t il2cpp_class_get_flags_fn = nullptr;
    il2cpp_class_get_parent_t il2cpp_class_get_parent_fn = nullptr;

    il2cpp_field_static_get_value_t il2cpp_field_static_get_value_fn = nullptr;
    il2cpp_type_get_class_t il2cpp_type_get_class_fn = nullptr;
    il2cpp_type_get_type_t il2cpp_type_get_type_fn = nullptr;
    il2cpp_class_is_enum_t il2cpp_class_is_enum_fn = nullptr;
};

// --- 方式1：按符号名解析 ---
template<typename T>
T get_symbol(void* libHandle, const char* symbolName, const char* errorMsg) {
    T fn = (T)dlsym(libHandle, symbolName);
    if (!fn) {
        LOGE(" - Missing %s: %s", symbolName, errorMsg);
    } else {
        LOGI(" - Found %s @ %p", symbolName, (void*)fn);
    }
    return fn;
}

// --- 方式2：按基址+偏移解析 ---
template<typename T>
T get_symbol(void* baseAddr, size_t offset, const char* symbolName) {
    T fn = (T)((uintptr_t)baseAddr + offset);
    if (!fn) {
        LOGE(" - Missing by offset %s (0x%zx)", symbolName, offset);
    } else {
        LOGI(" - Resolved %s @ %p (base=%p + offset=0x%zx)", symbolName, (void*)fn, baseAddr, offset);
    }
    return fn;
}



bool initialize_il2cpp_functions(void* libHandle, Il2CppFunctions& funcs) {
    LOGI("Attempting to initialize IL2CPP introspection functions...");

    funcs.il2cpp_domain_get_fn = get_symbol<il2cpp_domain_get_t>(libHandle, "il2cpp_domain_get", "Critical: Cannot get domain.");
    funcs.il2cpp_domain_get_assemblies_fn = get_symbol<il2cpp_domain_get_assemblies_t>(libHandle, "il2cpp_domain_get_assemblies", "Critical: Cannot list assemblies.");
    funcs.il2cpp_image_get_class_fn = get_symbol<il2cpp_image_get_class_t>(libHandle, "il2cpp_image_get_class", "Critical: Cannot get class from image.");
    funcs.il2cpp_class_get_namespace_fn = get_symbol<il2cpp_class_get_namespace_t>(libHandle, "il2cpp_class_get_namespace", "Critical: Cannot get class namespace.");
    funcs.il2cpp_class_get_name_fn = get_symbol<il2cpp_class_get_name_t>(libHandle, "il2cpp_class_get_name", "Critical: Cannot get class name.");
    funcs.il2cpp_image_get_class_count_fn = get_symbol<il2cpp_image_get_class_count_t>(libHandle, "il2cpp_image_get_class_count", "Critical: Cannot get class count.");
    funcs.il2cpp_class_get_fields_fn = get_symbol<il2cpp_class_get_fields_t>(libHandle, "il2cpp_class_get_fields", "Field enumeration might be incomplete.");
    funcs.il2cpp_field_get_name_fn = get_symbol<il2cpp_field_get_name_t>(libHandle, "il2cpp_field_get_name", "Field name resolution might be incomplete.");
    funcs.il2cpp_field_get_offset_fn = get_symbol<il2cpp_field_get_offset_t>(libHandle, "il2cpp_field_get_offset", "Field offset resolution might be incomplete.");
    funcs.il2cpp_class_get_methods_fn = get_symbol<il2cpp_class_get_methods_t>(libHandle, "il2cpp_class_get_methods", "Method enumeration might be incomplete.");
    funcs.il2cpp_method_get_name_fn = get_symbol<il2cpp_method_get_name_t>(libHandle, "il2cpp_method_get_name", "Method name resolution might be incomplete.");
    funcs.il2cpp_method_get_flags_fn = get_symbol<il2cpp_method_get_flags_t>(libHandle, "il2cpp_method_get_flags", "Method flags might be incomplete.");
    funcs.il2cpp_method_get_param_count_fn = get_symbol<il2cpp_method_get_param_count_t>(libHandle, "il2cpp_method_get_param_count", "Method parameter count might be incomplete.");
    funcs.il2cpp_method_get_param_fn = get_symbol<il2cpp_method_get_param_t>(libHandle, "il2cpp_method_get_param", "Method parameter types might be incomplete.");
    funcs.il2cpp_method_get_return_type_fn = get_symbol<il2cpp_method_get_return_type_t>(libHandle, "il2cpp_method_get_return_type", "Method return type might be incomplete.");
    funcs.il2cpp_type_get_name_fn = get_symbol<il2cpp_type_get_name_t>(libHandle, "il2cpp_type_get_name", "Type name resolution might be incomplete.");
    funcs.il2cpp_field_get_flags_fn = get_symbol<il2cpp_field_get_flags_t>(libHandle,"il2cpp_field_get_flags","ERROR: il2cpp_field_get_flags");
    funcs.il2cpp_class_get_flags_fn = get_symbol<il2cpp_class_get_flags_t>(libHandle,"il2cpp_class_get_flags","ERROR: il2cpp_class_get_flags");
    funcs.il2cpp_class_get_parent_fn = get_symbol<il2cpp_class_get_parent_t>(libHandle,"il2cpp_class_get_parent","ERROR: il2cpp_class_get_parent");
    funcs.il2cpp_field_static_get_value_fn = get_symbol<il2cpp_field_static_get_value_t>(libHandle,"il2cpp_field_static_get_value","ERROR: il2cpp_field_static_get_value");
    funcs.il2cpp_type_get_class_fn = get_symbol<il2cpp_type_get_class_t>(libHandle, "il2cpp_type_get_class_or_element_class", "ERROR: il2cpp_type_get_class");
    funcs.il2cpp_type_get_type_fn = get_symbol<il2cpp_type_get_type_t>(libHandle, "il2cpp_type_get_type", "ERROR: il2cpp_type_get_type");
    funcs.il2cpp_class_is_enum_fn = get_symbol<il2cpp_class_is_enum_t>(libHandle, "il2cpp_class_is_enum", "ERROR: il2cpp_class_is_enum");
    // --- 偏移方式（例：il2cpp_class_get_name 在偏移 0x123456）---
//    funcs.il2cpp_class_get_name_fn = get_symbol<il2cpp_class_get_name_t>(baseAddr, 0x123456, "il2cpp_class_get_name");


    // Check for critical functions only
    if (!funcs.il2cpp_domain_get_fn || !funcs.il2cpp_domain_get_assemblies_fn ||
        !funcs.il2cpp_image_get_class_fn || !funcs.il2cpp_class_get_namespace_fn ||
        !funcs.il2cpp_class_get_name_fn || !funcs.il2cpp_image_get_class_count_fn) {
        LOGE("Failed to get one or more critical IL2CPP symbols. Please verify libil2cpp.so is loaded and symbol names are correct.");
        return false;
    }

    LOGI("Successfully resolved all necessary IL2CPP functions.");
    return true;
}

inline const std::unordered_map<std::string, std::string> type_map = {
        {"Int64", "long"},
        {"Int64*", "long*"},
        {"Int32", "int"},
        {"Int32*", "int*"},
        {"Int16", "short"},
        {"Int16*", "short*"},
        {"Byte", "byte"},
        {"Byte*", "byte*"},
        {"Boolean", "bool"},
        {"Boolean*", "bool*"},
        {"String", "string"},
        {"String*", "string*"},
        {"Double", "double"},
        {"Double*", "double*"},
        {"Single", "float"},
        {"Single*", "float*"},
        {"Object", "object"},
        {"Object*", "object*"},
        {"Char", "char"},
        {"Char*", "char*"},
        {"UInt64", "uint64"},
        {"UInt64*", "uint64*"},
        {"UInt32", "uint"},
        {"UInt32*", "uint*"},
        {"UInt16", "ushort"},
        {"UInt16*", "ushort*"},
        {"SByte", "sbyte"},
        {"SByte*", "sbyte*"},
        {"Void", "void"},
        {"Void*", "void*"},
};


//获取base方法
uintptr_t get_base(const char* basename) {
    FILE* fp = fopen("/proc/self/maps", "r");
    if (!fp) {
        LOGE("open maps error!");
        return 0;
    }

    char line[512];
    while (fgets(line, sizeof(line), fp)) {
        if (strstr(line, basename)) {
            if (strstr(line, "[anon:") || strstr(line, "]")) continue;
            // 找到 '-' 的位置，获取起始地址部分
            char* dash = strchr(line, '-');
            if (!dash) continue;
            *dash = '\0';  // 把 '-' 替换为字符串结尾符，提取起始地址部分
            errno = 0;
            char* endptr = nullptr;
            uintptr_t addr = strtoul(line, &endptr, 16);
            if (endptr != line && errno == 0) {
                fclose(fp);
                return addr;
            }
        }
    }

    fclose(fp);
    LOGE("Base address not found for %s in /proc/self/maps", basename);
    return 0;
}


static std::string Il2CppStringToStdString(const Il2CppString* str)
{
    if (!str || str->length <= 0) return std::string();

    const uint16_t* data = reinterpret_cast<const uint16_t*>(str->chars);
    int len = str->length;
    std::string out;
    out.reserve((size_t)len * 3); // 预分配，通常够用

    for (int i = 0; i < len; ++i) {
        uint32_t codepoint = data[i];

        // 处理高代理项（surrogate pair）
        if (codepoint >= 0xD800 && codepoint <= 0xDBFF) {
            if (i + 1 < len) {
                uint32_t low = data[i + 1];
                if (low >= 0xDC00 && low <= 0xDFFF) {
                    codepoint = ((codepoint - 0xD800) << 10) + (low - 0xDC00) + 0x10000;
                    ++i; // 消耗低代理项
                } else {
                    // 非法序列，替换字符
                    codepoint = 0xFFFD;
                }
            } else {
                codepoint = 0xFFFD;
            }
        } else if (codepoint >= 0xDC00 && codepoint <= 0xDFFF) {
            // 不配对的低代理项，替换
            codepoint = 0xFFFD;
        }

        // UTF-8 编码
        if (codepoint <= 0x7F) {
            out.push_back(static_cast<char>(codepoint));
        } else if (codepoint <= 0x7FF) {
            out.push_back(static_cast<char>(0xC0 | ((codepoint >> 6) & 0x1F)));
            out.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
        } else if (codepoint <= 0xFFFF) {
            out.push_back(static_cast<char>(0xE0 | ((codepoint >> 12) & 0x0F)));
            out.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
        } else {
            out.push_back(static_cast<char>(0xF0 | ((codepoint >> 18) & 0x07)));
            out.push_back(static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
        }
    }

    return out;
}

// 初始化 Dump 日志文件
void init_dump_log(const char* apknames) {
    if (!apknames || apknames[0] == '\0') {
        LOGE("APK name is empty, cannot initialize dump log.");
        return;
    }
    std::string filepath = std::string("/storage/emulated/0/Android/data/") + apknames + "/files/dump.txt";
    LOGI("Dump log path: %s", filepath.c_str());

    // 以 'w' 模式打开清空旧日志，再以 'a' 模式打开准备写入
    // 简化操作：直接使用 'w' 模式，因为每次 Dump 都会重新开始
    g_dump_fp = fopen(filepath.c_str(), "w");
    if (!g_dump_fp) {
        // 使用 LOGE 记录错误，不使用 perror
        LOGE("Failed to open dump log file at %s", filepath.c_str());
    } else {
        LOGI("Dump log file opened successfully.");
    }
}

// 写入 Dump 日志
void write_dump_log(const char* format, ...) {
    if (!g_dump_fp) return;  // 未初始化或打开失败则返回

    va_list args;
    va_start(args, format);
    vfprintf(g_dump_fp, format, args);
    fprintf(g_dump_fp, "\n"); // 自动添加换行
    va_end(args);

    // 频繁 flush 影响性能，建议在重要节点或结束时调用
    // fflush(g_dump_fp);
}

// 关闭 Dump 日志文件
void close_dump_log() {
    if (g_dump_fp) {
        fflush(g_dump_fp);  // 确保所有数据写入
        fclose(g_dump_fp);
        g_dump_fp = nullptr;
        LOGI("Dump log file closed.");
    }
}


// 提取 APK 包名
void NameToApkName(const char* name) {
    if (!name) return;
    // 假设 name 类似于 /data/app/.../com.example.game-1/.../lib/arm64/libil2cpp.so
    // 我们要找 com.example.game
    const char* start = strstr(name, "/com");
    if (!start) return;
    start++; // 跳过 '/'

    // 检查是否以 com. 开头，稍微放宽到以 com 结尾
    if (!(start[0] == 'c' && start[1] == 'o' && start[2] == 'm')) return;

    // 向后扫描直到碰到第一个非合法包名字符
    const char* end = start;
    // 合法字符：字母、数字、点、下划线
    while (*end) {
        if ((*end >= 'a' && *end <= 'z') ||
            (*end >= 'A' && *end <= 'Z') ||
            (*end >= '0' && *end <= '9') ||
            *end == '.' || *end == '_') {
            end++;
        } else {
            break;
        }
    }
    size_t len = end - start;
    if (len == 0) return;

    // 释放旧的内存并分配新的
    if (apkname) {
        free(apkname);
        apkname = nullptr;
    }
    apkname = (char*)malloc(len + 1);
    if (!apkname) return;
    strncpy(apkname, start, len);
    apkname[len] = '\0';
    LOGI("Extracted APK Name: %s", apkname);
}
