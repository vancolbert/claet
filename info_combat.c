#include <ctype.h>
#include "client_serv.h"
#include "global.h"
#include "stats.h"
#include "multiplayer.h"
#include "errors.h"
#include "actors.h"
#include "text.h"
#include "counters.h"
#include "client_serv.h"
#include "asc.h"
#include "themes.h"
#include "interface.h"

#define LOG_INFO_COMBAT(color, buff) put_colored_text_in_buffer(color, CHAT_COMBAT, (const Uint8*)buff, -1)
#define ADD_FLOATING_MSG(act, buff, rgb) add_floating_message(act, buff, FLOATINGMESSAGE_NORTH, rgb.rouge, rgb.vert, rgb.bleu, 1500)

int info_combat_console=0;
int info_combat_float_msg=0;


void combat_info_type(Uint8 type, Uint16 target_id, Uint16 actor_id, Sint32 data)
{
	char buff[128] = {0};
	actor *act = get_actor_ptr_from_id(target_id);
	char *actor_name;

	if(act == NULL)
	{
		LOG_ERROR("INFO_COMBAT : %d is invalid actor_id!", target_id);
		return;
	}
   actor_name = strip_actor_name(act->actor_name);
   actor_name = trim(actor_name);

	switch(type)
	{
		case COUP_CRITIQUE:
			if(info_combat_float_msg && flottant_coup_critique)
			{
				snprintf(buff, sizeof(buff)-1, "Crit:%d", data);
				add_floating_message(target_id, buff, FLOATINGMESSAGE_NORTH, 0.7, 0.0, 0.0, 1500);
			}
			if(info_combat_console && console_coup_critique)
			{
				snprintf(buff, sizeof(buff)-1, "%s reçoit un coup critique de %d dégâts.", actor_name, data);
				LOG_INFO_COMBAT(c_red1, buff);
			}

			if(target_id == yourself)
			{
				increment_combat_info_counter("Critiques reçus", 1);
				set_max_combat_counter("Max critique reçu", data);
			}
			else if(actor_id == yourself)
			{
			        increment_combat_info_counter("Critiques donnés", 1);
			        set_max_combat_counter("Max critique donné", data);
			}
		break;
		case CHANCE_CRITIQUE:
			if(info_combat_float_msg && flottant_chance_critique)
			{
				snprintf(buff, sizeof(buff)-1, "Touche !");
				add_floating_message(target_id, buff, FLOATINGMESSAGE_NORTH, 0.0, 0.3, 0.7, 1500);
			}
			if(info_combat_console && console_chance_critique)
			{
				snprintf(buff, sizeof(buff)-1, "%s a reçu un critique touché !", actor_name);
				LOG_INFO_COMBAT(c_red3, buff);
			}

			if(target_id == yourself)
			{
				increment_combat_info_counter("Chance critique", 1);
			}
		break;
		case CHANCE_ESQUIVE:
			if(info_combat_float_msg && flottant_chance_esquive)
			{
				snprintf(buff, sizeof(buff)-1, "Esq !");
				add_floating_message(target_id, buff, FLOATINGMESSAGE_NORTH, 0.7, 0.7, 0.0, 1500);
			}
			if(info_combat_console && console_chance_esquive)
			{
				snprintf(buff, sizeof(buff)-1, "%s esquive le coup!", actor_name);
				LOG_INFO_COMBAT(c_green1, buff);
			}
		break;
		case BOUCLIER_PERSEE:
			/*if(info_combat_console)
			{
				snprintf(buff, sizeof(buff)-1, "%s est victime du bouclier de persé!", actor_name);
				LOG_INFO_COMBAT(c_red1, buff);
			}*/
		break;
		case DEFENSE_MALUS:
		break;
		case DEGAT_FROID:
			if(info_combat_console && console_degat_froid)
			{
				snprintf(buff, sizeof(buff)-1, "%s reçoit %d dégâts de froid.", actor_name, data);
				LOG_INFO_COMBAT(c_blue1, buff);
			}
			if(info_combat_float_msg && flottant_degat_froid)
			{
				snprintf(buff, sizeof(buff)-1, "Froid:%d", data);
				ADD_FLOATING_MSG(target_id, buff, couleur_degat_froid);
			}
			if(target_id == yourself)
			{
				set_max_combat_counter("Max dégât froid reçu", data);
			}
			else if(actor_id == yourself)
			{
				set_max_combat_counter("Max dégât froid donné", data);
			}
		break;
		case DEGAT_CHAUD:
			if(info_combat_console && console_degat_feu)
			{
				snprintf(buff, sizeof(buff)-1, "%s reçoit %d dégâts de chaleur.", actor_name, data);
				LOG_INFO_COMBAT(c_red2, buff);
			}
			if(info_combat_float_msg && flottant_degat_feu)
			{
				snprintf(buff, sizeof(buff)-1, "Feu:%d", data);
				ADD_FLOATING_MSG(target_id, buff, couleur_degat_feu);
			}
			if(target_id == yourself)
			{
				set_max_combat_counter("Max dégât chaud reçu", data);
			}
			else if(actor_id == yourself)
			{
			    	set_max_combat_counter("Max dégât chaud donné", data);
			}

		break;
		case DEGAT_MAGIE:
			if(info_combat_console && console_degat_magie)
			{
				snprintf(buff, sizeof(buff)-1, "%s reçoit %d dégâts arcaniques.", actor_name, data);
				LOG_INFO_COMBAT(c_yellow1, buff);
			}
			if(info_combat_float_msg && flottant_degat_magie)
			{
				snprintf(buff, sizeof(buff)-1, "Arcane:%d", data);
				ADD_FLOATING_MSG(target_id, buff, couleur_degat_magie);
			}

			if(target_id == yourself)
			{
				set_max_combat_counter("Max dégât arcaniques reçu", data);
			}
			else if(actor_id == yourself)
			{
				set_max_combat_counter("Max dégât arcaniques donné", data);
			}
		break;
		case DEGAT_LUMIERE:
			if(info_combat_console && console_degat_lumiere)
			{
				snprintf(buff, sizeof(buff)-1, "%s reçoit %d dégâts de lumière.", actor_name, data);
				LOG_INFO_COMBAT(c_purple1, buff);
			}
			if(info_combat_float_msg && flottant_degat_lumiere)
			{
				snprintf(buff, sizeof(buff)-1, "Lumi:%d", data);
				ADD_FLOATING_MSG(target_id, buff, couleur_degat_lumiere);
			}
			if(target_id == yourself)
			{
				set_max_combat_counter("Max dégât lumière reçu", data);
			}
			else if(actor_id == yourself)
			{
				set_max_combat_counter("Max dégât lumière donné", data);
			}
		break;
		case DEGAT_POISON:
		if(target_id == yourself)
			{
				increment_combat_info_counter("Doses poison reçus en combat", data);
				set_max_combat_counter("Dose de poison maximal reçue", data);
			}
			if(info_combat_console && console_degat_poison)
			{
				snprintf(buff, sizeof(buff)-1, "%s reçoit %d doses de poison.", actor_name, data);
				LOG_INFO_COMBAT(c_green1, buff);
			}
			if(info_combat_float_msg && flottant_degat_poison)
			{
				snprintf(buff, sizeof(buff)-1, "Poison:%d", data);
				ADD_FLOATING_MSG(target_id, buff, couleur_degat_poison);
			}
		break;
		default:
		LOG_ERROR("[PROTO] INFO_COMBAT : malformed packet! (unknown type : %u)", type);
	}
}
