#ifndef __INFO_COMBAT_H__
#define __INFO_COMBAT_H__
extern int info_combat_console;
extern int info_combat_float_msg;
/*
 * Gère les paquets concernant le proto INFO_COMBAT.
 * @PARAM type : type d'info. (coup critique, cape noire...)
 * @PARAM actor_id : actor se recevant le coup.
 * @PARAM data : données relatives au type.
 */
void combat_info_type(Uint8 type, Uint16 target_id, Uint16 actor_id, Sint32 data);
#endif //__INFO_COMBAT_H__
