#ifndef IS_MAIN
#include "base.c"
#include <stdio.h>
#endif

// #define PROFILE

#ifdef PROFILE

#define MAX_PROFILE_POINTS 50

#define assert_max_profile_points                                              \
  static_assert(__COUNTER__ < MAX_PROFILE_POINTS,                              \
                concat(can_not_have_more_than_, MAX_PROFILE_POINTS))

typedef struct {
  u64 start;
  u64 elapsed_inclusive;
  const char *profile_name;
  u16 profile_anchor;
  u16 parent_anchor;
} Profile_Start;

typedef struct {
  u64 elapsed_inclusive;
  u64 elapsed_exclusive;
  const char *profile_name;
  u32 hits;
} Profile_Anchor;

u16 current_anchor = __COUNTER__;
Profile_Anchor profile_anchors[MAX_PROFILE_POINTS];

void end_profile_block(Profile_Start *input) {
  Profile_Anchor *anchor = &profile_anchors[input->profile_anchor];
  Profile_Anchor *parent_anchor = &profile_anchors[input->parent_anchor];
  u64 elapsed = __rdtsc() - input->start;
  anchor->profile_name = input->profile_name;
  anchor->elapsed_inclusive = input->elapsed_inclusive + elapsed;
  anchor->elapsed_exclusive += elapsed;
  parent_anchor->elapsed_exclusive -= elapsed;
  anchor->hits += 1;

  current_anchor = input->parent_anchor;
}

void end_profile_and_print(u64 program_start) {
  u64 program_elapsed = __rdtsc() - program_start;
  u64 cpu_freq = get_cpu_freq(1000);

  printf("Total time: %.4fms (CPU freq %ld)\n",
         (f64)program_elapsed / (f64)cpu_freq * MS_PER_SEC, cpu_freq);
  for (u8 i = 1; i < MAX_PROFILE_POINTS; i++) {
    Profile_Anchor profile_anchor = profile_anchors[i];
    if (profile_anchor.elapsed_exclusive) {
      printf("  %s[%d]: %lu (%.2f%%", profile_anchor.profile_name,
             profile_anchor.hits, profile_anchor.elapsed_exclusive,
             (f64)profile_anchor.elapsed_exclusive / program_elapsed * 100);
      if (profile_anchor.elapsed_inclusive !=
          profile_anchor.elapsed_exclusive) {
        printf(", %.2f%% w/childrend",
               (f64)profile_anchor.elapsed_inclusive / program_elapsed * 100);
      }
      printf(")\n");
    }
  }
}

#define profile_block(name)                                                    \
  u16 parent_anchor = current_anchor;                                          \
  current_anchor = __COUNTER__;                                                \
  __attribute__((cleanup(end_profile_block))) Profile_Start concat(            \
      profile_block_start, __LINE__) = {                                       \
      .start = __rdtsc(),                                                      \
      .profile_name = name,                                                    \
      .elapsed_inclusive = profile_anchors[current_anchor].elapsed_inclusive,  \
      .profile_anchor = current_anchor,                                        \
      .parent_anchor = parent_anchor,                                          \
  };

#define profile_function profile_block(__func__)

#else

#define assert_max_profile_points

#define profile_block(...)

#define profile_function

void end_profile_and_print(u64 program_start) {
  u64 program_elapsed = __rdtsc() - program_start;
  u64 cpu_freq = get_cpu_freq(1000);

  printf("Total time: %.4fms (CPU freq %ld)\n",
         (f64)program_elapsed / (f64)cpu_freq * MS_PER_SEC, cpu_freq);
}

#endif

#define profile_begin u64 program_start = __rdtsc()

#define profile_end end_profile_and_print(program_start)
