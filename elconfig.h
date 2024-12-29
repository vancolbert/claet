#ifndef __ELCONFIG_H__
#define __ELCONFIG_H__
#include "misc.h"
#include "translate.h"
#ifdef __cplusplus
extern "C" {
#endif
extern int elconfig_win;
extern int elconfig_menu_x;
extern int elconfig_menu_y;
extern float water_tiles_extension;
extern int show_game_seconds;
extern int skybox_update_delay;
extern int skybox_local_weather;
#ifdef NEW_CURSOR
extern int big_cursors;
extern int sdl_cursors;
extern float pointer_size;
#endif // NEW_CURSOR
#ifdef NEW_TEXTURES
extern Uint32 max_actor_texture_handles;
#endif /* NEW_TEXTURES */
extern int write_ini_on_exit;
extern int gx_adjust;
extern int gy_adjust;
enum dtypescv { CVD_NONE, CVD_BOOL, CVD_INT, CVD_FLOAT, CVD_STRING };
enum flagscv { CVF_NONE, CVF_INI, CVF_MULTI, CVF_HORIZ=4, CVF_FUNCS=8, CVF_PASSWORD=16 };
typedef float (*fval_func)(void);
typedef int (*ival_func)(void);
typedef struct Snode { struct Snode *snext; cstr sval; } Snode;
typedef struct Cvar {
	cstr name, shortname;
	Uint8 cvdtype, cvflags, name_len, shortname_len, saved;
	int id_depend;
	struct { int tab_id; int label_id; int widget_id; } widgets;
	dichar display;
	void (*pfunc_setb)(int *);
	int *pint;
	void (*pfunc_seti)(int *, int);
	int imin, imax;
	ival_func pfunc_imin, pfunc_imax;
	float *pfloat;
	void (*pfunc_setf)(float *, float);
	float fmin, fmax, fstep;
	fval_func pfunc_fmin, pfunc_fmax, pfunc_fstep;
	void (*pfunc_sets)(char *, cstr, int);
	char *pstr;
	int slen;
	Snode *labels, **labels_ptail;
} Cvar;
typedef struct Allcvars {
	int n_cvars, n_snodes;
	Cvar cvars[256];
	Snode snodes[256];
} Allcvars;
extern Allcvars our_vars;
#define for_cvars(v) for (Cvar *v = our_vars.cvars, *v##e = v + our_vars.n_cvars; v < v##e; ++v)
typedef enum vnkind_t {
	VNK_CMD_SHORT,
	VNK_CMD_LONG,
	VNK_INI,
	VNK_GAME,
} vnkind_t;
extern int delai_sauve;
extern int delai_sauve_ms;
extern int derniere_sauvegarde;
void display_elconfig_win(void);
void change_language(cstr new_lang);
cstr get_option_description(cstr name, vnkind_t k);
int find_var(cstr name, vnkind_t k);
int check_var(char *name, vnkind_t k);
void init_vars(void);
void free_vars(void);
int read_el_ini(void);
int write_el_ini(void);
void check_options(void);
void change_windows_on_top(int *var);
void add_multi_option(cstr name, cstr label);
void change_windowed_chat (int *wc, int val);
int set_var_unsaved(cstr name, vnkind_t k);
int toggle_bool_var(cstr name);
#ifdef ELC
int set_int_var(cstr name, int new_value);
#endif
void toggle_follow_cam(int *fc);
void toggle_ext_cam(int *ec);
void options_loaded(void);
#ifdef __cplusplus
} // extern "C"
#endif
#endif
