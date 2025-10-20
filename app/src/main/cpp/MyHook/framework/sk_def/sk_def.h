/*
    About Author(s):
    Name: shuke
    Mail: shukebeita1126@gmail.com
    File: sk_def.h
    Data: 2023-04-09 15:35
*/

#ifndef SK_COMMON_SK_DEF_SK_DEF_H
#define SK_COMMON_SK_DEF_SK_DEF_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>

// arm32: sizeof(unsigned): 4, sizeof(unsigned long): 4, sizeof(long): 4, sizeof(long long): 8
// arm64: sizeof(unsigned): 4, sizeof(unsigned long): 8, sizeof(long): 8, sizeof(long long): 8

#define SK_FLAG          uint8_t
#define SK_BOOL          uint8_t
#define SK_TRUE          1
#define SK_FALSE         0
#define SK_OK            0
#define SK_ERR          -1

typedef char               s8;
typedef unsigned char      u8;
typedef short              s16;
typedef unsigned short     u16;
typedef int                s32;
typedef unsigned int       u32;
typedef long long          s64;
typedef unsigned long long u64;
typedef unsigned int       skInstrucation;
typedef unsigned long *    _vfptr;
typedef unsigned long *    _vbptr;
typedef unsigned long      sk_size;
typedef signed long        sk_ssize;
typedef unsigned long      skAddress;
typedef unsigned long      skOffset;
typedef unsigned int       skHashKey;
typedef void *             skPtr;
typedef unsigned int       skBitSec;
#define SK_INVALID_HASH    (skHashKey)(0xFFFFFFFF)
#define SK_IS64Bit         (sizeof(void *) == 8)
#define SK_POINTER_SZ      (sizeof(void *))
#define SK_1KB             (1024)
#define SK_1MB             (SK_1KB * 1024)
#define SK_1GB             (SK_1MB * 1024)

#define SK_ADDRESS(a) ((skAddress)(uintptr_t) (a))

#define SK_ANDROID_1$0   ( 1)
#define SK_ANDROID_1$1   ( 2)
#define SK_ANDROID_1$5   ( 3)
#define SK_ANDROID_1$6   ( 4)
#define SK_ANDROID_2$0   ( 5)
#define SK_ANDROID_2$0$1 ( 6)
#define SK_ANDROID_2$1   ( 7)
#define SK_ANDROID_2$2   ( 8)
#define SK_ANDROID_2$3   ( 9)
#define SK_ANDROID_2$3$3 (10)
#define SK_ANDROID_3$0   (11)
#define SK_ANDROID_3$1   (12)
#define SK_ANDROID_3$2   (13)
#define SK_ANDROID_4$0   (14)
#define SK_ANDROID_4$0$3 (15)
#define SK_ANDROID_4$1   (16)
#define SK_ANDROID_4$2   (17)
#define SK_ANDROID_4$3   (18)
#define SK_ANDROID_4$4   (19)
#define SK_ANDROID_4$4w  (20)
#define SK_ANDROID_5$0   (21)
#define SK_ANDROID_5$1   (22)
#define SK_ANDROID_6$0   (23)
#define SK_ANDROID_7$0   (24)
#define SK_ANDROID_7$1   (25)
#define SK_ANDROID_8$0   (26)
#define SK_ANDROID_8$1   (27)
#define SK_ANDROID_9$0   (28)
#define SK_ANDROID_10$0  (29)
#define SK_ANDROID_11$0  (30)
#define SK_ANDROID_12$0  (31)
#define SK_ANDROID_12$1  (32)
#define SK_ANDROID_13$0  (33)
#define SK_ANDROID_14$0  (34)

#define skcast(t, exp)       ((t)(exp))
#define skcast_s8(exp)       skcast(s8,(exp))
#define skcast_u8(exp)       skcast(u8,(exp))
#define skcast_pu8(exp)      skcast(u8*,(exp))
#define skcast_u32(exp)      skcast(u32,(exp))
#define skcast_pu32(exp)     skcast(u32*,(exp))
#define skconstcast(t, exp)  ((const t)(exp))
#define skccast_s8(exp)      skconstcast(s8,(exp))
#define skccast_u8(exp)      skconstcast(u8,(exp))
#define skccast_pu8(exp)     skconstcast(u8*,(exp))
#define skccast_u32(exp)     skconstcast(u32,(exp))
#define skccast_pu32(exp)    skconstcast(u32*,(exp))

#define sk_likely(x)         __builtin_expect(!!(x), 1)
#define sk_unlikely(x)       __builtin_expect(!!(x), 0)

#define SK_STATIC_ASSERT(expr)        static_assert(expr, "expr 表达式不符合预期!")
#define SK_GET_ARRAY_LEN(arr)         (sizeof(arr)/sizeof((arr)[0]))
#define SK_EXTERN_C                   extern "C"
#define KEEP_EXTERN_C                 SK_EXTERN_C
#define SK_UNUSED                     __attribute__((unused))
#define SK_WEAK                       __attribute__((weak))
#define SK_ALIGNED(x)                 __attribute__ ((__aligned__(x)))
#define SK_PACKED(x)                  __attribute__ ((__aligned__(x), __packed__))
#define SK_EXPORT                     __attribute__ ((visibility ("default")))
#define SK_ALWAYS_INLINE              inline __attribute__ ((always_inline))
#define SK_LOCAL                      static
#define SK_ALIGN_TYPE(val)            __attribute__ ((aligned(val)))
#define SK_ALIGN_OF(T)                __alignof__(T)
#define SK_ALIGN_FIELD(val)           SK_ALIGN_TYPE(val)

#define SK_US_SLEEP(x)                usleep(x)
#define SK_MS_SLEEP(x)                SK_US_SLEEP(x*1000)
#define SK_S_SLEEP(x)                 sleep(x)

#define SK_SIZEOF_STRUCT(x)           LOGE(#x ": 0x%x(%d)",sizeof(x),sizeof(x))
#define SK_CALC_ALIGNED_MEM(x)        (((skAddress)x)&-PAGE_SIZE)
#define SK_CALC_ALIGNED_PAGE(x,len)   (PAGE_SIZE * ((~(skAddress)(x) + (skAddress)(x) + len) / PAGE_SIZE + 1))

#define SK_DEFINE_FUNC_POINTER(f,r,...) r (*f)(__VA_ARGS__)
#define SK_IMPL_ADAPTER_FUNC(r,f,...)   SK_EXTERN_C r sk_##f(__VA_ARGS__)
#define SK_DEFINE_ADAPTER_FUNC(r,f,...) SK_EXTERN_C SK_WEAK r f(__VA_ARGS__)
#define SK_LOOP                         while(SK_TRUE)
#define SK_INTERFACE                    virtual
#define SK_IN_ARG
#define SK_OUT_ARG
#define SK_STMT_START                   do {
#define SK_STMT_END                     } while(0)

#define SK_COPY_C_STRING(dst, src)                  \
{                                                   \
    size_t _temp_sz = strlen((src)) + 1;            \
    dst = (char *)malloc(_temp_sz);                 \
    dst[_temp_sz - 1] = 0;                          \
    memcpy((void *)(dst), (void *)(src), _temp_sz); \
}

#define SK_MAX(a, b) (((a) > (b)) ? (a) : (b))
#define SK_MIN(a, b) (((a) < (b)) ? (a) : (b))
#define SK_ABS(a) (((a) < 0) ? -(a) : (a))
#define SK_PAGE_START(value, page_size) ((skAddress)(value) & ~ (skAddress)(page_size - 1))


// Need C11
#if defined(__ATOMIC_SEQ_CST)
#define sk_atomic_get_int(atomic)                                              \
({                                                                             \
    SK_STATIC_ASSERT(sizeof *(atomic) == sizeof (int32_t));                    \
    int32_t gaig_temp;                                                         \
    (void) (0 ? *(atomic) ^ *(atomic) : 1);                                    \
    __atomic_load ((int32_t *)(atomic), &gaig_temp, __ATOMIC_SEQ_CST);         \
    (int32_t) gaig_temp;                                                       \
})
#define sk_atomic_set_int(atomic, newval)                                      \
({                                                                             \
    SK_STATIC_ASSERT(sizeof *(atomic) == sizeof (int32_t));                    \
    int32_t gais_temp = (int32_t) (newval);                                    \
    (void) (0 ? *(atomic) ^ (newval) : 1);                                     \
    __atomic_store ((int32_t *)(atomic), &gais_temp, __ATOMIC_SEQ_CST);        \
})
#define sk_atomic_inc_int(atomic)                                              \
({                                                                             \
    SK_STATIC_ASSERT(sizeof *(atomic) == sizeof (int32_t));                    \
    (void) (0 ? *(atomic) ^ *(atomic) : 1);                                    \
    (void) __atomic_fetch_add ((atomic), 1, __ATOMIC_SEQ_CST);                 \
})
#define sk_atomic_dec_int(atomic)                                              \
({                                                                             \
    SK_STATIC_ASSERT(sizeof *(atomic) == sizeof (int32_t));                    \
    (void) (0 ? *(atomic) ^ *(atomic) : 1);                                    \
    (void) __atomic_fetch_sub ((atomic), 1, __ATOMIC_SEQ_CST);                 \
})
#define sk_atomic_exchange_int(atomic, newval)                                 \
({                                                                             \
    SK_STATIC_ASSERT(sizeof *(atomic) == sizeof (int32_t));                    \
    (void) (0 ? *(atomic) ^ (newval) : 1);                                     \
    (int32_t) __atomic_exchange_n ((atomic), (newval), __ATOMIC_SEQ_CST);      \
})
#define sk_atomic_add_int(atomic, val)                                         \
({                                                                             \
    SK_STATIC_ASSERT(sizeof *(atomic) == sizeof (int32_t));                    \
    (void) (0 ? *(atomic) ^ (val) : 1);                                        \
    (int32_t) __atomic_fetch_add ((atomic), (val), __ATOMIC_SEQ_CST);          \
})
#define sk_atomic_sub_int(atomic, val)                                         \
({                                                                             \
    SK_STATIC_ASSERT(sizeof *(atomic) == sizeof (int32_t));                    \
    (void) (0 ? *(atomic) ^ *(atomic) : 1);                                    \
    (int32_t) __atomic_fetch_sub ((atomic), (val), __ATOMIC_SEQ_CST);          \
})
#define sk_atomic_and_int(atomic, val)                                         \
({                                                                             \
    SK_STATIC_ASSERT(sizeof *(atomic) == sizeof (int32_t));                    \
    (void) (0 ? *(atomic) ^ (val) : 1);                                        \
    (uint32_t) __atomic_fetch_and ((atomic), (val), __ATOMIC_SEQ_CST);         \
})
#define sk_atomic_or_int(atomic, val)                                          \
({                                                                             \
    SK_STATIC_ASSERT(sizeof *(atomic) == sizeof (int32_t));                    \
    (void) (0 ? *(atomic) ^ (val) : 1);                                        \
    (uint32_t) __atomic_fetch_or ((atomic), (val), __ATOMIC_SEQ_CST);          \
})
#define sk_atomic_xor_int(atomic, val)                                         \
({                                                                             \
    SK_STATIC_ASSERT(sizeof *(atomic) == sizeof (int32_t));                    \
    (void) (0 ? *(atomic) ^ (val) : 1);                                        \
    (uint32_t) __atomic_fetch_xor ((atomic), (val), __ATOMIC_SEQ_CST);         \
})

#define sk_atomic_get_pointer(atomic)                                          \
({                                                                             \
    SK_STATIC_ASSERT(sizeof *(atomic) == sizeof (void *));                     \
    void * gapg_temp_newval;                                                   \
    void * *gapg_temp_atomic = (void * *)(atomic);                             \
    __atomic_load (gapg_temp_atomic, &gapg_temp_newval, __ATOMIC_SEQ_CST);     \
    gapg_temp_newval;                                                          \
})
#define sk_atomic_set_pointer(atomic, newval)                                  \
({                                                                             \
    SK_STATIC_ASSERT(sizeof *(atomic) == sizeof (void *));                     \
    void * *gaps_temp_atomic = (void * *)(atomic);                             \
    void * gaps_temp_newval = (void *)(newval);                                \
    (void) (0 ? (void *) *(atomic) : NULL);                                    \
    __atomic_store (gaps_temp_atomic, &gaps_temp_newval, __ATOMIC_SEQ_CST);    \
})
#define sk_atomic_exchange_pointer(atomic, newval)                             \
({                                                                             \
    SK_STATIC_ASSERT(sizeof *(atomic) == sizeof (void *));                     \
    (void) (0 ? (void *) *(atomic) : NULL);                                    \
    (void *) __atomic_exchange_n ((atomic), (newval), __ATOMIC_SEQ_CST);       \
})
#define sk_atomic_add_pointer(atomic, val)                                     \
({                                                                             \
    SK_STATIC_ASSERT(sizeof *(atomic) == sizeof (void *));                     \
    (void) (0 ? (void *) *(atomic) : NULL);                                    \
    (void) (0 ? (val) ^ (val) : 1);                                            \
    (sk_ssize) __atomic_fetch_add ((atomic), (val), __ATOMIC_SEQ_CST);         \
})
#define sk_atomic_and_pointer(atomic, val)                                     \
({                                                                             \
    sk_size *gapa_atomic = (sk_size *) (atomic);                               \
    SK_STATIC_ASSERT(sizeof *(atomic) == sizeof (void *));                     \
    SK_STATIC_ASSERT(sizeof *(atomic) == sizeof (sk_size));                    \
    (void) (0 ? (void *) *(atomic) : NULL);                                    \
    (void) (0 ? (val) ^ (val) : 1);                                            \
    (sk_size) __atomic_fetch_and (gapa_atomic, (val), __ATOMIC_SEQ_CST);       \
})
#define sk_atomic_or_pointer(atomic, val)                                      \
({                                                                             \
    sk_size *gapo_atomic = (sk_size *) (atomic);                               \
    SK_STATIC_ASSERT(sizeof *(atomic) == sizeof (void *));                     \
    SK_STATIC_ASSERT(sizeof *(atomic) == sizeof (sk_size));                    \
    (void) (0 ? (void *) *(atomic) : NULL);                                    \
    (void) (0 ? (val) ^ (val) : 1);                                            \
    (sk_size) __atomic_fetch_or (gapo_atomic, (val), __ATOMIC_SEQ_CST);        \
})
#define sk_atomic_xor_pointer(atomic, val)                                     \
({                                                                             \
    sk_size *gapx_atomic = (sk_size *) (atomic);                               \
    SK_STATIC_ASSERT(sizeof *(atomic) == sizeof (void *));                     \
    SK_STATIC_ASSERT(sizeof *(atomic) == sizeof (sk_size));                    \
    (void) (0 ? (void *) *(atomic) : NULL);                                    \
    (void) (0 ? (val) ^ (val) : 1);                                            \
    (sk_size) __atomic_fetch_xor (gapx_atomic, (val), __ATOMIC_SEQ_CST);       \
})

#define sk_atomic_def_value(c) static int32_t sk_atomic_##c = 0;               \
static int32_t sk_atomic_get_##c()                                             \
{ return sk_atomic_get_int(&sk_atomic_##c); }                                  \
static void sk_atomic_set_##c(int32_t v)                                       \
{ sk_atomic_set_int(&sk_atomic_##c, v); }                                      \
static void sk_atomic_inc_##c()                                                \
{ sk_atomic_inc_int(&sk_atomic_##c); }                                         \
static void sk_atomic_dec_##c()                                                \
{ sk_atomic_dec_int(&sk_atomic_##c); }
#else
#define sk_atomic_get_int(atomic)                                              \
({                                                                             \
    int32_t gaig_result;                                                       \
    SK_STATIC_ASSERT(sizeof *(atomic) == sizeof (int32_t));                    \
    (void) (0 ? *(atomic) ^ *(atomic) : 1);                                    \
    gaig_result = (int32_t) *(atomic);                                         \
    __sync_synchronize ();                                                     \
    __asm__ __volatile__ ("" : : : "memory");                                  \
    gaig_result;                                                               \
})
#define sk_atomic_set_int(atomic, newval)                                      \
({                                                                             \
    SK_STATIC_ASSERT(sizeof *(atomic) == sizeof (int32_t));                    \
    (void) (0 ? *(atomic) ^ (newval) : 1);                                     \
    __sync_synchronize ();                                                     \
    __asm__ __volatile__ ("" : : : "memory");                                  \
    *(atomic) = (newval);                                                      \
})
#define sk_atomic_inc_int(atomic)                                              \
({                                                                             \
    SK_STATIC_ASSERT(sizeof *(atomic) == sizeof (int32_t));                    \
    (void) (0 ? *(atomic) ^ *(atomic) : 1);                                    \
    (void) __sync_fetch_and_add ((atomic), 1);                                 \
})
#define sk_atomic_dec_int(atomic)                                              \
({                                                                             \
    SK_STATIC_ASSERT(sizeof *(atomic) == sizeof (int32_t));                    \
    (void) (0 ? *(atomic) ^ *(atomic) : 1);                                    \
    (void) __sync_fetch_and_sub ((atomic), 1);                                 \
})
#if defined(_GLIB_GCC_HAVE_SYNC_SWAP)
#define sk_atomic_exchange_int(atomic, newval)                                 \
({                                                                             \
    SK_STATIC_ASSERT(sizeof *(atomic) == sizeof (int32_t));                    \
    (void) (0 ? *(atomic) ^ (newval) : 1);                                     \
    (int32_t) __sync_swap ((atomic), (newval));                                \
})
#else
#define sk_atomic_exchange_int(atomic, newval)                                 \
({                                                                             \
    int32_t oldval;                                                            \
    SK_STATIC_ASSERT(sizeof *(atomic) == sizeof (int32_t));                    \
    (void) (0 ? *(atomic) ^ (newval) : 1);                                     \
    do                                                                         \
      {                                                                        \
        oldval = *atomic;                                                      \
      } while (!__sync_bool_compare_and_swap (atomic, oldval, newval));        \
    oldval;                                                                    \
})
#endif
#define sk_atomic_add_int(atomic, val)                                         \
({                                                                             \
    SK_STATIC_ASSERT(sizeof *(atomic) == sizeof (int32_t));                    \
    (void) (0 ? *(atomic) ^ (val) : 1);                                        \
    (int32_t) __sync_fetch_and_add ((atomic), (val));                          \
})
#define sk_atomic_sub_int(atomic, val)                                         \
({                                                                             \
    SK_STATIC_ASSERT(sizeof *(atomic) == sizeof (int32_t));                    \
    (void) (0 ? *(atomic) ^ *(atomic) : 1);                                    \
    (int32_t) __sync_fetch_and_sub ((atomic), (val));                          \
})
#define sk_atomic_and_int(atomic, val)                                         \
({                                                                             \
    SK_STATIC_ASSERT(sizeof *(atomic) == sizeof (int32_t));                    \
    (void) (0 ? *(atomic) ^ (val) : 1);                                        \
    (uint32_t) __sync_fetch_and_and ((atomic), (val));                         \
})
#define sk_atomic_or_int(atomic, val)                                          \
({                                                                             \
    SK_STATIC_ASSERT(sizeof *(atomic) == sizeof (int32_t));                    \
    (void) (0 ? *(atomic) ^ (val) : 1);                                        \
    (uint32_t) __sync_fetch_and_or ((atomic), (val));                          \
})
#define sk_atomic_xor_int(atomic, val)                                         \
({                                                                             \
    SK_STATIC_ASSERT(sizeof *(atomic) == sizeof (int32_t));                    \
    (void) (0 ? *(atomic) ^ (val) : 1);                                        \
    (uint32_t) __sync_fetch_and_xor ((atomic), (val));                         \
})

#define sk_atomic_get_pointer(atomic)                                          \
({                                                                             \
    void * gapg_result;                                                        \
    SK_STATIC_ASSERT(sizeof *(atomic) == sizeof (void *));                     \
    gapg_result = (void *) *(atomic);                                          \
    __sync_synchronize ();                                                     \
    __asm__ __volatile__ ("" : : : "memory");                                  \
    gapg_result;                                                               \
})
#define sk_atomic_set_pointer(atomic, newval)                                  \
({                                                                             \
    SK_STATIC_ASSERT(sizeof *(atomic) == sizeof (void *));                     \
    (void) (0 ? (void *) *(atomic) : NULL);                                    \
    __sync_synchronize ();                                                     \
    __asm__ __volatile__ ("" : : : "memory");                                  \
    *(atomic) = (void *) (gsize) (newval);                                     \
})
#if defined(_GLIB_GCC_HAVE_SYNC_SWAP)
#define sk_atomic_exchange_pointer(atomic, newval)                             \
({                                                                             \
    SK_STATIC_ASSERT(sizeof *(atomic) == sizeof (void *));                     \
    (void) (0 ? (void *) *(atomic) : NULL);                                    \
    (void *) __sync_swap ((atomic), (newval));                                 \
})
#else
#define sk_atomic_exchange_pointer(atomic, newval)                             \
({                                                                             \
    void * oldval;                                                             \
    SK_STATIC_ASSERT(sizeof *(atomic) == sizeof (void *));                     \
    (void) (0 ? (void *) *(atomic) : NULL);                                    \
    do                                                                         \
      {                                                                        \
        oldval = (void *) *atomic;                                             \
      } while (!__sync_bool_compare_and_swap (atomic, oldval, newval));        \
    oldval;                                                                    \
})
#endif
#define sk_atomic_add_pointer(atomic, val)                                     \
({                                                                             \
    SK_STATIC_ASSERT(sizeof *(atomic) == sizeof (void *));                     \
    (void) (0 ? (void *) *(atomic) : NULL);                                    \
    (void) (0 ? (val) ^ (val) : 1);                                            \
    (sk_ssize) __sync_fetch_and_add ((atomic), (val));                         \
})
#define sk_atomic_and_pointer(atomic, val)                                     \
({                                                                             \
    SK_STATIC_ASSERT(sizeof *(atomic) == sizeof (void *));                     \
    (void) (0 ? (void *) *(atomic) : NULL);                                    \
    (void) (0 ? (val) ^ (val) : 1);                                            \
    (sk_size) __sync_fetch_and_and ((atomic), (val));                          \
})
#define sk_atomic_or_pointer(atomic, val)                                      \
({                                                                             \
    SK_STATIC_ASSERT(sizeof *(atomic) == sizeof (void *));                     \
    (void) (0 ? (void *) *(atomic) : NULL);                                    \
    (void) (0 ? (val) ^ (val) : 1);                                            \
    (sk_size) __sync_fetch_and_or ((atomic), (val));                           \
})
#define sk_atomic_xor_pointer(atomic, val)                                     \
({                                                                             \
    SK_STATIC_ASSERT(sizeof *(atomic) == sizeof (void *));                     \
    (void) (0 ? (void *) *(atomic) : NULL);                                    \
    (void) (0 ? (val) ^ (val) : 1);                                            \
    (sk_size) __sync_fetch_and_xor ((atomic), (val));                          \
})

#define sk_atomic_def_value(c) static int32_t sk_atomic_##c = 0;               \
static int32_t sk_atomic_get_##c()                                             \
{ return sk_atomic_get_int(&sk_atomic_##c); }                                  \
static void sk_atomic_set_##c(int32_t v)                                       \
{ sk_atomic_set_int(&sk_atomic_##c, v); }                                      \
static void sk_atomic_inc_##c()                                                \
{ sk_atomic_inc_int(&sk_atomic_##c); }
#endif

#ifdef SK_DEBUG
#define SK_CALC_RUN_TIME_START \
struct timespec $start_time = { 0 }, $end_time = { 0 }; \
clock_gettime(CLOCK_MONOTONIC, &($start_time));
#define SK_CALC_RUN_TIME_FINISH \
clock_gettime(CLOCK_MONOTONIC, &$end_time); \
double dt = (double)($end_time.tv_sec - $start_time.tv_sec) * 1000.0 + (double)($end_time.tv_nsec - $start_time.tv_nsec) / 1000000.0; \
LOGW_NO_LINE("执行时间: %lf ms!", dt);
#endif

#ifdef __cplusplus
struct skNonCopyable
{
    skNonCopyable() = default;
    skNonCopyable(const skNonCopyable&) = delete;
    skNonCopyable & operator=(const skNonCopyable&) = delete;
};
class skNonCreatable
{
    skNonCreatable() = delete;
    skNonCreatable(const skNonCreatable&) = delete;
    skNonCreatable & operator=(const skNonCreatable&) = delete;
};
class skSingleton
{
protected:
    skSingleton() = default;
private:
    skSingleton(const skSingleton&) = delete;
    skSingleton & operator=(const skSingleton&) = delete;
};
#define SK_DELETE_OBJ_CHECK(x)  \
if(x)                           \
{                               \
    delete x;                   \
    x = NULL;                   \
}
#endif

#define SK_FREE_POINTER_CHECK(x)  \
if(x)                             \
{                                 \
    free(x);                      \
    x = NULL;                     \
}
#define SK_CHECK_POINTER_BREAK(x) \
if(NULL == (x))                   \
{                                 \
    break;                        \
}
#define SK_CHECK_POINTER_CONTINUE(x) \
if(NULL==(x))                        \
{                                    \
    continue;                        \
}
#define SK_CHECK_POINTER_RETURN_VALUE(x, y, z) \
if(NULL == (x))                                \
{                                              \
    LOGE(z);                                   \
    return y;                                  \
}

// 使用例子:
// class A
// {
//     void * other;
//     std::unique_ptr<C> B;
// };
// 获取 unique_ptr<C> B 成员变量的真实指针
// unsigned long B = sk_get_unique_ptr$__value(A_ins, sizeof(void *));
#define sk_get_unique_ptr$__value(ins, unique_ptr_offset) (*skcast(unsigned long *, skcast(char *, ins) + unique_ptr_offset))

// Mirror Unity
#define SK_TYPEDEF(x, y) typedef x y;
#define SK_TYPTDEF_C_ENUM(x,y) typedef enum x y;
#define SK_TYPTDEF_C_STRUCT(x,y) typedef struct x y;
#define SK_DEFINE_C_STRUCT(x) typedef struct x
#define SK_NEW_C_STRUCT(x,y)      \
x*y=skcast(x*,malloc(sizeof(x))); \
memset(y,0,sizeof(x));
#define SK_DELETE_C_OBJ(x) free(x)

#define SK_TYPEDEF_FAKE_FRIDA_STRUCT(x) \
    struct _##x;                        \
    typedef struct _##x x;

// Mirror Android
#define CTOR_FUNC
#define MEMBER_FUNC
#define STATIC_FUNC
#define MEMBER_FIELD
#define STATIC_FIELD
#define SK_FINAL
#define SK_Nullable


#endif  // SK_COMMON_SK_DEF_SK_DEF_H