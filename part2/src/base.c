#include <stdint.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef int32_t i32;
typedef uint32_t u32;
typedef int64_t i64;
typedef uint64_t u64;

typedef float f32;
typedef double f64;

f64 Haversine_ReferenceHaversine(f64 X0, f64 Y0, f64 X1, f64 Y1,
                                 f64 EarthRadius);

#ifdef BUILD_DEBUG
#define assert(val)                                                            \
  if (!(val)) {                                                                \
    __builtin_trap();                                                          \
  }
#else
#define assert(val) (void)(val)
#endif

#define printf_error(message, ...)                                             \
  fprintf(stderr, "\033[91mError:\033[0m " message, ##__VA_ARGS__)

u64 parse_u64(char *string) {
  u64 value = 0;
  char *current_char = string;
  while (*current_char >= '0' && *current_char <= '9') {
    u8 integer = *current_char - '0';
    value = value * 10 + integer;
    current_char += 1;
  }
  return value;
}
