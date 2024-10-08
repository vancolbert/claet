
#ifndef __COUNTERS_H__
#define __COUNTERS_H__

#include "actors.h"

#ifdef __cplusplus
extern "C" {
#endif

int now_harvesting(void);
void clear_now_harvesting(void);
void set_now_harvesting(void);

extern char harvest_name[32];
extern int counters_win;
extern Uint32 disconnect_time;
extern char last_spell_name[60];
extern unsigned int floating_counter_flags;
extern int floating_session_counters;

void load_counters();
void flush_counters();
void cleanup_counters();
void fill_counters_win();
void reset_session_counters();
void print_session_counters(const char *category);

void increment_death_counter(actor *a);
#ifdef FR_VERSION
void increment_critfail_counter(char *name);
#endif //FR_VERSION
void increment_break_counter(char *name);
void increment_harvest_counter(int quantity);
void decrement_harvest_counter(int quantity);
void increment_alchemy_counter();
void increment_crafting_counter();
#ifdef INGENIERIE
void increment_engineering_counter();
#endif //INGENIERIE
#ifdef ENGLISH
void increment_engineering_counter();
#endif //ENGLISH

#ifdef FR_VERSION
void increment_combat_info_counter(char *str_type, int num);
void set_max_combat_counter(char *str_type, int val);
#endif //FR_VERSION

void increment_tailoring_counter();
void increment_potions_counter();
void increment_manufacturing_counter();
void increment_spell_counter(int spell_id);
void increment_summon_manu_counter();
void increment_summon_counter(char *string);
int chat_to_counters_command(const char *text, int len);
void catch_counters_text(const char* text);

void counters_set_product_info(char *name, int count);
void counters_set_spell_name(int spell_id, char *name, int len);
int is_death_message (const char * RawText);

#ifdef FR_VERSION
char *strip_actor_name (const char *actor_name);
#endif //FR_VERSION

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __COUNTERS_H__ */

