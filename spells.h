/*!
 * \file
 * \ingroup spells_window
 * \brief Handling of sigils and spells
 */
#ifndef __SPELLS_H__
#define __SPELLS_H__
#include <SDL_types.h>
#include "eye_candy_wrapper.h"
#ifdef __cplusplus
extern "C" {
#endif
#define QUICKSPELLS_MAXSIZE 20
#define NUM_ACTIVE_SPELLS 10
/*!
 * \name Server spell messages
 */
/*! @{ */
typedef enum {
	S_SUCCES = 1, S_FAILED, S_INVALID, S_SELECT_TARGET, S_SELECT_TELE_LOCATION, S_NAME, } spell_errors;
/*! @} */
typedef struct {
	char spell_name[60]; // The spell_name
	Sint8 spell_image; // image_id
	Sint8 spell_id;
	Uint8 spell_str[30];
	// to be difficult, we will store the entire string ready
	// to be sent to the server, including CAST_SPELL and len bytes, len will be byte 2
} mqbdata;
extern mqbdata *mqb_data[QUICKSPELLS_MAXSIZE + 1]; // mqb_data will hold the magic quickbar name, image, pos.
extern int quickspell_mqb_selected;
extern mqbdata *mqb_data2[QUICKSPELLS_MAXSIZE + 1]; // mqb_data will hold the magic quickbar name, image, pos.
extern mqbdata *mqb_data3[QUICKSPELLS_MAXSIZE + 1]; // mqb_data will hold the magic quickbar name, image, pos.
extern mqbdata *mqb_data4[QUICKSPELLS_MAXSIZE + 1]; // mqb_data will hold the magic quickbar name, image, pos.
extern mqbdata *mqb_data5[QUICKSPELLS_MAXSIZE + 1]; // mqb_data will hold the magic quickbar name, image, pos.
extern int spell_temp, spell_dragged;
extern int quickspell_x_len;
extern int quickspell_y_len;
extern int quickspells_size; /*!< nombre de raccourcis affich�s sur la barre rapide */
extern int quickspells_dir; /*!< orientation de la barre (VERTICAL=1|HORIZONTAL=2) */
extern int quickspells_on_top; /*!< barre toujours affich�e au dessus ? */
extern int quickspells_draggable; /*!< barre de raccourcis d�pla�able ? */
int resize_quickspells(int nb);
/*!
 * \name windows handlers
 */
/*! @{ */
extern int sigil_win; /*!< handle for the sigil (spell) window */
extern int start_mini_spells; /*!< do we start minimized? */
extern int quickspell_win; /*!< quickbar windows handler */
/*! @} */
extern int quickspell_x;
extern int quickspell_y;
extern int sigil_menu_x;
extern int sigil_menu_y;
extern int sigils_text;
extern Uint8 spell_text[256]; /*!< buffer for the spell text */
extern int have_error_message; /*!< flag that indicates whether we got an error message */
extern int spell_result;
extern Uint8 last_spell_str[20];
extern int last_spell_len;
/*!
 * \ingroup spells_window
 * \brief Checks spells to determine if they�re castable
 *
 *      Checks castability checking reagents, sigils, mana and levels
 *
 */
void check_castability();
/*!
 * \ingroup spells_window
 * \brief Repeats the last used spell.
 *
 *      Repeats the spell that was last used. This is initiated by pressing Ctrl+R. hotkey
 *
 */
void repeat_spell();
/*!
 * \ingroup other
 * \brief Initializes variables used in magic.
 *
 *      Initializes and sets up the sigils list and other variables used in spell casting.
 *
 */
int init_spells();
/*!
 * \ingroup spells_window
 * \brief Changes the active_spell at the given \a pos to the given \a spell.
 *
 *      Changes the active_spell at the given \a pos to the given \a spell.
 *
 * \param pos   index in the list of which spell to change
 * \param spell the index of the spell to use.
 */
/*!
 * \param timer : information sur la dur�e restante du sort
 */
void get_active_spell(int pos, int spell, int timer);
/*!
 * \ingroup spells_window
 * \brief Removes the spell at the given \a pos from the list of active spells
 *
 *      Removes the spell at the given \a pos from the list of active spells
 *
 * \param pos   the index into the array of the spell to remove
 */
void remove_active_spell(int pos);
/*!
 * \ingroup spells_window
 * \brief Sets the list of active_spells to the given list \a my_spell_list.
 *
 *      Sets the list of active_spells to the given list \a my_spell_list.
 *
 * \param my_spell_list the spell list to use for active_spells.
 */
void get_active_spell_list(const Uint8 *my_spell_list);
/*!
 * \ingroup spells_window
 * \brief Draws the sigils that the actor currently owns
 *
 *      Draws the sigils that the actor currently owns
 *
 * \callgraph
 */
void display_spells_we_have();
/*!
 * \ingroup spells_window
 * \brief Displays the spells window
 *
 *      Displays the spells window
 *
 * \callgraph
 */
void display_sigils_menu();
/*!
 * \ingroup spells_window
 * \brief Sets the sigils in sigil_list according the mask in \a sigils_we_have.
 *
 *      sets the sigils in sigil_list according the mask in \a sigils_we_have.
 *
 * \param sigils_we_have    the mask to determine the sigils which the actor has.
 */
void get_sigils_we_have(Uint32 sigils_we_have, Uint32 sigils2);
/*!
 * \ingroup spells_window
 * \brief Processes a message from the server about the last spell
 *
 * 	Processes a message from the server about the outcome of the last spell, or gets the name of a given spell.
 *
 * \param data The network data
 * \param len The data length
 */
void process_network_spell(const char *data, int len);
/*!
 * \ingroup other
 * \brief Send a spell message to the server
 *
 * 	The preprepared spell message is sent to the server and stored, as last_spell_str and last_spell_len.
 *
 * \param str the spell message
 * \param len the length of the spell message
 */
void send_spell(Uint8 *str, int len);
/*!
 * \ingroup other
 * \brief Check if a keypress is a quick spell
 *
 * 	returns 1 if it is a quickspell, otherwise 0.
 *
 */int action_spell_keys(Uint32 key);
void load_quickspells();
void save_quickspells();
void init_quickspell();
void add_spell_to_quickbar();
int get_quickspell_y_base();
int we_are_poisoned();
void restart_active_spell_sounds(void);
// Trent en cours
int chargement_necro_recettes(int choix);
int une_bebete();
int plusieurs_bebetes();
int invocation_depuis_sorts(Uint8 quantity);
extern int selected_spell;
extern int selected_spell_sent;
extern int selected_spell_target;
void fast_spell_cast(void);
void fast_spell_teleport(void);
void fast_spell_cible(int spell_id);
void fast_spell_decible(void);
extern int set_fast_spell_target;
#ifdef __cplusplus
} // extern "C"
#endif
#endif
