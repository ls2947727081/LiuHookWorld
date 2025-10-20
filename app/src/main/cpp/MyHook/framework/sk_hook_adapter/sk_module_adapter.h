/*
    About Author(s):
    Name: shuke
    Mail: shukebeita1126@gmail.com
    File: sk_module.h
    Data: 2023-08-24 11:37
*/

#ifndef FRAMEWORK_SK_HOOK_ADAPTER_SK_MODULE_H
#define FRAMEWORK_SK_HOOK_ADAPTER_SK_MODULE_H

#include "sk_def/sk_def.h"
#include "sk_utils/sk_module_detail.h"
#include "sk_utils/sk_elf_helper.h"

struct skTextSectionInfo
{
    skAddress sz;
    skAddress start_off;
};

class skModule : skNonCopyable
{
public:
    skModule(const s8 * name);
    ~skModule();

    const char * get_name();
    const char * get_path();
    skELF * get_sk_elf();
    skAddress get_base_addr();
    const skModuleDetails * get_module_details();

    skAddress find_import(const s8 * sym_name);
    skAddress find_export(const s8 * sym_name);
    skAddress find_symbol(const s8 * sym_name);

    void enum_import(skFoundImportFunc func, void * user_data);
    void enum_export(skFoundExportFunc func, void * user_data);
    void enum_symbol(skFoundSymbolFunc func, void * user_data);

private:
    char * _name;
    void * _hd;
    skELF * _elf;
    skModuleDetails * _dt;
};

#endif // FRAMEWORK_SK_HOOK_ADAPTER_SK_MODULE_H