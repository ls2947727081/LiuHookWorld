#include <vector>
#include <string>
#include <algorithm>
#include <thread>
#include <map>

#include "sk_def/sk_def.h"
#include "sk_log/sk_log.h"
#include "sk_watch_dog.h"
#include "sk_utils/sk_hash.h"
#include "sk_utils/sk_package_utils.h"
#include "sk_adapter.h"
#include "sk_str_encrypt/str_encrypt.hpp"
#include "sk_utils/sk_jni_utils.h"
#include "cjson/cJSON.h"

#define SK_WATCH_DOG_DEBUG 0
#if SK_WATCH_DOG_DEBUG
#define DOGE_LOGD LOGD
#else
#define DOGE_LOGD(x, ...) ((void *)0)
#endif

using namespace std;

// Internal Use
#define _SK_BUILD_REPLACE_FUNC(symbol_name, return_type, ...)     \
SK_LOCAL return_type (* symbol_name##_implement)(__VA_ARGS__);    \
SK_LOCAL return_type symbol_name##_replace(__VA_ARGS__, void * user_data)
#define _SK_HOOK_FUNC(symbol_name) \
sk_get_hook_adapter()->inline_hook_func_address((skAddress)symbol_name##_address, (void *)symbol_name##_replace, (void **)&symbol_name##_implement)

#if defined(__arm__)
static const char * linker_name = "linker";
#elif defined(__aarch64__)
static const char * linker_name = "linker64";
#endif

#ifdef __cplusplus
#undef NULL
#define NULL nullptr
#endif

struct skWD_FoundSymbolContext
{
    char * name;
    skAddress address;
};

extern pthread_mutex_t g_main_lock;
skWatchDog * skWatchDog::ins = NULL;
cJSON * msg_details = NULL;
cJSON * watch_dog_event_array = NULL;
static std::map<std::string, std::string> watch_dog_event;

static void init_watch_dog_event_handle()
{
    watch_dog_event.clear();
    watch_dog_event.insert(std::pair<std::string, std::string>(ECStr("event"), ECStr("dev_log")));
    watch_dog_event.insert(std::pair<std::string, std::string>(ECStr("name"), ECStr("watch_dog_event")));
    watch_dog_event_array = cJSON_CreateArray();
}
static jobject std_map_to_hash_map(JNIEnv * env, std::map<std::string, std::string> *map)
{
    jclass map_clz = sk_jni_find_class(ECStr("java/util/HashMap"));
    if(map_clz == NULL) { LOGE("意料之外的 BUG, 拿不到 HashMap 类, 无法上传 fuck init_array 失败的埋点"); return NULL; }

    jmethodID init_mth = sk_jni_get_method_id(map_clz, ECStr("<init>"), ECStr("()V"));
    if(init_mth == NULL) { LOGE("意料之外的 BUG, 找不到构造函数, 无法上传 fuck init_array 失败的埋点"); return NULL; }

    jmethodID put_mth = sk_jni_get_method_id(map_clz, ECStr("put"), ECStr("(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;"));
    if(put_mth == NULL) { LOGE("意料之外的 BUG, 找不到 HashMap<E, E>.put(Object, Object) 函数, 无法上传 fuck init_array 失败的埋点"); return NULL; }

    jobject hash_map = env->NewObject(map_clz, init_mth);
    if(hash_map == NULL || env->ExceptionCheck())
    {
        LOGE("构造 HashMap<E,E> 对象失败!");
        env->ExceptionClear();
        return NULL;
    }

    for(auto & pair : *map)
    {
        jstring key = sk_jni_new_string(pair.first.c_str());
        jstring value = sk_jni_new_string(pair.second.c_str());
        env->CallObjectMethod(hash_map, put_mth, key, value);
        if(env->ExceptionCheck())
        {
            LOGE("插入 HashMap<E,E> 元素失败!");
            env->ExceptionClear();
            return NULL;
        }
        env->DeleteLocalRef(key);
        if(env->ExceptionCheck())
        {
            env->ExceptionClear();
            return NULL;
        }
        env->DeleteLocalRef(value);
        if(env->ExceptionCheck())
        {
            env->ExceptionClear();
            return NULL;
        }
    }
    return hash_map;
}
static void upload_watch_dog_event_msg()
{
    if(msg_details != NULL) { LOGE("只有一次上传机会, 已经上传过就无法在上传了!"); return; }

    {
        msg_details = cJSON_CreateObject();
        cJSON_AddItemToObject(msg_details, "details", watch_dog_event_array);

        char * details = cJSON_Print(msg_details);
        std::string std_details = details;
        std_details.erase(std::remove(std_details.begin(), std_details.end(), '\n'), std_details.end());
        std_details.erase(std::remove(std_details.begin(), std_details.end(), '\t'), std_details.end());
        free(details);

        watch_dog_event.insert(std::pair<std::string, std::string>(ECStr("msg"), std_details.c_str()));
    }

    JNIEnv * env = sk_jni_get_env();
    if(env == NULL) { LOGE("意料之外的 BUG, 拿不到 ENV, 无法上传 fuck init_array 失败的埋点"); return; }

    jclass sls_clz = sk_jni_find_class(ECStr("com/single/mod/core/statistics/SlsLog"));
    if(sls_clz == NULL) { LOGE("意料之外的 BUG, 拿不到 Sls 类, 无法上传 fuck init_array 失败的埋点"); return; }

    jmethodID sendLog_mid = sk_jni_get_static_method_id(sls_clz, ECStr("nativeSendLog"), ECStr("(Ljava/util/Map;)V"));
    if(sendLog_mid == NULL) { LOGE("意料之外的 BUG, 拿不到 Sls.sendLog(Map, boolean, boolean) 方法, 无法上传 fuck init_array 失败的埋点"); return; }

    jobject hash_map = std_map_to_hash_map(env, &watch_dog_event);
    if(hash_map == NULL) { LOGE("意料之外的 BUG, std::map 转 HashMap 失败, 无法上传 fuck init_array 失败的埋点"); return; }

    env->CallStaticVoidMethod(sls_clz, sendLog_mid, hash_map);
    if(env->ExceptionCheck())
    {
        LOGE("意料之外的 BUG, 调用 Sls.sendLog() 失败, 无法上传 fuck init_array 失败的埋点");
        env->ExceptionClear();
        return;
    }
    env->DeleteLocalRef(hash_map);
}
static void add_event(char * key, char * value)
{
    cJSON * temp = cJSON_CreateObject();
    cJSON_AddStringToObject(temp, key, value);
    cJSON_AddItemToArray(watch_dog_event_array, temp);
}
static void add_event(char * key, skAddress address)
{
    char buff[32];
    snprintf(buff, 32, "0x%lx", address);
    return add_event(key, buff);
}
static void add_event(char * key, std::string &value)
{
    return add_event(key, (char *)value.c_str());
}
static void add_event(char * key, std::string &&value)
{
    return add_event(key, (char *)value.c_str());
}







class skDogeTaskInfo : skNonCopyable
{
public:
    skDogeTaskInfo(const char * n, skNormal_cb cb, int32_t scan_limit)
    : type(kTaskNormal), module(NULL), one_shot_init(false), is_call_init(false), onInitEnter(NULL), onInitLeave(NULL)
    , one_shot_linker(false), is_call_linker(false), onEnter(NULL), onLeave(NULL), hdl(NULL)
    , one_shot_find_libraries(false), is_call_find_libraries(false), onFindLibrariesLoaded(NULL)
    , is_call_normal(false), scan_times(0), scan_limit(scan_limit), onLoaded(cb) { SK_COPY_C_STRING(name, n) }
    skDogeTaskInfo(const char * n, skLinker_Init_onEnter cb1, skLinker_Init_onEnter cb2, bool one_shot)
    : type(kTaskInit), module(NULL), one_shot_init(one_shot), is_call_init(false), onInitEnter(cb1), onInitLeave(cb2)
    , one_shot_linker(false), is_call_linker(false), onEnter(NULL), onLeave(NULL), hdl(NULL)
    , one_shot_find_libraries(false), is_call_find_libraries(false), onFindLibrariesLoaded(NULL)
    , is_call_normal(false), scan_times(0), scan_limit(0), onLoaded(NULL) { SK_COPY_C_STRING(name, n) }
    skDogeTaskInfo(const char * n, skLinker_onEnter cb1, skLinker_onLeave cb2, bool one_shot)
    : type(kTaskLinker), module(NULL), one_shot_init(false), is_call_init(false), onInitEnter(NULL), onInitLeave(NULL)
    , one_shot_linker(one_shot), is_call_linker(false), onEnter(cb1), onLeave(cb2), hdl(NULL)
    , one_shot_find_libraries(false), is_call_find_libraries(false), onFindLibrariesLoaded(NULL)
    , is_call_normal(false), scan_times(0), scan_limit(0), onLoaded(NULL) { SK_COPY_C_STRING(name, n) }
    skDogeTaskInfo(const char * n, skLinker_Find_Libraries_cb cb, bool one_shot_find_libraries)
    : type(kTaskFind_Libraries), module(NULL), one_shot_init(false), is_call_init(false), onInitEnter(NULL), onInitLeave(NULL)
    , one_shot_linker(false), is_call_linker(false), onEnter(NULL), onLeave(NULL), hdl(NULL)
    , one_shot_find_libraries(one_shot_find_libraries), is_call_find_libraries(false), onFindLibrariesLoaded(cb)
    , is_call_normal(false), scan_times(0), scan_limit(0), onLoaded(NULL) { SK_COPY_C_STRING(name, n) }
    void notify_user(bool is_called_ori = false)
    {
        switch(type)
        {
        case kTaskInit:
            if(module == NULL) { module = new skModule(name); }
            if(onInitEnter && !is_called_ori && ((!one_shot_init) || (!is_call_init))) { onInitEnter(module); break; }
            if(onInitLeave && ((!one_shot_init) || (!is_call_init))) { onInitLeave(module); is_call_init = true; }
            break;
        case kTaskLinker:
            if(!is_called_ori && !onEnter) { break; }
            if(!is_called_ori && ((!one_shot_linker) || (!is_call_linker))) { onEnter(); break; }
            if(module == NULL) { module = new skModule(name); }
            if(onLeave && ((!one_shot_linker) || (!is_call_linker))) { onLeave(module); is_call_linker = true; }
            break;
        case kTaskNormal:
            if(module == NULL) { module = new skModule(name); }
            if(onLoaded && !is_call_normal) { onLoaded(module); is_call_normal = true; }
            break;
        case kTaskFind_Libraries:
            if(module == NULL) { module = new skModule(name); }
            if(onFindLibrariesLoaded && ((!one_shot_find_libraries) || (!is_call_find_libraries))) { onFindLibrariesLoaded(module); is_call_find_libraries = true; }
            break;
        default: break;
        }
    }
public:
    char                            * name;
    skLinkerTaskType                type;
    skModule                        * module;
    bool                            is_call_init;
    bool                            one_shot_init;
    skLinker_Init_onEnter           onInitEnter;
    skLinker_Init_onLeave           onInitLeave;
    bool                            is_call_linker;
    bool                            one_shot_linker;
    skLinker_onEnter                onEnter;
    skLinker_onLeave                onLeave;
    void                            * hdl;
    bool                            is_call_normal;
    int32_t                         scan_times;
    int32_t                         scan_limit;
    skNormal_cb                     onLoaded;
    bool                            one_shot_find_libraries;
    bool                            is_call_find_libraries;
    skLinker_Find_Libraries_cb      onFindLibrariesLoaded;
};

skWatchDog * sk_get_watch_dog()
{
    if(skWatchDog::ins == NULL)
    {
        pthread_mutex_lock(&g_main_lock);
        if(skWatchDog::ins == NULL)
        {
            skWatchDog::ins = new skWatchDog();
            skWatchDog::ins->lock = PTHREAD_MUTEX_INITIALIZER;
            pthread_mutex_lock(&skWatchDog::ins->lock);
            skWatchDog::ins->linker_module = NULL;
            skWatchDog::ins->tasks = new list<void *>;
            skWatchDog::ins->tasks->clear();
            pthread_mutex_unlock(&skWatchDog::ins->lock);

        }
        pthread_mutex_unlock(&g_main_lock);
    }

    return skWatchDog::ins;
}








class skWD_FoundMatchInfo
{
public:
    skWD_FoundMatchInfo(char * name, uint32_t count)
    {
        this->name = name;
        this->arg_count = count;
        this->detail = NULL;
    }
    string name;
    uint32_t arg_count;
    skSymbolDetails * detail;
};

_SK_BUILD_REPLACE_FUNC(do_dlopen_2, void *, const char * filename, int flags)
{
    DOGE_LOGD("linker load: [%s]", filename);

    skWatchDog * doge = sk_get_watch_dog();
    if(sk_unlikely(doge == NULL)) { return do_dlopen_2_implement(filename, flags); }

    skDogeTaskInfo * task = skcast(skDogeTaskInfo *, doge->find_task(filename, kTaskLinker));
    if(sk_unlikely(task != NULL))
    {
        task->notify_user();
        task->hdl = do_dlopen_2_implement(filename, flags);
        task->notify_user(true);
        return task->hdl;
    }
    return do_dlopen_2_implement(filename, flags);
}
_SK_BUILD_REPLACE_FUNC(do_dlopen_3, void *, const char * filename, int flags, void * ext_info)
{
    DOGE_LOGD("linker load: [%s]", filename);

    skWatchDog * doge = sk_get_watch_dog();
    if(sk_unlikely(doge == NULL)) { return do_dlopen_3_implement(filename, flags, ext_info); }

    skDogeTaskInfo * task = skcast(skDogeTaskInfo *, doge->find_task(filename, kTaskLinker));
    if(sk_unlikely(task != NULL))
    {
        task->notify_user();
        task->hdl = do_dlopen_3_implement(filename, flags, ext_info);
        task->notify_user(true);
        return task->hdl;
    }
    return do_dlopen_3_implement(filename, flags, ext_info);
}
_SK_BUILD_REPLACE_FUNC(do_dlopen_4, void *, const char * filename, int flags, void * ext_info, void * caller_address)
{
    DOGE_LOGD("linker load: [%s]", filename);

    skWatchDog * doge = sk_get_watch_dog();
    if(sk_unlikely(doge == NULL)) { return do_dlopen_4_implement(filename, flags, ext_info, caller_address); }

    skDogeTaskInfo * task = skcast(skDogeTaskInfo *, doge->find_task(filename, kTaskLinker));
    if(sk_unlikely(task != NULL))
    {
        task->notify_user();
        task->hdl = do_dlopen_4_implement(filename, flags, ext_info, caller_address);
        task->notify_user(true);
        return task->hdl;
    }
    return do_dlopen_4_implement(filename, flags, ext_info, caller_address);
}
static SK_BOOL found_do_dlopen(const skSymbolDetails * detail, void * user_data)
{
    vector<skWD_FoundMatchInfo *> * sym_info = skcast(vector<skWD_FoundMatchInfo *> *, user_data);

    if(sk_unlikely(detail == NULL)) { return SK_TRUE; }
    if(strstr(detail->name, "_dlopen") == NULL) { return SK_TRUE; }
    for_each(sym_info->begin(), sym_info->end(), [&detail](vector<skWD_FoundMatchInfo *>::value_type & info)->void
    {
        if(info->name == detail->name)
        {
            info->detail = sk_elf_symbol_details_copy(detail);
        }
    });
    return SK_TRUE;
}
bool skWatchDog::fuck_linker_do_dlopen()
{
    if(sk_likely(find_task(kTaskLinker) == NULL)) { return true; }
    bool hook_result = false;
    vector<skWD_FoundMatchInfo *> sym_info = {
        new skWD_FoundMatchInfo(ECStr("__dl__Z9do_dlopenPKciPK17android_dlextinfoPKv"), 4),
        new skWD_FoundMatchInfo(ECStr("__dl__Z9do_dlopenPKciPK17android_dlextinfoPv"), 4),
        new skWD_FoundMatchInfo(ECStr("__dl__ZL10dlopen_extPKciPK17android_dlextinfoPv"), 4),
        new skWD_FoundMatchInfo(ECStr("__dl__Z20__android_dlopen_extPKciPK17android_dlextinfoPKv"), 4),
        new skWD_FoundMatchInfo(ECStr("__dl___loader_android_dlopen_ext"), 4),
        new skWD_FoundMatchInfo(ECStr("__dl__Z9do_dlopenPKciPK17android_dlextinfo"), 3),
        new skWD_FoundMatchInfo(ECStr("__dl__Z8__dlopenPKciPKv"), 3),
        new skWD_FoundMatchInfo(ECStr("__dl___loader_dlopen"), 3),
        new skWD_FoundMatchInfo(ECStr("__dl_dlopen"), 2)
    };

    if(this->linker_module == NULL) { this->linker_module = new skModule(linker_name); }
    this->linker_module->enum_symbol(found_do_dlopen, &sym_info);
    add_event(ECStr("linker_base_address"), this->linker_module->get_base_addr());

    for(int i = 0; i < sym_info.size(); ++i)
    {
        skWD_FoundMatchInfo * info = sym_info[i];
        if(info->detail == NULL) { continue; }

        switch(info->arg_count)
        {
            case 2:
            {
                DOGE_LOGD("2 name: %s, address: 0x%lx", info->detail->name, info->detail->address);
                add_event(ECStr("do_dlopen_arg_count"), (skAddress)2);
                add_event(ECStr("do_dlopen_symbol_name"), (char *)info->detail->name);
                add_event(ECStr("do_dlopen_symbol_address"), (skAddress)info->detail->address);
                skAddress do_dlopen_2_address = info->detail->address;
                hook_result = _SK_HOOK_FUNC(do_dlopen_2);
                add_event(ECStr("do_dlopen_hook_result"), hook_result ? (char *)"True" : (char *)"False");
                DOGE_LOGD("watch dog, hook do_dlopeon() 2, hook_result: %d", hook_result);
                break;
            }
            case 3:
            {
                DOGE_LOGD("3 name: %s, address: 0x%lx", info->detail->name, info->detail->address);
                add_event(ECStr("do_dlopen_arg_count"), (skAddress)3);
                add_event(ECStr("do_dlopen_symbol_name"), (char *)info->detail->name);
                add_event(ECStr("do_dlopen_symbol_address"), (skAddress)info->detail->address);
                skAddress do_dlopen_3_address = info->detail->address;
                hook_result = _SK_HOOK_FUNC(do_dlopen_3);
                add_event(ECStr("do_dlopen_hook_result"), hook_result ? (char *)"True" : (char *)"False");
                DOGE_LOGD("watch dog, hook do_dlopeon() 3, hook_result: %d", hook_result);
                break;
            }
            case 4:
            {
                DOGE_LOGD("4 name: %s, address: 0x%lx", info->detail->name, info->detail->address);
                add_event(ECStr("do_dlopen_arg_count"), (skAddress)4);
                add_event(ECStr("do_dlopen_symbol_name"), (char *)info->detail->name);
                add_event(ECStr("do_dlopen_symbol_address"), (skAddress)info->detail->address);
                skAddress do_dlopen_4_address = info->detail->address;
                hook_result = _SK_HOOK_FUNC(do_dlopen_4);
                add_event(ECStr("do_dlopen_hook_result"), hook_result ? (char *)"True" : (char *)"False");
                DOGE_LOGD("watch dog, hook do_dlopeon() 4, hook_result: %d", hook_result);
                break;
            }
            default:
            {
                add_event(ECStr("do_dlopen_arg_count"), (skAddress)0);
                add_event(ECStr("do_dlopen_symbol_name"), (char *)"NULL");
                add_event(ECStr("do_dlopen_symbol_address"), (skAddress)0);
                add_event(ECStr("do_dlopen_hook_result"), (char *)"False");
                LOGE("暂时没发现有任何 do_dlopen() 的参数个数是 > 5 或 < 2 的!");
                break;
            }
        }
        break;
    }
    for(int i = 0; i < sym_info.size(); ++i)
    {
        if (sym_info[i])
        {
            delete sym_info[i];
            sym_info[i] = NULL;
        }
    }
    return hook_result;
}

// android 10.0: template <typename F> static void call_array(const char* array_name __unused, F* functions, size_t arg_count, bool reverse, const char* realpath)
_SK_BUILD_REPLACE_FUNC(call_array, void, void * unused, unsigned long functions, size_t count, char * realpath)
{
    skDogeTaskInfo * task = (skDogeTaskInfo *)sk_get_watch_dog()->find_task(realpath, kTaskInit);
    if(task == NULL)
    {
        DOGE_LOGD("linker call init_array: [%s], func arg_count: %ld, func offset: 0x%lx", realpath, count, functions);
        return call_array_implement(unused, functions, count, realpath);
    }

#ifdef SK_DEBUG
#if SK_WATCH_DOG_DEBUG
    skModuleDetails * dt = sk_get_module_details_by_name(task->name);
    if(dt)
    {
        DOGE_LOGD("linker call init_array: [%s], func arg_count: %ld, func offset: 0x%lx", realpath, count, functions - dt->range->base_address);
    }
#endif
#endif

    task->notify_user();
    call_array_implement(unused, functions, count, realpath);
    task->notify_user(true);
}
static SK_BOOL found_init_array(const skSymbolDetails * detail, SK_UNUSED void * user_data)
{
    skWD_FoundSymbolContext * ctx = skcast(skWD_FoundSymbolContext *, user_data);

    if(strstr(detail->name, "call_array") != NULL)
    {
        ctx->address = detail->address;
        ctx->name = strdup(detail->name);
        return SK_FALSE;
    }
    return SK_TRUE;
}
bool skWatchDog::fuck_linker_init_array()
{
    if(sk_likely(find_task(kTaskInit) == NULL)) { return true; }

    if(this->linker_module == NULL) { this->linker_module = new skModule(linker_name); }
    skWD_FoundSymbolContext ctx = { .name = NULL, .address = 0 };
    this->linker_module->enum_symbol(found_init_array, &ctx);
    add_event(ECStr("linker_base_address"), this->linker_module->get_base_addr());
    add_event(ECStr("call_array_symbol_name"), ctx.name == NULL ? "NULL" : ctx.name);
    add_event(ECStr("call_array_symbol_address"), ctx.address);
    if(ctx.address != 0)
    {
        DOGE_LOGD("init_array find: base: 0x%lx, name: %s, address: 0x%lx", this->linker_module->get_base_addr(), ctx.name, ctx.address);
        skAddress call_array_address = ctx.address;
        return _SK_HOOK_FUNC(call_array);
    }
    return false;
}

// android 10.0: bool find_libraries(android_namespace_t* ns,
//                    soinfo* start_with,
//                    const char* const library_names[],
//                    size_t library_names_count,
//                    soinfo* soinfos[],
//                    std::vector<soinfo*>* ld_preloads,
//                    size_t ld_preloads_count,
//                    int rtld_flags,
//                    const android_dlextinfo* extinfo,
//                    bool add_as_children,
//                    std::vector<android_namespace_t*>* namespaces)
_SK_BUILD_REPLACE_FUNC(find_libraries, bool, void * ns, void* start_with, const char * const library_names[],
                       size_t library_names_count, void * soinfos, void * ld_preloads,
                       void * ld_preloads_count, int rtld_flags, void * extinfo,
                       bool add_as_children, bool search_linked_namespaces, void * namespaces)
{
    skDogeTaskInfo * task = NULL;
    //LOGD("find_libraries, library_names_count = %zu", library_names_count); // library_names_count 打印都是 1
    for (size_t i = 0; i < library_names_count; ++i)
    {
        const char * so_name = library_names[i];
        //LOGD("i = %zu, Loaded so: %s", i, so_name);
        task = (skDogeTaskInfo *)sk_get_watch_dog()->find_task(so_name, kTaskFind_Libraries);
    }
    bool ret = find_libraries_implement(ns, start_with, library_names, library_names_count,soinfos,
                                        ld_preloads,ld_preloads_count,rtld_flags,extinfo,
                                        add_as_children,search_linked_namespaces,namespaces);
    if(task != NULL) { task->notify_user(true); }
    return ret;
}
static SK_BOOL found_find_libraries(const skSymbolDetails * detail, SK_UNUSED void * user_data)
{
    skWD_FoundSymbolContext * ctx = skcast(skWD_FoundSymbolContext *, user_data);

    // pixel 4xl 有 6 个包含 find_libraries 的导出函数, 其他的干扰函数都有个 "_.__uniq." 字符串, 所以需要排除
    if(strstr(detail->name, "find_libraries") != NULL && strstr(detail->name, ".") == NULL)
    {
        ctx->address = detail->address;
        ctx->name = strdup(detail->name);
        return SK_FALSE;
    }
    return SK_TRUE;
}
bool skWatchDog::fuck_find_libraries()
{
    if(sk_likely(find_task(kTaskFind_Libraries) == NULL)) { return true; }

    if(this->linker_module == NULL) { this->linker_module = new skModule(linker_name); }
    skWD_FoundSymbolContext ctx = { .name = NULL, .address = 0 };
    this->linker_module->enum_symbol(found_find_libraries, &ctx);
    add_event(ECStr("linker_base_address"), this->linker_module->get_base_addr());
    add_event(ECStr("find_libraries_symbol_name"), ctx.name == NULL ? "NULL" : ctx.name);
    add_event(ECStr("find_libraries_symbol_address"), ctx.address);
    if(ctx.address != 0)
    {
        DOGE_LOGD("find_libraries find: base: 0x%lx, name: %s, address: 0x%lx", this->linker_module->get_base_addr(), ctx.name, ctx.address);
        skAddress find_libraries_address = ctx.address;
        return _SK_HOOK_FUNC(find_libraries);
    }
    return false;
}

void skWatchDog::fuck_map()
{
    if(sk_likely(find_task(kTaskNormal) == NULL)) { return; }
    std::thread([](skWatchDog * doge)->void { for(;;)
    {
        //pthread_mutex_lock(&doge->lock);  // no lock
        for(auto beg = doge->tasks->begin(); beg != doge->tasks->end(); ++beg)
        {
            skDogeTaskInfo * curr = skcast(skDogeTaskInfo *, *beg);
            if(sk_likely(curr->type != kTaskNormal || curr->is_call_normal || curr->scan_times >= curr->scan_limit)) { continue; }
            if(sk_likely(sk_module_details_is_load(curr->name))) { curr->notify_user(); }
            curr->scan_times = curr->scan_times + 1;
        }
        //pthread_mutex_unlock(&doge->lock);  // no lock
        usleep(WATCH_DOG_SLEEP_TIME * 1000);
    }}, this).detach();
}
bool skWatchDog::weak_up_the_doge(bool no_linker, bool no_init_array, bool no_linker_find_libraries)
{
    init_watch_dog_event_handle();

#if defined(__arm__)
    add_event(ECStr("arch"), ECStr("armeabi-v7a"));
#elif defined(__aarch64__)
    add_event(ECStr("arch"), ECStr("arm64-v8a"));
#endif
    if(!no_linker_find_libraries && !fuck_find_libraries()) { LOGE("watch dog, fuck find_libraries() failed!"); }
    if(!no_init_array && !fuck_linker_init_array()) { LOGE("watch dog, fuck init_array() failed!"); }
    if(sk_pkg_get_is_va() == SK_FALSE)
    {
        DOGE_LOGD("当前不是 VA 环境, doge 初始化所有的可能!");
        // VA 无法兼容 linker, init array 是可以, 但, 没有充足数据测试 Hook 的稳健性
        if(!no_linker && !fuck_linker_do_dlopen())  { LOGE("watch dog, fuck do_dlopeon() failed!"); return false; }
    }
    fuck_map();
    upload_watch_dog_event_msg();
    return true;
}



void * skWatchDog::find_task(const char * name, skLinkerTaskType type)
{
    if(name == NULL) { return NULL; }
    void * info = NULL;
    pthread_mutex_lock(&this->lock);
    auto it = find_if(this->tasks->begin(), this->tasks->end(), [&name, type](list<void *>::value_type & v) -> bool
    {
        skDogeTaskInfo * curr = skcast(skDogeTaskInfo *, v);
        if(curr->type != type) { return false; }
        return (strstr(name, curr->name) != NULL);
    });
    if(it != this->tasks->end()) { info = *it; }
    pthread_mutex_unlock(&this->lock);
    return info;
}
void * skWatchDog::find_task(skLinkerTaskType type)
{
    void * info = NULL;
    pthread_mutex_lock(&this->lock);
    auto it = find_if(this->tasks->begin(), this->tasks->end(), [type](list<void *>::value_type & v) -> bool
    {
        skDogeTaskInfo * curr = skcast(skDogeTaskInfo *, v);
        if(curr->type != type) { return false; }
        return true;
    });
    if(it != this->tasks->end()) { info = *it; }
    pthread_mutex_unlock(&this->lock);
    return info;
}


void skWatchDog::install_cb(const char * n, skNormal_cb cb, int32_t scan_limit)
{
    if(find_task(n, kTaskNormal) != NULL) return;
    skDogeTaskInfo * info = new skDogeTaskInfo(n, cb, scan_limit);
    pthread_mutex_lock(&this->lock);
    this->tasks->push_back(info);
    pthread_mutex_unlock(&this->lock);
}
void skWatchDog::install_init_cb(const char * n, skLinker_Init_onEnter cb1, skLinker_Init_onLeave cb2, bool one_shot)
{
    if(find_task(n, kTaskInit) != NULL) return;
    skDogeTaskInfo * info = new skDogeTaskInfo(n, cb1, cb2, one_shot);
    pthread_mutex_lock(&this->lock);
    this->tasks->push_back(info);
    pthread_mutex_unlock(&this->lock);
}
void skWatchDog::install_linker_cb(const char * n, skLinker_onEnter cb1, skLinker_onLeave cb2, bool one_shot)
{
    if(sk_pkg_get_is_va() == SK_FALSE)
    {
        if(find_task(n, kTaskLinker) != NULL) { return; }
        skDogeTaskInfo * info = new skDogeTaskInfo(n, cb1, cb2, one_shot);
        pthread_mutex_lock(&this->lock);
        this->tasks->push_back(info);
        pthread_mutex_unlock(&this->lock);
    }
    else
    {
        DOGE_LOGD("当前是 VA 环境, 把 hook linker 改为 maps scanner!");

        // VA 无法兼容 linker, init_array[x] 是可以, 但, 没有充足数据测试 Hook 的稳健性
        this->install_cb(n, cb2, (1000/WATCH_DOG_SLEEP_TIME)*300); // 扫 300 秒
    }
}
void skWatchDog::install_linker_find_libraries_cb(const char * n, skLinker_Find_Libraries_cb cb, bool one_shot_find_libraries)
{
    if(find_task(n, kTaskFind_Libraries) != NULL) return;
    skDogeTaskInfo * info = new skDogeTaskInfo(n, cb, one_shot_find_libraries);
    pthread_mutex_lock(&this->lock);
    this->tasks->push_back(info);
    pthread_mutex_unlock(&this->lock);
}

#ifdef __cplusplus
#undef NULL
#define NULL ((void *)0)
#endif