//
// Created by admin on 2025/10/17.
//
// HOOK JNI_OnLoad
#include"myhook.h"
#include "sk_hook_macro.h"
#include "unityDumper.h"

//全局方法
Il2CppFunctions funcs;
//获取 Il2CppDomain
void* GetIl2CppDomain(void* handle, Il2CppFunctions& funcs);
//获取所有的 Assembly 列表
Il2CppAssembly** GetIl2CppAssemblies(void *domain, size_t *assemblyCount, Il2CppFunctions& funcs);
// Class 修饰符构建
std::string ClassBuildFlags(uint32_t flags);
// 获取类的类型（class/interface/enum）
const char* GetClassType(Il2CppClass* klass, Il2CppFunctions funcs);
// 字段修饰符构建
std::string FieldBuildFlags(int flags);
//统一构建 Il2CppType 对应的 C# 类型名称
std::string BuildTypeName(const Il2CppType* type, const Il2CppFunctions& funcs);
// 方法修饰符构建
std::string MethodBuildFlags(uint32_t flags);
// 构建方法参数字符串
std::string MethodBuildParamString(MethodInfo* method, const Il2CppFunctions& funcs);


// 输出 Fields 信息
bool DumpClassFields(Il2CppClass* klass, const Il2CppFunctions& funcs) {
    void* fields_iter = nullptr;
    FieldInfo* field = nullptr;

    write_dump_log("    // Fields");
    while ((field = funcs.il2cpp_class_get_fields_fn(klass, &fields_iter)) != nullptr) {
        const char* field_name = funcs.il2cpp_field_get_name_fn(field);
        int flags = funcs.il2cpp_field_get_flags_fn(field);
        std::string field_modifiers = FieldBuildFlags(flags);
        std::string field_type = BuildTypeName(field->type, funcs);
        int32_t offset = funcs.il2cpp_field_get_offset_fn(field);

        std::string static_value_str;
        if (flags & FIELD_ATTRIBUTE_STATIC) {
            Il2CppClass* type_class = funcs.il2cpp_type_get_class_fn(field->type);
            int type_code = funcs.il2cpp_type_get_type_fn(field->type);
            const Il2CppClass* type_class_c = type_class;
            bool type_emun = funcs.il2cpp_class_is_enum_fn(type_class_c);

            if (type_class && funcs.il2cpp_class_is_enum_fn(type_class)) {
                int val = 0;
                funcs.il2cpp_field_static_get_value_fn(field, &val);
                static_value_str = " = " + std::to_string(val);
            }

                //枚举类型
            else if (type_class && type_emun) {
                int val = 0;
                funcs.il2cpp_field_static_get_value_fn(field, &val);
                static_value_str = " = " + std::to_string(val);
            }

                // 整型与布尔类型
            else if (type_code == IL2CPP_TYPE_I4 ||
                     type_code == IL2CPP_TYPE_U4 ||
                     type_code == IL2CPP_TYPE_I2 ||
                     type_code == IL2CPP_TYPE_U2 ||
                     type_code == IL2CPP_TYPE_I1 ||
                     type_code == IL2CPP_TYPE_U1 ||
                     type_code == IL2CPP_TYPE_BOOLEAN) {
                int val = 0;
                funcs.il2cpp_field_static_get_value_fn(field, &val);
                static_value_str = " = " + std::to_string(val);
            }
                // float 类型
            else if (type_code == IL2CPP_TYPE_R4) {
                float val = 0;
                funcs.il2cpp_field_static_get_value_fn(field, &val);
                static_value_str = " = " + std::to_string(val);
            }
                // double 类型
            else if (type_code == IL2CPP_TYPE_R8) {
                double val = 0;
                funcs.il2cpp_field_static_get_value_fn(field, &val);
                static_value_str = " = " + std::to_string(val);
            }
                // string 类型
            else if (type_code == IL2CPP_TYPE_STRING) {
                Il2CppString *str = nullptr;
                if (field->parent->static_fields) {
                    funcs.il2cpp_field_static_get_value_fn(field, &str);
                    static_value_str = " = " +  Il2CppStringToStdString(str);
                }
            }
                // 其他类型直接跳过
            else {
                static_value_str = "";
            }
        }

        write_dump_log("    %s%s %s %s; // 0x%x",
                       field_modifiers.c_str(),
                       field_type.c_str(),
                       field_name,
                       static_value_str.c_str(),
                       offset);
    }
    return true;
}

// 输出 Methods 信息
bool DumpClassMethods(Il2CppClass* klass, const char* ns, const char* name, const Il2CppFunctions& funcs) {
    void* method_iter = nullptr;
    MethodInfo* method;
    write_dump_log("    // Methods");
    while ((method = funcs.il2cpp_class_get_methods_fn(klass, &method_iter)) != nullptr) {
        const char* method_name = funcs.il2cpp_method_get_name_fn(method);
        if (!method_name) continue;

        uint32_t flags = funcs.il2cpp_method_get_flags_fn(method, nullptr);
        std::string method_modifiers = MethodBuildFlags(flags);
        // 获取返回类型
        std::string return_type_name = BuildTypeName(funcs.il2cpp_method_get_return_type_fn(method), funcs);
        std::string param_types_str = MethodBuildParamString(method, funcs);

        // 记录方法详细信息
        if (method->methodPointer) {
            uintptr_t realAddr = (uintptr_t)(method->methodPointer);
            uintptr_t offset = realAddr - il2cppbase;
            write_dump_log("    // RVA:0x%lx", offset);
            write_dump_log("    %s%s %s%s;",
                           method_modifiers.c_str(), // 修饰符已包含尾随空格（如果非空）
                           return_type_name.c_str(),
                           method_name,
                           param_types_str.c_str()
            );
        } else {
            // 可能是 extern 或 abstract 方法，没有 RVA
            write_dump_log("    %s%s %s%s;",
                           method_modifiers.c_str(),
                           return_type_name.c_str(),
                           method_name,
                           param_types_str.c_str()
            );
        }
        write_dump_log("");
    }
    return true;
}

// 输出单个 Assembly 中的所有 Class
bool DumpClassesInImage(Il2CppImage* image, Il2CppFunctions funcs) {
    uint32_t classCount = funcs.il2cpp_image_get_class_count_fn(image);
    write_dump_log("// classCount: %d", classCount);

    for (uint32_t j = 0; j < classCount; ++j) {
        Il2CppClass* klass = funcs.il2cpp_image_get_class_fn(image, j);
        if (!klass) {
            write_dump_log("  // WARNING: Il2CppClass is NULL for DLL '%s' index %u. Skipping.",
                           image->name, j);
            continue;
        }

        const char* ns = funcs.il2cpp_class_get_namespace_fn(klass);
        const char* name = funcs.il2cpp_class_get_name_fn(klass);
        if (!name) {
            write_dump_log("  // WARNING: Class name is NULL for DLL '%s' index %u. Skipping.",
                           image->name, j);
            continue;
        }

        uint32_t flags = funcs.il2cpp_class_get_flags_fn(klass);
        std::string class_flags = ClassBuildFlags(flags);
        const char* type_name = GetClassType(klass, funcs);

        // 绘制 class/struct/interface/enum 的头部
        if (ns && ns[0] != '\0') {
            write_dump_log("// Namespace: %s\n%s%s %s {", ns, class_flags.c_str(), type_name, name);
        } else {
            write_dump_log("%s%s %s {", class_flags.c_str(), type_name, name);
        }

        // 输出 Fields 信息
        DumpClassFields(klass, funcs);
        write_dump_log("");

        // 输出 Methods 信息
        DumpClassMethods(klass, ns, name, funcs);

        // 绘制 class 的尾部
        write_dump_log("}\n");
    }
    return true;
}

// 绘制所有 Assembly/DLL 的信息
bool DumpIl2CppAssemblyInfo(Il2CppAssembly** assemblies, size_t assemblyCount, Il2CppFunctions funcs) {
    LOGI("Begin Dumping Assemblies (Count: %zu)", assemblyCount);

    for (size_t i = 0; i < assemblyCount; ++i) {
        Il2CppImage* image = assemblies[i]->image;
        if (!image) {
            write_dump_log("// WARNING: Assembly at index %zu has a NULL image. Skipping.", i);
            continue;
        }
        const char* imageName = image->name;
        write_dump_log("// DLL: %s", imageName);

        // 绘制 Class 列表
        DumpClassesInImage(image, funcs);
    }
    return true;
}

// 重点，执行绘制操作
void DumpIl2CppClassesOnce(void* il2cpp_handle) {
    LOGI("Begin DumpIl2cppClassesOnce");

    // 1. 准备工作，初始化函数指针

    if (!initialize_il2cpp_functions(il2cpp_handle, funcs)) {
        LOGE("Failed to initialize il2cpp functions.");
        return;
    }

    // 2. 初始化 dump.txt 文件
    if (apkname) {
        init_dump_log(apkname);
    } else {
        LOGE("APK name is not set, cannot initialize dump log.");
        return;
    }
    // 如果文件打开失败，直接返回
    if (!g_dump_fp) return;

    // 3. 执行 il2cpp_domain_get 得到 domain
    void* domain = GetIl2CppDomain(il2cpp_handle, funcs);
    if (!domain) {
        LOGE("Failed to get Il2CppDomain.");
        close_dump_log();
        return;
    }

    // 4. 执行 il2cpp_domain_get_assemblies 得到 assemblies[] 和 assemblyCount
    size_t assemblyCount = 0;
    Il2CppAssembly** assemblies = GetIl2CppAssemblies(domain, &assemblyCount, funcs);
    if (!assemblies || assemblyCount == 0) {
        LOGE("Failed to get assemblies or assembly count is zero.");
        close_dump_log();
        return;
    }

    // 5. 开始执行 assemblyCount 的数量循环输出所有 DLL
    DumpIl2CppAssemblyInfo(assemblies, assemblyCount, funcs);

    // 6. 关闭 dump.txt 文件
    close_dump_log();
    LOGI("Finish DumpIl2cppClassesOnce");
}


SK_HOOK_FUNC("libil2cpp.so",
             "il2cpp_init",
             hook_il2cpp,
             int,
             (const char* domain_name),
             0)
{
    LOGI("[hook] il2cpp_init called, domain_name: %s", domain_name ? domain_name : "null");
    int result = SK_CALL_ORIG(hook_il2cpp,domain_name);
    if (result) {
        LOGI("[hook] il2cpp_init success! il2cppbase: 0x%lx, g_il2cpp_handle: %p", il2cppbase, g_il2cpp_handle);
        // il2cpp 初始化成功后执行 Dump
        DumpIl2CppClassesOnce(g_il2cpp_handle);
    } else {
        LOGE("[hook] il2cpp_init failed!");
    }
    return result;
}


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
            hook_il2cpp_register();
        }
    }

    // 可修改返回值
    return result;
}

// 你自己的函数里调用注册
static jboolean nativeHookTestGamecpp() {
    LOGE("START Unity Dumper!!!");
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
        nativeHookTestGamecpp();
    }

    return JNI_VERSION_1_6;
}



/**
 * 获取 Il2CppDomain
 * @param handle libil2cpp.so 的句柄
 * @param funcs Il2CppFunctions 结构体
 * @return Il2CppDomain 指针
 */
void* GetIl2CppDomain(void* handle, Il2CppFunctions& funcs) {
    LOGI("Getting Il2CppDomain");
    if (!handle) {
        LOGE("Invalid handle");
        return nullptr;
    }
    return funcs.il2cpp_domain_get_fn();
}

/**
 * 获取所有的 Assembly 列表
 * @param domain Il2CppDomain 指针
 * @param assemblyCount 接收 Assembly 数量的指针
 * @param funcs Il2CppFunctions 结构体
 * @return Il2CppAssembly** 指针
 */
Il2CppAssembly** GetIl2CppAssemblies(void *domain, size_t *assemblyCount, Il2CppFunctions& funcs) {
    LOGI("Getting Il2CppAssemblies");
    if (!domain || !assemblyCount) {
        LOGE("GetIl2CppAssemblies: Invalid domain or assemblyCount pointer.");
        *assemblyCount = 0;
        return nullptr;
    }
    return funcs.il2cpp_domain_get_assemblies_fn(domain, assemblyCount);
}

// ===============================
//  Class 修饰符解析工具函数
//  适配 IL2CPP 宏定义 (TYPE_ATTRIBUTE_*)
// ===============================
std::string ClassBuildFlags(uint32_t flags)
{
    std::string modifiers;

    // --- 访问修饰符 ---
    switch (flags & TYPE_ATTRIBUTE_VISIBILITY_MASK) {
        case TYPE_ATTRIBUTE_PUBLIC:               modifiers += "public"; break;
        case TYPE_ATTRIBUTE_NESTED_PUBLIC:        modifiers += "public"; break;
        case TYPE_ATTRIBUTE_NESTED_PRIVATE:       modifiers += "private"; break;
        case TYPE_ATTRIBUTE_NESTED_FAMILY:        modifiers += "protected"; break;
        case TYPE_ATTRIBUTE_NESTED_ASSEMBLY:      modifiers += "internal"; break;
        case TYPE_ATTRIBUTE_NESTED_FAM_OR_ASSEM:  modifiers += "protected internal"; break;
        default: break; // TYPE_ATTRIBUTE_NOT_PUBLIC 或未知情况
    }

    // --- 辅助添加修饰符 ---
    auto append_modifier = [&](const char* mod) {
        if (!modifiers.empty()) modifiers += " ";
        modifiers += mod;
    };

    // --- 抽象 / 密封 / 静态 ---
    bool is_abstract = (flags & TYPE_ATTRIBUTE_ABSTRACT) != 0;
    bool is_sealed   = (flags & TYPE_ATTRIBUTE_SEALED)   != 0;

    if (is_abstract && is_sealed) {
        append_modifier("static"); // static class (abstract + sealed)
    } else {
        if (is_abstract) append_modifier("abstract");
        if (is_sealed)   append_modifier("sealed");
    }

    // --- 接口类型 ---
    if (flags & TYPE_ATTRIBUTE_INTERFACE)
        append_modifier("interface");

    return modifiers.empty() ? "" : modifiers + " ";
}



// 获取类型（class/interface/enum）
const char* GetClassType(Il2CppClass* klass, Il2CppFunctions funcs) {
    if (!klass) return "class"; // 默认返回 class
    uint32_t flags = funcs.il2cpp_class_get_flags_fn(klass);

    // 判断是否为枚举（parent 为 System.Enum）
    Il2CppClass* klassparent = funcs.il2cpp_class_get_parent_fn(klass);
    if(klassparent){
        const char* parentname = funcs.il2cpp_class_get_name_fn(klassparent);
        if (parentname && strcmp(parentname, "Enum") == 0) {
            return "enum";
        }
    }
    // 判断是否为接口 (TypeAttributes.Interface 0x20)
    if ((flags & 0x20) != 0) {
        return "interface";
    }

    // 判断是否为值类型 (parent 为 System.ValueType)
    if(klassparent){
        const char* parentname = funcs.il2cpp_class_get_name_fn(klassparent);
        if (parentname && strcmp(parentname, "ValueType") == 0) {
            // 需要进一步判断是否是 struct (C# struct) 或 primitive type
            // il2cpp 层面 ValueType 是 struct 的父类
            return "struct"; // 简单返回 struct
        }
    }

    // 默认当作 class
    return "class";
}

// 字段修饰符构建
std::string FieldBuildFlags(int flags) {
    std::string field_modifiers;
    // 访问修饰符
    int access_flags = flags & FIELD_ATTRIBUTE_FIELD_ACCESS_MASK;
    switch (access_flags) {
        case FIELD_ATTRIBUTE_PRIVATE: field_modifiers += "private"; break;
        case FIELD_ATTRIBUTE_FAM_AND_ASSEM:
        case FIELD_ATTRIBUTE_FAM_OR_ASSEM: field_modifiers += "protected internal"; break;
        case FIELD_ATTRIBUTE_ASSEMBLY: field_modifiers += "internal"; break;
        case FIELD_ATTRIBUTE_FAMILY: field_modifiers += "protected"; break;
        case FIELD_ATTRIBUTE_PUBLIC: field_modifiers += "public"; break;
        default: break; // 其他情况（如 FIELD_ATTRIBUTE_COMPILER_CONTROLLED）
    }

    auto append_modifier = [&](const char* mod) {
        if (!field_modifiers.empty()) field_modifiers += " ";
        field_modifiers += mod;
    };

    // 其他修饰符
    if (flags & FIELD_ATTRIBUTE_STATIC) append_modifier("static");
    if (flags & FIELD_ATTRIBUTE_INIT_ONLY) append_modifier("readonly");
    if (flags & FIELD_ATTRIBUTE_LITERAL) append_modifier("const");
    if (flags & FIELD_ATTRIBUTE_NOT_SERIALIZED) append_modifier("nonserialized");

    // 返回时确保末尾有空格，如果非空
    return field_modifiers.empty() ? "" : field_modifiers + " ";
}

/**
 * 统一构建 Il2CppType 对应的 C# 类型名称
 * @param type Il2CppType 指针
 * @param funcs Il2CppFunctions 结构体引用
 * @return C# 友好的类型名称
 */
std::string BuildTypeName(const Il2CppType* type, const Il2CppFunctions& funcs) {
    std::string type_name = "unknown";
    if (!type) {
        return type_name;
    }

    const char* type_name_c = funcs.il2cpp_type_get_name_fn(type);
    if (type_name_c) {
        type_name = type_name_c;
        const std::string system_prefix = "System.";
        // 检查是否以 "System." 开头，如果是则尝试使用 C# 别名
        if (type_name.rfind(system_prefix, 0) == 0) {
            std::string short_name = type_name.substr(system_prefix.length());
            if (!short_name.empty()) {
                // 查找映射，如果匹配，则替换成对应 C# 类型（如 Int32 -> int）
                auto it = type_map.find(short_name);
                if (it != type_map.end()) {
                    return it->second;
                }
            }
        }
    }
    return type_name;
}

// 方法修饰符构建
std::string MethodBuildFlags(uint32_t flags) {
    std::string method_modifiers_description;
    // 访问修饰符
    uint32_t access_flags = flags & METHOD_ATTRIBUTE_MEMBER_ACCESS_MASK;
    switch (access_flags) {
        case METHOD_ATTRIBUTE_PRIVATE: method_modifiers_description += "private"; break;
        case METHOD_ATTRIBUTE_FAM_AND_ASSEM:
        case METHOD_ATTRIBUTE_FAM_OR_ASSEM: method_modifiers_description += "protected internal"; break;
        case METHOD_ATTRIBUTE_ASSEM: method_modifiers_description += "internal"; break;
        case METHOD_ATTRIBUTE_FAMILY: method_modifiers_description += "protected"; break;
        case METHOD_ATTRIBUTE_PUBLIC: method_modifiers_description += "public"; break;
        default: break; // 其他情况
    }

    // 其他修饰符
    auto append_modifier = [&](const char* mod) {
        if (!method_modifiers_description.empty()) method_modifiers_description += " ";
        method_modifiers_description += mod;
    };

    if (flags & METHOD_ATTRIBUTE_STATIC) append_modifier("static");
    if (flags & METHOD_ATTRIBUTE_ABSTRACT) append_modifier("abstract");
    if (flags & METHOD_ATTRIBUTE_FINAL) append_modifier("final");
    // 区分 virtual 和 override
    if ((flags & METHOD_ATTRIBUTE_VIRTUAL)) {
        if (!(flags & METHOD_ATTRIBUTE_ABSTRACT)) { // 抽象方法是隐式虚方法
            if (flags & METHOD_ATTRIBUTE_NEW_SLOT)
                append_modifier("virtual");    // 新槽，不是重写
            else
                append_modifier("override");   // 覆盖基类方法
        }
    }
    // 返回时确保末尾有空格，如果非空
    return method_modifiers_description.empty() ? "" : method_modifiers_description + " ";
}

// 构建方法参数字符串
std::string MethodBuildParamString(MethodInfo* method, const Il2CppFunctions& funcs) {
    std::string param_types_str = "(";
    int param_count = funcs.il2cpp_method_get_param_count_fn(method);
    for (int i = 0; i < param_count; ++i) {
        const Il2CppType* param_type = funcs.il2cpp_method_get_param_fn(method, i);
        std::string param_type_name = BuildTypeName(param_type, funcs);
        // 此处只输出类型，不输出参数名，保持与 C# 签名接近
        param_types_str += param_type_name;
        if (i < param_count - 1) param_types_str += ", ";
    }
    param_types_str += ")";

    return param_types_str;
}