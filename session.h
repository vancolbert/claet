#ifndef __EXT_SESSION_H__
#define __EXT_SESSION_H__

#ifdef __cplusplus
extern "C" {
#endif

extern int session_win;
extern int exp_log_threshold;
extern int affixp;

void fill_session_win(void);
void init_session(void);
int session_reset_handler(void);
int session_affichagexp(void);
void set_last_skill_exp(size_t skill, int exp);
void update_session_distance(void);

#ifdef __cplusplus
} // extern "C"
#endif

#endif
