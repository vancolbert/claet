/*!
 * \file
 * \ingroup init
 * \brief global include file
 */
#ifndef __GLOBAL_H__
#define __GLOBAL_H__

#include <SDL_types.h>
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;
typedef float f32;
typedef double f64;
typedef const char *cstr;
#define countof(a) (sizeof(a)/sizeof(*a))
#define clamp(a, v, b) (v < a ? a : (v > b ? b : v))
#ifdef __cplusplus
extern "C" {
#endif

extern Uint32 cur_time, last_time; /*!< timestamps to check whether we need to resync */

#ifdef SKY_FPV_OPTIONAL
#define font_scale 10.0f
#endif // SKY_FPV_OPTIONAL

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __GLOBAL_H__ */
