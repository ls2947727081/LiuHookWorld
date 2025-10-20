#include <algorithm>
#include <functional>
#include <list>

#include "sk_def/sk_def.h"
#include "sk_log/sk_log.h"
#include "sk_adapter.h"
#include "sk_utils/sk_hash.h"

#if defined(__arm__)
#include "substrate/CydiaSubstrate.h"
#define SK_ANCIENT_HOOK_FUNC(x,y,z) MSHookFunction((void *)x,(void *)y,(void **)z)
#elif defined(__aarch64__)
#include "and64/And64InlineHook.hpp"
#define SK_ANCIENT_HOOK_FUNC(x,y,z) A64HookFunction((void *)x,(void *)y,(void **)z)
#endif

using namespace std;

extern pthread_mutex_t g_main_lock;
class skHookInfo
{
public:
    void * stub;  // TODO: New Feture, 现在并没有实现, 时间不够
    uint32_t hash;
    skAddress address;
};
static std::list<skHookInfo *> sk_hooks;
skHookAdapter * skHookAdapter::ins = NULL;

static bool dft_inline_hook(skAddress address, void *cb, void **ori_cb, unsigned long SK_UNUSED data)
{
    *(unsigned long *)ori_cb = 0;
    SK_ANCIENT_HOOK_FUNC(address, cb, ori_cb);
#ifdef SK_DEBUG
    if(*(unsigned long *)ori_cb == 0) { LOGE("Ancient " "skHook Address: 0x%lX Error, Msg: %s", address, "Unkonw"); }
#endif
    return (*(unsigned long *)ori_cb);
}
SK_DEFINE_ADAPTER_FUNC(bool, sk_hook_engine_init) { return true; }
SK_DEFINE_ADAPTER_FUNC(bool, sk_inline_hook, skAddress address, void * cb, void ** ori_cb, unsigned long data){ return false; }

skHookAdapter * sk_get_hook_adapter()
{
    if(skHookAdapter::ins == NULL)
    {
        pthread_mutex_lock(&g_main_lock);
        if(skHookAdapter::ins == NULL)
        {
            skHookAdapter::ins = new skHookAdapter();
            if(sk_unlikely(sk_hook_engine_init() == false)) { LOGE("Hook Engine Init Failed, Connect shuke!"); goto INIT_ERROR; }
        }
        pthread_mutex_unlock(&g_main_lock);
    }
    return skHookAdapter::ins;

INIT_ERROR:
    pthread_mutex_unlock(&g_main_lock);
    return NULL;
}
bool skHookAdapter::inline_hook_func_address(skAddress address, void * cb, void ** ori_cb, bool ignore_dft)
{
    uint32_t hash;
    skHookInfo * hook_info = NULL;
    if(address == 0 || cb == NULL || ori_cb == NULL) { LOGE("参数不合法 address: 0x%lx, cb: %p, ori_cb: %p!", address, cb, ori_cb); goto HOOK_FAILED; }

    hash = sk_hash_hook_hash("%lu%lu%lu", address, cb, ori_cb);
    if(find_if(sk_hooks.begin(), sk_hooks.end(), [=](list<skHookInfo *>::value_type & v)->bool
    { return (v->hash == hash); }) != sk_hooks.end()) { LOGE("重复 Hook"); goto HOOK_FAILED; }

    hook_info = new skHookInfo { NULL, hash, address };
    if(sk_inline_hook(address, cb, ori_cb, false)) { goto HOOK_SUCCEED; }
    if(!ignore_dft && dft_inline_hook(address, cb, ori_cb, 0)) { goto HOOK_SUCCEED; }

HOOK_FAILED:
    SK_DELETE_OBJ_CHECK(hook_info);
    LOGE("Hook 0x%lx 失败了!", address);
    return false;

HOOK_SUCCEED:
    pthread_mutex_lock(&g_main_lock);
    sk_hooks.push_back(hook_info);
    pthread_mutex_unlock(&g_main_lock);
    return true;
}