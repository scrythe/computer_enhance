#include <math.h>
typedef double f64;
#include "../../computer_enhance/perfaware/part2/listing_0065_haversine_formula.cpp"

extern "C" f64 Haversine_ReferenceHaversine(f64 X0, f64 Y0, f64 X1, f64 Y1,
                                            f64 EarthRadius) {
  return ReferenceHaversine(X0, Y0, X1, Y1, EarthRadius);
}
