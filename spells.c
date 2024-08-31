#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "spells.h"
#include "asc.h"
#include "cursors.h"
#include "console.h"
#include "context_menu.h"
#include "elwindows.h"
#include "gamewin.h"
#include "gl_init.h"
#include "hud.h"
#include "init.h"
#include "interface.h"
#include "items.h"
#include "item_info.h"
#include "stats.h"
#include "colors.h"
#include "multiplayer.h"
#include "pathfinder.h"
#include "textures.h"
#include "translate.h"
#include "counters.h"
#include "errors.h"
#include "io/elpathwrapper.h"
#include "sound.h"
#include "themes.h"
#include "highlight.h"
#define SIGILS_NO 64
#define NUM_SIGILS_LINE 12 // how many sigils per line displayed
#define NUM_SIGILS_ROW  3 // how many rows of sigils are there?
#define MAX_DATA_FILE_SIZE 1850 // QUICKSPELLS_MAXSIZE * 92 + 1 ? (default 560)
#define QUICKSPELLS_MAXSIZE 20
#define SIGILS_NO 64
#define SPELLS_NO 32
#define GROUPS_NO 8
#define NECRO_NO 5
#define UNCASTABLE_REAGENTS 1
#define UNCASTABLE_SIGILS 2
#define UNCASTABLE_MANA 4
#define UNCASTABLE_LVLS 8
#define UNCASTABLE_SIGILS_STR "(rune(s) manquante(s))"
#define UNCASTABLE_REAGENTS_STR "(pas assez d'essences)"
#define UNCASTABLE_MANA_STR "(pas assez de mana)"
#define UNCASTABLE_LVLS_STR "(pas le niveau)"
#define GET_UNCASTABLE_STR(cast) ((cast & UNCASTABLE_SIGILS) ? (UNCASTABLE_SIGILS_STR):((cast & UNCASTABLE_LVLS) ? (UNCASTABLE_LVLS_STR):((cast & UNCASTABLE_MANA) ? (UNCASTABLE_MANA_STR):((cast & UNCASTABLE_REAGENTS) ? (UNCASTABLE_REAGENTS_STR):("")))))
#define SPELLS_ALIGN_X 7
#define MAX(a, b) (((a) > (b))?(a):(b))
#define NECRO_ALIGN_X 1
#define NECRO_SPACE_X 10 // espace entre bord gauche de la fenêtre et la grille de nécro
#define NECRO_SPACE_Y 10 // espace entre bord haut de la fenêtre et la grille de nécro
#define NECRO_SPACE_Y_BETWEEN 2 // espace vertical entre les différentes icones de nécro
#define SET_COLOR(x) glColor4f((float)colors_list[x].r1 / 255.0f, (float)colors_list[x].g1 / 255.0f, (float)colors_list[x].b1 / 255.0f, 1.0f)
typedef struct {
	int sigil_img;
	char name[32];
	char description[64];
	int have_sigil;
} sigil_def;
int selected_spell = -1;
int selected_spell_sent = 0;
int selected_spell_target = -1;
int fast_spell_cible_bool = 0;
int set_fast_spell_target;
sigil_def sigils_list[SIGILS_NO];
int sigils_text;
int sigils_we_have;
typedef struct {
	int necro_img; // image de l'icone : position dans gamebuttons.dds cf liste de #define dans hud.c
	char name[60]; // contient la commande sous forme #commande
	char description[120]; // description de la commande - contient le texte à afficher
} necro_def;
necro_def necro_list[NECRO_NO + 1]; // +1 pour le sort de soin des invoqués
int mouseover_necro = -1; // permet d'afficher dans display_necro_handler() le nom de la commande lorsque la souris passe sur l'icone. valeur par défaut : -1 (souris ailleurs), sinon : entier >= 0
// pour le sort de soin des invoqués : positions x et y
int pos_x_sdi;
int pos_y_sdi;
typedef struct {
	int id; // The spell server id
	char name[60]; // The spell name
	char desc[120]; // The spell description
	int image; // image_id
	int sigils[6]; // index of required sigils in sigils_list
	int mana; // required mana
	attrib_16 *lvls[NUM_WATCH_STAT]; // pointers to your_info lvls
	int lvls_req[NUM_WATCH_STAT]; // minimum lvls requirement
	int reagents_id[4]; // reagents needed
	Uint16 reagents_uid[4]; // reagents needed, unique item id
	int reagents_qt[4]; // their quantities
	int uncastable; // 0 if castable, otherwise if something missing
} spell_info;
spell_info spells_list[SPELLS_NO];
int num_spells = 0;
Uint8 spell_text[256];
unsigned char spell_help[256];
Sint8 on_cast[6];
Uint8 last_spell_str[20];
int last_spell_len = 0;
int spell_result = 0;
int have_error_message = 0;
int we_have_spell = -1; // selected spell
int on_spell = -1; // mouse over this spell
typedef struct {
	unsigned char desc[120];
	int spells;
	int spells_id[SPELLS_NO];
	int x, y;
} group_def;
int num_groups = 0;
group_def groups_list[GROUPS_NO];
typedef struct {
	Sint8 spell;
	Uint32 cast_time;
	Uint32 duration;
	unsigned int sound;
} spell_def;
spell_def active_spells[NUM_ACTIVE_SPELLS];
// windows related
int sigil_win = -1; // this is referred externally so we will change it when we switch windows
int sigils_win = -1;
int spell_win = -1;
int spell_mini_win = -1;
int last_win = -1;
int necro_win = -1;
int creature_en_cours = 0;
int double_invoc = 0;
int securite_invoc = 1;
int nb_recettes_necro = -1; // Nombre recettes de nécro		45
char **nom_bestiole; // Tableau des noms des invocation	char nom_bestiole[45][30]
int **liste_items_necro; // Tableau des recettes			int liste_items_necro[45][20]    (mars 2020)
static char items_string[350] = {0};
static size_t last_items_string_id = 0;
int start_mini_spells = 0; // do we start minimized?
int init_ok = 0;
int sigil_menu_x = 10;
int sigil_menu_y = 20;
// big window
int spell_x_len = 0;
int spell_y_len = 0;
int spell_y_len_ext = 0;
// sigil window
int sigil_x_len = NUM_SIGILS_LINE * 33 + 20;
int sigil_y_len = (3 + NUM_SIGILS_ROW) * 33;
// mini window
int spell_mini_x_len = 0;
int spell_mini_y_len = 0;
int spell_mini_rows = 0;
// necro window
int necro_x_len = (NECRO_NO + 1) * 33 + NECRO_SPACE_Y + 40 + NECRO_SPACE_Y_BETWEEN * 8;
int necro_y_len = NECRO_ALIGN_X * 33 + NECRO_SPACE_X + 55 + 190 - 15;
// QUICKSPELLS
int clear_mouseover = 0;
int cast_mouseover = 0;
mqbdata *mqb_data[QUICKSPELLS_MAXSIZE + 1] = {NULL}; // mqb_data will hold the magic quickbar name, image, pos.
int quickspell_mqb_selected = 0;
mqbdata *mqb_data2[QUICKSPELLS_MAXSIZE + 1] = {NULL}; // mqb_data will hold the magic quickbar name, image, pos.
mqbdata *mqb_data3[QUICKSPELLS_MAXSIZE + 1] = {NULL}; // mqb_data will hold the magic quickbar name, image, pos.
mqbdata *mqb_data4[QUICKSPELLS_MAXSIZE + 1] = {NULL}; // mqb_data will hold the magic quickbar name, image, pos.
mqbdata *mqb_data5[QUICKSPELLS_MAXSIZE + 1] = {NULL}; // mqb_data will hold the magic quickbar name, image, pos.
int quickspells_nb = 0; // nombre de raccourcis existants
int quickspells_size = 6; // nombre de raccourcis    (cf ini)
int quickspells_dir = VERTICAL; // orientation par défaut  (cf cfg)
int quickspells_on_top = 1; // toujours au dessus ?    (cf cfg)
int quickspells_draggable = 0; // fenêtre déplaçable ?    (cf cfg)
int quickspell_x = HUD_MARGIN_X; // position x par défaut   (cf cfg)
int quickspell_y = HUD_MARGIN_X; // position y par défaut   (cf cfg)
int quickspell_x_len = (1 + 30) * 1 + 1; // largeur par défaut
int quickspell_y_len = (1 + 30) * 1 + 1; // longueur par défaut
int quickspells_loaded = 0;
int cast_handler();
int prepare_for_cast();
void draw_spell_icon(int id, int x_start, int y_start, int gridsize, int alpha, int grayed);
void set_spell_help_text(int spell);
void init_sigils();
size_t cm_quickspells_win_id = CM_INIT_VALUE;
size_t cm_quickspells_id = CM_INIT_VALUE;
void cm_update_quickspells(void);
void repeat_spell() {
	if (last_spell_len > 0) {
		my_tcp_send(my_socket, last_spell_str, last_spell_len);
	}
}
// returns a node with tagname, starts searching from the_node
xmlNode *get_XML_node(xmlNode *the_node, char *tagname) {
	xmlNode *node = the_node;
	while (node) {
		if (node->type == XML_ELEMENT_NODE && xmlStrcasecmp(node->name, (xmlChar *)tagname) == 0) {
			return node;
		} else {
			node = node->next;
		}
	}
	return node;
}
attrib_16 *get_skill_address(const char *skillname) {
	if (strcmp(skillname, (char *)attributes.manufacturing_skill.shortname) == 0) {
		return &your_info.manufacturing_skill;
	}
	if (strcmp(skillname, (char *)attributes.alchemy_skill.shortname) == 0) {
		return &your_info.alchemy_skill;
	}
	if (strcmp(skillname, (char *)attributes.magic_skill.shortname) == 0) {
		return &your_info.magic_skill;
	}
	if (strcmp(skillname, (char *)attributes.summoning_skill.shortname) == 0) {
		return &your_info.summoning_skill;
	}
	if (strcmp(skillname, (char *)attributes.attack_skill.shortname) == 0) {
		return &your_info.attack_skill;
	}
	if (strcmp(skillname, (char *)attributes.defense_skill.shortname) == 0) {
		return &your_info.defense_skill;
	}
	if (strcmp(skillname, (char *)attributes.crafting_skill.shortname) == 0) {
		return &your_info.crafting_skill;
	}
	if (strcmp(skillname, (char *)attributes.engineering_skill.shortname) == 0) {
		return &your_info.engineering_skill;
	}
	if (strcmp(skillname, (char *)attributes.potion_skill.shortname) == 0) {
		return &your_info.potion_skill;
	}
	if (strcmp(skillname, (char *)attributes.tailoring_skill.shortname) == 0) {
		return &your_info.tailoring_skill;
	}
	if (strcmp(skillname, (char *)attributes.ranging_skill.shortname) == 0) {
		return &your_info.ranging_skill;
	}
	if (strcmp(skillname, (char *)attributes.overall_skill.shortname) == 0) {
		return &your_info.overall_skill;
	}
	if (strcmp(skillname, (char *)attributes.harvesting_skill.shortname) == 0) {
		return &your_info.harvesting_skill;
	}
	return NULL;
}
int put_on_cast() {
	if (we_have_spell >= 0) {
		int i;
		for (i = 0; i < 6; i++) {
			if (spells_list[we_have_spell].sigils[i] >= 0) {
				if (!sigils_list[spells_list[we_have_spell].sigils[i]].have_sigil) {
					// we miss at least a sigil, clear on_cast
					int j;
					for (j = 0; j < 6; j++) {
						on_cast[j] = -1;
					}
					return 0;
				}
			}
		}
		for (i = 0; i < 6; i++) {
			on_cast[i] = spells_list[we_have_spell].sigils[i];
		}
		return 1;
	}
	return 0;
}
int init_spells() {
	int i, j;
	xmlNode *root;
	xmlDoc *doc;
	int ok = 1;
	char *fname = "./spells.xml";
	// init textures and structs
	sigils_text = load_texture_cached("textures/sigils.dds", tt_gui);
	for (i = 0; i < SIGILS_NO; i++) {
		sigils_list[i].have_sigil = 0;
	}
	for (i = 0; i < SPELLS_NO; i++) {
		spells_list[i].image = -1;
		for (j = 0; j < 6; j++) {
			spells_list[i].sigils[j] = -1;
		}
		for (j = 0; j < 4; j++) {
			spells_list[i].reagents_id[j] = -1;
			spells_list[i].reagents_uid[j] = unset_item_uid;
		}
		for (j = 0; j < NUM_WATCH_STAT; j++) {
			spells_list[i].lvls[j] = NULL;
		}
		spells_list[i].uncastable = 0;
	}
	for (i = 0; i < GROUPS_NO; i++) {
		groups_list[i].spells = 0;
		for (j = 0; j < SPELLS_NO; j++) {
			groups_list[i].spells_id[j] = -1;
		}
	}
	spell_text[0] = spell_help[0] = 0;
	i = 0;
	// parse xml
	doc = xmlReadFile(fname, NULL, 0);
	if (doc == 0) {
		LOG_ERROR("Unable to read spells definition file %s: %s", fname, strerror(errno));
		ok = 0;
	}
	root = xmlDocGetRootElement(doc);
	if (root == 0) {
		LOG_ERROR("Unable to parse spells definition file %s", fname);
		ok = 0;
	} else if (xmlStrcasecmp(root->name, (xmlChar *)"Magic") != 0) {
		LOG_ERROR("Unknown key \"%s\" (\"Magic\" expected).", root->name);
		ok = 0;
	} else {
		xmlNode *node;
		xmlNode *data;
		char name[200];
		i = 0;
		// parse spells
		node = get_XML_node(root->children, "Spell_list");
		node = get_XML_node(node->children, "spell");
		while (node) {
			int j;
			// Pour gerer correctement les accents dans ce fichier
			char *nom_fr = NULL;
			char *desc_fr = NULL;
			memset(name, 0, sizeof(name));
			data = get_XML_node(node->children, "name");
			if (data == 0) {
				LOG_ERROR("No name for %d spell", i);
			}
			nom_fr = fromUTF8(data->children->content, strlen((const char *)data->children->content));
			safe_strncpy(spells_list[i].name, nom_fr, sizeof(spells_list[i].name));
			if (nom_fr) {
				free(nom_fr);
			}
			data = get_XML_node(node->children, "desc");
			if (data == 0) {
				LOG_ERROR("No desc for spell '%s'[%d]", name, i);
			}
			desc_fr = fromUTF8(data->children->content, strlen((const char *)data->children->content));
			safe_strncpy(spells_list[i].desc, desc_fr, sizeof(spells_list[i].desc));
			if (desc_fr) {
				free(desc_fr);
			}
			data = get_XML_node(node->children, "id");
			if (data == 0) {
				LOG_ERROR("No id for spell '%s'[%d]", name, i);
			}
			spells_list[i].id = get_int_value(data);
			data = get_XML_node(node->children, "icon");
			if (data == 0) {
				LOG_ERROR("No icon for spell '%s'[%d]", name, i);
			}
			spells_list[i].image = get_int_value(data);
			data = get_XML_node(node->children, "mana");
			if (data == 0) {
				LOG_ERROR("No mana for spell '%s'[%d]", name, i);
			}
			spells_list[i].mana = get_int_value(data);
			data = get_XML_node(node->children, "lvl");
			if (data == 0) {
				LOG_ERROR("No lvl for spell '%s'[%d]", name, i);
			}
			j = 0;
			while (data) {
				const char *skill = get_string_property(data, "skill");
				spells_list[i].lvls_req[j] = get_int_value(data);
				spells_list[i].lvls[j] = get_skill_address(skill);
				j++;
				data = get_XML_node(data->next, "lvl");
			}
			data = get_XML_node(node->children, "group");
			if (data == 0) {
				LOG_ERROR("No group for spell '%s'[%d]", name, i);
			}
			while (data) {
				int g;
				g = get_int_value(data);
				groups_list[g].spells_id[groups_list[g].spells] = i;
				groups_list[g].spells++;
				data = get_XML_node(data->next, "group");
			}
			data = get_XML_node(node->children, "sigil");
			if (data == 0) {
				LOG_ERROR("No sigil for spell '%s'[%d]", name, i);
			}
			j = 0;
			while (data) {
				spells_list[i].sigils[j] = get_int_value(data);
				j++;
				data = get_XML_node(data->next, "sigil");
			}
			data = get_XML_node(node->children, "reagent");
			if (data == 0) {
				LOG_ERROR("No reagent for spell '%s'[%d]", name, i);
			}
			j = 0;
			while (data) {
				int tmpval = -1;
				spells_list[i].reagents_id[j] = get_int_property(data, "id");
				if ((tmpval = get_int_property(data, "uid")) >= 0) {
					spells_list[i].reagents_uid[j] = (Uint16)tmpval;
				}
				spells_list[i].reagents_qt[j] = get_int_value(data);
				j++;
				data = get_XML_node(data->next, "reagent");
			}
			node = get_XML_node(node->next, "spell");
			i++;
		}
		num_spells = i;
		// parse sigils
		node = get_XML_node(root->children, "Sigil_list");
		node = get_XML_node(node->children, "sigil");
		while (node) {
			char *runes_fr = NULL;
			int k;
			k = get_int_property(node, "id");
			sigils_list[k].sigil_img = k;
			runes_fr = fromUTF8(node->children->content, strlen((const char *)node->children->content));
			safe_strncpy(sigils_list[k].name, runes_fr, sizeof(sigils_list[k].name));
			if (runes_fr) {
				free(runes_fr);
			}
			data = get_XML_node(node->children, "id");
			sigils_list[k].have_sigil = 1;
			node = get_XML_node(node->next, "sigil");
		}
		// parse necro
		node = get_XML_node(root->children, "Necro_list");
		node = get_XML_node(node->children, "necro");
		while (node) {
			int k = get_int_property(node, "id"); // id de la commande
			int img = get_int_property(node, "img"); // n° de l'image pour le fichier texture gamebuttons
			necro_list[k].necro_img = img;
			get_string_value(necro_list[k].description, sizeof(necro_list[k].description), node); // description du sort
			safe_strncpy((char *)necro_list[k].name, get_string_property(node, "name"), sizeof(necro_list[k].name)); // #commande
			node = get_XML_node(node->next, "necro");
		}
		// Soin des invoqués :
		safe_strncpy((char *)necro_list[5].description, "Soigner", sizeof(necro_list[5].description));
		// parse groups
		num_groups = 0;
		node = get_XML_node(root->children, "Groups");
		node = get_XML_node(node->children, "group");
		while (node) {
			char *groupes_fr = NULL;
			int k;
			k = get_int_property(node, "id");
			groupes_fr = fromUTF8(node->children->content, strlen((const char *)node->children->content));
			safe_strncpy((char *)groups_list[k].desc, groupes_fr, sizeof(groups_list[k].desc));
			if (groupes_fr) {
				free(groupes_fr);
			}
			num_groups++;
			node = get_XML_node(node->next, "group");
		}
	}
	xmlFreeDoc(doc);
	// init arrays
	for (i = 0; i < 6; i++) {
		on_cast[i] = -1;
	}
	for (i = 0; i < NUM_ACTIVE_SPELLS; i++) {
		active_spells[i].spell = -1;
		active_spells[i].cast_time = 0;
		active_spells[i].duration = 0;
		if (active_spells[i].sound > 0) {
			stop_sound(active_spells[i].sound);
		}
	}
	if (!ok) { // xml failed, init sigils manually
		init_sigils();
	}
	init_ok = ok;
	return ok;
}
void check_castability() {
	int i, j, k, l;
	for (i = 0; i < num_spells; i++) {
		spells_list[i].uncastable = 0;
		// Check Mana
		if (have_stats && your_info.ethereal_points.cur < spells_list[i].mana) {
			spells_list[i].uncastable |= UNCASTABLE_MANA;
		}
		// Check Sigils
		for (j = 0; j < 6; j++) {
			k = spells_list[i].sigils[j];
			if (k >= 0 && !sigils_list[k].have_sigil) {
				spells_list[i].uncastable |= UNCASTABLE_SIGILS;
			}
		}
		// Check Reagents
		for (j = 0; j < 4 && spells_list[i].reagents_id[j] >= 0; j++) {
			l = 0;
			for (k = 0; k < ITEM_WEAR_START; k++) {
				if ((item_list[k].quantity > 0) && (item_list[k].image_id == spells_list[i].reagents_id[j]) && ((item_list[k].id == unset_item_uid) || (spells_list[i].reagents_uid[j] == unset_item_uid) || (item_list[k].id == spells_list[i].reagents_uid[j]))) {
					l = 1;
					if (item_list[k].quantity < spells_list[i].reagents_qt[j]) {
						spells_list[i].uncastable |= UNCASTABLE_REAGENTS;
						break;
					}
				}
			}
			if (!l) {
				// no reagent j found
				spells_list[i].uncastable |= UNCASTABLE_REAGENTS;
			}
		}
		// Check Levels
		for (j = 0; j < NUM_WATCH_STAT && spells_list[i].lvls[j]; j++) {
			if (spells_list[i].lvls[j]) {
				if (spells_list[i].lvls[j]->cur < spells_list[i].lvls_req[j]) {
					spells_list[i].uncastable |= UNCASTABLE_LVLS;
				}
			}
		}
	}
	// when castabilitychanges, update spell_help
	set_spell_help_text(we_have_spell);
}
// ACTIVE SPELLS
// durée restante obtenue du serveur (l'US utilise le fichier XML)
void get_active_spell(int pos, int spell, int timer) {
	active_spells[pos].spell = spell;
	// mémorisation de la date de fin plutot que celle du début
	active_spells[pos].cast_time = cur_time + timer * 1000;
	active_spells[pos].sound = add_spell_sound(spell);
	// conservation de la durée totale (max) plutot que celle restante
	if (timer > active_spells[pos].duration) {
		active_spells[pos].duration = timer;
	}
}
void remove_active_spell(int pos) {
#if defined(BUFF_DURATION_DEBUG)
	if (active_spells[pos].duration > 0) {
		duration_debug(active_spells[pos].spell, diff_game_time_sec(active_spells[pos].cast_time), "actual duration");
	}
#endif
	if (active_spells[pos].spell == 2) {
		active_spells[pos].spell = -1;
	}
	active_spells[pos].cast_time = 0;
	active_spells[pos].duration = 0;
	if (active_spells[pos].sound > 0) {
		stop_sound(active_spells[pos].sound);
	}
}
void get_active_spell_list(const Uint8 *my_spell_list) {
	Uint32 i;
	for (i = 0; i < NUM_ACTIVE_SPELLS; i++) {
		// le serveur FR envoit des données différentes
		active_spells[i].spell = my_spell_list[i * 3];
		// récupération de la durée restante
		active_spells[i].duration = SDL_SwapLE16(*((Uint16 *)(my_spell_list + i * 3 + 1)));
		if (active_spells[i].spell < 0) {
			active_spells[i].duration = 0;
		}
		// mémorisation de la date de fin du sort
		active_spells[i].cast_time = cur_time + active_spells[i].duration * 1000;
		active_spells[i].sound = add_spell_sound(active_spells[i].spell);
	}
}
void restart_active_spell_sounds(void) {
	Uint32 i;
	for (i = 0; i < NUM_ACTIVE_SPELLS; i++) {
		if (active_spells[i].sound > 0) {
			stop_sound(active_spells[i].sound);
		}
		if (active_spells[i].spell != -1) {
			active_spells[i].sound = add_spell_sound(active_spells[i].spell);
		}
	}
}
int we_are_poisoned() {
	Uint32 i;
	for (i = 0; i < NUM_ACTIVE_SPELLS; i++) {
		if (active_spells[i].spell == 2) {
			return 1;
		}
	}
	return 0;
}
// détermine la couleur de la jauge
// un pourcentage négatif indique une inversion du dégradé (rouge->vert)
void set_timer_color(float percent, float multiplier) {
	float r, g;
	if (percent < 0) {
		percent += 1.0f;
	}
	r = (1.0f - percent) * 2.5f;
	g = (percent / 3.0f) * 5.0f;
	if (r < 0.0f) {
		r = 0.0f;
	} else if (r > 1.0f) {
		r = 1.0f;
	}
	if (g < 0.0f) {
		g = 0.0f;
	} else if (g > 1.0f) {
		g = 1.0f;
	}
	glColor3f(r * multiplier, g * multiplier, 0.0f);
}
void time_out(const float x_start, const float y_start, const float gridsize, const float progress) {
	glDisable(GL_TEXTURE_2D);
	glColor3f(0.0f, 0.7f, 0.0f);
	glBegin(GL_QUADS);
	// Affichage d'une jauge avec coloration variable
	set_timer_color(progress, 1.0f);
	glVertex2f(x_start, y_start + gridsize);
	glVertex2f(x_start, y_start + gridsize - gridsize * fabs(progress));
	set_timer_color(progress, 0.3f);
	glVertex2f(x_start + 5, y_start + gridsize - gridsize * fabs(progress));
	glVertex2f(x_start + 5, y_start + gridsize);
	glEnd();
	glEnable(GL_TEXTURE_2D);
	glColor3f(1.0f, 1.0f, 1.0f);
}
void display_spells_we_have() {
	Uint32 i;
	float scale, duration, cur_time;
	// numéro de position globale incrémentée ne laissant pas de trou entre les sorts
	int cur_pos = 0;
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	cur_time = SDL_GetTicks();
	// ok, now let's draw the objects...
	for (i = 0; i < NUM_ACTIVE_SPELLS; i++) {
		if (active_spells[i].spell != -1) {
			int cur_spell;
			char string[10];
			int x_start, y_start;
			// get the UV coordinates.
			cur_spell = active_spells[i].spell + 32; // the first 32 icons are the sigils
			// get the x and y
			cur_pos++;
			x_start = 8; // la jauge prend place de 0 à 5
			y_start = window_height - 120 - 36 * (cur_pos); // ne pas tenir compte du hud_y
			// calcul du temps restant en secondes
			duration = (active_spells[i].cast_time - cur_time + 980) / 1000.0f;
			if (duration < 0) {
				continue;
			}
			sprintf(string, "%u", (int)duration);
			// affichage d'une jauge colorée
			scale = (active_spells[i].duration) ? (duration / active_spells[i].duration) : 0;
			if (active_spells[i].spell == 2) {
				scale = -scale;
			}
			time_out(0, y_start, 32, scale);
			// clignotement sur les 5 dernières secondes
			if ((duration <= 5) && ((int)(cur_time / 250) % 2)) {
				draw_string_small_shadowed(x_start + 16 - 4 * strlen(string), y_start + 10, (unsigned char *)string, 1, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
				continue;
			}
			glEnable(GL_BLEND);
			draw_spell_icon(cur_spell, x_start, y_start, 32, 0, 0);
			glDisable(GL_BLEND);
			// affichage du temps restant par dessus l'icone
			draw_string_small_shadowed(x_start + 16 - 4 * strlen(string), y_start + 10, (unsigned char *)string, 1, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f);
		}
	}
}
int show_last_spell_help = 0;
// DISPLAY HANDLERS
int draw_switcher(window_info *win) {
	glDisable(GL_TEXTURE_2D);
	glEnable(GL_POINT_SMOOTH);
	glPointSize(ELW_BOX_SIZE - 8);
	glColor3f(0.77f, 0.57f, 0.39f);
	glBegin(GL_POINTS);
	glVertex3i(win->len_x - ELW_BOX_SIZE * 0.5, ELW_BOX_SIZE * 1.5, 0);
	glVertex3i(win->len_x - ELW_BOX_SIZE * 0.5, ELW_BOX_SIZE * 2.5, 0);
	glVertex3i(win->len_x - ELW_BOX_SIZE * 0.5, ELW_BOX_SIZE * 3.5, 0);
	glVertex3i(win->len_x - ELW_BOX_SIZE * 0.5, ELW_BOX_SIZE * 4.5, 0);
	glEnd();
	glPointSize(ELW_BOX_SIZE - 10);
	glColor3f(0.0f, 0.0f, 0.0f);
	glBegin(GL_POINTS);
	glVertex3i(win->len_x - ELW_BOX_SIZE * 0.5, ELW_BOX_SIZE * 1.5, 0);
	glVertex3i(win->len_x - ELW_BOX_SIZE * 0.5, ELW_BOX_SIZE * 2.5, 0);
	glVertex3i(win->len_x - ELW_BOX_SIZE * 0.5, ELW_BOX_SIZE * 3.5, 0);
	glVertex3i(win->len_x - ELW_BOX_SIZE * 0.5, ELW_BOX_SIZE * 4.5, 0);
	glEnd();
	glPointSize(ELW_BOX_SIZE - 14);
	glColor3f(0.77f, 0.57f, 0.39f);
	glBegin(GL_POINTS);
	if (sigil_win == sigils_win) {
		glVertex3i(win->len_x - ELW_BOX_SIZE * 0.5, ELW_BOX_SIZE * 1.5, 0);
	} else if (sigil_win == spell_mini_win) {
		glVertex3i(win->len_x - ELW_BOX_SIZE * 0.5, ELW_BOX_SIZE * 2.5, 0);
	} else if (sigil_win == spell_win) {
		glVertex3i(win->len_x - ELW_BOX_SIZE * 0.5, ELW_BOX_SIZE * 3.5, 0);
	} else if (sigil_win == necro_win) {
		glVertex3i(win->len_x - ELW_BOX_SIZE * 0.5, ELW_BOX_SIZE * 4.5, 0);
	}
	glEnd();
	glPointSize(1.0);
	glDisable(GL_POINT_SMOOTH);
	glEnable(GL_TEXTURE_2D);
	return 1;
}
void draw_spell_icon(int id, int x_start, int y_start, int gridsize, int alpha, int grayed) {
	float u_start, v_start, u_end, v_end;
	u_start = 0.125f / 2 * (id % 16);
	v_start = 0.125f / 2 * (id / 16);
	u_end = u_start + 0.125f / 2;
	v_end = v_start + 0.125f / 2;
	bind_texture(sigils_text);
	if (alpha) {
		glEnable(GL_ALPHA_TEST);
		glAlphaFunc(GL_GREATER, 0.05f);
		glBegin(GL_QUADS);
		draw_2d_thing(u_start, v_start, u_end, v_end, x_start, y_start, x_start + gridsize, y_start + gridsize);
		glEnd();
		glDisable(GL_ALPHA_TEST);
	} else {
		glBegin(GL_QUADS);
		draw_2d_thing(u_start, v_start, u_end, v_end, x_start, y_start, x_start + gridsize, y_start + gridsize);
		glEnd();
	}
	if (grayed) {
		gray_out(x_start, y_start, gridsize);
	}
}
void draw_current_spell(int x, int y, int sigils_too) {
	glEnable(GL_TEXTURE_2D);
	glColor3f(1.0f, 1.0f, 1.0f);
	if (we_have_spell >= 0) {
		int i, j;
		unsigned char str[4];
		// we have a current spell (cliked or casted) !!mqb_data[0] can still be null!!
		j = we_have_spell;
		draw_spell_icon(spells_list[j].image, x, y, 32, 1, 0);
		if (sigils_too) {
			// draw sigils
			x += 33 * 2;
			for (i = 0; i < 6; i++) {
				if (spells_list[j].sigils[i] < 0) {
					break;
				}
				draw_spell_icon(spells_list[j].sigils[i], x + 33 * i, y, 32, 0, spells_list[j].uncastable & UNCASTABLE_SIGILS);
				if (spells_list[j].uncastable & UNCASTABLE_SIGILS && !sigils_list[spells_list[j].sigils[i]].have_sigil) {
					gray_out(x + 33 * i, y, 32);
				}
			}
		}
		// draw reagents
		x += (sigils_too) ? (33 * 6 + 33):(33 + 16);
		// Modification car sur les sorts avec 4 essences différentes
		// l'affichage pose soucis
		for (i = 0; spells_list[j].reagents_id[i] > 0 && i < 4; i++) {
			draw_item(spells_list[j].reagents_id[i], x + 33 * i, y, 33);
			safe_snprintf((char *)str, sizeof(str), "%i", spells_list[j].reagents_qt[i]);
			draw_string_small_shadowed(x + 33 * i, y + 21, (unsigned char *)str, 1, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f);
			if (spells_list[j].uncastable & UNCASTABLE_REAGENTS) {
				gray_out(x + 33 * i, y + 1, 32);
			}
		}
		// draw mana
		x += (sigils_too) ? (33 * 5):(33 * 4 + 17);
		safe_snprintf((char *)str, sizeof(str), "%i", spells_list[j].mana);
		if (spells_list[j].uncastable & UNCASTABLE_MANA) {
			glColor3f(1.0f, 0.0f, 0.0f);
		} else {
			glColor3f(0.0, 1.0, 0.0);
		}
		i = (33 - get_string_width(str)) / 2;
		j = (33 - get_char_width(str[0])) / 2;
		draw_string(x + i, y + j, str, 1);
	}
	// draw strings
	x = 20;
	glColor3f(0.77f, 0.57f, 0.39f);
	if (sigils_too) {
		x += 33 * 2;
		draw_string_small(x, y - 15, (unsigned char *)"Runes", 1);
		x += 33 * 6 + 33;
	} else {
		x += 33 + 16;
	}
	draw_string_small(x, y - 15, (unsigned char *)"Essences", 1);
	x += 33 * 4 + ((sigils_too) ? (33):(17));
	draw_string_small(x, y - 15, (unsigned char *)"Mana", 1);
	// draw grids
	glDisable(GL_TEXTURE_2D);
	x = 20;
	if (sigils_too) {
		x += 33 * 2;
		rendergrid(6, 1, x, y, 33, 33);
		x += 33 * 6 + 33;
	} else {
		x += 33 + 16;
	}
	rendergrid(4, 1, x, y, 33, 33);
	x += 33 * 4 + ((sigils_too) ? (33):(17));
	rendergrid(1, 1, x, y, 33, 33);
}
int display_sigils_handler(window_info *win) {
	int i;
	int x_start, y_start;
	if (init_ok) {
		draw_switcher(win);
	}
	glEnable(GL_TEXTURE_2D);
	glColor3f(1.0f, 1.0f, 1.0f);
	// let's add the new spell icon if we have one
	x_start = 350;
	y_start = 112;
	if (mqb_data[0] && mqb_data[0]->spell_id != -1) {
		draw_spell_icon(mqb_data[0]->spell_image, x_start, y_start, 32, 1, 0);
	}
	// ok, now let's draw the objects...
	for (i = 0; i < SIGILS_NO; i++) {
		if (sigils_list[i].have_sigil) {
			// get the x and y
			x_start = 33 * (i % NUM_SIGILS_LINE) + 1;
			y_start = 33 * (i / NUM_SIGILS_LINE);
			draw_spell_icon(sigils_list[i].sigil_img, x_start, y_start, 32, 0, 0);
		}
	}
	// ok, now let's draw the sigils on the list
	for (i = 0; i < 6; i++) {
		if (on_cast[i] != -1) {
			// get the x and y
			x_start = 33 * (i % 6) + 5;
			y_start = sigil_y_len - 37;
			draw_spell_icon(on_cast[i], x_start, y_start, 32, 0, 0);
		}
	}
	// now, draw the inventory text, if any.
	draw_string_small(4, sigil_y_len - 90, spell_text, 4);
	// Render the grid *after* the images. It seems impossible to code
	// it such that images are rendered exactly within the boxes on all
	// cards
	glDisable(GL_TEXTURE_2D);
	glColor3f(0.77f, 0.57f, 0.39f);
	rendergrid(NUM_SIGILS_LINE, NUM_SIGILS_ROW, 0, 0, 33, 33);
	rendergrid(6, 1, 5, sigil_y_len - 37, 33, 33);
	glEnable(GL_TEXTURE_2D);
	if (show_last_spell_help && mqb_data[0] && mqb_data[0]->spell_id != -1) {
		show_help(mqb_data[0]->spell_name, 350 - 8 * strlen(mqb_data[0]->spell_name), 120);
	}
	show_last_spell_help = 0;
	return 1;
}
int display_spells_handler(window_info *win) {
	int i, j, k, x, y;
	draw_switcher(win);
	// Draw spell groups
	for (i = 0; i < num_groups; i++) {
		x = groups_list[i].x;
		y = groups_list[i].y;
		glEnable(GL_TEXTURE_2D);
		glColor3f(1.0f, 1.0f, 1.0f);
		draw_string_small(x, y - 15, groups_list[i].desc, 1);
		for (k = 0, j = 0; j < groups_list[i].spells; j++) {
			draw_spell_icon(spells_list[groups_list[i].spells_id[j]].image, x + 33 * (k % SPELLS_ALIGN_X), y + 33 * (k / SPELLS_ALIGN_X), 32, 0, spells_list[groups_list[i].spells_id[j]].uncastable);
			k++;
		}
		glDisable(GL_TEXTURE_2D);
		glColor3f(0.77f, 0.57f, 0.39f);
		rendergrid(SPELLS_ALIGN_X, groups_list[i].spells / (SPELLS_ALIGN_X + 1) + 1, x, y, 33, 33);
	}
	glEnable(GL_TEXTURE_2D);
	// draw spell text & help
	glColor3f(1.0f, 1.0f, 1.0f);
	draw_string_small(20, spell_y_len - 100, spell_text, 4);
	draw_string_small(20, spell_y_len + 5, spell_help, 3);
	// draw the bottom bar
	draw_current_spell(20, spell_y_len - 37, 1);
	if (we_have_spell >= 0 && spells_list[we_have_spell].uncastable) {
		// not castable
		glColor3f(1.0f, 0.0f, 0.0f);
		rendergrid(1, 1, 20, spell_y_len - 37, 33, 33);
	}
	return 1;
}
int display_spells_mini_handler(window_info *win) {
	int i, j, x, y, cg, cs;
	draw_switcher(win);
	glEnable(GL_TEXTURE_2D);
	x = 20;
	y = 10;
	glColor3f(1.0f, 1.0f, 1.0f);
	for (i = 0, cs = 0, cg = 0; i < spell_mini_rows; i++) {
		for (j = 0; j < SPELLS_ALIGN_X; j++) {
			if (cs == groups_list[cg].spells) {
				cs = 0;
				cg++;
				break;
			}
			draw_spell_icon(spells_list[groups_list[cg].spells_id[cs]].image, x + j * 33, y + 33 * i, 32, 0, spells_list[groups_list[i].spells_id[j]].uncastable);
			cs++;
			// @tosh : correction d'un bug qui survient si la ligne est remplie de runes.
			if (j == SPELLS_ALIGN_X - 1) {
				cg++;
				cs = 0;
			}
		}
	}
	// draw spell help
	if (on_spell == -2) {
		// mouse over the bottom-left selected spell icon, show uncastability
		int l = (int)(get_string_width((unsigned char *)GET_UNCASTABLE_STR(spells_list[we_have_spell].uncastable)) * (float)DEFAULT_SMALL_RATIO);
		SET_COLOR(c_red2);
		draw_string_small(20 + (33 * SPELLS_ALIGN_X - l) / 2, spell_mini_y_len - 37 - 35, (unsigned char *)GET_UNCASTABLE_STR(spells_list[we_have_spell].uncastable), 1);
	} else {
		i = (on_spell >= 0) ? (on_spell):(we_have_spell);
		if (i >= 0) {
			int l = (int)(get_string_width((unsigned char *)spells_list[i].name) * (float)DEFAULT_SMALL_RATIO);
			if (on_spell >= 0) {
				SET_COLOR(c_grey1);
			} else {
				SET_COLOR(c_green3);
			}
			draw_string_small(20 + (33 * SPELLS_ALIGN_X - l) / 2, spell_mini_y_len - 37 - 35, (unsigned char *)spells_list[i].name, 1);
		}
	}
	// draw the current spell
	draw_current_spell(x, spell_mini_y_len - 37, 0);
	glDisable(GL_TEXTURE_2D);
	glColor3f(0.77f, 0.57f, 0.39f);
	rendergrid(SPELLS_ALIGN_X, spell_mini_rows, x, y, 33, 33);
	if (we_have_spell >= 0 && spells_list[we_have_spell].uncastable) {
		// not castable, red grid
		glColor3f(1.0f, 0.0f, 0.0f);
		rendergrid(1, 1, 20, spell_mini_y_len - 37, 33, 33);
	}
	glEnable(GL_TEXTURE_2D);
	return 1;
}
int display_necro_handler(window_info *win) {
	int i, j;
	float u_start, v_start, u_end, v_end;
	Uint8 nombre[80];
	int grille_x;
	int grille_y;
	int place_fleches_x;
	int place_fleches_y;
	int texte_options_x;
	int texte_options_y;
	int double_invoc_x;
	int double_invoc_y;
	int blocage_invoc_x;
	int blocage_invoc_y;
	int messages_invoc_x;
	int messages_invoc_y;
	int objet_recette[6][3] = {0};
	int total = 0;
	if (nb_recettes_necro > 0) {
		blocage_invoc_x = 13;
		blocage_invoc_y = 80 - 26;
		texte_options_x = 23;
		texte_options_y = 73;
		grille_x = 12;
		grille_y = 94 - 12;
		place_fleches_x = grille_x + 198;
		place_fleches_y = grille_y + 1;
		double_invoc_x = 13;
		double_invoc_y = 175 - 15;
		messages_invoc_x = 6;
		messages_invoc_y = 228 - 15;
		total = liste_items_necro[creature_en_cours][0];
		for (i = 0; i < total; i++) {
			objet_recette[i][0] = liste_items_necro[creature_en_cours][i + 1];
		}
		if (double_invoc) {
			total++;
			objet_recette[total - 1][0] = 41;
		}
		for (i = 0; i < 36; i++) {
			if (item_list[i].quantity > 0) {
				for (j = 0; j < total; j++) {
					if (item_list[i].id == objet_recette[j][0]) {
						objet_recette[j][1] = item_list[i].quantity;
						objet_recette[j][2] = item_list[i].pos;
					}
				}
			}
		}
	}
	draw_switcher(win);
	bind_texture(divers_text);
	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER, 0.05f);
	glBegin(GL_QUADS);
	for (i = 0 ; i < NECRO_NO ; i++) {
		u_start = 0.125f * (necro_list[i].necro_img % 8);
		u_end = u_start + 0.125f;
		v_start = ((float)32 / 256 * (necro_list[i].necro_img / 8));
		v_end = v_start + 0.125f;
		if (i >= 4) {
			j = 6;
		} else {
			if (i >= 2) {
				j = 3;
			} else {
				j = 0;
			}
		}
		draw_2d_thing(u_start, v_start, u_end, v_end, NECRO_SPACE_Y + i * (33 + NECRO_SPACE_Y_BETWEEN) + NECRO_SPACE_Y_BETWEEN * j, NECRO_SPACE_X, NECRO_SPACE_Y + i * (33 + NECRO_SPACE_Y_BETWEEN) + NECRO_SPACE_Y_BETWEEN * j + 33, NECRO_SPACE_X + 33);
	}
	// ajout du sort de soin des invoqués :
	glEnd();
	glDisable(GL_ALPHA_TEST);
	bind_texture_id(sigils_text); // icones de sorts
	pos_y_sdi = NECRO_SPACE_X;
	pos_x_sdi = NECRO_SPACE_Y + 5 * (33 + NECRO_SPACE_Y_BETWEEN) + NECRO_SPACE_Y_BETWEEN * 5;
	draw_spell_icon(spells_list[groups_list[1].spells_id[4]].image, pos_x_sdi, pos_y_sdi, 32, 0, spells_list[groups_list[1].spells_id[4]].uncastable);
	// on affiche le cadre :
	glDisable(GL_TEXTURE_2D);
	glColor3f(0.77f, 0.57f, 0.39f);
	rendergrid(1, 1, pos_x_sdi, pos_y_sdi, 34, 34);
	glEnable(GL_TEXTURE_2D);
	// on écrit la légende de l'action sélectionnée
	glColor3f(1.0f, 1.0f, 1.0f);
	if (mouseover_necro != -1) {
		draw_string_small(NECRO_SPACE_X, NECRO_SPACE_Y * 2 + 33 + 10, (unsigned char *)necro_list[mouseover_necro].description, 1);
	}
	if (nb_recettes_necro > 0) {
		if (last_items_string_id != inventory_item_string_id) {
			put_small_text_in_box((unsigned char *)inventory_item_string, strlen(inventory_item_string), necro_x_len - 12, items_string);
			last_items_string_id = inventory_item_string_id;
		}
		draw_string_small(messages_invoc_x, messages_invoc_y, (unsigned char *)items_string, 4);
		glDisable(GL_TEXTURE_2D);
		glColor3f(0.77f, 0.57f, 0.39f);
		rendergrid(6, 1, grille_x, grille_y, 32, 32);
		glColor3f(0.77f, 0.57f, 0.39f);
		glBegin(GL_QUADS);
		glVertex3i(place_fleches_x + 4, place_fleches_y, 0); // top
		glVertex3i(place_fleches_x, place_fleches_y + 10, 0); // left
		glVertex3i(place_fleches_x + 8, place_fleches_y + 10, 0); // right
		glVertex3i(place_fleches_x + 4, place_fleches_y, 0); // top
		glEnd();
		glBegin(GL_QUADS);
		glVertex3i(place_fleches_x + 4, place_fleches_y + 21 + 10, 0); // top
		glVertex3i(place_fleches_x, place_fleches_y + 21, 0); // left
		glVertex3i(place_fleches_x + 8, place_fleches_y + 21, 0); // right
		glVertex3i(place_fleches_x + 4, place_fleches_y + 21 + 10, 0); // top
		glEnd();
		glEnable(GL_POINT_SMOOTH);
		glPointSize(12);
		glColor3f(0.77f, 0.57f, 0.39f);
		glBegin(GL_POINTS);
		glVertex3i(double_invoc_x, double_invoc_y, 0);
		glEnd();
		glBegin(GL_POINTS);
		glVertex3i(blocage_invoc_x, blocage_invoc_y, 0);
		glEnd();
		glPointSize(20 - 10);
		glColor3f(0.0f, 0.0f, 0.0f);
		glBegin(GL_POINTS);
		glVertex3i(double_invoc_x, double_invoc_y, 0);
		glEnd();
		glBegin(GL_POINTS);
		glVertex3i(blocage_invoc_x, blocage_invoc_y, 0);
		glEnd();
		glPointSize(20 - 14);
		glColor3f(0.77f, 0.57f, 0.39f);
		glBegin(GL_POINTS);
		if (double_invoc) {
			glVertex3i(double_invoc_x, double_invoc_y, 0);
		}
		if (securite_invoc) {
			glVertex3i(blocage_invoc_x, blocage_invoc_y, 0);
		}
		glEnd();
		glEnable(GL_TEXTURE_2D);
		glColor3f(1.0f, 1.0f, 1.0f);
		for (i = 0; i < liste_items_necro[creature_en_cours][0]; i++) {
			glColor3f(1.0f, 1.0f, 1.0f);
			draw_item((liste_items_necro[creature_en_cours][i + 1 + liste_items_necro[creature_en_cours][0]]), grille_x + i * 32, grille_y + 1, 32);
			safe_snprintf((char *)nombre, sizeof(nombre), "%i", (liste_items_necro[creature_en_cours][i + 1 + liste_items_necro[creature_en_cours][0] * 2]));
			if (objet_recette[i][1] >= (liste_items_necro[creature_en_cours][i + 1 + liste_items_necro[creature_en_cours][0] * 2])) {
				draw_string_small_shadowed(grille_x + i * 32, grille_y + 1, nombre, 1, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f);
			} else {
				draw_string_small_shadowed(grille_x + i * 32, grille_y + 1, nombre, 1, 0.8f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
			}
		}
		if (double_invoc) {
			glColor3f(1.0f, 1.0f, 1.0f);
			draw_item(60, grille_x + (liste_items_necro[creature_en_cours][0]) * 32, grille_y + 1, 32);
			safe_snprintf((char *)nombre, sizeof(nombre), "%i", 1);
			if (objet_recette[total - 1][1] >= 1) {
				draw_string_small_shadowed(grille_x + (liste_items_necro[creature_en_cours][0]) * 32, grille_y + 1, nombre, 1, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f);
			} else {
				draw_string_small_shadowed(grille_x + (liste_items_necro[creature_en_cours][0]) * 32, grille_y + 1, nombre, 1, 0.8f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
			}
		}
		glColor3f(0.77f, 0.57f, 0.39f);
		draw_string_small(texte_options_x, texte_options_y - 26, (unsigned char *)("Bloquer tuer invocations"), 1);
		draw_string_small(texte_options_x - 10, texte_options_y + 60 - 15, (unsigned char *)("Invocation:"), 1);
		glColor3f(0.2f, 1.0f, 0.2f);
		draw_string_small(texte_options_x - 10, texte_options_y + 75 - 15, (unsigned char *)(nom_bestiole[creature_en_cours]), 1);
		glColor3f(0.77f, 0.57f, 0.39f);
		draw_string_small(texte_options_x, texte_options_y + 95 - 15, (unsigned char *)("Double Invocation"), 1);
	}
	return 1;
}
int chargement_necro_recettes(int choix) {
	int encours = 0;
	int k = 0;
	char prov[30] = "";
	int bestiole_en_cours = 0;
	int nb_item_bestiole_en_cours;
	int monstre = -1;
	int prochain_monstre = -1;
	FILE *fichnecro;
	fichnecro = NULL;
	fichnecro = fopen("creatnecro", "r");
	if (fichnecro != NULL) {
		while (fgets(prov, 30, fichnecro) != NULL) {
			if (k == 0) {
				monstre = 1;
				prochain_monstre = 1;
				bestiole_en_cours = -1;
			} else {
				if (k == prochain_monstre) {
					monstre = prochain_monstre;
					bestiole_en_cours += 1;
					if (choix) {
						nom_bestiole[bestiole_en_cours] = strdup(prov);
					}
				} else {
					if (k == monstre + 1) {
						nb_item_bestiole_en_cours = atoi(prov);
						prochain_monstre = monstre + (nb_item_bestiole_en_cours * 3) + 2;
						if (choix) {
							liste_items_necro[bestiole_en_cours][0] = nb_item_bestiole_en_cours; // total objets différents
						}
						encours = monstre + 1;
					} else {
						if (choix) {
							liste_items_necro[bestiole_en_cours][k - encours] = atoi(prov);
						}
					}
				}
			}
			k++;
		}
		return bestiole_en_cours + 1; // fichier ok
	} else {
		return -1; // Pas de fichier
	}
}
// CLICK HANDLERS
int switch_handler(int new_win) {
	window_info *win;
	int this_win;
	last_win = sigil_win;
	this_win = new_win;
	win = &windows_list.window[last_win];
	windows_list.window[this_win].opaque = windows_list.window[last_win].opaque;
	hide_window(last_win);
	// positionnement en conservant l'alignement à droite (pour rester sur les switchs)
	move_window(this_win, win->pos_id, win->pos_loc, win->pos_x + win->len_x - windows_list.window[this_win].len_x, win->pos_y);
	show_window(this_win);
	select_window(this_win);
	sigil_win = this_win;
	if (new_win == sigils_win) {
		start_mini_spells = 0;
	} else if (new_win == spell_mini_win) {
		start_mini_spells = 1;
	} else if (new_win == spell_win) {
		start_mini_spells = 2;
	} else if (new_win == necro_win) {
		start_mini_spells = 3;
	}
	return 1;
}
int mouseover_switcher_handler(window_info *win, int mx, int my) {
	if (mx >= win->len_x - ELW_BOX_SIZE && my > ELW_BOX_SIZE) {
		if (my < ELW_BOX_SIZE * 2) {
			show_help("Liste des Runes", win->len_x + 5, ELW_BOX_SIZE * 1);
		} else if (my < ELW_BOX_SIZE * 3) {
			show_help("Liste des Sorts", win->len_x + 5, ELW_BOX_SIZE * 2);
		} else if (my < ELW_BOX_SIZE * 4) {
			show_help("Liste détaillée", win->len_x + 5, ELW_BOX_SIZE * 3);
		} else if (my < ELW_BOX_SIZE * 5) {
			show_help("Nécromancie", win->len_x + 5, ELW_BOX_SIZE * 4);
		}
	}
	return 0;
}
int click_switcher_handler(window_info *win, int mx, int my, Uint32 flags) {
	int pos = 0;
	if (win->window_id == necro_win) {
		pos = get_mouse_pos_in_grid(mx, my, 1, 1, 12, 82, 181, 33);
		if (pos == 0) {
			if (flags & ELW_WHEEL_UP) {
				// haut
				if (nb_recettes_necro != -1) {
					if (creature_en_cours == nb_recettes_necro - 1) {
						creature_en_cours = -1;
					}
					creature_en_cours++;
				}
				return 0;
			} else if (flags & ELW_WHEEL_DOWN) {
				// bas
				if (nb_recettes_necro != -1) {
					if (creature_en_cours == 0) {
						creature_en_cours = nb_recettes_necro;
					}
					creature_en_cours--;
				}
				return 0;
			}
		}
	}
	if (mx >= win->len_x - ELW_BOX_SIZE && my > ELW_BOX_SIZE) {
		if (my < ELW_BOX_SIZE * 2) {
			if (sigil_win != sigils_win) {
				do_click_sound();
				switch_handler(sigils_win);
			}
		} else if (my < ELW_BOX_SIZE * 3) {
			if (sigil_win != spell_mini_win) {
				do_click_sound();
				switch_handler(spell_mini_win);
			}
		} else if (my < ELW_BOX_SIZE * 4) {
			if (sigil_win != spell_win) {
				do_click_sound();
				switch_handler(spell_win);
			}
		} else if (my < ELW_BOX_SIZE * 5) {
			if (sigil_win != necro_win) {
				do_click_sound();
				switch_handler(necro_win);
			}
		}
	}
	return 0;
}
int click_sigils_handler(window_info *win, int mx, int my, Uint32 flags) {
	// only handle real clicks, not scroll wheel moves
	if ((flags & ELW_MOUSE_BUTTON) == 0) {
		return 0;
	} else if (mx >= 350 && mx <= 381 && my >= 112 && my <= 143 && mqb_data[0] && mqb_data[0]->spell_id != -1) {
		add_spell_to_quickbar();
		return 1;
	} else if (mx > 0 && mx < NUM_SIGILS_LINE * 33 && my > 0 && my < NUM_SIGILS_ROW * 33) {
		int pos = get_mouse_pos_in_grid(mx, my, NUM_SIGILS_LINE, NUM_SIGILS_ROW, 0, 0, 33, 33);
		if (pos >= 0 && sigils_list[pos].have_sigil) {
			int j;
			int image_id = sigils_list[pos].sigil_img;
			// see if it is already on the list
			for (j = 0; j < 6; j++) {
				if (on_cast[j] == image_id) {
					return 1;
				}
			}
			for (j = 0; j < 6; j++) {
				if (on_cast[j] == -1) {
					on_cast[j] = image_id;
					return 1;
				}
			}
			return 1;
		}
	} else if (mx > 5 && mx < 6 * 33 + 5 && my > sigil_y_len - 37 && my < sigil_y_len - 5) {
		int pos = get_mouse_pos_in_grid(mx, my, 6, 1, 5, sigil_y_len - 37, 33, 33);
		if (pos >= 0) {
			on_cast[pos] = -1;
		}
	}
	if (init_ok) {
		click_switcher_handler(win, mx, my, flags);
	}
	return 0;
}
int click_spells_handler(window_info *win, int mx, int my, Uint32 flags) {
	int pos, i, the_group = -1, the_spell = -1;
	static int last_clicked = 0;
	static int last_pos = -1;
	if (!(flags & ELW_MOUSE_BUTTON)) {
		return 0;
	}
	for (i = 0; i < num_groups; i++) {
		pos = get_mouse_pos_in_grid(mx, my, SPELLS_ALIGN_X, groups_list[i].spells / (SPELLS_ALIGN_X + 1) + 1, groups_list[i].x, groups_list[i].y, 33, 33);
		if (pos >= 0 && pos < groups_list[i].spells) {
			the_group = i;
			the_spell = pos;
			break;
		}
	}
	if (the_spell != -1) {
		// a spell has been clicked
		int code_pos = (the_group * 256 + the_spell);
		we_have_spell = groups_list[the_group].spells_id[the_spell];
		put_on_cast();
		// handle double click && cast spell
		if (((SDL_GetTicks() - last_clicked) < 400) && last_pos == code_pos) {
			cast_handler();
		} else {
			have_error_message = 0; // if not double click, clear server msg
		}
		last_pos = code_pos;
	} else {
		last_pos = -1;
		// check spell icon
		if (we_have_spell >= 0 && mx > 20 && mx < 53 && my > spell_y_len - 37 && my < spell_y_len - 4) {
			if (flags & ELW_LEFT_MOUSE) {
				// cast spell
				if (put_on_cast()) {
					cast_handler();
				}
			} else if (flags & ELW_RIGHT_MOUSE) {
				// add to quickbar
				if (put_on_cast()) {
					prepare_for_cast();
					add_spell_to_quickbar();
				}
			}
		} else {
			click_switcher_handler(win, mx, my, flags);
		}
	}
	last_clicked = SDL_GetTicks();
	return 0;
}
int click_spells_mini_handler(window_info *win, int mx, int my, Uint32 flags) {
	int pos;
	static int last_clicked = 0;
	static int last_pos = -1;
	if (!(flags & ELW_MOUSE_BUTTON)) {
		return 0;
	}
	pos = get_mouse_pos_in_grid(mx, my, SPELLS_ALIGN_X, spell_mini_rows, 20, 10, 33, 33);
	if (pos >= 0) {
		int i, j, cs, cg, the_spell = -1, the_group = -1, the_pos = pos;
		// find the spell clicked
		for (i = 0, cs = 0, cg = 0; i < spell_mini_rows && the_pos >= 0; i++) {
			for (j = 0; j < SPELLS_ALIGN_X; j++) {
				the_pos--;
				if (the_pos == -1) {
					the_spell = cs;
					the_group = cg;
				} else if (the_pos < -1) {
					break;
				}
				if (cs == groups_list[cg].spells - 1) {
					cs = 0;
					cg++;
					the_pos -= (SPELLS_ALIGN_X - j - 1);
					break;
				} else {
					cs++;
				}
			}
		}
		// put it on the cast bar
		if (the_spell != -1) {
			we_have_spell = groups_list[the_group].spells_id[the_spell];
			put_on_cast();
			// handle double click
			if (((SDL_GetTicks() - last_clicked) < 400) && last_pos == pos) {
				cast_handler();
			}
		}
	} else {
		// check if clicked on the spell icon
		if (we_have_spell >= 0 && mx >= 20 && mx <= 53 && my >= spell_mini_y_len - 37 && my <= spell_mini_y_len - 4) {
			if (flags & ELW_LEFT_MOUSE) {
				if (put_on_cast()) {
					cast_handler();
				}
			} else if (flags & ELW_RIGHT_MOUSE) {
				// add to quickbar
				if (put_on_cast()) {
					prepare_for_cast();
					add_spell_to_quickbar();
				}
			}
		} else {
			click_switcher_handler(win, mx, my, flags);
		}
	}
	last_pos = pos;
	last_clicked = SDL_GetTicks();
	return 0;
}
int click_necro_handler(window_info *win, int mx, int my, Uint32 flags) {
	int pos, i, clique_bouton = 0;
	for (i = 0 ; i < NECRO_NO; i++) {
		if (i >= 4) {
			pos = get_mouse_pos_in_grid(mx, my, 1, 1, NECRO_SPACE_Y + NECRO_SPACE_Y_BETWEEN * (i + 6) + i * 33, NECRO_SPACE_X, 33, 33);
		} else {
			if (i >= 2) {
				pos = get_mouse_pos_in_grid(mx, my, 1, 1, NECRO_SPACE_Y + NECRO_SPACE_Y_BETWEEN * (i + 3) + i * 33, NECRO_SPACE_X, 33, 33);
			} else {
				pos = get_mouse_pos_in_grid(mx, my, 1, 1, NECRO_SPACE_Y + NECRO_SPACE_Y_BETWEEN * i + i * 33, NECRO_SPACE_X, 33, 33);
			}
		}
		if (pos == 0) {
			if (nb_recettes_necro > 0) {
				if ((i == 4) && (securite_invoc)) {
					LOG_TO_CONSOLE(c_red1, "Impossible de tuer les invocations lorsque le blocage est actif...");
					set_shown_string(c_red2, "Impossible de tuer les invocations lorsque le blocage est actif...");
				} else {
					test_for_console_command(necro_list[i].name, strlen(necro_list[i].name));
				}
				clique_bouton = 1;
				break;
			} else {
				test_for_console_command(necro_list[i].name, strlen(necro_list[i].name));
				clique_bouton = 1;
				break;
			}
		}
	}
	// sort de soin des invoqués
	pos = get_mouse_pos_in_grid(mx, my, 1, 1, pos_x_sdi, pos_y_sdi, 33, 33);
	if (pos == 0) {
		we_have_spell = groups_list[1].spells_id[4];
		put_on_cast();
		cast_handler();
	}
	if (nb_recettes_necro > 0) {
		pos = get_mouse_pos_in_grid(mx, my, 1, 1, 8, 155, 10, 10);
		if (pos == 0) {
			if (double_invoc == 1) {
				double_invoc = 0;
			} else {
				double_invoc++;
			}
		}
		pos = get_mouse_pos_in_grid(mx, my, 1, 1, 8, 49, 10, 10);
		if (pos == 0) {
			if (securite_invoc == 1) {
				securite_invoc = 0;
			} else {
				securite_invoc++;
			}
		}
		pos = get_mouse_pos_in_grid(mx, my, 1, 1, 209, 83, 10, 10);
		if (pos == 0) {
			// haut
			if (nb_recettes_necro != -1) {
				if (creature_en_cours == nb_recettes_necro - 1) {
					creature_en_cours = -1;
				}
				creature_en_cours++;
			}
		}
		pos = get_mouse_pos_in_grid(mx, my, 1, 1, 209, 104, 10, 10);
		if (pos == 0) {
			// bas
			if (nb_recettes_necro != -1) {
				if (creature_en_cours == 0) {
					creature_en_cours = nb_recettes_necro;
				}
				creature_en_cours--;
			}
		}
	}
	if (!clique_bouton) {
		click_switcher_handler(win, mx, my, flags);
	}
	return 0;
}
// MOUSEOVER HANDLERS
int mouseover_sigils_handler(window_info *win, int mx, int my) {
	if (!have_error_message) {
		spell_text[0] = 0;
	}
	if (mx >= 350 && mx <= 381 && my >= 112 && my <= 143 && mqb_data[0] && mqb_data[0]->spell_name[0]) {
		show_last_spell_help = 1;
	}
	// see if we clicked on any sigil in the main category
	if (mx > 0 && mx < NUM_SIGILS_LINE * 33 && my > 0 && my < NUM_SIGILS_ROW * 33) {
		int pos = get_mouse_pos_in_grid(mx, my, NUM_SIGILS_LINE, NUM_SIGILS_ROW, 0, 0, 33, 33);
		if (pos >= 0 && sigils_list[pos].have_sigil) {
			my_strcp((char *)spell_text, sigils_list[pos].name);
			have_error_message = 0;
		}
		return 0;
	}
	// see if we clicked on any sigil from "on cast"
	if (mx > 5 && mx < 6 * 33 + 5 && my > sigil_y_len - 37 && my < sigil_y_len - 5) {
		int pos = get_mouse_pos_in_grid(mx, my, 6, 1, 5, sigil_y_len - 37, 33, 33);
		if (pos >= 0 && on_cast[pos] != -1) {
			my_strcp((char *)spell_text, sigils_list[on_cast[pos]].name);
			have_error_message = 0;
		}
		return 0;
	}
	if (mx >= 350 && mx <= 381 && my >= 112 && my <= 143 && mqb_data[0] && mqb_data[0]->spell_id != -1) {
		safe_snprintf((char *)spell_text, sizeof(spell_text), click_to_add_str);
		return 0;
	}
	mouseover_switcher_handler(win, mx, my);
	return 0;
}
void set_spell_help_text(int spell) {
	char clr[4];
	if (spell < 0) {
		spell_help[0] = 0;
		return;
	}
	// Set spell name color
	if (spell == we_have_spell) {
		spell_help[0] = 127 + c_green3;
	} else {
		spell_help[0] = 127 + c_orange2;
	}
	spell_help[1] = 0;
	// Set spell name
	safe_strcat((char *)spell_help, spells_list[spell].name, sizeof(spell_help));
	// Set uncastable message
	if (spells_list[spell].uncastable) {
		clr[0] = 127 + c_red2;
		clr[1] = clr[2] = ' ';
		clr[3] = 0;
		safe_strcat((char *)spell_help, clr, sizeof(spell_help));
		safe_strcat((char *)spell_help, GET_UNCASTABLE_STR(spells_list[spell].uncastable), sizeof(spell_help));
	}
	safe_strcat((char *)spell_help, "\n", sizeof(spell_help));
	clr[0] = 127 + c_grey1;
	clr[1] = 0;
	safe_strcat((char *)spell_help, clr, sizeof(spell_help));
	safe_strcat((char *)spell_help, spells_list[spell].desc, sizeof(spell_help));
}
int mouseover_spells_handler(window_info *win, int mx, int my) {
	int i, pos;
	if (!have_error_message) {
		spell_text[0] = 0;
	}
	on_spell = -1;
	for (i = 0; i < num_groups; i++) {
		pos = get_mouse_pos_in_grid(mx, my, SPELLS_ALIGN_X, groups_list[i].spells / (SPELLS_ALIGN_X + 1) + 1, groups_list[i].x, groups_list[i].y, 33, 33);
		if (pos >= 0 && pos < groups_list[i].spells) {
			on_spell = groups_list[i].spells_id[pos];
			set_spell_help_text(on_spell);
			return 0;
		}
	}
	set_spell_help_text(we_have_spell);
	// check spell icon
	if (mx > 20 && mx < 53 && my > spell_y_len - 37 && my < spell_y_len - 4 && we_have_spell >= 0) {
		safe_snprintf((char *)spell_text, sizeof(spell_text), "Clique gauche pour lancer le sort\nClique droit pour ajouter le sort dans la barre rapide");
		have_error_message = 0;
	}
	mouseover_switcher_handler(win, mx, my);
	return 0;
}
int mouseover_spells_mini_handler(window_info *win, int mx, int my) {
	int pos = get_mouse_pos_in_grid(mx, my, SPELLS_ALIGN_X, spell_mini_rows, 20, 10, 33, 33);
	on_spell = -1;
	if (pos >= 0) {
		int i, j, cs, cg, the_spell = -1, the_group = -1, the_pos = pos;
		// find the spell clicked
		for (i = 0, cs = 0, cg = 0; i < spell_mini_rows && the_pos >= 0; i++) {
			for (j = 0; j < SPELLS_ALIGN_X; j++) {
				the_pos--;
				if (the_pos == -1) {
					the_spell = cs;
					the_group = cg;
				} else if (the_pos < -1) {
					break;
				}
				if (cs == groups_list[cg].spells - 1) {
					cs = 0;
					cg++;
					the_pos -= (SPELLS_ALIGN_X - j - 1);
					break;
				} else {
					cs++;
				}
			}
		}
		if (the_spell != -1) {
			on_spell = groups_list[the_group].spells_id[the_spell];
		}
	} else if (mx > 20 && mx < 53 && my > spell_mini_y_len - 37 && my < spell_mini_y_len - 4 && we_have_spell >= 0) {
		// check spell icon
		on_spell = -2; // draw uncastability reason
	}
	mouseover_switcher_handler(win, mx, my);
	return 0;
}
int mouseover_necro_handler(window_info *win, int mx, int my) {
	int i, pos;
	for (i = 0 ; i < NECRO_NO ; i++) {
		if (i >= 4) {
			pos = get_mouse_pos_in_grid(mx, my, 1, 1, NECRO_SPACE_Y + NECRO_SPACE_Y_BETWEEN * (i + 6) + i * 33, NECRO_SPACE_X, 33, 33);
		} else {
			if (i >= 2) {
				pos = get_mouse_pos_in_grid(mx, my, 1, 1, NECRO_SPACE_Y + NECRO_SPACE_Y_BETWEEN * (i + 3) + i * 33, NECRO_SPACE_X, 33, 33);
			} else {
				pos = get_mouse_pos_in_grid(mx, my, 1, 1, NECRO_SPACE_Y + NECRO_SPACE_Y_BETWEEN * i + i * 33, NECRO_SPACE_X, 33, 33);
			}
		}
		if (pos == 0) {
			mouseover_necro = i;
			break;
		} else {
			mouseover_necro = -1;
		}
	}
	// sort de soin des invoqués
	pos = get_mouse_pos_in_grid(mx, my, 1, 1, pos_x_sdi, pos_y_sdi, 33, 33);
	if (pos == 0) {
		mouseover_necro = 5;
	}
	mouseover_switcher_handler(win, mx, my);
	return 0;
}
// MISC FUNCTIONS
void get_sigils_we_have(Uint32 sigils_we_have, Uint32 sigils2) {
	int i;
	int po2 = 1;
	// the first 32 sigils
	for (i = 0; i < 32; i++) {
		if ((sigils_we_have & po2)) {
			sigils_list[i].have_sigil = 1;
		} else {
			sigils_list[i].have_sigil = 0;
		}
		po2 *= 2;
	}
	// the next optional sigils
	po2 = 1;
	for (i = 32; i < SIGILS_NO; i++) {
		if ((sigils2 & po2)) {
			sigils_list[i].have_sigil = 1;
		} else {
			sigils_list[i].have_sigil = 0;
		}
		po2 *= 2;
	}
	check_castability();
}
int have_spell_name(int spell_id) {
	int i;
	for (i = 1; i <= QUICKSPELLS_MAXSIZE; i++) {
		if (mqb_data[i] && mqb_data[i]->spell_id == spell_id && mqb_data[i]->spell_name[0]) {
			if (mqb_data[0]) {
				safe_snprintf(mqb_data[0]->spell_name, sizeof(mqb_data[0]->spell_name), "%s", mqb_data[i]->spell_name);
			}
			return 1;
		}
	}
	return 0;
}
void set_spell_name(int id, const char *data, int len) {
	int i;
	if (len >= 60) {
		return;
	}
	counters_set_spell_name(id, (char *)data, len);
	for (i = 0; i < QUICKSPELLS_MAXSIZE; i++) {
		if (mqb_data[i] != NULL && mqb_data[i]->spell_id == id) {
			safe_snprintf(mqb_data[i]->spell_name, sizeof(mqb_data[i]->spell_name), "%.*s", len, data);
		}
	}
}
void process_network_spell(const char *data, int len) {
	last_spell_name[0] = '\0';
	switch (data[0]) {
	case S_INVALID:
		spell_result = 0;
		LOG_TO_CONSOLE(c_red1, invalid_spell_str);
		return;
	case S_NAME:
		set_spell_name(data[1], &data[2], len - 2); // Will set the spell name of the given ID
		return;
	case S_SELECT_TARGET: // spell_result==3
		spell_result = 3;
		if (selected_spell != -1 && selected_spell_sent) {
			fast_spell_cast();
		}
		action_mode = ACTION_WAND;
		break;
	case S_SELECT_TELE_LOCATION: // spell_result==2
		// we're about to teleport, don't let the pathfinder
		// interfere with our destination
		if (pf_follow_path) {
			pf_destroy_path();
		}
		if (selected_spell != -1 && selected_spell_sent) {
			fast_spell_teleport();
		}
		spell_result = 2;
		action_mode = ACTION_WAND;
		break;
	case S_SUCCES: // spell_result==1
		spell_result = 1;
		if (selected_spell_sent) {
			action_mode = ACTION_ATTACK;
			selected_spell_sent = 0;
		} else {
			action_mode = ACTION_WALK;
		}
		break;
	case S_FAILED:
		if (selected_spell_sent) {
			selected_spell_sent = 0;
			action_mode = ACTION_ATTACK;
		} else {
			action_mode = ACTION_WALK;
		}
		spell_result = 0;
		return;
	}
	if (!mqb_data[0]) {
		mqb_data[0] = (mqbdata *)calloc(1, sizeof(mqbdata));
		mqb_data[0]->spell_id = -1;
	}
	if (mqb_data[0]->spell_id != data[1]) {
		if (!have_spell_name(data[1])) {
			Uint8 str[2];
			str[0] = SPELL_NAME;
			str[1] = data[1];
			my_tcp_send(my_socket, str, 2);
		}
		mqb_data[0]->spell_id = data[1];
		mqb_data[0]->spell_image = data[2];
	}
}
/****** QUICKSPELLS FUNCTIONS *****/
/* Retourne l'indice de la case de la barre rapide correspondant aux coordonnées de la souris */
int get_quickspell_from_mouse(int mx, int my) {
	int pos;
	int width = (quickspells_dim - quickspells_ico) / 2;
	if (quickspells_dir != VERTICAL) {
		int tmp = mx;
		mx = my;
		my = tmp;
	}
	if (mx < width) {
		return -1;
	}
	if (mx > width + quickspells_ico) {
		return -1;
	}
	for (pos = 0; pos < quickspells_size; pos++) {
		int y = 1 + (int)(pos / quickspells_div) * quickspells_sep + (quickspells_dim + 1) * pos + width;
		if ((my >= y) && (my <= y + quickspells_ico)) {
			return pos;
		}
	}
	return -1;
}
/* Modification de la barre pour la rendre déplaçable (avec barre de titre) ou non */
void toggle_quickspells_draggable() {
	Uint32 flags = get_flags(quickspell_win);
	if (!quickspells_draggable) {
		flags |= ELW_TITLE_BAR | ELW_DRAGGABLE | ELW_USE_BACKGROUND | ELW_SWITCHABLE_OPAQUE;
		change_flags(quickspell_win, flags);
		quickspells_draggable = 1;
	} else {
		flags &= ~(ELW_TITLE_BAR | ELW_DRAGGABLE | ELW_USE_BACKGROUND | ELW_SWITCHABLE_OPAQUE);
		change_flags(quickspell_win, flags);
		quickspells_draggable = 0;
		quickspell_x = window_width - windows_list.window[quickspell_win].cur_x;
		quickspell_y = windows_list.window[quickspell_win].cur_y;
	}
}
/* Changement de l'orientation de la barre rapide (vertical/horizontal) */
void flip_quickspells() {
	int temp = quickspell_x_len;
	quickspell_x_len = quickspell_y_len;
	quickspell_y_len = temp;
	quickspells_dir = (quickspells_dir == VERTICAL) ? HORIZONTAL : VERTICAL;
	resize_window(quickspell_win, quickspell_x_len, quickspell_y_len);
}
/* Redéfinition de la barre rapide dans sa position par défaut */
void reset_quickspells() {
	quickspells_on_top = 1;
	quickspells_draggable = 0;
	change_flags(quickspell_win, ELW_TITLE_NONE | ELW_CLICK_TRANSPARENT); // |ELW_SHOW|ELW_SHOW_LAST
	quickspell_x = HUD_MARGIN_X;
	quickspell_y = (hud_x) ? HUD_MARGIN_X : 0;
	move_window(quickspell_win, -1, 0, window_width - quickspell_x, quickspell_y);
	quickspells_dir = VERTICAL;
	quickspell_x_len = (1 + quickspells_dim) * 1 + 1;
	quickspell_y_len = (1 + quickspells_dim) * quickspells_nb + (int)((quickspells_nb - 1) / quickspells_div) * quickspells_sep + 1;
	resize_window(quickspell_win, quickspell_x_len, quickspell_y_len);
}
/* Mise à jour du nombre d'icones affichées et redimensionnement de la barre si nécessaire */
int resize_quickspells(int nb) {
	if (nb >= 0) {
		if (nb > quickspells_size) {
			nb = quickspells_size;
		}
		while ((nb < quickspells_size) && (mqb_data[nb + 1])) {
			nb++;
		}
		if (nb < 1) {
			nb = 1; // minimum 1 case, même sans icone
		}
	}
	if (nb != quickspells_nb) {
		if (nb > 0) {
			quickspells_nb = nb;
		}
		if (quickspells_dir == VERTICAL) {
			quickspell_y_len = (1 + quickspells_dim) * quickspells_nb + (int)((quickspells_nb - 1) / quickspells_div) * quickspells_sep + 1;
		} else {
			quickspell_x_len = (1 + quickspells_dim) * quickspells_nb + (int)((quickspells_nb - 1) / quickspells_div) * quickspells_sep + 1;
		}
		if (quickspell_win >= 0) {
			resize_window(quickspell_win, quickspell_x_len, quickspell_y_len);
		}
	}
	return nb;
}
// Quickspell I/O start
void add_spell_to_quickbar() {
	int i;
	if (!mqb_data[0]) {
		return;
	}
	switch (quickspell_mqb_selected) {
	case 0:
		// recherche si le raccourci existe déjà dans la barre
		for (i = 1; i <= QUICKSPELLS_MAXSIZE; i++) {
			if (mqb_data[i] && mqb_data[0]->spell_id == mqb_data[i]->spell_id) {
				// échange avec le dernier visible s'il se trouvait au delà
				if (i > quickspells_size) {
					mqbdata *mqb_temp;
					mqb_temp = mqb_data[quickspells_size];
					mqb_data[quickspells_size] = mqb_data[i];
					mqb_data[i] = mqb_temp;
					save_quickspells();
				}
				return;
			}
		}
		for (i = 1; i <= QUICKSPELLS_MAXSIZE; i++) {
			if (mqb_data[i] == NULL) {
				// Free slot
				mqb_data[i] = calloc(1, sizeof(mqbdata));
				resize_quickspells(i);
				break;
			}
		}
		if (i > QUICKSPELLS_MAXSIZE) {
			// barre complète : on remplace le dernier raccourci visible
			memcpy(mqb_data[quickspells_size], mqb_data[0], sizeof(mqbdata));
		} else if (i > quickspells_size) {
			// place dispo au delà des raccourcis visible : échange avec le dernier
			memcpy(mqb_data[i], mqb_data[quickspells_size], sizeof(mqbdata));
			memcpy(mqb_data[quickspells_size], mqb_data[0], sizeof(mqbdata));
		} else {
			memcpy(mqb_data[i], mqb_data[0], sizeof(mqbdata));
		}
		break;
	case 1:
		// recherche si le raccourci existe déjà dans la barre
		for (i = 1; i <= QUICKSPELLS_MAXSIZE; i++) {
			if (mqb_data2[i] && mqb_data[0]->spell_id == mqb_data2[i]->spell_id) {
				// échange avec le dernier visible s'il se trouvait au delà
				if (i > quickspells_size) {
					mqbdata *mqb_temp;
					mqb_temp = mqb_data2[quickspells_size];
					mqb_data2[quickspells_size] = mqb_data2[i];
					mqb_data2[i] = mqb_temp;
					save_quickspells();
				}
				return;
			}
		}
		for (i = 1; i <= QUICKSPELLS_MAXSIZE; i++) {
			if (mqb_data2[i] == NULL) {
				// Free slot
				mqb_data2[i] = calloc(1, sizeof(mqbdata));
				resize_quickspells(i);
				break;
			}
		}
		if (i > QUICKSPELLS_MAXSIZE) {
			// barre complète : on remplace le dernier raccourci visible
			memcpy(mqb_data2[quickspells_size], mqb_data[0], sizeof(mqbdata));
		} else if (i > quickspells_size) {
			// place dispo au delà des raccourcis visible : échange avec le dernier
			memcpy(mqb_data2[i], mqb_data2[quickspells_size], sizeof(mqbdata));
			memcpy(mqb_data2[quickspells_size], mqb_data[0], sizeof(mqbdata));
		} else {
			memcpy(mqb_data2[i], mqb_data[0], sizeof(mqbdata));
		}
		break;
	case 2:
		// recherche si le raccourci existe déjà dans la barre
		for (i = 1; i <= QUICKSPELLS_MAXSIZE; i++) {
			if (mqb_data3[i] && mqb_data[0]->spell_id == mqb_data3[i]->spell_id) {
				// échange avec le dernier visible s'il se trouvait au delà
				if (i > quickspells_size) {
					mqbdata *mqb_temp;
					mqb_temp = mqb_data3[quickspells_size];
					mqb_data3[quickspells_size] = mqb_data3[i];
					mqb_data3[i] = mqb_temp;
					save_quickspells();
				}
				return;
			}
		}
		for (i = 1; i <= QUICKSPELLS_MAXSIZE; i++) {
			if (mqb_data3[i] == NULL) {
				// Free slot
				mqb_data3[i] = calloc(1, sizeof(mqbdata));
				resize_quickspells(i);
				break;
			}
		}
		if (i > QUICKSPELLS_MAXSIZE) {
			// barre complète : on remplace le dernier raccourci visible
			memcpy(mqb_data3[quickspells_size], mqb_data[0], sizeof(mqbdata));
		} else if (i > quickspells_size) {
			// place dispo au delà des raccourcis visible : échange avec le dernier
			memcpy(mqb_data3[i], mqb_data3[quickspells_size], sizeof(mqbdata));
			memcpy(mqb_data3[quickspells_size], mqb_data[0], sizeof(mqbdata));
		} else {
			memcpy(mqb_data3[i], mqb_data[0], sizeof(mqbdata));
		}
		break;
	case 3:
		// recherche si le raccourci existe déjà dans la barre
		for (i = 1; i <= QUICKSPELLS_MAXSIZE; i++) {
			if (mqb_data4[i] && mqb_data[0]->spell_id == mqb_data4[i]->spell_id) {
				// échange avec le dernier visible s'il se trouvait au delà
				if (i > quickspells_size) {
					mqbdata *mqb_temp;
					mqb_temp = mqb_data4[quickspells_size];
					mqb_data4[quickspells_size] = mqb_data4[i];
					mqb_data4[i] = mqb_temp;
					save_quickspells();
				}
				return;
			}
		}
		for (i = 1; i <= QUICKSPELLS_MAXSIZE; i++) {
			if (mqb_data4[i] == NULL) {
				// Free slot
				mqb_data4[i] = calloc(1, sizeof(mqbdata));
				resize_quickspells(i);
				break;
			}
		}
		if (i > QUICKSPELLS_MAXSIZE) {
			// barre complète : on remplace le dernier raccourci visible
			memcpy(mqb_data4[quickspells_size], mqb_data[0], sizeof(mqbdata));
		} else if (i > quickspells_size) {
			// place dispo au delà des raccourcis visible : échange avec le dernier
			memcpy(mqb_data4[i], mqb_data4[quickspells_size], sizeof(mqbdata));
			memcpy(mqb_data4[quickspells_size], mqb_data[0], sizeof(mqbdata));
		} else {
			memcpy(mqb_data4[i], mqb_data[0], sizeof(mqbdata));
		}
		break;
	case 4:
		// recherche si le raccourci existe déjà dans la barre
		for (i = 1; i <= QUICKSPELLS_MAXSIZE; i++) {
			if (mqb_data5[i] && mqb_data[0]->spell_id == mqb_data5[i]->spell_id) {
				// échange avec le dernier visible s'il se trouvait au delà
				if (i > quickspells_size) {
					mqbdata *mqb_temp;
					mqb_temp = mqb_data5[quickspells_size];
					mqb_data5[quickspells_size] = mqb_data5[i];
					mqb_data5[i] = mqb_temp;
					save_quickspells();
				}
				return;
			}
		}
		for (i = 1; i <= QUICKSPELLS_MAXSIZE; i++) {
			if (mqb_data5[i] == NULL) {
				// Free slot
				mqb_data5[i] = calloc(1, sizeof(mqbdata));
				resize_quickspells(i);
				break;
			}
		}
		if (i > QUICKSPELLS_MAXSIZE) {
			// barre complète : on remplace le dernier raccourci visible
			memcpy(mqb_data5[quickspells_size], mqb_data[0], sizeof(mqbdata));
		} else if (i > quickspells_size) {
			// place dispo au delà des raccourcis visible : échange avec le dernier
			memcpy(mqb_data5[i], mqb_data5[quickspells_size], sizeof(mqbdata));
			memcpy(mqb_data5[quickspells_size], mqb_data[0], sizeof(mqbdata));
		} else {
			memcpy(mqb_data5[i], mqb_data[0], sizeof(mqbdata));
		}
		break;
	default:
		break;
	}
	save_quickspells();
}
void remove_spell_from_quickbar(int pos) {
	int i = 0;
	switch (quickspell_mqb_selected) {
	case 0:
		if (pos < 1 || pos > QUICKSPELLS_MAXSIZE || mqb_data[pos] == NULL) {
			return;
		}
		// remonte les raccourcis suivants (tant qu'il en existe)
		for (i = pos; i < QUICKSPELLS_MAXSIZE; i++) {
			mqb_data[i] = mqb_data[i + 1];
			if (mqb_data[i] == NULL) {
				break;
			}
		}
		mqb_data[QUICKSPELLS_MAXSIZE] = NULL;
		break;
	case 1:
		if (pos < 1 || pos > QUICKSPELLS_MAXSIZE || mqb_data2[pos] == NULL) {
			return;
		}
		// remonte les raccourcis suivants (tant qu'il en existe)
		for (i = pos; i < QUICKSPELLS_MAXSIZE; i++) {
			mqb_data2[i] = mqb_data2[i + 1];
			if (mqb_data2[i] == NULL) {
				break;
			}
		}
		mqb_data2[QUICKSPELLS_MAXSIZE] = NULL;
		break;
	case 2:
		if (pos < 1 || pos > QUICKSPELLS_MAXSIZE || mqb_data3[pos] == NULL) {
			return;
		}
		// remonte les raccourcis suivants (tant qu'il en existe)
		for (i = pos; i < QUICKSPELLS_MAXSIZE; i++) {
			mqb_data3[i] = mqb_data3[i + 1];
			if (mqb_data3[i] == NULL) {
				break;
			}
		}
		mqb_data3[QUICKSPELLS_MAXSIZE] = NULL;
		break;
	case 3:
		if (pos < 1 || pos > QUICKSPELLS_MAXSIZE || mqb_data4[pos] == NULL) {
			return;
		}
		// remonte les raccourcis suivants (tant qu'il en existe)
		for (i = pos; i < QUICKSPELLS_MAXSIZE; i++) {
			mqb_data4[i] = mqb_data4[i + 1];
			if (mqb_data4[i] == NULL) {
				break;
			}
		}
		mqb_data4[QUICKSPELLS_MAXSIZE] = NULL;
		break;
	case 4:
		if (pos < 1 || pos > QUICKSPELLS_MAXSIZE || mqb_data5[pos] == NULL) {
			return;
		}
		// remonte les raccourcis suivants (tant qu'il en existe)
		for (i = pos; i < QUICKSPELLS_MAXSIZE; i++) {
			mqb_data5[i] = mqb_data5[i + 1];
			if (mqb_data5[i] == NULL) {
				break;
			}
		}
		mqb_data5[QUICKSPELLS_MAXSIZE] = NULL;
		break;
	default:
		break;
	}
	resize_quickspells(i - 1);
	save_quickspells();
}
void move_spell_on_quickbar(int pos, int direction) {
	int i = pos;
	mqbdata *mqb_temp;
	switch (quickspell_mqb_selected) {
	case 0:
		if (pos < 1 || pos > QUICKSPELLS_MAXSIZE || mqb_data[pos] == NULL) {
			return;
		}
		switch (direction) {
		case 0: // up
			if (pos == 1) {
				return;
			}
			mqb_temp = mqb_data[i - 1];
			mqb_data[i - 1] = mqb_data[i];
			mqb_data[i] = mqb_temp;
			break;
		case 1: // down
			if (pos == quickspells_nb) {
				return;
			}
			mqb_temp = mqb_data[i + 1];
			mqb_data[i + 1] = mqb_data[i];
			mqb_data[i] = mqb_temp;
			break;
		case 2: // first
			if (pos == 1) {
				return;
			}
			mqb_temp = mqb_data[pos];
			for (i = pos; i > 1; i--) {
				mqb_data[i] = mqb_data[i - 1];
			}
			mqb_data[1] = mqb_temp;
			break;
		case 3: // last
			if (pos == quickspells_nb) {
				return;
			}
			mqb_temp = mqb_data[pos];
			for (i = pos; i < quickspells_nb; i++) {
				mqb_data[i] = mqb_data[i + 1];
			}
			mqb_data[quickspells_nb] = mqb_temp;
			break;
		}
		break;
	case 1:
		if (pos < 1 || pos > QUICKSPELLS_MAXSIZE || mqb_data2[pos] == NULL) {
			return;
		}
		switch (direction) {
		case 0: // up
			if (pos == 1) {
				return;
			}
			mqb_temp = mqb_data2[i - 1];
			mqb_data2[i - 1] = mqb_data2[i];
			mqb_data2[i] = mqb_temp;
			break;
		case 1: // down
			if (pos == quickspells_nb) {
				return;
			}
			mqb_temp = mqb_data2[i + 1];
			mqb_data2[i + 1] = mqb_data2[i];
			mqb_data2[i] = mqb_temp;
			break;
		case 2: // first
			if (pos == 1) {
				return;
			}
			mqb_temp = mqb_data2[pos];
			for (i = pos; i > 1; i--) {
				mqb_data2[i] = mqb_data2[i - 1];
			}
			mqb_data2[1] = mqb_temp;
			break;
		case 3: // last
			if (pos == quickspells_nb) {
				return;
			}
			mqb_temp = mqb_data2[pos];
			for (i = pos; i < quickspells_nb; i++) {
				mqb_data2[i] = mqb_data2[i + 1];
			}
			mqb_data2[quickspells_nb] = mqb_temp;
			break;
		}
		break;
	case 2:
		if (pos < 1 || pos > QUICKSPELLS_MAXSIZE || mqb_data3[pos] == NULL) {
			return;
		}
		switch (direction) {
		case 0: // up
			if (pos == 1) {
				return;
			}
			mqb_temp = mqb_data3[i - 1];
			mqb_data3[i - 1] = mqb_data3[i];
			mqb_data3[i] = mqb_temp;
			break;
		case 1: // down
			if (pos == quickspells_nb) {
				return;
			}
			mqb_temp = mqb_data3[i + 1];
			mqb_data3[i + 1] = mqb_data3[i];
			mqb_data3[i] = mqb_temp;
			break;
		case 2: // first
			if (pos == 1) {
				return;
			}
			mqb_temp = mqb_data3[pos];
			for (i = pos; i > 1; i--) {
				mqb_data3[i] = mqb_data3[i - 1];
			}
			mqb_data3[1] = mqb_temp;
			break;
		case 3: // last
			if (pos == quickspells_nb) {
				return;
			}
			mqb_temp = mqb_data3[pos];
			for (i = pos; i < quickspells_nb; i++) {
				mqb_data3[i] = mqb_data3[i + 1];
			}
			mqb_data3[quickspells_nb] = mqb_temp;
			break;
		}
		break;
	case 3:
		if (pos < 1 || pos > QUICKSPELLS_MAXSIZE || mqb_data4[pos] == NULL) {
			return;
		}
		switch (direction) {
		case 0: // up
			if (pos == 1) {
				return;
			}
			mqb_temp = mqb_data4[i - 1];
			mqb_data4[i - 1] = mqb_data4[i];
			mqb_data4[i] = mqb_temp;
			break;
		case 1: // down
			if (pos == quickspells_nb) {
				return;
			}
			mqb_temp = mqb_data4[i + 1];
			mqb_data4[i + 1] = mqb_data4[i];
			mqb_data4[i] = mqb_temp;
			break;
		case 2: // first
			if (pos == 1) {
				return;
			}
			mqb_temp = mqb_data4[pos];
			for (i = pos; i > 1; i--) {
				mqb_data4[i] = mqb_data4[i - 1];
			}
			mqb_data4[1] = mqb_temp;
			break;
		case 3: // last
			if (pos == quickspells_nb) {
				return;
			}
			mqb_temp = mqb_data4[pos];
			for (i = pos; i < quickspells_nb; i++) {
				mqb_data4[i] = mqb_data4[i + 1];
			}
			mqb_data4[quickspells_nb] = mqb_temp;
			break;
		}
		break;
	case 4:
		if (pos < 1 || pos > QUICKSPELLS_MAXSIZE || mqb_data5[pos] == NULL) {
			return;
		}
		switch (direction) {
		case 0: // up
			if (pos == 1) {
				return;
			}
			mqb_temp = mqb_data5[i - 1];
			mqb_data5[i - 1] = mqb_data5[i];
			mqb_data5[i] = mqb_temp;
			break;
		case 1: // down
			if (pos == quickspells_nb) {
				return;
			}
			mqb_temp = mqb_data5[i + 1];
			mqb_data5[i + 1] = mqb_data5[i];
			mqb_data5[i] = mqb_temp;
			break;
		case 2: // first
			if (pos == 1) {
				return;
			}
			mqb_temp = mqb_data5[pos];
			for (i = pos; i > 1; i--) {
				mqb_data5[i] = mqb_data5[i - 1];
			}
			mqb_data5[1] = mqb_temp;
			break;
		case 3: // last
			if (pos == quickspells_nb) {
				return;
			}
			mqb_temp = mqb_data5[pos];
			for (i = pos; i < quickspells_nb; i++) {
				mqb_data5[i] = mqb_data5[i + 1];
			}
			mqb_data5[quickspells_nb] = mqb_temp;
			break;
		}
		break;
	default:
		break;
	}
	save_quickspells();
}
static mqbdata *build_quickspell_data(const Uint32 spell_id) {
	Uint8 str[20];
	mqbdata *result;
	Uint32 i, count, index, len, size;
	index = 0xFFFFFFFF;
	for (i = 0; i < SPELLS_NO; i++) {
		if (spells_list[i].id == spell_id) {
			index = i;
			break;
		}
	}
	if (index == 0xFFFFFFFF) {
		LOG_WARNING("Invalid spell id %d", spell_id);
		return 0;
	}
	memset(str, 0, sizeof(str));
	count = 0;
	for (i = 0; i < 6; i++) {
		if (spells_list[index].sigils[i] != -1) {
			str[count + 2] = spells_list[index].sigils[i];
			count++;
		}
	}
	str[0] = CAST_SPELL;
	str[1] = count;
	result = calloc(1, sizeof(mqbdata));
	if (result == 0) {
		LOG_WARNING("Can't allocate memory for spell");
		return 0;
	}
	result->spell_id = spells_list[index].id;
	result->spell_image = spells_list[index].image;
	size = sizeof(result->spell_name);
	len = strlen(spells_list[index].name);
	if (size > len) {
		size = len;
	} else {
		size -= 1;
	}
	memset(result->spell_name, 0, size);
	memset(result->spell_str, 0, sizeof(result->spell_str));
	memcpy(result->spell_name, spells_list[index].name, len);
	memcpy(result->spell_str, str, count + 2);
	return result;
}
void load_quickspells() {
	char fname[128];
	char fname2[128];
	char fname3[128];
	char fname4[128];
	char fname5[128];
	Uint8 num_spells;
	FILE *fp;
	FILE *fp2;
	FILE *fp3;
	FILE *fp4;
	FILE *fp5;
	Uint32 i, index, max_index;
	// Grum: move this over here instead of at the end of the function,
	// so that quickspells are always saved when the player logs in.
	// (We're only interested in if this function is called, not if it
	// succeeds)
	quickspells_loaded = 1;
	// open the data file
	safe_snprintf(fname, sizeof(fname), "spells_%s.dat", username_str);
	my_tolower(fname);
	safe_snprintf(fname2, sizeof(fname2), "spells_%s%d.dat", username_str, 2);
	my_tolower(fname2);
	safe_snprintf(fname3, sizeof(fname3), "spells_%s%d.dat", username_str, 3);
	my_tolower(fname3);
	safe_snprintf(fname4, sizeof(fname4), "spells_%s%d.dat", username_str, 4);
	my_tolower(fname4);
	safe_snprintf(fname5, sizeof(fname5), "spells_%s%d.dat", username_str, 5);
	my_tolower(fname5);
	/* First MQB*/
	/* sliently ignore non existing file */
	if (file_exists_config(fname) != 1) {
		return;
	}
	fp = open_file_config(fname, "rb");
	if (fp == NULL) {
		LOG_ERROR("%s: %s \"%s\": %s\n", reg_error_str, cant_open_file, fname, strerror(errno));
		return;
	}
	if (fread(&num_spells, sizeof(num_spells), 1, fp) != 1) {
		LOG_ERROR("%s() read failed for [%s] \n", __FUNCTION__, fname);
		fclose(fp);
		return;
	}
	ENTER_DEBUG_MARK("load spells");
	if (num_spells > 0) {
		num_spells--;
	}
	if (num_spells > QUICKSPELLS_MAXSIZE) {
		LOG_WARNING("Too many spells (%d), only %d spells allowed", num_spells, QUICKSPELLS_MAXSIZE);
		num_spells = QUICKSPELLS_MAXSIZE;
	}
	memset(mqb_data, 0, sizeof(mqb_data));
	LOG_DEBUG("Reading %d spells from file '%s'", num_spells, fname);
	index = 1;
	max_index = index;
	for (i = 0; i < num_spells; i++) {
		mqbdata tmp;
		if (fread(&tmp, sizeof(mqbdata), 1, fp) != 1) {
			LOG_ERROR("Failed reading spell %d from file '%s'", i, fname);
			continue;
		}
		mqb_data[index] = build_quickspell_data(tmp.spell_id);
		if (mqb_data[index] == 0) {
			continue;
		}
		LOG_DEBUG("Added quickspell %d '%s' at index %d", i, mqb_data[index]->spell_name, index);
		index++;
	}
	fclose(fp);
	if (max_index < index) {
		max_index = index;
	}
	resize_quickspells(max_index - 1);
	/* /First MQB*/
	/* Second MQB*/
	/* sliently ignore non existing file */
	if (file_exists_config(fname2) != 1) {
		return;
	}
	fp2 = open_file_config(fname2, "rb");
	if (fp2 == NULL) {
		LOG_ERROR("%s: %s \"%s\": %s\n", reg_error_str, cant_open_file, fname2, strerror(errno));
		return;
	}
	if (fread(&num_spells, sizeof(num_spells), 1, fp2) != 1) {
		LOG_ERROR("%s() read failed for [%s] \n", __FUNCTION__, fname2);
		fclose(fp2);
		return;
	}
	ENTER_DEBUG_MARK("load spells");
	if (num_spells > 0) {
		num_spells--;
	}
	if (num_spells > QUICKSPELLS_MAXSIZE) {
		LOG_WARNING("Too many spells (%d), only %d spells allowed", num_spells, QUICKSPELLS_MAXSIZE);
		num_spells = QUICKSPELLS_MAXSIZE;
	}
	memset(mqb_data2, 0, sizeof(mqb_data2));
	LOG_DEBUG("Reading %d spells from file '%s'", num_spells, fname2);
	index = 1;
	max_index = index;
	for (i = 0; i < num_spells; i++) {
		mqbdata tmp;
		if (fread(&tmp, sizeof(mqbdata), 1, fp2) != 1) {
			LOG_ERROR("Failed reading spell %d from file '%s'", i, fname2);
			continue;
		}
		mqb_data2[index] = build_quickspell_data(tmp.spell_id);
		if (mqb_data2[index] == 0) {
			continue;
		}
		LOG_DEBUG("Added quickspell %d '%s' at index %d", i, mqb_data2[index]->spell_name, index);
		index++;
	}
	fclose(fp2);
	if (max_index < index) {
		max_index = index;
	}
	resize_quickspells(max_index - 1);
	/* /Second MQB*/
	/* Third MQB*/
	/* sliently ignore non existing file */
	if (file_exists_config(fname3) != 1) {
		return;
	}
	fp3 = open_file_config(fname3, "rb");
	if (fp3 == NULL) {
		LOG_ERROR("%s: %s \"%s\": %s\n", reg_error_str, cant_open_file, fname3, strerror(errno));
		return;
	}
	if (fread(&num_spells, sizeof(num_spells), 1, fp3) != 1) {
		LOG_ERROR("%s() read failed for [%s] \n", __FUNCTION__, fname3);
		fclose(fp3);
		return;
	}
	ENTER_DEBUG_MARK("load spells");
	if (num_spells > 0) {
		num_spells--;
	}
	if (num_spells > QUICKSPELLS_MAXSIZE) {
		LOG_WARNING("Too many spells (%d), only %d spells allowed", num_spells, QUICKSPELLS_MAXSIZE);
		num_spells = QUICKSPELLS_MAXSIZE;
	}
	memset(mqb_data3, 0, sizeof(mqb_data3));
	LOG_DEBUG("Reading %d spells from file '%s'", num_spells, fname3);
	index = 1;
	max_index = index;
	for (i = 0; i < num_spells; i++) {
		mqbdata tmp;
		if (fread(&tmp, sizeof(mqbdata), 1, fp3) != 1) {
			LOG_ERROR("Failed reading spell %d from file '%s'", i, fname3);
			continue;
		}
		mqb_data3[index] = build_quickspell_data(tmp.spell_id);
		if (mqb_data3[index] == 0) {
			continue;
		}
		LOG_DEBUG("Added quickspell %d '%s' at index %d", i, mqb_data3[index]->spell_name, index);
		index++;
	}
	fclose(fp3);
	if (max_index < index) {
		max_index = index;
	}
	resize_quickspells(max_index - 1);
	/* /Third MQB*/
	/* Fourth MQB*/
	/* sliently ignore non existing file */
	if (file_exists_config(fname4) != 1) {
		return;
	}
	fp4 = open_file_config(fname4, "rb");
	if (fp4 == NULL) {
		LOG_ERROR("%s: %s \"%s\": %s\n", reg_error_str, cant_open_file, fname4, strerror(errno));
		return;
	}
	if (fread(&num_spells, sizeof(num_spells), 1, fp4) != 1) {
		LOG_ERROR("%s() read failed for [%s] \n", __FUNCTION__, fname4);
		fclose(fp4);
		return;
	}
	ENTER_DEBUG_MARK("load spells");
	if (num_spells > 0) {
		num_spells--;
	}
	if (num_spells > QUICKSPELLS_MAXSIZE) {
		LOG_WARNING("Too many spells (%d), only %d spells allowed", num_spells, QUICKSPELLS_MAXSIZE);
		num_spells = QUICKSPELLS_MAXSIZE;
	}
	memset(mqb_data4, 0, sizeof(mqb_data4));
	LOG_DEBUG("Reading %d spells from file '%s'", num_spells, fname4);
	index = 1;
	max_index = index;
	for (i = 0; i < num_spells; i++) {
		mqbdata tmp;
		if (fread(&tmp, sizeof(mqbdata), 1, fp4) != 1) {
			LOG_ERROR("Failed reading spell %d from file '%s'", i, fname4);
			continue;
		}
		mqb_data4[index] = build_quickspell_data(tmp.spell_id);
		if (mqb_data4[index] == 0) {
			continue;
		}
		LOG_DEBUG("Added quickspell %d '%s' at index %d", i, mqb_data4[index]->spell_name, index);
		index++;
	}
	fclose(fp4);
	if (max_index < index) {
		max_index = index;
	}
	resize_quickspells(max_index - 1);
	/* /Fourth MQB*/
	/* Fifth MQB*/
	/* sliently ignore non existing file */
	if (file_exists_config(fname5) != 1) {
		return;
	}
	fp5 = open_file_config(fname5, "rb");
	if (fp5 == NULL) {
		LOG_ERROR("%s: %s \"%s\": %s\n", reg_error_str, cant_open_file, fname5, strerror(errno));
		return;
	}
	if (fread(&num_spells, sizeof(num_spells), 1, fp5) != 1) {
		LOG_ERROR("%s() read failed for [%s] \n", __FUNCTION__, fname5);
		fclose(fp5);
		return;
	}
	ENTER_DEBUG_MARK("load spells");
	if (num_spells > 0) {
		num_spells--;
	}
	if (num_spells > QUICKSPELLS_MAXSIZE) {
		LOG_WARNING("Too many spells (%d), only %d spells allowed", num_spells, QUICKSPELLS_MAXSIZE);
		num_spells = QUICKSPELLS_MAXSIZE;
	}
	memset(mqb_data5, 0, sizeof(mqb_data5));
	LOG_DEBUG("Reading %d spells from file '%s'", num_spells, fname5);
	index = 1;
	max_index = index;
	for (i = 0; i < num_spells; i++) {
		mqbdata tmp;
		if (fread(&tmp, sizeof(mqbdata), 1, fp5) != 1) {
			LOG_ERROR("Failed reading spell %d from file '%s'", i, fname5);
			continue;
		}
		mqb_data5[index] = build_quickspell_data(tmp.spell_id);
		if (mqb_data5[index] == 0) {
			continue;
		}
		LOG_DEBUG("Added quickspell %d '%s' at index %d", i, mqb_data5[index]->spell_name, index);
		index++;
	}
	fclose(fp5);
	if (max_index < index) {
		max_index = index;
	}
	resize_quickspells(max_index - 1);
	/* /Fifth MQB*/
	LEAVE_DEBUG_MARK("load spells");
}
void save_quickspells() {
	char fname[128];
	char fname2[128];
	char fname3[128];
	char fname4[128];
	char fname5[128];
	FILE *fp;
	FILE *fp2;
	FILE *fp3;
	FILE *fp4;
	FILE *fp5;
	Uint8 i;
	if (!quickspells_loaded) {
		return;
	}
	// write to the data file, to ensure data integrity, we will write all the information
	safe_snprintf(fname, sizeof(fname), "spells_%s.dat", username_str);
	my_tolower(fname);
	fp = open_file_config(fname, "wb");
	safe_snprintf(fname2, sizeof(fname2), "spells_%s%d.dat", username_str, 2);
	my_tolower(fname2);
	fp2 = open_file_config(fname2, "wb");
	safe_snprintf(fname3, sizeof(fname3), "spells_%s%d.dat", username_str, 3);
	my_tolower(fname3);
	fp3 = open_file_config(fname3, "wb");
	safe_snprintf(fname4, sizeof(fname4), "spells_%s%d.dat", username_str, 4);
	my_tolower(fname4);
	fp4 = open_file_config(fname4, "wb");
	safe_snprintf(fname5, sizeof(fname5), "spells_%s%d.dat", username_str, 5);
	my_tolower(fname5);
	fp5 = open_file_config(fname5, "wb");
	if (fp == NULL) {
		LOG_ERROR("%s: %s \"%s\": %s\n", reg_error_str, cant_open_file, fname, strerror(errno));
		return;
	}
	if (fp2 == NULL) {
		LOG_ERROR("%s: %s \"%s\": %s\n", reg_error_str, cant_open_file, fname2, strerror(errno));
		return;
	}
	if (fp3 == NULL) {
		LOG_ERROR("%s: %s \"%s\": %s\n", reg_error_str, cant_open_file, fname3, strerror(errno));
		return;
	}
	if (fp4 == NULL) {
		LOG_ERROR("%s: %s \"%s\": %s\n", reg_error_str, cant_open_file, fname4, strerror(errno));
		return;
	}
	if (fp5 == NULL) {
		LOG_ERROR("%s: %s \"%s\": %s\n", reg_error_str, cant_open_file, fname5, strerror(errno));
		return;
	}
	for (i = 1; i < QUICKSPELLS_MAXSIZE + 1; i++) {
		if (mqb_data[i] == NULL) {
			break;
		}
	}
	ENTER_DEBUG_MARK("save spells");
	// write the number of spells + 1
	fwrite(&i, sizeof(i), 1, fp);
	LOG_DEBUG("Writing %d spells to file '%s'", i, fname);
	for (i = 1; i < (QUICKSPELLS_MAXSIZE + 1); i++) {
		if (mqb_data[i] == 0) {
			break;
		}
		if (fwrite(mqb_data[i], sizeof(mqbdata), 1, fp) != 1) {
			LOG_ERROR("Failed writing spell '%s' to file '%s'", mqb_data[i]->spell_name, fname);
			break;
		}
		LOG_DEBUG("Wrote spell '%s' to file '%s'", mqb_data[i]->spell_name, fname);
	}
	fclose(fp);
	for (i = 1; i < QUICKSPELLS_MAXSIZE + 1; i++) {
		if (mqb_data2[i] == NULL) {
			break;
		}
	}
	// write the number of spells + 1
	fwrite(&i, sizeof(i), 1, fp2);
	LOG_DEBUG("Writing %d spells to file '%s'", i, fname2);
	for (i = 1; i < (QUICKSPELLS_MAXSIZE + 1); i++) {
		if (mqb_data2[i] == 0) {
			break;
		}
		if (fwrite(mqb_data2[i], sizeof(mqbdata), 1, fp2) != 1) {
			LOG_ERROR("Failed writing spell '%s' to file '%s'", mqb_data2[i]->spell_name, fname2);
			break;
		}
		LOG_DEBUG("Wrote spell '%s' to file '%s'", mqb_data2[i]->spell_name, fname2);
	}
	fclose(fp2);
	for (i = 1; i < QUICKSPELLS_MAXSIZE + 1; i++) {
		if (mqb_data3[i] == NULL) {
			break;
		}
	}
	// write the number of spells + 1
	fwrite(&i, sizeof(i), 1, fp3);
	LOG_DEBUG("Writing %d spells to file '%s'", i, fname3);
	for (i = 1; i < (QUICKSPELLS_MAXSIZE + 1); i++) {
		if (mqb_data3[i] == 0) {
			break;
		}
		if (fwrite(mqb_data3[i], sizeof(mqbdata), 1, fp3) != 1) {
			LOG_ERROR("Failed writing spell '%s' to file '%s'", mqb_data3[i]->spell_name, fname3);
			break;
		}
		LOG_DEBUG("Wrote spell '%s' to file '%s'", mqb_data3[i]->spell_name, fname3);
	}
	fclose(fp3);
	for (i = 1; i < QUICKSPELLS_MAXSIZE + 1; i++) {
		if (mqb_data4[i] == NULL) {
			break;
		}
	}
	// write the number of spells + 1
	fwrite(&i, sizeof(i), 1, fp4);
	LOG_DEBUG("Writing %d spells to file '%s'", i, fname4);
	for (i = 1; i < (QUICKSPELLS_MAXSIZE + 1); i++) {
		if (mqb_data4[i] == 0) {
			break;
		}
		if (fwrite(mqb_data4[i], sizeof(mqbdata), 1, fp4) != 1) {
			LOG_ERROR("Failed writing spell '%s' to file '%s'", mqb_data4[i]->spell_name, fname4);
			break;
		}
		LOG_DEBUG("Wrote spell '%s' to file '%s'", mqb_data4[i]->spell_name, fname4);
	}
	fclose(fp4);
	for (i = 1; i < QUICKSPELLS_MAXSIZE + 1; i++) {
		if (mqb_data5[i] == NULL) {
			break;
		}
	}
	// write the number of spells + 1
	fwrite(&i, sizeof(i), 1, fp5);
	LOG_DEBUG("Writing %d spells to file '%s'", i, fname5);
	for (i = 1; i < (QUICKSPELLS_MAXSIZE + 1); i++) {
		if (mqb_data5[i] == 0) {
			break;
		}
		if (fwrite(mqb_data5[i], sizeof(mqbdata), 1, fp5) != 1) {
			LOG_ERROR("Failed writing spell '%s' to file '%s'", mqb_data5[i]->spell_name, fname5);
			break;
		}
		LOG_DEBUG("Wrote spell '%s' to file '%s'", mqb_data5[i]->spell_name, fname5);
	}
	fclose(fp5);
	LEAVE_DEBUG_MARK("save spells");
}
// Quickspell window start
int quickspell_over = -1;
/*	returns the y coord position of the active base
        of the quickspell window.  If spell slots are unused
        the base is higher */
int get_quickspell_y_base() {
	if (quickspells_draggable) {
		return 0;
	}
	if (quickspell_x > HUD_MARGIN_X) {
		return 0;
	}
	return quickspell_y + quickspell_y_len;
}
int display_quickspell_handler(window_info *win) {
	int x, y, width, i;
	int over = -1;
	width = (quickspells_dim - quickspells_ico) / 2;
	glDisable(GL_TEXTURE_2D);
	glColor3f(0.77f, 0.57f, 0.39f);
	if (quickspells_dir != VERTICAL) {
		glBegin(GL_QUADS);
		glVertex3i(4, 0, 0); // top
		glVertex3i(0, 10, 0); // left
		glVertex3i(8, 10, 0); // right
		glVertex3i(4, 0, 0); // top
		glEnd();
		glBegin(GL_QUADS);
		glVertex3i(4, 22 + 10, 0); // top
		glVertex3i(0, 22, 0); // left
		glVertex3i(8, 22, 0); // right
		glVertex3i(4, 22 + 10, 0); // top
		glEnd();
	} else {
		glBegin(GL_QUADS);
		glVertex3i(0, 4, 0); // top
		glVertex3i(10, 8, 0); // left
		glVertex3i(10, 0, 0); // right
		glVertex3i(0, 4, 0); // top
		glEnd();
		glBegin(GL_QUADS);
		glVertex3i(10 + 22, 4, 0); // top
		glVertex3i(22, 8, 0); // left
		glVertex3i(22, 0, 0); // right
		glVertex3i(10 + 22, 4, 0); // top
		glEnd();
	}
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER, 0.20f);
	glEnable(GL_BLEND); // Turn Blending On
	glBlendFunc(GL_SRC_ALPHA, GL_DST_ALPHA);
	switch (quickspell_mqb_selected) {
	case 0:
		for (i = 1; i <= quickspells_size; i++) {
			if (!mqb_data[i] || !mqb_data[i]->spell_name[0]) {
				break;
			}
			y = (1 + quickspells_dim) * (i - 1) + (int)(i / quickspells_div) * quickspells_sep;
			if (quickspell_over == i) {
				over = y;
				glColor4f(1.0f, 1.0f, 1.0f, 1.5f);
			} else {
				glColor4f(1.0f, 1.0f, 1.0f, 0.8f);
			}
			if (quickspells_dir != VERTICAL) {
				x = y;
				y = 1;
			} else {
				x = 1;
			}
			draw_spell_icon(mqb_data[i]->spell_image, x + width, y + width, quickspells_ico, 0, 0);
		}
		break;
	case 1:
		for (i = 1; i <= quickspells_size; i++) {
			if (!mqb_data2[i] || !mqb_data2[i]->spell_name[0]) {
				break;
			}
			y = (1 + quickspells_dim) * (i - 1) + (int)(i / quickspells_div) * quickspells_sep;
			if (quickspell_over == i) {
				over = y;
				glColor4f(1.0f, 1.0f, 1.0f, 1.5f);
			} else {
				glColor4f(1.0f, 1.0f, 1.0f, 0.8f);
			}
			if (quickspells_dir != VERTICAL) {
				x = y;
				y = 1;
			} else {
				x = 1;
			}
			draw_spell_icon(mqb_data2[i]->spell_image, x + width, y + width, quickspells_ico, 0, 0);
		}
		break;
	case 2:
		for (i = 1; i <= quickspells_size; i++) {
			if (!mqb_data3[i] || !mqb_data3[i]->spell_name[0]) {
				break;
			}
			y = (1 + quickspells_dim) * (i - 1) + (int)(i / quickspells_div) * quickspells_sep;
			if (quickspell_over == i) {
				over = y;
				glColor4f(1.0f, 1.0f, 1.0f, 1.5f);
			} else {
				glColor4f(1.0f, 1.0f, 1.0f, 0.8f);
			}
			if (quickspells_dir != VERTICAL) {
				x = y;
				y = 1;
			} else {
				x = 1;
			}
			draw_spell_icon(mqb_data3[i]->spell_image, x + width, y + width, quickspells_ico, 0, 0);
		}
		break;
	case 3:
		for (i = 1; i <= quickspells_size; i++) {
			if (!mqb_data4[i] || !mqb_data4[i]->spell_name[0]) {
				break;
			}
			y = (1 + quickspells_dim) * (i - 1) + (int)(i / quickspells_div) * quickspells_sep;
			if (quickspell_over == i) {
				over = y;
				glColor4f(1.0f, 1.0f, 1.0f, 1.5f);
			} else {
				glColor4f(1.0f, 1.0f, 1.0f, 0.8f);
			}
			if (quickspells_dir != VERTICAL) {
				x = y;
				y = 1;
			} else {
				x = 1;
			}
			draw_spell_icon(mqb_data4[i]->spell_image, x + width, y + width, quickspells_ico, 0, 0);
		}
		break;
	case 4:
		for (i = 1; i <= quickspells_size; i++) {
			if (!mqb_data5[i] || !mqb_data5[i]->spell_name[0]) {
				break;
			}
			y = (1 + quickspells_dim) * (i - 1) + (int)(i / quickspells_div) * quickspells_sep;
			if (quickspell_over == i) {
				over = y;
				glColor4f(1.0f, 1.0f, 1.0f, 1.5f);
			} else {
				glColor4f(1.0f, 1.0f, 1.0f, 0.8f);
			}
			if (quickspells_dir != VERTICAL) {
				x = y;
				y = 1;
			} else {
				x = 1;
			}
			draw_spell_icon(mqb_data5[i]->spell_image, x + width, y + width, quickspells_ico, 0, 0);
		}
		break;
	default:
		break;
	}
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	glDisable(GL_BLEND); // Turn Blending Off
	glDisable(GL_ALPHA_TEST);
	if (selected_spell != -1) {
		int loc = (1 + quickspells_dim) * (selected_spell - 1) + (int)(selected_spell / quickspells_div) * quickspells_sep;
		if (quickspells_dir == VERTICAL) {
			show_help("*", -5 - 8, loc + (quickspells_dim - SMALL_FONT_Y_LEN) / 2);
		} else {
			show_help("*", loc + width, 5 + quickspells_dim);
		}
	}
	mqbdata **m = mqb_data;
	mqbdata **ml[] = {mqb_data, mqb_data2, mqb_data3, mqb_data4, mqb_data5};
	if ((unsigned)quickspell_mqb_selected < 5) {
		m = ml[quickspell_mqb_selected];
	}
	mqbdata *d = m[quickspell_over];
	if (quickspell_over > 0 && d) {
		const char *n = d->spell_name;
		for (int i = 0; i < SPELLS_NO; ++i) {
			if (spells_list[i].id == d->spell_id) {
				n = spells_list[i].name;
				break;
			}
		}
		if (quickspells_dir == VERTICAL) {
			show_help(n, -5 - strlen(n) * 8, over + (quickspells_dim - SMALL_FONT_Y_LEN) / 2);
		} else {
			show_help(n, over + width, 5 + quickspells_dim);
		}
	}
	quickspell_over = -1;
	return 1;
}
int mouseover_quickspell_handler(window_info *win, int mx, int my) {
	int pos;
	pos = get_quickspell_from_mouse(mx, my) + 1;
	switch (quickspell_mqb_selected) {
	case 0:
		if (pos > 0 && mqb_data[pos] && mqb_data[pos]->spell_name[0]) {
			quickspell_over = pos;
			elwin_mouse = CURSOR_WAND;
			return 1;
		}
		break;
	case 1:
		if (pos > 0 && mqb_data2[pos] && mqb_data2[pos]->spell_name[0]) {
			quickspell_over = pos;
			elwin_mouse = CURSOR_WAND;
			return 1;
		}
		break;
	case 2:
		if (pos > 0 && mqb_data3[pos] && mqb_data3[pos]->spell_name[0]) {
			quickspell_over = pos;
			elwin_mouse = CURSOR_WAND;
			return 1;
		}
		break;
	case 3:
		if (pos > 0 && mqb_data4[pos] && mqb_data4[pos]->spell_name[0]) {
			quickspell_over = pos;
			elwin_mouse = CURSOR_WAND;
			return 1;
		}
		break;
	case 4:
		if (pos > 0 && mqb_data5[pos] && mqb_data5[pos]->spell_name[0]) {
			quickspell_over = pos;
			elwin_mouse = CURSOR_WAND;
			return 1;
		}
		break;
	default:
		break;
	}
	return 0;
}
int click_quickspell_handler(window_info *win, int mx, int my, Uint32 flags) {
	int pos;
	int ctrl_on = flags & ELW_CTRL;
	int shift_on = flags & ELW_SHIFT;
	// actions sur la barre (clic droit + modifieur)
	if (flags & ELW_RIGHT_MOUSE) {
		if (ctrl_on && shift_on) {
			reset_quickspells();
			return 1;
		} else if (ctrl_on) {
			toggle_quickspells_draggable();
			return 1;
		} else if (shift_on) {
			flip_quickspells();
			return 1;
		}
	}
	pos = get_quickspell_from_mouse(mx, my) + 1;
	// menus contextuels (clic droit simple)
	if (flags & ELW_RIGHT_MOUSE) {
		// clic droit sur un icone : affichage du menu contextuel des icones (général inclus)
		switch (quickspell_mqb_selected) {
		case 0:
			if (pos > 0 && mqb_data[pos] && mqb_data[pos]->spell_name[0]) {
				cm_show_direct(cm_quickspells_id, quickspell_win, -1);
				return 1;
			}
			break;
		case 1:
			if (pos > 0 && mqb_data2[pos] && mqb_data2[pos]->spell_name[0]) {
				cm_show_direct(cm_quickspells_id, quickspell_win, -1);
				return 1;
			}
			break;
		case 2:
			if (pos > 0 && mqb_data3[pos] && mqb_data3[pos]->spell_name[0]) {
				cm_show_direct(cm_quickspells_id, quickspell_win, -1);
				return 1;
			}
			break;
		case 3:
			if (pos > 0 && mqb_data4[pos] && mqb_data4[pos]->spell_name[0]) {
				cm_show_direct(cm_quickspells_id, quickspell_win, -1);
				return 1;
			}
			break;
		case 4:
			if (pos > 0 && mqb_data5[pos] && mqb_data5[pos]->spell_name[0]) {
				cm_show_direct(cm_quickspells_id, quickspell_win, -1);
				return 1;
			}
			break;
		default:
			break;
		}
		// clic droit en dehors des icones : affichage du menu contextuel général
		cm_show_direct(cm_quickspells_win_id, quickspell_win, -1);
		return 1;
	}
	mqbdata **m = mqb_data;
	mqbdata **ml[] = {mqb_data, mqb_data2, mqb_data3, mqb_data4, mqb_data5};
	if ((unsigned)quickspell_mqb_selected < 5) {
		m = ml[quickspell_mqb_selected];
	}
	// actions sur un raccourci (clic gauche)
	mqbdata *d = m[pos];
	if (pos > 0 && d && d->spell_name[0]) {
		if (!ctrl_on && !shift_on) {
			if (!d->spell_str[0]) {
				return 0;
			}
			if (set_fast_spell_target) {
				fast_spell_cible(pos);
				set_fast_spell_target = 0;
			} else {
				send_spell(d->spell_str, d->spell_str[1] + 2);
			}
			return 1;
		} else if (!ctrl_on && shift_on) {
			remove_spell_from_quickbar(pos);
			return 1;
		} else if (ctrl_on && !shift_on) {
			move_spell_on_quickbar(pos, 2); // en premier
			return 1;
		} else if (ctrl_on && shift_on) {
			move_spell_on_quickbar(pos, 3); // en dernier
			return 1;
		}
	}
	if (pos == 0) {
		if (quickspells_dir != VERTICAL) {
			if (mx >= 0 && mx <= 8 && my >= 0 && my <= 10) {
				quickspell_mqb_selected++;
				if (quickspell_mqb_selected > 4) {
					quickspell_mqb_selected = 0;
				}
				return 1;
			} else if (mx >= 0 && mx <= 8 && my >= 22 && my <= 32) {
				quickspell_mqb_selected--;
				if (quickspell_mqb_selected < 0) {
					quickspell_mqb_selected = 4;
				}
				return 1;
			}
		} else {
			if (mx >= 0 && mx <= 10 && my >= 0 && my <= 8) {
				quickspell_mqb_selected--;
				if (quickspell_mqb_selected < 0) {
					quickspell_mqb_selected = 4;
				}
				return 1;
			} else if (mx >= 22 && mx <= 32 && my >= 0 && my <= 8) {
				quickspell_mqb_selected++;
				if (quickspell_mqb_selected > 4) {
					quickspell_mqb_selected = 0;
				}
				return 1;
			}
		}
	}
	return 0;
}
// menu contextuel général (barre de titre ou fenêtre hors icones)
static int context_quickspellwin_handler(window_info *win, int widget_id, int mx, int my, int option) {
	switch (option) {
	case 0:
		quickspells_draggable ^= 1;
		toggle_quickspells_draggable();
		break;
	case 1:
		flip_quickspells();
		break;
	case 2:
		reset_quickspells();
		break;
	}
	return 1;
}
static int context_quickspell_handler(window_info *win, int widget_id, int mx, int my, int option) {
	int pos = get_quickspell_from_mouse(mx, my) + 1;
	if (pos > 0 && mqb_data[pos] && mqb_data[pos]->spell_name[0]) {
		switch (option) {
		case 0:
			move_spell_on_quickbar(pos, 0);
			break;
		case 1:
			move_spell_on_quickbar(pos, 1);
			break;
		case 2:
			move_spell_on_quickbar(pos, 2);
			break;
		case 3:
			move_spell_on_quickbar(pos, 3);
			break;
		case 4:
			remove_spell_from_quickbar(pos);
			break;
		case 5:
			fast_spell_cible(pos);
			break;
		case 7:
			quickspells_draggable ^= 1;
			toggle_quickspells_draggable();
			break;
		case 8:
			flip_quickspells();
			break;
		case 9:
			reset_quickspells();
			break;
		}
	}
	return 1;
}
void init_quickspell() {
	// on remonte/descend les barres rapides placées sur le hud sous le logo
	if ((!hud_x) && (!quickspells_draggable) && (quickspells_dir == VERTICAL) && (quickspell_x <= HUD_MARGIN_X)) {
		if (quickspell_y == HUD_MARGIN_X) {
			quickspell_y = 0;
		}
	}
	if (quickspell_win < 0) {
		quickspell_win = create_window("Quickspell", -1, 0, window_width - quickspell_x, quickspell_y, quickspell_x_len, quickspell_y_len, ELW_CLICK_TRANSPARENT | ELW_TITLE_NONE | ELW_SHOW_LAST);
		set_window_handler(quickspell_win, ELW_HANDLER_DISPLAY, &display_quickspell_handler);
		set_window_handler(quickspell_win, ELW_HANDLER_CLICK, &click_quickspell_handler);
		set_window_handler(quickspell_win, ELW_HANDLER_MOUSEOVER, &mouseover_quickspell_handler);
		// menu contextuel général (barre de titre + clic sur fond hors icones)
		cm_quickspells_win_id = cm_create(cm_quickspellwin_menu_str, &context_quickspellwin_handler);
		cm_bool_line(cm_quickspells_win_id, 0, &quickspells_draggable, NULL);
		cm_bool_line(cm_quickspells_win_id, 3, &quickspells_on_top, NULL);
		// menu contextuel des icones (menu général intégré à la fin)
		cm_quickspells_id = cm_create(cm_quickspell_menu_str, &context_quickspell_handler);
		cm_add(cm_quickspells_id, cm_quickspellwin_menu_str, NULL);
		cm_bool_line(cm_quickspells_id, 5, &fast_spell_cible_bool, NULL);
		cm_bool_line(cm_quickspells_id, 7, &quickspells_draggable, NULL);
		cm_bool_line(cm_quickspells_id, 10, &quickspells_on_top, NULL);
	} else {
		change_flags(quickspell_win, ELW_CLICK_TRANSPARENT | ELW_TITLE_NONE | ELW_SHOW_LAST);
		if (quickspells_draggable) {
			show_window(quickspell_win);
		} else if (quickspell_y > window_height - 10 || quickspell_x < 10) {
			move_window(quickspell_win, -1, 0, window_width - HUD_MARGIN_X, HUD_MARGIN_X);
		} else {
			move_window(quickspell_win, -1, 0, window_width - quickspell_x, quickspell_y);
		}
	}
	// on change les flags de la fenêtre draggable seulement maintenant pour éviter
	// qu'elle soit créée avec le menu contextuel par défaut de la barre de titre
	if (quickspells_draggable) {
		change_flags(quickspell_win, ELW_CLICK_TRANSPARENT | ELW_SHOW_LAST | ELW_TITLE_BAR | ELW_DRAGGABLE | ELW_USE_BACKGROUND | ELW_SWITCHABLE_OPAQUE);
	}
}
// Quickspells end
// CAST FUNCTIONS
int spell_clear_handler() {
	int i;
	for (i = 0; i < 6; i++) {
		on_cast[i] = -1;
	}
	we_have_spell = -1;
	spell_text[0] = 0;
	return 1;
}
void send_spell(Uint8 *str, int len) {
	my_tcp_send(my_socket, str, len);
	memcpy(last_spell_str, str, len);
	last_spell_len = len;
}
int action_spell_keys(Uint32 key) {
	size_t i;
	Uint32 keys[] = {
		K_SPELL1, K_SPELL2, K_SPELL3, K_SPELL4, K_SPELL5, K_SPELL6,
		K_SPELL7, K_SPELL8, K_SPELL9, K_SPELL10, K_SPELL11, K_SPELL12
	};
	for (i = 0; (i < sizeof(keys) / sizeof(Uint32)) & (i < quickspells_size); i++) {
		if (key == keys[i]) {
			if (set_fast_spell_target) {
				fast_spell_cible(i + 1);
				set_fast_spell_target = 0;
				return 1;
			}
			switch (quickspell_mqb_selected) {
			case 0:
				if (mqb_data[i + 1] && mqb_data[i + 1]->spell_str[0]) {
					send_spell(mqb_data[i + 1]->spell_str, mqb_data[i + 1]->spell_str[1] + 2);
				}
				return 1;
				break;
			case 1:
				if (mqb_data2[i + 1] && mqb_data2[i + 1]->spell_str[0]) {
					send_spell(mqb_data2[i + 1]->spell_str, mqb_data2[i + 1]->spell_str[1] + 2);
				}
				return 1;
				break;
			case 2:
				if (mqb_data3[i + 1] && mqb_data3[i + 1]->spell_str[0]) {
					send_spell(mqb_data3[i + 1]->spell_str, mqb_data3[i + 1]->spell_str[1] + 2);
				}
				return 1;
				break;
			case 3:
				if (mqb_data4[i + 1] && mqb_data4[i + 1]->spell_str[0]) {
					send_spell(mqb_data4[i + 1]->spell_str, mqb_data4[i + 1]->spell_str[1] + 2);
				}
				return 1;
				break;
			case 4:
				if (mqb_data5[i + 1] && mqb_data5[i + 1]->spell_str[0]) {
					send_spell(mqb_data5[i + 1]->spell_str, mqb_data5[i + 1]->spell_str[1] + 2);
				}
				return 1;
				break;
			default:
				break;
			}
		}
	}
	if (key == K_PREVQUICKSPELLBAR) {
		quickspell_mqb_selected--;
		if (quickspell_mqb_selected < 0) {
			quickspell_mqb_selected = 4;
		}
		return 1;
	} else if (key == K_NEXTQUICKSPELLBAR) {
		quickspell_mqb_selected++;
		if (quickspell_mqb_selected > 4) {
			quickspell_mqb_selected = 0;
		}
		return 1;
	}
	return 0;
}
int prepare_for_cast() {
	Uint8 str[20];
	int count = 0;
	int sigils_no = 0;
	int i;
	for (i = 0; i < 6; i++) {
		if (on_cast[i] != -1) {
			count++;
		}
	}
	if (count < 2) {
		safe_snprintf((char *)spell_text, sizeof(spell_text), "%c%s", 127 + c_red2, sig_too_few_sigs);
		have_error_message = 1;
		return 0;
	}
	str[0] = CAST_SPELL;
	for (i = 0; i < 6; i++) {
		if (on_cast[i] != -1) {
			str[sigils_no + 2] = on_cast[i];
			sigils_no++;
		}
	}
	str[1] = sigils_no;
	if (!mqb_data[0]) {
		mqb_data[0] = (mqbdata *)calloc(1, sizeof(mqbdata));
		mqb_data[0]->spell_id = -1;
	}
	if (sigil_win != sigils_win && we_have_spell >= 0) {
		mqb_data[0]->spell_id = spells_list[we_have_spell].id;
		mqb_data[0]->spell_image = spells_list[we_have_spell].image;
		memcpy(mqb_data[0]->spell_name, spells_list[we_have_spell].name, 60);
	}
	memcpy(mqb_data[0]->spell_str, str, sigils_no + 2); // Copy the last spell send to the server
	return sigils_no;
}
int cast_handler() {
	// Cast?
	int sigils_no = prepare_for_cast();
	// ok, send it to the server...
	if (sigils_no) {
		send_spell(mqb_data[0]->spell_str, sigils_no + 2);
	}
	return 1;
}
// Calc windows size based on xml data
void calc_spell_windows() {
	int i, gy = 0, y;
	// calc spell_win
	for (i = 0; i < num_groups; i += 2) {
		gy += MAX(33 * (groups_list[i].spells / (SPELLS_ALIGN_X + 1) + 1) + 20, 33 * (groups_list[i + 1].spells / (SPELLS_ALIGN_X + 1) + 1) + 20);
	}
	spell_x_len = SPELLS_ALIGN_X * 33 * 2 + 33 + 50;
	spell_y_len = 10 + gy + 50 + 15 + 37;
	spell_y_len_ext = spell_y_len + 35;
	y = 10;
	for (i = 0; i < num_groups; i++) {
		groups_list[i].x = 20;
		groups_list[i].y = y + 15;
		if (i == num_groups - 1) {
			groups_list[i].x += ((2 * 33 * SPELLS_ALIGN_X + 33) - (33 * SPELLS_ALIGN_X)) / 2; // if groups are odd, last one is drawn in the middle
		}
		i++;
		if (i >= num_groups) {
			break;
		}
		groups_list[i].x += 20 + 33 + 33 * SPELLS_ALIGN_X;
		groups_list[i].y = y + 15;
		y += 20 + 33 * MAX(groups_list[i - 1].spells / (SPELLS_ALIGN_X + 1) + 1, groups_list[i].spells / (SPELLS_ALIGN_X + 1) + 1);
	}
	// calc spell_mini_win
	spell_mini_rows = 0;
	for (i = 0; i < num_groups; i++) {
		spell_mini_rows += groups_list[i].spells / (SPELLS_ALIGN_X + 1) + 1;
	}
	spell_mini_x_len = SPELLS_ALIGN_X * 33 + 50;
	spell_mini_y_len = 10 + 33 * spell_mini_rows + 20 + 30 + 37;
}
int invocation_depuis_sorts(Uint8 quantity) {
	if (nb_recettes_necro > 0) {
		int i, j, k, l, total;
		int objet_recette[6][3] = {0};
		Uint8 str[32];
		int items_no = 0;
		total = liste_items_necro[creature_en_cours][0];
		for (i = 0; i < total; i++) {
			objet_recette[i][0] = liste_items_necro[creature_en_cours][i + 1];
		}
		if (double_invoc) {
			total++;
			objet_recette[total - 1][0] = 41;
		}
		for (i = 0; i < 36; i++) {
			if (item_list[i].quantity > 0) {
				for (j = 0; j < total; j++) {
					if (item_list[i].id == objet_recette[j][0]) {
						objet_recette[j][1] = item_list[i].quantity;
						objet_recette[j][2] = item_list[i].pos;
					}
				}
			}
		}
		if (quantity > 1) {
			l = 2;
		} else {
			l = 1;
		}
		k = 0;
		for (i = 0; i < liste_items_necro[creature_en_cours][0]; i++) {
			if (objet_recette[i][1] >= (liste_items_necro[creature_en_cours][i + 1 + liste_items_necro[creature_en_cours][0] * 2])) {
				k++;
			}
		}
		if (double_invoc) {
			if (objet_recette[total - 1][1] >= (1)) {
				k++;
			}
		}
		if (k == total) {
			str[0] = MANUFACTURE_THIS;
			for (i = 0; i < total; i++) {
				str[items_no * 3 + 2] = objet_recette[i][2];
				if (double_invoc && i == total - 1) {
					*((Uint16 *)(str + items_no * 3 + 2 + 1)) = SDL_SwapLE16(1);
				} else {
					*((Uint16 *)(str + items_no * 3 + 2 + 1)) = SDL_SwapLE16(liste_items_necro[creature_en_cours][i + 1 + liste_items_necro[creature_en_cours][0] * 2]);
				}
				items_no++;
			}
			str[1] = items_no;
			if (items_no) {
				str[items_no * 3 + 2] = quantity;
				my_tcp_send(my_socket, str, items_no * 3 + 3);
			}
		} else {
			if (l == 1) {
				set_shown_string(c_red2, "Il manque des ingrédients pour pouvoir lancer l' invocation !");
			} else {
				set_shown_string(c_red2, "Il manque des ingrédients pour pouvoir lancer plusieurs invocations !");
			}
		}
	}
	return 1;
}
int une_bebete() {
	if (nb_recettes_necro > 0) {
		return invocation_depuis_sorts(1);
	} else {
		return 1;
	}
}
int plusieurs_bebetes() {
	if (nb_recettes_necro > 0) {
		return invocation_depuis_sorts(255);
	} else {
		return 1;
	}
}
// Create and show/hide our windows
void display_sigils_menu() {
	static int checked_reagents = 0;
	if (!checked_reagents) {
		if (item_info_available()) {
			int i, j;
			// check item ids/uid all give unique items
			for (i = 0; i < SPELLS_NO; i++) {
				for (j = 0; j < 4; j++) {
					if (spells_list[i].reagents_id[j] >= 0) {
						if (get_item_count(spells_list[i].reagents_uid[j], spells_list[i].reagents_id[j]) != 1) {
							LOG_ERROR("Invalid spell.xml reagents spells_list[%d].reagents_uid[%d]=%d spells_list[%d].reagents_id[%d]=%d\n", i, j, spells_list[i].reagents_uid[j], i, j, spells_list[i].reagents_id[j]);
						}
					}
				}
			}
		}
		checked_reagents = 1;
	}
	calc_spell_windows();
	if (sigils_win < 0) {
		// create sigil win
		static int cast_button_id = 100;
		static int clear_button_id = 101;
		widget_list *w_cast = NULL;
		widget_list *w_clear = NULL;
		int but_space = 0;
		int our_root_win = -1;
		if (!windows_on_top) {
			our_root_win = game_root_win;
		}
		sigils_win = create_window(win_sigils, our_root_win, 0, sigil_menu_x, sigil_menu_y, sigil_x_len, sigil_y_len, ELW_WIN_DEFAULT);
		set_window_handler(sigils_win, ELW_HANDLER_DISPLAY, &display_sigils_handler);
		set_window_handler(sigils_win, ELW_HANDLER_CLICK, &click_sigils_handler);
		set_window_handler(sigils_win, ELW_HANDLER_MOUSEOVER, &mouseover_sigils_handler);
		cast_button_id = button_add_extended(sigils_win, cast_button_id, NULL, 0, 0, 0, 0, 0, 1.0f, 0.77f, 0.57f, 0.39f, cast_str);
		widget_set_OnClick(sigils_win, cast_button_id, cast_handler);
		clear_button_id = button_add_extended(sigils_win, clear_button_id, NULL, 0, 0, 0, 0, 0, 1.0f, 0.77f, 0.57f, 0.39f, clear_str);
		widget_set_OnClick(sigils_win, clear_button_id, spell_clear_handler);
		w_cast = widget_find(sigils_win, cast_button_id);
		w_clear = widget_find(sigils_win, clear_button_id);
		but_space = (sigil_x_len - (33 * 6 + 5) - w_cast->len_x - w_clear->len_x) / 3;
		widget_move(sigils_win, cast_button_id, 33 * 6 + 5 + but_space + 5, sigil_y_len - w_cast->len_y - 4);
		widget_move(sigils_win, clear_button_id, w_cast->pos_x + w_cast->len_x + but_space, sigil_y_len - w_clear->len_y - 4);
		hide_window(sigils_win);
	}
	if (spell_win < 0) {
		// create spell win
		static int cast2_button_id = 102;
		widget_list *w_cast = NULL;
		int our_root_win = -1;
		if (!windows_on_top) {
			our_root_win = game_root_win;
		}
		// @tosh : traduction de la fenêtre de sorts
		spell_win = create_window("Grimoire - Recettes", our_root_win, 0, sigil_menu_x, sigil_menu_y, spell_x_len, spell_y_len_ext, ELW_WIN_DEFAULT);
		set_window_handler(spell_win, ELW_HANDLER_DISPLAY, &display_spells_handler);
		set_window_handler(spell_win, ELW_HANDLER_CLICK, &click_spells_handler);
		set_window_handler(spell_win, ELW_HANDLER_MOUSEOVER, &mouseover_spells_handler);
		cast2_button_id = button_add_extended(spell_win, cast2_button_id, NULL, 0, 0, 0, 0, 0, 1.0f, 0.77f, 0.57f, 0.39f, cast_str);
		widget_set_OnClick(spell_win, cast2_button_id, cast_handler);
		w_cast = widget_find(spell_win, cast2_button_id);
		widget_move(spell_win, cast2_button_id, spell_x_len - 20 - 10 - w_cast->len_x, spell_y_len_ext - w_cast->len_y - 4);
		hide_window(spell_win);
	}
	if (spell_mini_win < 0) {
		// create mini spell win
		int our_root_win = -1;
		if (!windows_on_top) {
			our_root_win = game_root_win;
		}
		// @tosh : Traduction du titre de la fenêtre
		spell_mini_win = create_window("Grimoire - Sorts", our_root_win, 0, sigil_menu_x, sigil_menu_y, spell_mini_x_len, spell_mini_y_len, ELW_WIN_DEFAULT);
		set_window_handler(spell_mini_win, ELW_HANDLER_DISPLAY, &display_spells_mini_handler);
		set_window_handler(spell_mini_win, ELW_HANDLER_CLICK, &click_spells_mini_handler);
		set_window_handler(spell_mini_win, ELW_HANDLER_MOUSEOVER, &mouseover_spells_mini_handler);
		hide_window(spell_mini_win);
	}
	if (necro_win < 0) {
		int our_root_win = -1;
		int i;
		int necro_mixone_button_id = 1001;
		int necro_mixall_button_id = 1002;
		int placement_x = 25;
		int placement_y = 190 - 15;
		nb_recettes_necro = chargement_necro_recettes(0);
		if (nb_recettes_necro == -1) {
			LOG_ERROR("Le fichier creatnecro est absent...");
		} else {
			nom_bestiole = (char **)malloc(nb_recettes_necro * sizeof(char *));
			liste_items_necro = malloc(nb_recettes_necro * sizeof(int *));
			for (i = 0 ; i < nb_recettes_necro ; i++) {
				liste_items_necro[i] = malloc(20 * sizeof(int));
			}
			nb_recettes_necro = chargement_necro_recettes(1);
		}
		if (!windows_on_top) {
			our_root_win = game_root_win;
		}
		necro_win = create_window("Nécromancie", our_root_win, 0, sigil_menu_x, sigil_menu_y, necro_x_len, necro_y_len, ELW_WIN_DEFAULT);
		set_window_handler(necro_win, ELW_HANDLER_DISPLAY, &display_necro_handler);
		set_window_handler(necro_win, ELW_HANDLER_CLICK, &click_necro_handler);
		set_window_handler(necro_win, ELW_HANDLER_MOUSEOVER, &mouseover_necro_handler);
		if (nb_recettes_necro > 0) {
			necro_mixone_button_id = button_add_extended(necro_win, necro_mixone_button_id, NULL, placement_x, placement_y, 43, 0, 0, 1.0f, 0.77f, 0.57f, 0.39f, ">");
			widget_set_OnClick(necro_win, necro_mixone_button_id, une_bebete);
			necro_mixall_button_id = button_add_extended(necro_win, necro_mixall_button_id, NULL, placement_x + 85, placement_y, 66, 0, 0, 1.0f, 0.77f, 0.57f, 0.39f, ">>");
			widget_set_OnClick(necro_win, necro_mixall_button_id, plusieurs_bebetes);
		}
		hide_window(necro_win);
	}
	check_castability();
	if (start_mini_spells == 0) {
		sigil_win = sigils_win;
	} else if (start_mini_spells == 1) {
		sigil_win = spell_mini_win;
	} else if (start_mini_spells == 2) {
		sigil_win = spell_win;
	} else if (start_mini_spells == 3) {
		sigil_win = necro_win;
	}
	switch_handler((init_ok) ? (sigil_win):(sigils_win));
}
void init_sigils() {
	int i;
	i = 0;
	// TODO: load this data from a file
	sigils_list[i].sigil_img = 0;
	my_strcp(sigils_list[i].name, (char *)sig_change.str);
	my_strcp(sigils_list[i].description, (char *)sig_change.desc);
	sigils_list[i].have_sigil = 1;
	i++;
	sigils_list[i].sigil_img = 1;
	my_strcp(sigils_list[i].name, (char *)sig_restore.str);
	my_strcp(sigils_list[i].description, (char *)sig_restore.desc);
	sigils_list[i].have_sigil = 1;
	i++;
	sigils_list[i].sigil_img = 2;
	my_strcp(sigils_list[i].name, (char *)sig_space.str);
	my_strcp(sigils_list[i].description, (char *)sig_space.desc);
	sigils_list[i].have_sigil = 1;
	i++;
	sigils_list[i].sigil_img = 3;
	my_strcp(sigils_list[i].name, (char *)sig_increase.str);
	my_strcp(sigils_list[i].description, (char *)sig_increase.desc);
	sigils_list[i].have_sigil = 1;
	i++;
	sigils_list[i].sigil_img = 4;
	my_strcp(sigils_list[i].name, (char *)sig_decrease.str);
	my_strcp(sigils_list[i].description, (char *)sig_decrease.desc);
	sigils_list[i].have_sigil = 1;
	i++;
	sigils_list[i].sigil_img = 5;
	my_strcp(sigils_list[i].name, (char *)sig_temp.str);
	my_strcp(sigils_list[i].description, (char *)sig_temp.desc);
	sigils_list[i].have_sigil = 1;
	i++;
	sigils_list[i].sigil_img = 6;
	my_strcp(sigils_list[i].name, (char *)sig_perm.str);
	my_strcp(sigils_list[i].description, (char *)sig_perm.desc);
	sigils_list[i].have_sigil = 1;
	i++;
	sigils_list[i].sigil_img = 7;
	my_strcp(sigils_list[i].name, (char *)sig_move.str);
	my_strcp(sigils_list[i].description, (char *)sig_move.desc);
	sigils_list[i].have_sigil = 1;
	i++;
	sigils_list[i].sigil_img = 8;
	my_strcp(sigils_list[i].name, (char *)sig_local.str);
	my_strcp(sigils_list[i].description, (char *)sig_local.desc);
	sigils_list[i].have_sigil = 1;
	i++;
	sigils_list[i].sigil_img = 9;
	my_strcp(sigils_list[i].name, (char *)sig_global.str);
	my_strcp(sigils_list[i].description, (char *)sig_global.desc);
	sigils_list[i].have_sigil = 1;
	i++;
	sigils_list[i].sigil_img = 10;
	my_strcp(sigils_list[i].name, (char *)sig_fire.str);
	my_strcp(sigils_list[i].description, (char *)sig_fire.desc);
	sigils_list[i].have_sigil = 1;
	i++;
	sigils_list[i].sigil_img = 11;
	my_strcp(sigils_list[i].name, (char *)sig_water.str);
	my_strcp(sigils_list[i].description, (char *)sig_water.desc);
	sigils_list[i].have_sigil = 1;
	i++;
	sigils_list[i].sigil_img = 12;
	my_strcp(sigils_list[i].name, (char *)sig_air.str);
	my_strcp(sigils_list[i].description, (char *)sig_air.desc);
	sigils_list[i].have_sigil = 1;
	i++;
	sigils_list[i].sigil_img = 13;
	my_strcp(sigils_list[i].name, (char *)sig_earth.str);
	my_strcp(sigils_list[i].description, (char *)sig_earth.desc);
	sigils_list[i].have_sigil = 1;
	i++;
	sigils_list[i].sigil_img = 14;
	my_strcp(sigils_list[i].name, (char *)sig_spirit.str);
	my_strcp(sigils_list[i].description, (char *)sig_spirit.desc);
	sigils_list[i].have_sigil = 1;
	i++;
	sigils_list[i].sigil_img = 15;
	my_strcp(sigils_list[i].name, (char *)sig_matter.str);
	my_strcp(sigils_list[i].description, (char *)sig_matter.desc);
	sigils_list[i].have_sigil = 1;
	i++;
	sigils_list[i].sigil_img = 16;
	my_strcp(sigils_list[i].name, (char *)sig_energy.str);
	my_strcp(sigils_list[i].description, (char *)sig_energy.desc);
	sigils_list[i].have_sigil = 1;
	i++;
	sigils_list[i].sigil_img = 17;
	my_strcp(sigils_list[i].name, (char *)sig_magic.str);
	my_strcp(sigils_list[i].description, (char *)sig_magic.desc);
	sigils_list[i].have_sigil = 1;
	i++;
	sigils_list[i].sigil_img = 18;
	my_strcp(sigils_list[i].name, (char *)sig_destroy.str);
	my_strcp(sigils_list[i].description, (char *)sig_destroy.desc);
	sigils_list[i].have_sigil = 1;
	i++;
	sigils_list[i].sigil_img = 19;
	my_strcp(sigils_list[i].name, (char *)sig_create.str);
	my_strcp(sigils_list[i].description, (char *)sig_create.desc);
	sigils_list[i].have_sigil = 1;
	i++;
	sigils_list[i].sigil_img = 20;
	my_strcp(sigils_list[i].name, (char *)sig_knowledge.str);
	my_strcp(sigils_list[i].description, (char *)sig_knowledge.desc);
	sigils_list[i].have_sigil = 1;
	i++;
	sigils_list[i].sigil_img = 21;
	my_strcp(sigils_list[i].name, (char *)sig_protection.str);
	my_strcp(sigils_list[i].description, (char *)sig_protection.desc);
	sigils_list[i].have_sigil = 1;
	i++;
	sigils_list[i].sigil_img = 22;
	my_strcp(sigils_list[i].name, (char *)sig_remove.str);
	my_strcp(sigils_list[i].description, (char *)sig_remove.desc);
	sigils_list[i].have_sigil = 1;
	i++;
	sigils_list[i].sigil_img = 23;
	my_strcp(sigils_list[i].name, (char *)sig_health.str);
	my_strcp(sigils_list[i].description, (char *)sig_health.desc);
	sigils_list[i].have_sigil = 1;
	i++;
	sigils_list[i].sigil_img = 24;
	my_strcp(sigils_list[i].name, (char *)sig_life.str);
	my_strcp(sigils_list[i].description, (char *)sig_life.desc);
	sigils_list[i].have_sigil = 1;
	i++;
	sigils_list[i].sigil_img = 25;
	my_strcp(sigils_list[i].name, (char *)sig_death.str);
	my_strcp(sigils_list[i].description, (char *)sig_death.desc);
	sigils_list[i].have_sigil = 1;
	i++;
	sigils_list[i].sigil_img = 26;
	my_strcp(sigils_list[i].name, (char *)sig_chant.str);
	my_strcp(sigils_list[i].description, (char *)sig_chant.desc);
	sigils_list[i].have_sigil = 1;
}
void fast_spell_cible(int spell_id) {
	char str[100];
	if (selected_spell == -1) {
		fast_spell_cible_bool = 1;
		selected_spell = spell_id;
	} else if (selected_spell == spell_id) {
		fast_spell_cible_bool = 0;
		selected_spell = -1;
	} else {
		fast_spell_cible_bool = 1;
		selected_spell = spell_id;
	}
	mqbdata **m = mqb_data;
	mqbdata **ml[] = {mqb_data, mqb_data2, mqb_data3, mqb_data4, mqb_data5};
	if ((unsigned)quickspell_mqb_selected < 5) {
		m = ml[quickspell_mqb_selected];
	}
	if (m[spell_id]) {
		if (fast_spell_cible_bool) {
			safe_snprintf(str, sizeof(str), "Tu as sélectionné \"%s\" !", m[spell_id]->spell_name);
			LOG_TO_CONSOLE(c_green1, str);
		} else {
			LOG_TO_CONSOLE(c_orange1, "Tu ne séléctionne plus aucuns sorts.");
		}
	}
}
void fast_spell_cast(void) {
	Uint8 str[10];
	if (selected_spell_target >= 0) {
		actor *this_actor = get_actor_ptr_from_id(selected_spell_target);
		if (this_actor != NULL) {
			add_highlight(this_actor->x_tile_pos, this_actor->y_tile_pos, HIGHLIGHT_TYPE_SPELL_TARGET);
			str[0] = TOUCH_PLAYER;
			*((int *)(str + 1)) = SDL_SwapLE32((int)selected_spell_target);
			my_tcp_send(my_socket, str, 5);
		}
	}
}
void fast_spell_teleport(void) {
	Uint8 str[10];
	if (selected_spell_target >= 0) {
		str[0] = ATTACK_SOMEONE;
		*((int *)(str + 1)) = SDL_SwapLE32((int)selected_spell_target);
		my_tcp_send(my_socket, str, 5);
	}
}
void fast_spell_decible(void) {
	fast_spell_cible_bool = 0;
	selected_spell = -1;
	LOG_TO_CONSOLE(c_orange1, "Tu ne séléctionne plus aucuns sorts.");
}
