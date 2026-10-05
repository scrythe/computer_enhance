#include <stdint.h>

typedef uint8_t u8;
typedef int32_t i32;
typedef uint32_t u32;
typedef uint64_t u64;

typedef float f32;
typedef double f64;

typedef struct {
  f64 x0;
  f64 y0;
  f64 x1;
  f64 y1;
} HaversineData;

typedef struct {
  HaversineData *ptr;
  u32 len;
} HaversineDataSlice;
