#define GLOBAL     static
#define INTERNAL   static
#define PERSISTENT static

typedef int8_t   s8;
typedef int16_t  s16;
typedef int32_t  s32;
typedef int64_t  s64;

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

#ifdef __SIZEOF_INT128__
typedef __int128_t s128;
typedef __uint128_t u128;
#endif

typedef bool     b8;
typedef u32      b32;

typedef float    f32;
typedef double   f64;

#define S8_MIN  SCHAR_MIN
#define S8_MAX  SCHAR_MAX
#define S16_MIN SHRT_MIN
#define S16_MAX SHRT_MAX
#define S32_MIN INT_MIN
#define S32_MAX INT_MAX
#define S64_MIN LLONG_MIN
#define S64_MAX LLONG_MAX

#define U8_MAX  UCHAR_MAX
#define U16_MAX USHRT_MAX
#define U32_MAX UINT_MAX
#define U64_MAX ULLONG_MAX

#ifdef DEVELOPMENT
#if defined(__clang__) || defined(__GNUC__)
#define DEBUG_BREAK __builtin_trap()
#elif defined(MSVC) // NOTE(leo): MSVC must be defined as compiler option (-D) or before this file.
#define DEBUG_BREAK __debugbreak()
#else
#define DEBUG_BREAK *(int *)NULL = 0
#endif
#define ASSERT(expression) if(expression) {} else { DEBUG_BREAK; }
#define INVALID_CODE_PATH DEBUG_BREAK
#define ASSERT_FUNCTION(function_call) ASSERT(function_call)
#define ASSERT_FUNCTION_COMPARE(function_call, comparison_operator, value_to_compare) ASSERT(function_call comparison_operator value_to_compare)
#else
#define ASSERT(expression)
#define INVALID_CODE_PATH
#define ASSERT_FUNCTION(function_call) function_call
#define ASSERT_FUNCTION_COMPARE(function_call, comparison_operator, value_to_compare) function_call
#endif // DEVELOPMENT

#define CHECK_BIT(value, bit_index) (((value) >> bit_index) & 1)
#define ARRAY_COUNT(array) (sizeof((array))/sizeof((array)[0]))
