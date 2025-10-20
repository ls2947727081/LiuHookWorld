#include "sk_def/sk_def.h"
#include "sk_utils/sk_elf_helper.h"

#if defined(__arm__)
#include "frida_gum/v16.1.4/headers/frida-gum-armeabi-v7a.h"
#elif defined(__aarch64__)
#include "frida_gum/v16.1.4/headers/frida-gum-arm64-v8a.h"
#endif

#define SK_HIDDEN_CODE SK##_IMPL##_ADAPTER##_FUNC

SK_TYPTDEF_C_STRUCT(_skFoundModuleContext, skFoundModuleContext)
SK_TYPTDEF_C_STRUCT(_skFoundImportContext, skFoundImportContext)
SK_TYPTDEF_C_STRUCT(_skFoundExportContext, skFoundExportContext)
SK_TYPTDEF_C_STRUCT(_skFoundSymbolContext, skFoundSymbolContext)
SK_TYPTDEF_C_STRUCT(_skFoundImportContext2, skFoundImportContext2)

struct _skFoundModuleContext
{
    const gchar * name;
    skModuleDetails ** p_dt;
};
struct _skFoundImportContext2
{
    const s8 * name;
    skAddress address;
};
struct _skFoundImportContext
{
    skFoundImportFunc func;
    void * user_data;
};
struct _skFoundExportContext
{
    skFoundExportFunc func;
    void * user_data;
};
struct _skFoundSymbolContext
{
    skFoundSymbolFunc func;
    void * user_data;
};

SK_LOCAL gboolean found_module_func(const GumModuleDetails * details, gpointer user_data)
{
    if(sk_unlikely(details == NULL || details->name == NULL)) { return SK_TRUE; }

    skFoundModuleContext * ctx = skcast(skFoundModuleContext *, user_data);
    if(strstr(details->path, ctx->name) != NULL)
    {
        skModuleDetails * dt = sk_module_details_new_1(details->name);
        sk_module_details_set_path(dt, details->path);
        sk_module_details_set_size(dt, details->range->size);
        sk_module_details_set_base(dt, details->range->base_address);
        *ctx->p_dt = dt;
        return SK_FALSE;
    }
    return SK_TRUE;
}
SK_LOCAL gboolean found_symbol_func(const GumSymbolDetails * details, gpointer user_data)
{
    if(sk_unlikely(details == NULL)) { return SK_TRUE; }
    skFoundSymbolContext * ctx = skcast(skFoundSymbolContext *, user_data);
    skSymbolDetails sk_details = {
            .is_global = (SK_BOOL)details->is_global,
            .type = (skSymbolType)details->type,
            .name = details->name,
            .address = (skAddress)details->address,
            .size = (size_t)details->size
    };
    return ctx->func(&sk_details, ctx->user_data);
}
SK_LOCAL gboolean found_export_func(const GumExportDetails * details, gpointer user_data)
{
    if(sk_unlikely(details == NULL)) { return SK_TRUE; }
    skFoundExportContext * ctx = skcast(skFoundExportContext *, user_data);
    skExportDetails sk_details = {
            .type = (skExportType)details->type,
            .name = details->name,
            .address = (skAddress)details->address
    };
    return ctx->func(&sk_details, ctx->user_data);
}
SK_LOCAL gboolean found_import_func(const GumImportDetails * details, gpointer user_data)
{
    if(sk_unlikely(details == NULL)) { return SK_TRUE; }
    skFoundImportContext * ctx = skcast(skFoundImportContext *, user_data);
    skImportDetails sk_details = {
        .type = (skImportType)details->type,
        .name = details->name,
        .address = (skAddress)details->address,
        .slot = (skAddress)details->slot
    };
    return ctx->func(&sk_details, ctx->user_data);
}
SK_LOCAL SK_BOOL found_import_func_2(const skImportDetails * details, void * user_data)
{
    if(details == NULL || details->name == NULL || details->address == 0) { return SK_TRUE; }
    skFoundImportContext2 * ctx = skcast(skFoundImportContext2 *, user_data);
    if(strcmp(ctx->name, details->name) == 0)
    {
        ctx->address = details->address;
        return SK_FALSE;
    }
    return SK_TRUE;
}
SK_HIDDEN_CODE(void *, get_handle, const s8 * n, void ** p_dt)
{
    skFoundModuleContext ctx = {
        .name = n,
        .p_dt = skcast(skModuleDetails **, p_dt)
    };
    gum_process_enumerate_modules(found_module_func, &ctx);
    return *ctx.p_dt;
}
SK_HIDDEN_CODE(void, enum_export, void * h, void * cb, void * user_data)
{
    if(sk_unlikely(h == NULL)) { return; }
    skFoundExportContext ctx = {
            .func = (skFoundExportFunc)cb,
            .user_data = user_data
    };
    gum_module_enumerate_exports(((skELF *)h)->module_details->name, found_export_func, &ctx);
}
SK_HIDDEN_CODE(void, enum_import, void * h, void * cb, void * user_data)
{
    if(sk_unlikely(h == NULL)) { return; }
    skFoundImportContext ctx = {
        .func = (skFoundImportFunc)cb,
        .user_data = user_data
    };
    gum_module_enumerate_imports(((skELF *)h)->module_details->name, found_import_func, &ctx);
}
SK_HIDDEN_CODE(void, enum_symbol, void * h, void * cb, void * user_data)
{
    if(sk_unlikely(h == NULL)) { return; }
    skFoundSymbolContext ctx = {
        .func = (skFoundSymbolFunc)cb,
        .user_data = user_data
    };
    gum_module_enumerate_symbols(((skELF *)h)->module_details->name, found_symbol_func, &ctx);
}
SK_HIDDEN_CODE(skAddress, find_import, void * h, const s8 * n)
{
    if(h == NULL) { return 0; }
    skFoundImportContext2 ctx = {
        .name = n,
        .address = 0
    };
    sk_enum_import(h, (void *)found_import_func_2, (void *)&ctx);
    return ctx.address;
}
SK_HIDDEN_CODE(skAddress, find_export, void * h, const s8 * n)
{
    if(h == NULL) { return 0; }
    skModuleDetails * dt = skcast(skModuleDetails *, h);
    return skcast(skAddress, gum_module_find_export_by_name(dt->name, n));
}
SK_HIDDEN_CODE(skAddress, find_symbol, void * h, const s8 * n)
{
    if(sk_unlikely(h == NULL)) { return 0; }
    skModuleDetails * dt = skcast(skModuleDetails *, h);
    return skcast(skAddress, gum_module_find_symbol_by_name(dt->name, n));
}
SK_HIDDEN_CODE(bool, hook_engine_init) { gum_init_embedded(); return true; }
SK_HIDDEN_CODE(bool, inline_hook, skAddress address, void * cb, void ** ori_cb, unsigned long data)
{
    GumInterceptor * ins = gum_interceptor_obtain();
    if(ins == NULL) { return false; }
    return (gum_interceptor_replace_fast(ins, (gpointer)address, cb, ori_cb) == GUM_REPLACE_OK);
}