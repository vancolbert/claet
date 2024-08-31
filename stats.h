/*!
 * \file
 * \ingroup stats_window
 * \brief Attributes und statistics handling
 */
#ifndef __STATS_H__
#define __STATS_H__
#include <SDL_types.h>
#ifdef __cplusplus
extern "C" {
#endif
#define FLOATINGMESSAGE_NORTH   1
#define FLOATINGMESSAGE_EAST    2
#define FLOATINGMESSAGE_SOUTH   3
#define FLOATINGMESSAGE_WEST    4
#define FLOATINGMESSAGE_MIDDLE  5
/*!
 * \name Windows handlers
 */
/*! @{ */
extern int stats_win; /*!< handle for the stats window */
/*! @} */
/*!
 * The names structure is used for all sort of attributes, skill, nexi to give them a long and a short name.
 */
typedef struct {
	unsigned char name[21]; /*!< the common, long name */
	unsigned char shortname[7]; /*!< a short-name for the given name */
} names;
/*!
 * The attrib_16 structure is used to store the base and the current value of an attribute.
 */
typedef struct {
	Sint16 base; /*!< the base value of the attribute */
	Sint16 cur; /*!< the current value of the attribute. This might be modified by blessings, for example. */
} attrib_16;
/*!
 * The attrib_16f structure stores two function pointers, that are used to calculate the base and current value of a cross attribute.
 */
typedef struct {
	Sint16 (*base)(void); /*!< function pointer to aquire the base value of a cross attribute. */
	Sint16 (*cur)(void); /*!< funtion pointer to aquire the current value of a cross attribute */
} attrib_16f;
/*!
 * attributes_struct stores all the names and short names of all the attributes, cross attributes, nexi, skills, food level, pickpoints, material and ethereal points and carry capacity.
 */
struct attributes_struct {
	unsigned char base[30]; /*!< buffer to store the \see names of a base attribute */
	names phy; /*!< name and short name of physic base attribute */
	names coo; /*!< name and short name of coordination base attribute */
	names rea; /*!< name and short name of reasoning base attribute */
	names wil; /*!< name and short name of will base attribute */
	names ins; /*!< name and short name of instinct base attribute */
	names vit; /*!< name and short name of vitality base attribute */
	unsigned char cross[30]; /*!< buffer to store the \see names of a cross attribute */
	names might; /*!< name and short name of might cross attribute */
	names matter; /*!< name and short name of matter cross attribute */
	names tough; /*!< name and short name of toughness cross attribute */
	names react; /*!< name and short name of reaction cross attribute */
	names charm; /*!< name and short name of charm cross attribute */
	names perc; /*!< name and short name of perception cross attribute */
	names ration; /*!< name and short name of rationality cross attribute */
	names dext; /*!< name and short name of dexterity cross attribute */
	names eth; /*!< name and short name of ethereality cross attribute */
	unsigned char nexus[30]; /*!< buffer to store the \see names of a nexus */
	names defense_nexus;
	names attaque_nexus;
	names necro_nexus;
	names potion_nexus;
	names recolte_nexus;
	names fabrication_nexus;
	names artisanat_nexus;
	names magie_nexus;
	names alchimie_nexus;
	names ingenierie_nexus;
	unsigned char skills[30]; /*!< buffer to store the \see names of a skill */
	names manufacturing_skill; /*!< name and short name of manufacturing skill */
	names harvesting_skill; /*!< name and short name of harvesting skill */
	names alchemy_skill; /*!< name and short name of alchemy skill */
	names overall_skill; /*!< name and short name of overall skill */
	names attack_skill; /*!< name and short name of attack skill */
	names defense_skill; /*!< name and short name of defense skill */
	names magic_skill; /*!< name and short name of magic skill */
	names potion_skill; /*!< name and short name of potion skill */
	names summoning_skill; /*!< name and short name of summoning skill */
	names crafting_skill; /*!< name and short name of crafting skill */
	names engineering_skill; /*!< name and short name of engineering skill */
	names tailoring_skill; /*!< name and short name of tailoring skill */
	names ranging_skill; /*!< name and short name of ranging skill */
	names food; /*!< name and short name of food level */
	unsigned char pickpoints[30]; /*!< available pickpoints */
	names material_points; /*!< name and short name of material points */
	names ethereal_points; /*!< name and short name of ethereal points */
	names carry_capacity; /*!< name and short name of carry capacity */
	names notoriete;
	names religion;
};
extern struct attributes_struct attributes; /*!< global variable for an actors attributes */
/*!
 * The player_attribs structure takes care of all the attributes of a player.
 */
typedef struct {
	unsigned char name[20]; /*!< name (of what? player? current selected attrib/skill/...?) */
	attrib_16 phy; /*!< base and current value of the physic base attribute */
	attrib_16 coo; /*!< base and current value of the coordination base attribute */
	attrib_16 rea; /*!< base and current value of the reasoning base attribute */
	attrib_16 wil; /*!< base and current value of the will base attribute */
	attrib_16 ins; /*!< base and current value of the instinct base attribute */
	attrib_16 vit; /*!< base and current value of the vitality base attribute */
	attrib_16 might; /*!< functions to get the base and current value of the might cross attribute */
	attrib_16 matter; /*!< functions to get the base and current value of the matter cross attribute */
	attrib_16 tough; /*!< functions to get the base and current value of the toughness cross attribute */
	attrib_16 charm; /*!< functions to get the base and current value of the charm cross attribute */
	attrib_16 react; /*!< functions to get the base and current value of the reaction cross attribute */
	attrib_16 perc; /*!< functions to get the base and current value of the perception cross attribute */
	attrib_16 ration; /*!< functions to get the base and current value of the rationality cross attribute */
	attrib_16 dext; /*!< functions to get the base and current value of the dexterity cross attribute */
	attrib_16 eth; /*!< functions to get the base and current value of the ethereality cross attribute */
	// TODO TRINITA Nexus implementation !
	attrib_16 defense_nexus; // nexus old human_nexus
	attrib_16 attaque_nexus; // nexus old human_nexus @Trinita not use for this moment
	attrib_16 necro_nexus; // nexus old animal_nexus
	attrib_16 potion_nexus; // nexus old vegetal_nexus
	attrib_16 recolte_nexus; // nexus old inorganic_nexus
	attrib_16 fabrication_nexus; // nexus old artificial_nexus
	attrib_16 artisanat_nexus; // nexus old magic_nexus
	attrib_16 magie_nexus; // nexus new
	attrib_16 alchimie_nexus; // nexus new
	attrib_16 ingenierie_nexus; // nexus new
	attrib_16 material_points; /*!< base and current value of the players material points */
	attrib_16 ethereal_points; /*!< base and current value of the players ethereal points */
	attrib_16 manufacturing_skill; /*!< base and current value of the manu skill */
	attrib_16 harvesting_skill; /*!< base and current value of the harvesting skill */
	attrib_16 alchemy_skill; /*!< base and current value of the alchemy skill */
	attrib_16 overall_skill; /*!< base and current value of the overall skill */
	attrib_16 attack_skill; /*!< base and current value of the attack skill */
	attrib_16 defense_skill; /*!< base and current value of the defense skill */
	attrib_16 magic_skill; /*!< base and current value of the magic skill */
	attrib_16 potion_skill; /*!< base and current value of the potion skill */
	attrib_16 summoning_skill; /*!< base and current value of the summoning skill */
	attrib_16 crafting_skill; /*!< base and current value of the crafting skill */
	attrib_16 engineering_skill; /*!< base and current value of the engineering skill */
	attrib_16 tailoring_skill; /*!< base and current value of the tailoring skill */
	attrib_16 ranging_skill; /*!< base and current value of the ranging skill */
	attrib_16 carry_capacity; /*!< base and current value of the carry capacity */
	Sint8 food_level; /*!< current food level */
	Uint32 manufacturing_exp; /*!< current manu experience */
	Uint32 manufacturing_exp_next_lev; /*!< experience level to reach next manu level */
	Uint32 harvesting_exp; /*!< current harvesting experience */
	Uint32 harvesting_exp_next_lev; /*!< experience level to reach next harvesting level */
	Uint32 alchemy_exp; /*!< current alchemy experience */
	Uint32 alchemy_exp_next_lev; /*!< experience level to reach next alchemy level */
	Uint32 overall_exp; /*!< current overal experience */
	Uint32 overall_exp_next_lev; /*!< experience level to reach next overall level */
	Uint32 attack_exp; /*!< current attack experience */
	Uint32 attack_exp_next_lev; /*!< experience level to reach next attack level */
	Uint32 defense_exp; /*!< current defense experience */
	Uint32 defense_exp_next_lev; /*!< experience level to reach next defense level */
	Uint32 magic_exp; /*!< current magic experience */
	Uint32 magic_exp_next_lev; /*!< experience level to reach next magic level */
	Uint32 potion_exp; /*!< current potion experience */
	Uint32 potion_exp_next_lev; /*!< experience level to reach next potion level */
	Uint32 summoning_exp; /*!< current summoning level */
	Uint32 summoning_exp_next_lev; /*!< experience level to reach next summoning level */
	Uint32 crafting_exp; /*!< current crafting experience */
	Uint32 crafting_exp_next_lev; /*!< experience level to reach next crafting level */
	Uint32 engineering_exp; /*!< current engineering experience */
	Uint32 engineering_exp_next_lev; /*!< experience level to reach next engineering level */
	Uint32 tailoring_exp; /*!< current tailoring experience */
	Uint32 tailoring_exp_next_lev; /*!< experience level to reach next tailoring level */
	Uint32 ranging_exp; /*!< current ranging experience */
	Uint32 ranging_exp_next_lev; /*!< experience level to reach next ranging level */
	Sint8 notoriete;
	Uint16 religion;
	Uint16 niv_rel;
	Uint16 race;
	Uint16 researching; /*!< flag to indicate whether a player is currently researching anything or not */
	Uint16 research_completed; /*!< if a player is currently researching anything, this value will show how much pages are already read */
	Uint16 research_total; /*!< if a player is currently researching anything, this value show the total amount of pages to read, until the book is completely read. */
} player_attribs;
/*	Array for skills info required by stats bar.  Stored in an array
        to allow processing in a loop and avoiding duplicating the code.
        Possible TBD: Really, the skills should be stored in an array
        at source so more duplicate code can be removed and new skills
        added more simply. */
struct stats_struct {
	Uint32 *exp;
	Uint32 *next_lev;
	attrib_16 *skillattr;
	names *skillnames;
	int is_selected;
};
/*!
 * Indexes for plat stats arrays
 */
enum {SI_ATT = 0, SI_DEF, SI_HAR, SI_ALC, SI_MAG, SI_POT, SI_SUM, SI_MAN, SI_CRA, SI_ENG, SI_ALL, };
/*!
 * An array of pointers to the player stats information - enables looping rather than duplicate code.
 */
extern struct stats_struct statsinfo[];
#define NUM_SKILLS 11 /*!< the number of skills */
#define NUM_WATCH_STAT  12 /*!< allow watching stats 0-11 */
#define MAX_WATCH_STATS 5 /*!< max number of stats watchable in hud */
extern int attrib_menu_x;
extern int attrib_menu_y;
extern int watch_this_stats[]; /*!< used for displaying more than 1 stat in the hud */
extern int max_disp_stats; /*!< max number of stats displayable in hud, depending on screen resolution */
extern int have_stats; /*!< indicator for whether or not the stats have been send to us yet*/
extern player_attribs your_info; /*!< the players attributes */
/*!
 * \ingroup stats_window
 * \brief   Retrieves the statistics of the player.
 *
 *      Retrieves all the statistics of the player and stores them in the parameter stats.
 *
 * \param stats
 *
 * \callgraph
 */
void get_the_stats(Sint16 *stats);
/*!
 * \ingroup stats_window
 * \brief   Gets the part of the stats that is specified by name.
 *
 *      Gets the part of the stats that is specified by name.
 *
 * \param name  The name of the stat to get. Can be an attribute, cross attribute, skill or nexus
 * \param value The value of the stat to get.
 */
void get_partial_stat(unsigned char name, Sint32 value);
/*!
 * \ingroup other
 * \brief   Initializes the callbacks used to calculate base and current value of the cross attributes.
 *
 *      Initializes the callbacks used to calculate base and current value of the cross attributes.
 *
 */
void init_attribf(void);
/*!
 * \ingroup stats_window
 * \brief Sets the window handler functions for the statistics window
 *
 *      Sets the window handler functions for the statistics window
 *
 * \callgraph
 */
void fill_stats_win();
extern int floatingmessages_enabled;
void drawactor_floatingmessages(int actor_id, float healthbar_z);
void add_floating_message(int actor_id, char *str, int direction, float r, float g, float b, int active_time);
void init_statsinfo_array(void);
/*
 * decide if/where to display the given stat in the hud
 * uses the flags of the calling window to check for ALT- or SHIFT-key
 */
extern void handle_stats_selection(int stat, Uint32 flags);
#ifdef __cplusplus
} // extern "C"
#endif
#endif
