/*!
 * \file
 * \ingroup video
 * \brief OpenGL fsaa value functions
 */
#ifndef __FSAA_H__
#define __FSAA_H__

#ifdef __cplusplus
extern "C" {
#endif
#include "../misc.h"

extern unsigned int fsaa; /*!< flag that inidicates what level of fsaa to use */

void init_fsaa_modes();
int get_fsaa_mode_count();
int get_fsaa_mode(int i);
cstr get_fsaa_mode_str(int i);

#ifdef __cplusplus
} // extern "C"
#endif

#endif	/* __FSAA_H__ */

