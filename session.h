#ifndef __EXT_SESSION_H__
#define __EXT_SESSION_H__
#ifdef POPUP_AIDE_FR
#include <SDL_types.h>
#endif //POPUP_AIDE_FR

#ifdef __cplusplus
extern "C" {
#endif

extern int session_win;
extern int exp_log_threshold;
#ifdef POPUP_AIDE_FR
Uint32 fullsession_start_time;
#endif //POPUP_AIDE_FR
#ifdef FR_VERSION
extern int affixp;
#endif //FR_VERSION

void fill_session_win(void);
void init_session(void);
int session_reset_handler(void);
#ifdef FR_VERSION
int session_affichagexp(void);
#endif //FR_VERSION
#ifdef MISSILES
int get_session_exp_ranging(void);
#endif //MISSILES
void set_last_skill_exp(size_t skill, int exp);
void update_session_distance(void);

#ifdef __cplusplus
} // extern "C"
#endif

#endif
