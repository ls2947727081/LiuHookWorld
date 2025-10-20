#include <unistd.h>

#include "sk_log/sk_log.h"
#include "sk_module_adapter.h"
#include "sk_utils/sk_elf_helper.h"
#include "sk_str_encrypt/str_encrypt.hpp"

SK_DEFINE_ADAPTER_FUNC(void *, sk_get_handle, const s8 * n, void ** p_dt) { return NULL; }
SK_DEFINE_ADAPTER_FUNC(skAddress, sk_find_import, void * h, const s8 * n) { return 0; }
SK_DEFINE_ADAPTER_FUNC(skAddress, sk_find_export, void * h, const s8 * n) { return 0; }
SK_DEFINE_ADAPTER_FUNC(skAddress, sk_find_symbol, void * h, const s8 * n) { return 0; }
SK_DEFINE_ADAPTER_FUNC(void, sk_enum_import, void * h, void * cb, void * user_data) { if(sk_likely(h)) { sk_elf_enum_import(skcast(skELF *, h), skcast(skFoundImportFunc, cb), user_data); } }
SK_DEFINE_ADAPTER_FUNC(void, sk_enum_export, void * h, void * cb, void * user_data) { if(sk_likely(h)) { sk_elf_enum_export(skcast(skELF *, h), skcast(skFoundExportFunc, cb), user_data); } }
SK_DEFINE_ADAPTER_FUNC(void, sk_enum_symbol, void * h, void * cb, void * user_data) { if(sk_likely(h)) { sk_elf_enum_symbol(skcast(skELF *, h), skcast(skFoundSymbolFunc, cb), user_data); } }
SK_DEFINE_ADAPTER_FUNC(void, sk_delete_module, void * h) { }

skModule::skModule(const s8 * name) : skNonCopyable()
{
    //LOGD("skModule New, name: %s", name);
    _dt = NULL;
    _name = strdup(name);
    _hd   = sk_get_handle(name, (void **)&_dt);
    _elf  = sk_elf_new(name);
    if(sk_unlikely(_dt == NULL && _elf)) { _dt = sk_module_details_copy(_elf->module_details); }
    //LOGD("skModule New, _dt: %p, _name: %p, _hd: %p, _elf: %p", _dt, _name, _hd, _elf);
}
skModule::~skModule()
{
    sk_delete_module(_hd); _hd = NULL;
    sk_elf_delete(_elf); _elf = NULL;
    sk_module_details_delete(_dt); _dt = NULL;
    SK_FREE_POINTER_CHECK(_name);
}

skAddress skModule::find_import(const s8 * sym_name)
{
    skAddress address = 0;
    if(sk_likely(_hd != NULL))
    {
        address = sk_find_import(_hd, sym_name);
        if(address != 0) { goto FINISH; }
    }
    if(sk_likely(_elf != NULL)) { address = sk_elf_find_import(_elf, sym_name); }

    FINISH:
    return address;
}
skAddress skModule::find_export(const s8 * sym_name)
{
    skAddress address = 0;
    if(sk_likely(_hd != NULL))
    {
        address = sk_find_export(_hd, sym_name);
        if(address != 0) { goto FINISH; }
    }
    if(sk_likely(_elf != NULL)) { address = sk_elf_find_export(_elf, sym_name); }

    FINISH:
    return address;
}
skAddress skModule::find_symbol(const s8 * sym_name)
{
    skAddress address = 0;
    if(sk_likely(_hd != NULL))
    {
        address = sk_find_symbol(_hd, sym_name);
        if(address != 0) { goto FINISH; }
    }
    if(sk_likely(_elf != NULL)) { address = sk_elf_find_symbol(_elf, sym_name); }

FINISH:
    return address;
}

// enum_xxx() 不做兜底方案
void skModule::enum_import(skFoundImportFunc func, void * user_data)
{
    if(sk_unlikely(func == NULL)) { return; }
    sk_enum_import((void *)_elf, (void *)func, user_data);
}
void skModule::enum_export(skFoundExportFunc func, void * user_data)
{
    if(sk_unlikely(func == NULL)) { return; }
    sk_enum_export((void *)_elf, (void *)func, user_data);
}
void skModule::enum_symbol(skFoundSymbolFunc func, void * user_data)
{
    if(sk_unlikely(func == NULL)) { return; }
    sk_enum_symbol((void *)_elf, (void *)func, user_data);
}

const char * skModule::get_name()
{
    if(sk_unlikely(_dt == NULL)) { LOGW("因为 skELF 解析失败, get_name() 无法使用!"); return NULL; }
    return _dt->name;
}
const char * skModule::get_path()
{
    if(sk_unlikely(_dt == NULL)) { LOGW("因为 skELF 解析失败, get_path() 无法使用!"); return NULL; }
    return _dt->path;
}
skAddress skModule::get_base_addr()
{
    if(sk_unlikely(_dt == NULL)) { LOGW("因为 skELF 解析失败, get_base_addr() 无法使用!"); return 0; }
    return _dt->range->base_address;
}
const skModuleDetails * skModule::get_module_details()
{
    if(sk_unlikely(_dt == NULL)) { LOGW("因为 skELF 解析失败, get_module_details() 无法使用!"); return NULL; }
    return _dt;
}
skELF * skModule::get_sk_elf()
{
    if(sk_unlikely(_dt == NULL)) { LOGW("因为 skELF 解析失败, get_sk_elf() 无法使用!"); return NULL; }
    return _elf;
}