typedef struct {
  f64 x0;
  f64 y0;
  f64 x1;
  f64 y1;
} HaversineData;

typedef struct {
  f64 *ptr;
  u32 len;
} F64Slice;

typedef struct {
  HaversineData *ptr;
  u32 len;
} HaversineDataSlice;

typedef struct {
  HaversineData *ptr;
  u32 len;
  u32 capacity;
} HaversineDataVector;
