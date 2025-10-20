/*
    About Author(s):
    Name: shuke
    Mail: shukebeita1126@gmail.com
    File: sk_watch_dog.h
    Data: 2023-08-29 14:47
*/

#ifndef FRAMEWORK_SK_HOOK_ADAPTER_SK_WATCH_DOG_H
#define FRAMEWORK_SK_HOOK_ADAPTER_SK_WATCH_DOG_H

#include <pthread.h>
#include <list>
#include "sk_def/sk_def.h"
#include "sk_module_adapter.h"

constexpr int32_t SCAN_SECOND = 60;  // 扫 60 秒, 如果游戏有更新可能会超时
constexpr int32_t WATCH_DOG_SLEEP_TIME = 5;
constexpr int32_t DEFAULT_MAX_SCAN_LIMIT = ((1000/WATCH_DOG_SLEEP_TIME)*SCAN_SECOND);

class skWatchDog;

using skLinker_onEnter           = void (*)();
using skLinker_onLeave           = void (*)(skModule *);
using skLinker_Find_Libraries_cb = void (*)(skModule *);
using skLinker_Init_onEnter      = void (*)(skModule *);
using skLinker_Init_onLeave      = void (*)(skModule *);
using skNormal_cb                = void (*)(skModule *);

enum skLinkerTaskType : int32_t { kTaskInit = 0, kTaskLinker = 1, kTaskNormal = 2, kTaskFind_Libraries = 3 };

skWatchDog * sk_get_watch_dog();
class skWatchDog : skNonCopyable
{
public:
    void * find_task(const char * name, skLinkerTaskType type);

    bool weak_up_the_doge(bool no_linker, bool no_init_array, bool no_linker_find_libraries);

    void install_cb(const char * n, skNormal_cb cb, int32_t scan_limit = DEFAULT_MAX_SCAN_LIMIT);
    void install_init_cb(const char * n, skLinker_Init_onEnter cb1, skLinker_Init_onLeave cb2, bool one_shot = true);  // 不推荐使用
    void install_linker_cb(const char * n, skLinker_onEnter cb1, skLinker_onLeave cb2, bool one_shot = true);
    void install_linker_find_libraries_cb(const char * n, skLinker_Find_Libraries_cb cb, bool one_shot_find_libraries = true);

private:
    skModule * linker_module;
    pthread_mutex_t lock;
    std::list<void *> * tasks;

    void fuck_map();
    bool fuck_linker_do_dlopen();
    bool fuck_linker_init_array();
    bool fuck_find_libraries();
    void * find_task(skLinkerTaskType type);

    friend skWatchDog * sk_get_watch_dog();
    skWatchDog() = default;
    ~skWatchDog() = default;
    static skWatchDog * ins;
};

#endif // FRAMEWORK_SK_HOOK_ADAPTER_SK_WATCH_DOG_H