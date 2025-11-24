#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "spells.h"
#ifdef ENGLISH
#include "actors.h"
#endif //ENGLISH
#include "asc.h"
#include "cursors.h"
#ifdef FR_FENETRE_NECRO
#include "console.h"
#endif //FR_FENETRE_NECRO
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
#ifdef ENGLISH
#include "named_colours.h"
#endif //ENGLISH
#include "pathfinder.h"
#include "textures.h"
#include "translate.h"
#include "counters.h"
#include "errors.h"
#include "io/elpathwrapper.h"
#include "sound.h"
#ifdef FR_VERSION
#include "themes.h"
#endif //FR_VERSION

#ifdef FR_FAST_SPELL
#include "highlight.h"
#endif

#define SIGILS_NO 64
#define	NUM_SIGILS_LINE	12	// how many sigils per line displayed
#define	NUM_SIGILS_ROW	3	// how many rows of sigils are there?
#ifdef ENGLISH
#define MAX_DATA_FILE_SIZE 560
#else //ENGLISH
#define MAX_DATA_FILE_SIZE 1850 // QUICKSPELLS_MAXSIZE * 92 + 1 ? (default 560)
#define QUICKSPELLS_MAXSIZE 20
#endif //ENGLISH
#define SIGILS_NO 64
#define SPELLS_NO 32
#define GROUPS_NO 8
#ifdef FR_FENETRE_NECRO
#define NECRO_NO 5
#endif //FR_FENETRE_NECRO

#define UNCASTABLE_REAGENTS 1
#define UNCASTABLE_SIGILS 2
#define UNCASTABLE_MANA 4
#define UNCASTABLE_LVLS 8

#ifdef ENGLISH
#define UNCASTABLE_SIGILS_STR "(missing sigils)"
#define UNCASTABLE_REAGENTS_STR "(not enough reagents)"
#define UNCASTABLE_MANA_STR "(not enough mana)"
#define UNCASTABLE_LVLS_STR "(not enough levels)"
#else //ENGLISH
#define UNCASTABLE_SIGILS_STR "(rune(s) manquante(s))"
#define UNCASTABLE_REAGENTS_STR "(pas assez d'essences)"
#define UNCASTABLE_MANA_STR "(pas assez de mana)"
#define UNCASTABLE_LVLS_STR "(pas le niveau)"
#endif //ENGLISH

#define GET_UNCASTABLE_STR(cast) (  (cast&UNCASTABLE_SIGILS) ? (UNCASTABLE_SIGILS_STR):( (cast&UNCASTABLE_LVLS) ? (UNCASTABLE_LVLS_STR):(  (cast&UNCASTABLE_MANA) ? (UNCASTABLE_MANA_STR):( (cast&UNCASTABLE_REAGENTS) ? (UNCASTABLE_REAGENTS_STR):("")   )          )        )            )

#define SPELLS_ALIGN_X 7
#define MAX(a,b) (((a)>(b))?(a):(b))

#ifdef FR_FENETRE_NECRO
#define NECRO_ALIGN_X 1
#define NECRO_SPACE_X 10 // espace entre bord gauche de la fenêtre et la grille de nécro
#define NECRO_SPACE_Y 10 // espace entre bord haut de la fenêtre et la grille de nécro
#define NECRO_SPACE_Y_BETWEEN 2 //espace vertical entre les différentes icones de nécro
#endif //FR_FENETRE_NECRO

#define SET_COLOR(x) glColor4f((float) colors_list[x].r1 / 255.0f,(float) colors_list[x].g1 / 255.0f,(float) colors_list[x].b1 / 255.0f,1.0f)
typedef struct
{
	int sigil_img;
	char name[32];
	char description[64];
	int have_sigil;
}sigil_def;

int selected_spell = -1;
int selected_spell_sent;
int selected_spell_target = -1;
int fast_spell_cible_bool;
int set_fast_spell_target;

sigil_def sigils_list[SIGILS_NO];
int sigils_text;
int sigils_we_have;

#ifdef FR_FENETRE_NECRO
typedef struct {
    int necro_img; //image de l'icone : position dans gamebuttons.dds cf liste de #define dans hud.c
    char name[60]; //contient la commande sous forme #commande
    char description [120]; //description de la commande - contient le texte à afficher
} necro_def;

//necro_list se voit assigner des valeurs par la fonction de parsage du fichier spells.xml cf init_spells();
necro_def necro_list[NECRO_NO+1]; //+1 pour le sort de soin des invoqués
int mouseover_necro = -1; //permet d'afficher dans display_necro_handler() le nom de la commande lorsque la souris passe sur l'icone. valeur par défaut : -1 (souris ailleurs), sinon : entier >= 0

//pour le sort de soin des invoqués : positions x et y
int pos_x_sdi;
int pos_y_sdi;
#endif //FR_FENETRE_NECRO

typedef struct {
	int id;//The spell server id
	char name[60];//The spell name
	char desc[120];//The spell description
	int image;//image_id
	int sigils[6];//index of required sigils in sigils_list
	int mana;//required mana
	attrib_16 *lvls[NUM_WATCH_STAT];//pointers to your_info lvls
	int lvls_req[NUM_WATCH_STAT];//minimum lvls requirement
	int reagents_id[4]; //reagents needed
	Uint16 reagents_uid[4]; //reagents needed, unique item id
	int reagents_qt[4]; //their quantities
#ifdef ENGLISH
	Uint32 buff;
#endif //ENGLISH
	int uncastable; //0 if castable, otherwise if something missing
} spell_info;

spell_info spells_list[SPELLS_NO];
int num_spells;
Uint8 spell_text[256];
unsigned char spell_help[256];
Sint8 on_cast[6];
Uint8 last_spell_str[20];
int last_spell_len;
int spell_result;
int have_error_message;
int we_have_spell = -1; //selected spell
int on_spell = -1;//mouse over this spell

typedef struct {
	unsigned char desc[120];
	int spells;
	int spells_id[SPELLS_NO];
	int x,y;
} group_def;
int num_groups;
group_def groups_list[GROUPS_NO];

typedef struct
{
	Sint8 spell;
	Uint32 cast_time;
	Uint32 duration;
#ifdef NEW_SOUND
	unsigned int sound;
#endif
} spell_def;
spell_def active_spells[NUM_ACTIVE_SPELLS];

//windows related
int sigil_win=-1; //this is referred externally so we will change it when we switch windows
int sigils_win=-1;
int spell_win=-1;
int spell_mini_win=-1;
int last_win=-1;
#ifdef FR_FENETRE_NECRO
int necro_win = -1;
#ifdef FR_NECRO_RECETTES
int creature_en_cours;
int double_invoc;
int securite_invoc = 1;
int nb_recettes_necro = -1;//Nombre recettes de nécro		45
char **nom_bestiole;	//Tableau des noms des invocation	char nom_bestiole[45][30]
int **liste_items_necro;//Tableau des recettes			int liste_items_necro[45][20]    (mars 2020)
static char items_string[350];
static size_t last_items_string_id;
#endif //FR_NECRO_RECETTES
#endif //FR_FENETRE_NECRO
int start_mini_spells; //do we start minimized?
int init_ok;
int sigil_menu_x=10;
int sigil_menu_y=20;
//big window
int spell_x_len;
int spell_y_len;
int spell_y_len_ext;
//sigil window
int sigil_x_len=NUM_SIGILS_LINE*33+20;
int sigil_y_len=(3+NUM_SIGILS_ROW)*33;
//mini window
int spell_mini_x_len;
int spell_mini_y_len;
int spell_mini_rows;
#ifdef FR_FENETRE_NECRO
//necro window
int necro_x_len = (NECRO_NO+1) * 33 + NECRO_SPACE_Y + 40 + NECRO_SPACE_Y_BETWEEN * 8;
int necro_y_len = NECRO_ALIGN_X * 33 + NECRO_SPACE_X + 55 + 190 - 15;
#endif //FR_FENETRE_NECRO
int clear_mouseover;
int cast_mouseover;
Mqbspell *mqbars[5][QUICKSPELLS_MAXSIZE + 1];
int quickspell_mqb_selected;
int quickspells_nb;             // nombre de raccourcis existants
int quickspells_size = 6;           // nombre de raccourcis    (cf ini)
int quickspells_dir = VERTICAL;     // orientation par défaut  (cf cfg)
int quickspells_on_top = 1;         // toujours au dessus ?    (cf cfg)
int quickspells_draggable;      // fenêtre déplaçable ?    (cf cfg)
int quickspell_x = HUD_MARGIN_X;    // position x par défaut   (cf cfg)
int quickspell_y = HUD_MARGIN_X;    // position y par défaut   (cf cfg)
int quickspell_x_len = (1+30)*1 +1; // largeur par défaut
int quickspell_y_len = (1+30)*1 +1; // longueur par défaut
int quickspells_loaded;

int cast_handler();
int prepare_for_cast();
void draw_spell_icon(int id,int x_start, int y_start, int gridsize, int alpha, int grayed);
void set_spell_help_text(int spell);
void init_sigils();

#ifdef FR_VERSION
size_t cm_quickspells_win_id = CM_INIT_VALUE;
#endif //FR_VERSION
size_t cm_quickspells_id = CM_INIT_VALUE;
void cm_update_quickspells(void);

#ifdef FR_RCM_MAGIE
int sort_menu_x=10;
int sort_menu_y=20;
int sort_menu_x_len=9*33+20;
int sort_menu_y_len=4*33+115;
static int button_y_top = 4*33+40;
static int button_y_bot = 4*33+60;
#endif

void repeat_spell(){
	if(last_spell_len > 0)
		my_tcp_send(my_socket, last_spell_str, last_spell_len);
}

//returns a node with tagname, starts searching from the_node
xmlNode *get_XML_node(xmlNode *the_node, char *tagname){
	xmlNode *node=the_node;

	while(node) {
		if(node->type==XML_ELEMENT_NODE && xmlStrcasecmp (node->name, (xmlChar*)tagname) == 0) return node;
		else node=node->next;
	}
	return node;
}


attrib_16 *get_skill_address(const char *skillname)
{
	if(strcmp(skillname,(char*)attributes.manufacturing_skill.shortname)==0) return &your_info.manufacturing_skill;
	if(strcmp(skillname,(char*)attributes.alchemy_skill.shortname)==0) return &your_info.alchemy_skill;
	if(strcmp(skillname,(char*)attributes.magic_skill.shortname)==0) return &your_info.magic_skill;
	if(strcmp(skillname,(char*)attributes.summoning_skill.shortname)==0) return &your_info.summoning_skill;
	if(strcmp(skillname,(char*)attributes.attack_skill.shortname)==0) return &your_info.attack_skill;
	if(strcmp(skillname,(char*)attributes.defense_skill.shortname)==0) return &your_info.defense_skill;
	if(strcmp(skillname,(char*)attributes.crafting_skill.shortname)==0) return &your_info.crafting_skill;
	if(strcmp(skillname,(char*)attributes.engineering_skill.shortname)==0) return &your_info.engineering_skill;
	if(strcmp(skillname,(char*)attributes.potion_skill.shortname)==0) return &your_info.potion_skill;
	if(strcmp(skillname,(char*)attributes.tailoring_skill.shortname)==0) return &your_info.tailoring_skill;
	if(strcmp(skillname,(char*)attributes.ranging_skill.shortname)==0) return &your_info.ranging_skill;
	if(strcmp(skillname,(char*)attributes.overall_skill.shortname)==0) return &your_info.overall_skill;
	if(strcmp(skillname,(char*)attributes.harvesting_skill.shortname)==0) return &your_info.harvesting_skill;
	return NULL;
}

int put_on_cast(){
	if(we_have_spell>=0){
		int i;
		for(i=0;i<6;i++)
			if(spells_list[we_have_spell].sigils[i]>=0)
				if(!sigils_list[spells_list[we_have_spell].sigils[i]].have_sigil) {
					//we miss at least a sigil, clear on_cast
					int j;
					for(j=0;j<6;j++) on_cast[j]=-1;
					return 0;
				}
		for(i=0;i<6;i++) on_cast[i]=spells_list[we_have_spell].sigils[i];
		return 1;
	}
	return 0;
}

int init_spells ()
{
	int i,j;
	xmlNode *root;
	xmlDoc *doc;
	int ok = 1;
	char *fname="./spells.xml";

#ifdef ENGLISH
	buff_duration_colour_id = elglGetColourId("buff.duration.background");
#endif //ENGLISH
	//init textures and structs
#ifdef	NEW_TEXTURES
	sigils_text = load_texture_cached("textures/sigils.dds", tt_gui);
#else	/* NEW_TEXTURES */
	sigils_text = load_texture_cache ("./textures/sigils.bmp", 0);
#endif	/* NEW_TEXTURES */
	for (i = 0; i < SIGILS_NO; i++)
		sigils_list[i].have_sigil = 0;
	for (i = 0; i < SPELLS_NO; i++){
		spells_list[i].image = -1;
		for(j=0;j<6;j++)
			spells_list[i].sigils[j] =-1;
		for(j=0;j<4;j++) {
			spells_list[i].reagents_id[j] = -1;
			spells_list[i].reagents_uid[j] = unset_item_uid;
		}
		for(j=0;j<NUM_WATCH_STAT;j++)
			spells_list[i].lvls[j] = NULL;
		spells_list[i].uncastable=0;
	}
	for (i = 0; i < GROUPS_NO; i++){
		groups_list[i].spells = 0;
		for(j=0;j<SPELLS_NO;j++) groups_list[i].spells_id[j]=-1;
	}


	spell_text[0]=spell_help[0]=0;
	i = 0;
	//parse xml
	doc = xmlReadFile(fname, NULL, 0);
	if (doc == 0) {
		LOG_ERROR("Unable to read spells definition file %s: %s", fname, strerror(errno));
		ok = 0;
	}

	root = xmlDocGetRootElement (doc);
	if (root == 0)
	{
		LOG_ERROR("Unable to parse spells definition file %s", fname);
		ok = 0;
	}
	else if (xmlStrcasecmp (root->name, (xmlChar*)"Magic") != 0)
	{
		LOG_ERROR("Unknown key \"%s\" (\"Magic\" expected).", root->name);
		ok = 0;
	}
	else
	{
		xmlNode *node;
		xmlNode *data;
#ifdef ENGLISH
		char tmp[200]
#endif //ENGLISH
        char name[200];
#ifdef ENGLISH
		const int expected_version = 1;
		int actual_version = -1;
#endif //ENGLISH
		i = 0;

#ifdef ENGLISH
		if ((actual_version = xmlGetInt(root,(xmlChar*)"version")) < expected_version)
		{
			safe_snprintf(tmp, sizeof(tmp), "Warning: %s file is out of date expecting %d, actual %d.", fname, expected_version, actual_version);
			LOG_TO_CONSOLE (c_red1, tmp);
		}
#endif //ENGLISH

		//parse spells
		node = get_XML_node(root->children, "Spell_list");
		node = get_XML_node(node->children, "spell");

		while (node)
		{
			int j;
#ifndef ENGLISH
            // Pour gerer correctement les accents dans ce fichier
            char* nom_fr = NULL;
            char* desc_fr = NULL;
#endif //ENGLISH
			memset(name, 0, sizeof(name));

			data=get_XML_node(node->children,"name");

			if (data == 0)
			{
				LOG_ERROR("No name for %d spell", i);
			}

#ifdef ENGLISH
			get_string_value(name, sizeof(name), data);
			safe_strncpy(spells_list[i].name, name,
				sizeof(spells_list[i].name));
#else //ENGLISH
            nom_fr = fromUTF8 (data->children->content, strlen ((const char*) data->children->content));
			safe_strncpy(spells_list[i].name, nom_fr, sizeof(spells_list[i].name));
            if (nom_fr)
            {
                free(nom_fr);
            }
#endif //ENGLISH

			data=get_XML_node(node->children, "desc");

			if (data == 0)
			{
				LOG_ERROR("No desc for spell '%s'[%d]",
					name, i);
			}
#ifdef ENGLISH
			get_string_value(tmp, sizeof(tmp), data);
			safe_strncpy(spells_list[i].desc, tmp,
				sizeof(spells_list[i].desc));
#else //ENGLISH
            desc_fr = fromUTF8 (data->children->content, strlen ((const char*) data->children->content));
			safe_strncpy(spells_list[i].desc, desc_fr, sizeof(spells_list[i].desc));
            if (desc_fr)
            {
                free(desc_fr);
            }
#endif //ENGLISH

			data=get_XML_node(node->children, "id");

			if (data == 0)
			{
				LOG_ERROR("No id for spell '%s'[%d]",
					name, i);
			}

			spells_list[i].id=get_int_value(data);

			data=get_XML_node(node->children,"icon");

			if (data == 0)
			{
				LOG_ERROR("No icon for spell '%s'[%d]",
					name, i);
			}

			spells_list[i].image = get_int_value(data);

			data=get_XML_node(node->children, "mana");

			if (data == 0)
			{
				LOG_ERROR("No mana for spell '%s'[%d]",
					name, i);
			}

			spells_list[i].mana = get_int_value(data);

			data=get_XML_node(node->children,"lvl");

			if (data == 0)
			{
				LOG_ERROR("No lvl for spell '%s'[%d]",
					name, i);
			}

			j = 0;
			while (data)
            {
				const char *skill = get_string_property(data,"skill");
				spells_list[i].lvls_req[j] = get_int_value(data);
				spells_list[i].lvls[j] = get_skill_address(skill);
				j++;
				data = get_XML_node(data->next,"lvl");
			}

			data = get_XML_node(node->children,"group");

			if (data == 0)
			{
				LOG_ERROR("No group for spell '%s'[%d]",
					name, i);
			}

			while (data)
			{
				int g;

				g = get_int_value(data);
				groups_list[g].spells_id[groups_list[g].spells] = i;
				groups_list[g].spells++;
				data = get_XML_node(data->next, "group");
			}

			data = get_XML_node(node->children, "sigil");

			if (data == 0)
			{
				LOG_ERROR("No sigil for spell '%s'[%d]",
					name, i);
			}

			j = 0;
			while (data)
			{
				spells_list[i].sigils[j] = get_int_value(data);
				j++;
				data = get_XML_node(data->next, "sigil");
			}

			data=get_XML_node(node->children, "reagent");

			if (data == 0)
			{
				LOG_ERROR("No reagent for spell '%s'[%d]",
					name, i);
			}

			j = 0;
			while (data)
			{
				int tmpval = -1;
				spells_list[i].reagents_id[j] =
					get_int_property(data, "id");
				if ((tmpval = get_int_property(data, "uid")) >= 0)
					spells_list[i].reagents_uid[j] = (Uint16)tmpval;
				spells_list[i].reagents_qt[j] =
					get_int_value(data);
				j++;
				data = get_XML_node(data->next, "reagent");
			}

#ifdef ENGLISH
			data = get_XML_node(node->children, "duration");

			if (data != 0)
			{
				spells_list[i].duration = get_int_value(data);
			}
			else
			{
				spells_list[i].duration = 0;
			}

			data = get_XML_node(node->children, "buff");

			if (data != 0)
			{
				spells_list[i].buff = get_int_value(data);
			}
			else
			{
				spells_list[i].buff = 0xFFFFFFFF;
			}
#endif //ENGLISH
			node = get_XML_node(node->next, "spell");
	i++;
		}
		num_spells = i;

		//parse sigils
		node = get_XML_node(root->children, "Sigil_list");
		node = get_XML_node(node->children, "sigil");
		while (node)
        {
#ifndef ENGLISH
            char* runes_fr = NULL;
#endif //ENGLISH
			int k;
			k = get_int_property(node, "id");
			sigils_list[k].sigil_img = k;
#ifdef ENGLISH
			get_string_value(sigils_list[k].description,
				sizeof(sigils_list[k].description), node);
			safe_strncpy((char*)sigils_list[k].name,
				get_string_property(node, "name"),
				sizeof(sigils_list[k].name));
#else //ENGLISH
            runes_fr = fromUTF8 (node->children->content, strlen ((const char*) node->children->content));
			safe_strncpy(sigils_list[k].name, runes_fr, sizeof(sigils_list[k].name));
            if (runes_fr)
            {
                free(runes_fr);
            }
#endif //ENGLISH

			data=get_XML_node(node->children,"id");
			sigils_list[k].have_sigil = 1;
			node = get_XML_node(node->next, "sigil");
		}

#ifdef FR_FENETRE_NECRO
		//parse necro
		node = get_XML_node(root->children, "Necro_list");
		node = get_XML_node(node->children, "necro");
		while (node)
        {
			int k = get_int_property(node,"id"); //id de la commande
			int img = get_int_property(node, "img"); //n° de l'image pour le fichier texture gamebuttons
			necro_list[k].necro_img=img;

			get_string_value(necro_list[k].description, sizeof(necro_list[k].description), node); //description du sort
			safe_strncpy((char*)necro_list[k].name, get_string_property(node,"name"), sizeof(necro_list[k].name)); //#commande

			node = get_XML_node(node->next, "necro");
		}
		//Soin des invoqués :
		safe_strncpy((char*)necro_list[5].description, "Soigner", sizeof(necro_list[5].description));
#endif //FR_FENETRE_NECRO

		//parse groups
		num_groups = 0;
		node = get_XML_node(root->children,"Groups");
		node = get_XML_node(node->children,"group");
		while (node)
        {
#ifndef ENGLISH
            char* groupes_fr = NULL;
#endif //ENGLISH
			int k;
			k = get_int_property(node, "id");
#ifdef ENGLISH
			get_string_value(tmp, sizeof(tmp), node);
			safe_strncpy((char*)groups_list[k].desc, tmp,
				sizeof(groups_list[k].desc));
#else //ENGLISH
            groupes_fr = fromUTF8 (node->children->content, strlen ((const char*) node->children->content));
			safe_strncpy((char*)groups_list[k].desc, groupes_fr, sizeof(groups_list[k].desc));
            if (groupes_fr)
            {
                free(groupes_fr);
            }
#endif //ENGLISH
			num_groups++;
			node = get_XML_node(node->next, "group");
		}
	}

	xmlFreeDoc (doc);

	//init arrays
	for (i = 0; i < 6; i++)
	{
		on_cast[i] = -1;
	}
	for (i = 0; i < NUM_ACTIVE_SPELLS; i++)
	{
		active_spells[i].spell = -1;
		active_spells[i].cast_time = 0;
		active_spells[i].duration = 0;
#ifdef NEW_SOUND
		if (active_spells[i].sound > 0)
		{
			stop_sound(active_spells[i].sound);
		}
#endif // NEW_SOUND
	}

	if (!ok) //xml failed, init sigils manually
	{
		init_sigils();
	}

	init_ok = ok;

	return ok;
}

void check_castability()
{
	int i,j,k,l;

	for(i=0;i<num_spells;i++){
		spells_list[i].uncastable=0;
		//Check Mana
		if (have_stats && your_info.ethereal_points.cur<spells_list[i].mana) spells_list[i].uncastable|=UNCASTABLE_MANA;
		//Check Sigils
		for(j=0;j<6;j++){
			k=spells_list[i].sigils[j];
			if(k>=0 && !sigils_list[k].have_sigil) spells_list[i].uncastable|=UNCASTABLE_SIGILS;
		}
		//Check Reagents
		for(j=0;j<4&&spells_list[i].reagents_id[j]>=0;j++){
			l=0;
			for(k=0;k<ITEM_WEAR_START;k++) {
				if ((item_list[k].quantity > 0) &&
					(item_list[k].image_id == spells_list[i].reagents_id[j]) &&
					((item_list[k].id == unset_item_uid) ||
						(spells_list[i].reagents_uid[j] == unset_item_uid) ||
						(item_list[k].id == spells_list[i].reagents_uid[j])) ) {
					l=1;
					if(item_list[k].quantity<spells_list[i].reagents_qt[j]) {
						spells_list[i].uncastable|=UNCASTABLE_REAGENTS;
						break;
					}
				}
			}
			if(!l){
				//no reagent j found
				spells_list[i].uncastable|=UNCASTABLE_REAGENTS;
			}
		}
		//Check Levels
		for(j=0;j<NUM_WATCH_STAT&&spells_list[i].lvls[j];j++)
			if(spells_list[i].lvls[j])
				if(spells_list[i].lvls[j]->cur<spells_list[i].lvls_req[j]) spells_list[i].uncastable|=UNCASTABLE_LVLS;

	}
	//when castabilitychanges, update spell_help
	set_spell_help_text(we_have_spell);
}

#ifdef ENGLISH
/* called each time we get poisoned - perhaps */
void increment_poison_incidence(void)
{
	poison_drop_counter++;
}

/* called from display_game_handler() so we are in a position to draw text */
void draw_spell_icon_strings(void)
{
	size_t i;
	int x_start = 0;
	int y_start = window_height-hud_y-64-SMALL_FONT_Y_LEN;

	for (i = 0; i < NUM_ACTIVE_SPELLS; i++)
	{
		unsigned char str[20];
		/* handle the poison count */
		if ((poison_drop_counter > 0) && (active_spells[i].spell == 2) && show_poison_count)
		{
			safe_snprintf((char*)str, sizeof(str), "%d", poison_drop_counter );
			draw_string_small_shadowed(x_start+(33-strlen((char *)str)*SMALL_FONT_X_LEN)/2, y_start, str, 1, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f);
		}
		/* other strings on spell icons, timers perhaps .....*/
		x_start += 33;
	}

}
#endif //ENGLISH





//ACTIVE SPELLS
#ifdef ENGLISH
void get_active_spell(int pos, int spell)
#else //ENGLISH
// durée restante obtenue du serveur (l'US utilise le fichier XML)
void get_active_spell(int pos, int spell, int timer)
#endif //ENGLISH
{
#ifdef ENGLISH
	Uint32 i;
#endif //ENGLISH

	active_spells[pos].spell = spell;
#ifdef ENGLISH
	active_spells[pos].cast_time = 0;
	request_buff_duration(spell);
#else //ENGLISH
	// mémorisation de la date de fin plutot que celle du début
	active_spells[pos].cast_time = cur_time + timer * 1000;
#endif //ENGLISH
#ifdef NEW_SOUND
	active_spells[pos].sound = add_spell_sound(spell);
#endif // NEW_SOUND

#ifndef ENGLISH
	// conservation de la durée totale (max) plutot que celle restante
	if (timer > active_spells[pos].duration) active_spells[pos].duration = timer;
#endif //ENGLISH
}

void remove_active_spell(int pos)
{
#if defined(BUFF_DURATION_DEBUG)
	if (active_spells[pos].duration > 0)
		duration_debug(active_spells[pos].spell, diff_game_time_sec(active_spells[pos].cast_time), "actual duration");
#endif
	if (active_spells[pos].spell == 2)
#ifdef ENGLISH
		poison_drop_counter = 0;
#endif //ENGLISH
	active_spells[pos].spell = -1;
	active_spells[pos].cast_time = 0;
	active_spells[pos].duration = 0;
#ifdef NEW_SOUND
	if (active_spells[pos].sound > 0)
		stop_sound(active_spells[pos].sound);
#endif // NEW_SOUND
}

#ifdef ENGLISH
static void rerequest_durations(void)
{
	size_t i;
	for (i = 0; i < NUM_ACTIVE_SPELLS; i++)
	{
		if (active_spells[i].spell >= 0)
			request_buff_duration(active_spells[i].spell);
	}
}

#if defined(BUFF_DURATION_DEBUG)
int command_buff_duration(char *text, int len)
{
	LOG_TO_CONSOLE(c_green1, "Request buff durations");
	rerequest_durations();
	return 1;
}
#endif
#endif //ENGLISH

void get_active_spell_list(const Uint8 *my_spell_list)
{
#ifdef ENGLISH
	Uint32 i, j;
	int cur_spell;
#else //ENGLISH
	Uint32 i;
#endif //ENGLISH

	for (i = 0; i < NUM_ACTIVE_SPELLS; i++)
	{
#ifdef ENGLISH
		active_spells[i].spell = my_spell_list[i];
		active_spells[i].duration = active_spells[i].cast_time = 0;
		if (active_spells[i].spell >= 0)
			request_buff_duration(active_spells[i].spell);
#else //ENGLISH
		// le serveur FR envoit des données différentes
		active_spells[i].spell = my_spell_list[i*3];
		// récupération de la durée restante
		active_spells[i].duration = unpack_u16_le(my_spell_list+i*3+1);
		if (active_spells[i].spell < 0) active_spells[i].duration = 0;
		// mémorisation de la date de fin du sort
		active_spells[i].cast_time = cur_time + active_spells[i].duration * 1000;
#endif //ENGLISH
#ifdef NEW_SOUND
		active_spells[i].sound = add_spell_sound(active_spells[i].spell);
#endif // NEW_SOUND
#ifdef ENGLISH
		if (active_spells[i].spell == 2)
			increment_poison_incidence();
#endif //ENGLISH
	}
}

#ifdef NEW_SOUND
void restart_active_spell_sounds(void)
{
	Uint32 i;

	for (i = 0; i < NUM_ACTIVE_SPELLS; i++)
	{
		if (active_spells[i].sound > 0)
		{
			stop_sound(active_spells[i].sound);
		}
		if (active_spells[i].spell != -1)
		{
			active_spells[i].sound = add_spell_sound(active_spells[i].spell);
	    }
	}
}
#endif // NEW_SOUND

int we_are_poisoned()
{
	Uint32 i;

	for (i = 0; i < NUM_ACTIVE_SPELLS; i++)
	{
		if (active_spells[i].spell == 2)
		{
			return 1;
		}
	}
	return 0;
}

#ifndef ENGLISH
// détermine la couleur de la jauge
// un pourcentage négatif indique une inversion du dégradé (rouge->vert)
void set_timer_color(float percent, float multiplier)
{
	float r, g;
	if (percent < 0) percent+= 1.0f;
	r = (1.0f - percent) * 2.5f;
	g = (percent / 3.0f) * 5.0f;
	if (r < 0.0f) r = 0.0f;
	else if (r > 1.0f) r = 1.0f;
	if (g < 0.0f) g = 0.0f;
	else if (g > 1.0f) g = 1.0f;
	glColor3f(r*multiplier, g*multiplier, 0.0f);
}
#endif //ENGLISH

void time_out(const float x_start, const float y_start, const float gridsize,
	const float progress)
{
	glDisable(GL_TEXTURE_2D);
#ifdef ENGLISH
	glEnable(GL_BLEND);

	elglColourI(buff_duration_colour_id);
#else //ENGLISH
	glColor3f(0.0f, 0.7f, 0.0f);
#endif //ENGLISH

	glBegin(GL_QUADS);
#ifdef ENGLISH
		glVertex2f(x_start, y_start + gridsize * progress);
		glVertex2f(x_start + gridsize, y_start + gridsize * progress);
		glVertex2f(x_start + gridsize, y_start + gridsize);
		glVertex2f(x_start, y_start + gridsize);
#else //ENGLISH
		// Affichage d'une jauge avec coloration variable
		set_timer_color(progress, 1.0f);
		glVertex2f(x_start, y_start + gridsize);
		glVertex2f(x_start, y_start + gridsize - gridsize * fabs(progress));
		set_timer_color(progress, 0.3f);
		glVertex2f(x_start + 5, y_start + gridsize - gridsize * fabs(progress));
		glVertex2f(x_start + 5, y_start + gridsize);
#endif //ENGLISH
	glEnd();
#ifdef ENGLISH
	glDisable(GL_BLEND);
#endif //ENGLISH
	glEnable(GL_TEXTURE_2D);
	glColor3f(1.0f, 1.0f, 1.0f);
}

void display_spells_we_have()
{
	Uint32 i;
#ifdef ENGLISH
	float scale, duration;
#else //ENGLISH
	float scale, duration, cur_time;
#endif //ENGLISH
#ifndef ENGLISH
	// numéro de position globale incrémentée ne laissant pas de trou entre les sorts
	int cur_pos = 0;
#endif //ENGLISH

#ifdef ENGLISH
	if (your_actor != NULL)
	{
		static int last_actor_type = -1;
		if (last_actor_type < 0)
			last_actor_type = your_actor->actor_type;
		if (last_actor_type != your_actor->actor_type)
		{
			last_actor_type = your_actor->actor_type;
			rerequest_durations();
		}
	}
#endif //ENGLISH

#ifdef OPENGL_TRACE
	CHECK_GL_ERRORS();
#endif //OPENGL_TRACE
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

#ifndef ENGLISH
	cur_time = SDL_GetTicks();
#endif //ENGLISH

	//ok, now let's draw the objects...
	for (i = 0; i < NUM_ACTIVE_SPELLS; i++)
	{
		if (active_spells[i].spell != -1)
		{
#ifndef ENGLISH
			int cur_spell;
			char string[10];
#else //ENGLISH
			int cur_spell,cur_pos;
#endif //ENGLISH
			int x_start,y_start;

			//get the UV coordinates.
			cur_spell = active_spells[i].spell + 32;	//the first 32 icons are the sigils

			//get the x and y
#ifdef ENGLISH
			cur_pos=i;

			x_start=33*cur_pos;
			y_start=window_height-hud_y-64;

			duration = active_spells[i].duration;

			if (duration > 0.0)
			{
				scale = diff_game_time_sec(active_spells[i].cast_time) / duration;

				if ((scale >= 0.0) && (scale <= 1.0))
				{
					time_out(x_start, y_start, 32, scale);
				}
			}
#else //ENGLISH
			cur_pos++;

			x_start = 8; // la jauge prend place de 0 à 5
			y_start = window_height - 120 - 36*(cur_pos); // ne pas tenir compte du hud_y

			// calcul du temps restant en secondes
			duration = (active_spells[i].cast_time - cur_time + 980) / 1000.0f;
			if (duration < 0) continue;
			sprintf(string, "%u", (int)duration);

			// affichage d'une jauge colorée
			scale = (active_spells[i].duration) ? (duration / active_spells[i].duration) : 0;
			if (active_spells[i].spell == 2) scale = -scale;
			time_out(0, y_start, 32, scale);

			// clignotement sur les 5 dernières secondes
			if ((duration <= 5) && ((int)(cur_time / 250) % 2))
			{
				draw_string_small_shadowed(x_start+16 - 4*strlen(string), y_start+10, (unsigned char*)string, 1, 1.0f,0.0f,0.0f, 0.0f,0.0f,0.0f);
				continue;
			}
#endif //ENGLISH

			glEnable(GL_BLEND);
			draw_spell_icon(cur_spell,x_start,y_start,32,0,0);
			glDisable(GL_BLEND);
#ifndef ENGLISH
			// affichage du temps restant par dessus l'icone
			draw_string_small_shadowed(x_start+16 - 4*strlen(string), y_start+10, (unsigned char*)string, 1, 1.0f,1.0f,1.0f, 0.0f,0.0f,0.0f);
#endif //ENGLISH
		}
	}
#ifdef OPENGL_TRACE
	CHECK_GL_ERRORS();
#endif //OPENGL_TRACE
}

int show_last_spell_help=0;



//DISPLAY HANDLERS

int draw_switcher(window_info *win){

	glDisable(GL_TEXTURE_2D);
#ifdef FR_VERSION
	glEnable(GL_POINT_SMOOTH);
	glPointSize(ELW_BOX_SIZE - 8);
	glColor3f(0.77f,0.57f,0.39f);
	glBegin(GL_POINTS);
		glVertex3i(win->len_x - ELW_BOX_SIZE*0.5, ELW_BOX_SIZE*1.5, 0);
		glVertex3i(win->len_x - ELW_BOX_SIZE*0.5, ELW_BOX_SIZE*2.5, 0);
		glVertex3i(win->len_x - ELW_BOX_SIZE*0.5, ELW_BOX_SIZE*3.5, 0);
#ifdef FR_FENETRE_NECRO
        glVertex3i(win->len_x - ELW_BOX_SIZE*0.5, ELW_BOX_SIZE*4.5, 0);
#endif //FR_FENETRE_NECRO
	glEnd();
	glPointSize(ELW_BOX_SIZE - 10);
	glColor3f(0.0f,0.0f,0.0f);
	glBegin(GL_POINTS);
		glVertex3i(win->len_x - ELW_BOX_SIZE*0.5, ELW_BOX_SIZE*1.5, 0);
		glVertex3i(win->len_x - ELW_BOX_SIZE*0.5, ELW_BOX_SIZE*2.5, 0);
		glVertex3i(win->len_x - ELW_BOX_SIZE*0.5, ELW_BOX_SIZE*3.5, 0);
#ifdef FR_FENETRE_NECRO
		glVertex3i(win->len_x - ELW_BOX_SIZE*0.5, ELW_BOX_SIZE*4.5, 0);
#endif //FR_FENETRE_NECRO
	glEnd();
	glPointSize(ELW_BOX_SIZE - 14);
	glColor3f(0.77f,0.57f,0.39f);
	glBegin(GL_POINTS);
		if      (sigil_win==sigils_win)     glVertex3i(win->len_x - ELW_BOX_SIZE*0.5, ELW_BOX_SIZE*1.5, 0);
		else if (sigil_win==spell_mini_win) glVertex3i(win->len_x - ELW_BOX_SIZE*0.5, ELW_BOX_SIZE*2.5, 0);
		else if (sigil_win==spell_win)      glVertex3i(win->len_x - ELW_BOX_SIZE*0.5, ELW_BOX_SIZE*3.5, 0);
#ifdef FR_FENETRE_NECRO
        else if (sigil_win==necro_win)      glVertex3i(win->len_x - ELW_BOX_SIZE*0.5, ELW_BOX_SIZE*4.5, 0);
#endif //FR_FENETRE_NECRO
	glEnd();
	glPointSize(1.0);
	glDisable(GL_POINT_SMOOTH);
#else //FR_VERSION
	glColor3f(0.77f,0.57f,0.39f);

	//Draw switcher spells <-> sigils
	glBegin(GL_LINES);
		glVertex3i(win->len_x-20,40,0);
		glVertex3i(win->len_x,40,0);
		glVertex3i(win->len_x-20,40,0);
		glVertex3i(win->len_x-20,20,0);
	glEnd();
	glBegin(GL_QUADS);
		glVertex3i(win->len_x-15,35,0);
		glVertex3i(win->len_x-5,35,0);
		glVertex3i(win->len_x-5,25,0);
		glVertex3i(win->len_x-15,25,0);
	glEnd();

	if (sigil_win==spell_win || sigil_win==spell_mini_win) {
		//Draw switcher spells <-> mini
		glBegin(GL_LINES);
			glVertex3i(win->len_x-20,60,0);
			glVertex3i(win->len_x,60,0);
			glVertex3i(win->len_x-20,60,0);
			glVertex3i(win->len_x-20,40,0);
			if(sigil_win==spell_win) {
			//arrow down
					glVertex3i(win->len_x-15,45,0);
					glVertex3i(win->len_x-10,55,0);
					glVertex3i(win->len_x-10,55,0);
					glVertex3i(win->len_x-5,45,0);
			} else {
			//arrow up
					glVertex3i(win->len_x-15,55,0);
					glVertex3i(win->len_x-10,45,0);
					glVertex3i(win->len_x-10,45,0);
					glVertex3i(win->len_x-5,55,0);
			}
		glEnd();
	}
#endif //FR_VERSION
	glEnable(GL_TEXTURE_2D);
	return 1;
}

void draw_spell_icon(int id,int x_start, int y_start, int gridsize, int alpha, int grayed){

		float u_start,v_start,u_end,v_end;

#ifdef	NEW_TEXTURES
#ifdef FR_VERSION
	u_start = 0.125f/2 * (id % 16);
	v_start = 0.125f/2 * (id / 16);
	u_end = u_start + 0.125f/2;
	v_end = v_start + 0.125f/2;
#else //FR_VERSION
	u_start = 0.125f * (id % 8);
	v_start = 0.125f * (id / 8);
	u_end = u_start + 0.125f;
	v_end = v_start + 0.125f;
#endif //FR_VERSION

	bind_texture(sigils_text);
#else	/* NEW_TEXTURES */
#ifdef FR_VERSION
	u_start=0.125f/2*(id%16);
	v_start=1.0f-((float)32/512*(id/16));
	u_end=u_start+0.125f/2;
	v_end=v_start-0.125f/2;
#else //FR_VERSION
	u_start=0.125f*(id%8);
	v_start=1.0f-((float)32/256*(id/8));
	u_end=u_start+0.125f;
	v_end=v_start-0.125f;
#endif //FR_VERSION

	get_and_set_texture_id(sigils_text);
#endif	/* NEW_TEXTURES */
	if(alpha) {
		glEnable(GL_ALPHA_TEST);
		glAlphaFunc(GL_GREATER, 0.05f);
		glBegin(GL_QUADS);
			draw_2d_thing(u_start,v_start,u_end,v_end, x_start,y_start,x_start+gridsize,y_start+gridsize);
		glEnd();
		glDisable(GL_ALPHA_TEST);
	} else {
		glBegin(GL_QUADS);
			draw_2d_thing(u_start,v_start,u_end,v_end, x_start,y_start,x_start+gridsize,y_start+gridsize);
		glEnd();
	}

	if(grayed) gray_out(x_start,y_start,gridsize);

}

void draw_current_spell(int x, int y, int sigils_too){

	glEnable(GL_TEXTURE_2D);
	glColor3f(1.0f,1.0f,1.0f);
	if(we_have_spell>=0){
		int i,j;
		unsigned char str[4];
		j=we_have_spell;
		draw_spell_icon(spells_list[j].image,x,y,32,1,0);

		if(sigils_too){
			//draw sigils
			x+=33*2;
			for(i=0;i<6;i++){
				if (spells_list[j].sigils[i]<0) break;
				draw_spell_icon(spells_list[j].sigils[i],x+33*i,y,32,0,spells_list[j].uncastable&UNCASTABLE_SIGILS);
				if(spells_list[j].uncastable&UNCASTABLE_SIGILS&&!sigils_list[spells_list[j].sigils[i]].have_sigil) gray_out(x+33*i,y,32);
			}
	}

		//draw reagents
		x+= (sigils_too) ? (33*6+33):(33+16);
        // Modification car sur les sorts avec 4 essences différentes
        // l'affichage pose soucis
		for (i = 0; i < 4 && spells_list[j].reagents_id[i] > 0; ++i) {
			draw_item(spells_list[j].reagents_id[i],x+33*i,y,33);
			safe_snprintf((char *)str, sizeof(str), "%i",spells_list[j].reagents_qt[i]);
			draw_string_small_shadowed(x+33*i, y+21, (unsigned char*)str, 1,1.0f,1.0f,1.0f, 0.0f, 0.0f, 0.0f);
			if(spells_list[j].uncastable&UNCASTABLE_REAGENTS) gray_out(x+33*i,y+1,32);
		}
		//draw mana
		x+=(sigils_too) ? (33*5):(33*4+17);
		safe_snprintf((char *)str, sizeof(str), "%i",spells_list[j].mana);
		if (spells_list[j].uncastable&UNCASTABLE_MANA) glColor3f(1.0f,0.0f,0.0f);
		else glColor3f(0.0,1.0,0.0);
		i=(33-get_string_width(str))/2;
		j=(33-get_char_width(str[0]))/2;
		draw_string(x+i,y+j,str,1);
	}

	//draw strings
	x=20;
	glColor3f(0.77f,0.57f,0.39f);
	if(sigils_too) {
		x+=33*2;
#ifdef ENGLISH
		draw_string_small(x,y-15,(unsigned char*)"Sigils",1);
#else //ENGLISH
		draw_string_small(x,y-15,(unsigned char*)"Runes",1);
#endif //ENGLISH
		x+=33*6+33;
	} else x+=33+16;

#ifdef ENGLISH
	draw_string_small(x,y-15,(unsigned char*)"Reagents",1);
#else //ENGLISH
	draw_string_small(x,y-15,(unsigned char*)"Essences",1);
#endif //ENGLISH
	x+=33*4+((sigils_too) ? (33):(17));
	draw_string_small(x,y-15,(unsigned char*)"Mana",1);

	//draw grids
	glDisable(GL_TEXTURE_2D);
	x=20;
	if(sigils_too) {
		x+=33*2;
		rendergrid (6, 1, x, y, 33, 33);
		x+=33*6+33;
	} else x+=33+16;
	rendergrid (4, 1, x, y, 33, 33);
	x+=33*4+((sigils_too) ? (33):(17));
	rendergrid (1, 1, x, y, 33, 33);
}

static inline Mqbspell **cur_mqb(void) {
	return mqbars[quickspell_mqb_selected];
}
int display_sigils_handler(window_info *win)
{
	int i;
	int x_start,y_start;

	if (init_ok) draw_switcher(win);

	glEnable(GL_TEXTURE_2D);
	glColor3f(1.0f,1.0f,1.0f);

	//let's add the new spell icon if we have one
	x_start=350;
	y_start=112;
	Mqbspell **m = cur_mqb();
	if (m[0] && m[0]->spell_id != -1) draw_spell_icon(m[0]->spell_image, x_start, y_start, 32, 1, 0);

	//ok, now let's draw the objects...
	for(i=0;i<SIGILS_NO;i++){
		if(sigils_list[i].have_sigil){
			//get the x and y
			x_start=33*(i%NUM_SIGILS_LINE)+1;
			y_start=33*(i/NUM_SIGILS_LINE);
			draw_spell_icon(sigils_list[i].sigil_img,x_start,y_start,32,0,0);
		}
	}

	//ok, now let's draw the sigils on the list
	for(i=0;i<6;i++)
	{
		if(on_cast[i]!=-1)
		{
			//get the x and y
			x_start=33*(i%6)+5;
			y_start=sigil_y_len-37;
			draw_spell_icon(on_cast[i],x_start,y_start,32,0,0);
		}
	}

	//now, draw the inventory text, if any.
	draw_string_small(4,sigil_y_len-90,spell_text,4);

	// Render the grid *after* the images. It seems impossible to code
	// it such that images are rendered exactly within the boxes on all
	// cards
	glDisable(GL_TEXTURE_2D);
	glColor3f(0.77f,0.57f,0.39f);

	rendergrid (NUM_SIGILS_LINE, NUM_SIGILS_ROW, 0, 0, 33, 33);
	rendergrid (6, 1, 5, sigil_y_len-37, 33, 33);

	glEnable(GL_TEXTURE_2D);

	if (show_last_spell_help && m[0] && m[0]->spell_id != -1) show_help(m[0]->spell_name, 350 - 8*strlen(m[0]->spell_name), 120);
	show_last_spell_help=0;
#ifdef OPENGL_TRACE
CHECK_GL_ERRORS();
#endif //OPENGL_TRACE

	return 1;
}

int display_spells_handler(window_info *win){

	int i,j,k,x,y;

	draw_switcher(win);

	//Draw spell groups
	for(i=0;i<num_groups;i++){
		x=groups_list[i].x;
		y=groups_list[i].y;
		glEnable(GL_TEXTURE_2D);
		glColor3f(1.0f,1.0f,1.0f);
		draw_string_small(x,y-15,groups_list[i].desc,1);
		for(k=0,j=0;j<groups_list[i].spells;j++){
			draw_spell_icon(spells_list[groups_list[i].spells_id[j]].image,
					x+33*(k%SPELLS_ALIGN_X),
					y+33*(k/SPELLS_ALIGN_X),32,
					0,spells_list[groups_list[i].spells_id[j]].uncastable);
			k++;
		}
		glDisable(GL_TEXTURE_2D);
		glColor3f(0.77f,0.57f,0.39f);
		rendergrid(SPELLS_ALIGN_X,groups_list[i].spells/(SPELLS_ALIGN_X+1)+1,x,y,33,33);
	}

	glEnable(GL_TEXTURE_2D);

	//draw spell text & help
	glColor3f(1.0f,1.0f,1.0f);
	draw_string_small(20,spell_y_len-100,spell_text,4);
#ifdef ENGLISH
	draw_string_small(20,spell_y_len+5,spell_help,2);
#else //ENGLISH
	draw_string_small(20,spell_y_len+5,spell_help,3);
#endif //ENGLISH

	//draw the bottom bar
	draw_current_spell(20,spell_y_len-37,1);
	if(we_have_spell>=0&&spells_list[we_have_spell].uncastable){
		//not castable
		glColor3f(1.0f,0.0f,0.0f);
		rendergrid(1,1,20,spell_y_len-37,33,33);

	}

#ifdef OPENGL_TRACE
CHECK_GL_ERRORS();
#endif //OPENGL_TRACE
	return 1;
}


int display_spells_mini_handler(window_info *win){

	int i,j,x,y,cg,cs;

	draw_switcher(win);

	glEnable(GL_TEXTURE_2D);
	x=20;y=10;
	glColor3f(1.0f,1.0f,1.0f);
	for (i=0,cs=0,cg=0;i<spell_mini_rows;i++)
		for (j=0;j<SPELLS_ALIGN_X;j++){
			if (cs==groups_list[cg].spells) {cs=0; cg++; break;}
			draw_spell_icon(spells_list[groups_list[cg].spells_id[cs]].image,
					x+j*33,y+33*i,32,
					0,spells_list[groups_list[i].spells_id[j]].uncastable);
			cs++;
#ifndef ENGLISH
			//@tosh : correction d'un bug qui survient si la ligne est remplie de runes.
			if(j == SPELLS_ALIGN_X-1){cg++; cs = 0;}
#endif
		}

	//draw spell help
	if(on_spell==-2) {
		//mouse over the bottom-left selected spell icon, show uncastability
		int l=(int)(get_string_width((unsigned char*)GET_UNCASTABLE_STR(spells_list[we_have_spell].uncastable))*(float)DEFAULT_SMALL_RATIO);
		SET_COLOR(c_red2);
		draw_string_small(20+(33*SPELLS_ALIGN_X-l)/2,spell_mini_y_len-37-35,(unsigned char*)GET_UNCASTABLE_STR(spells_list[we_have_spell].uncastable),1);
	} else {
		i=(on_spell>=0) ? (on_spell):(we_have_spell);
		if(i>=0){
			int l=(int)(get_string_width((unsigned char*)spells_list[i].name)*(float)DEFAULT_SMALL_RATIO);
			if (on_spell>=0) SET_COLOR(c_grey1);
			else SET_COLOR(c_green3);
			draw_string_small(20+(33*SPELLS_ALIGN_X-l)/2,spell_mini_y_len-37-35,(unsigned char*)spells_list[i].name,1);
		}
	}


	//draw the current spell
	draw_current_spell(x,spell_mini_y_len-37,0);
	glDisable(GL_TEXTURE_2D);
	glColor3f(0.77f,0.57f,0.39f);
	rendergrid(SPELLS_ALIGN_X,spell_mini_rows,x,y,33,33);

	if(we_have_spell>=0&&spells_list[we_have_spell].uncastable){
		//not castable, red grid
		glColor3f(1.0f,0.0f,0.0f);
		rendergrid(1,1,20,spell_mini_y_len-37,33,33);
	}

	glEnable(GL_TEXTURE_2D);

#ifdef OPENGL_TRACE
CHECK_GL_ERRORS();
#endif //OPENGL_TRACE

	return 1;
}

#ifdef FR_FENETRE_NECRO
int display_necro_handler(window_info *win)
{
    int i, j;
    float u_start, v_start, u_end, v_end;
#ifdef FR_NECRO_RECETTES
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
    if(nb_recettes_necro > 0)
    {
	blocage_invoc_x = 13;
	blocage_invoc_y = 80 - 26;
	texte_options_x = 23;
	texte_options_y= 73;
	grille_x = 12;
	grille_y = 94 - 12;
	place_fleches_x = grille_x + 198;
	place_fleches_y = grille_y + 1;
	double_invoc_x = 13;
	double_invoc_y = 175 - 15;
	messages_invoc_x = 6;
	messages_invoc_y = 228 - 15;
	total = liste_items_necro[creature_en_cours][0];
	for(i = 0; i < total; i++)
	{
	    objet_recette[i][0] = liste_items_necro[creature_en_cours][i+1];
	}
	if(double_invoc)
	{
	    total++;
	    objet_recette[total-1][0] = 41;
	}
	for(i = 0; i < 36; i++)
	{
	    if(item_list[i].quantity > 0)
	    {
		for(j = 0; j < total; j++)
		{
		    if(item_list[i].id == objet_recette[j][0])
		    {
			objet_recette[j][1] = item_list[i].quantity;
			objet_recette[j][2] = item_list[i].pos;
		    }
		}
	    }
	}
    }
#endif //FR_NECRO_RECETTES
    draw_switcher(win);
#ifdef NEW_TEXTURES
    bind_texture(divers_text);
#else //NEW_TEXTURES
    bind_texture_id(divers_text);
    get_and_set_texture_id(divers_text);
#endif //NEW_TEXTUES
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0.05f);
    glBegin(GL_QUADS);
    for (i = 0 ; i < NECRO_NO ; i++)
    {
	u_start = 0.125f * (necro_list[i].necro_img%8);
	u_end = u_start + 0.125f;
#ifdef NEW_TEXTURES
	v_start =((float)32/256*(necro_list[i].necro_img/8));
	v_end = v_start + 0.125f;
#else //NEW_TEXTURES
	v_start = 1.0f - ((float)32/256*(necro_list[i].necro_img/8));
	v_end = v_start - 0.125f;
#endif //NEW_TEXTURES

	if(i >= 4)
	{
	    j = 6;
	}
	else
	{
	    if(i >= 2)
	    {
		j = 3;
	    }
	    else
	    {
		j = 0;
	    }
	}
	draw_2d_thing(u_start, v_start, u_end, v_end, NECRO_SPACE_Y+i*(33+NECRO_SPACE_Y_BETWEEN)+NECRO_SPACE_Y_BETWEEN*j, NECRO_SPACE_X, NECRO_SPACE_Y+i*(33+NECRO_SPACE_Y_BETWEEN)+NECRO_SPACE_Y_BETWEEN*j+33, NECRO_SPACE_X+33);
    }
    //ajout du sort de soin des invoqués :
    glEnd();
    glDisable(GL_ALPHA_TEST);
#ifdef NEW_TEXTURES
    bind_texture_id(sigils_text); //icones de sorts
#else //NEW_TEXTURES
    bind_texture_id(sigils_text); //icones de sorts
    get_and_set_texture_id(sigils_text); // icones de sorts
#endif //NEW_TEXTURES
    pos_y_sdi = NECRO_SPACE_X;
    pos_x_sdi = NECRO_SPACE_Y+5*(33+NECRO_SPACE_Y_BETWEEN)+NECRO_SPACE_Y_BETWEEN*5;
    draw_spell_icon(spells_list[groups_list[1].spells_id[4]].image,pos_x_sdi,pos_y_sdi,32,0,spells_list[groups_list[1].spells_id[4]].uncastable);
    //on affiche le cadre :
    glDisable(GL_TEXTURE_2D);
    glColor3f(0.77f,0.57f,0.39f);
    rendergrid(1,1,pos_x_sdi,pos_y_sdi,34,34);
    glEnable(GL_TEXTURE_2D);

    //on écrit la légende de l'action sélectionnée
    glColor3f(1.0f, 1.0f, 1.0f);
    if (mouseover_necro != -1)
    {
	draw_string_small(NECRO_SPACE_X, NECRO_SPACE_Y*2+33+10, (unsigned char*)necro_list[mouseover_necro].description, 1);
    }
#ifdef FR_NECRO_RECETTES
    if(nb_recettes_necro > 0)
    {
	if (last_items_string_id != inventory_item_string_id)
	{
	    put_small_text_in_box((unsigned char*)inventory_item_string, strlen(inventory_item_string), necro_x_len - 12, items_string);
	    last_items_string_id = inventory_item_string_id;
	}
	draw_string_small(messages_invoc_x, messages_invoc_y,(unsigned char *)items_string,4);
	glDisable(GL_TEXTURE_2D);
	glColor3f(0.77f,0.57f,0.39f);
	rendergrid(6, 1, grille_x, grille_y, 32, 32);
	glColor3f(0.77f,0.57f,0.39f);
	glBegin(GL_QUADS);
	glVertex3i(place_fleches_x+4, place_fleches_y, 0); //top
	glVertex3i(place_fleches_x, place_fleches_y+10, 0); //left
	glVertex3i(place_fleches_x+8, place_fleches_y+10, 0); //right
	glVertex3i(place_fleches_x+4, place_fleches_y, 0); //top
	glEnd();
	glBegin(GL_QUADS);
	glVertex3i(place_fleches_x+4, place_fleches_y+21+10, 0); //top
	glVertex3i(place_fleches_x, place_fleches_y+21, 0); //left
	glVertex3i(place_fleches_x+8, place_fleches_y+21, 0); //right
	glVertex3i(place_fleches_x+4, place_fleches_y+21+10, 0); //top
	glEnd();
	glEnable(GL_POINT_SMOOTH);
	glPointSize(12);
	glColor3f(0.77f,0.57f,0.39f);
	glBegin(GL_POINTS);
	glVertex3i(double_invoc_x, double_invoc_y, 0);
	glEnd();
	glBegin(GL_POINTS);
	glVertex3i(blocage_invoc_x, blocage_invoc_y, 0);
	glEnd();
	glPointSize(20 - 10);
	glColor3f(0.0f,0.0f,0.0f);
	glBegin(GL_POINTS);
	glVertex3i(double_invoc_x, double_invoc_y, 0);
	glEnd();
	glBegin(GL_POINTS);
	glVertex3i(blocage_invoc_x, blocage_invoc_y, 0);
	glEnd();
	glPointSize(20 - 14);
	glColor3f(0.77f,0.57f,0.39f);
	glBegin(GL_POINTS);
	if(double_invoc)
	{
	    glVertex3i(double_invoc_x, double_invoc_y, 0);
	}
	if(securite_invoc)
	{
	    glVertex3i(blocage_invoc_x, blocage_invoc_y, 0);
	}
	glEnd();
	glEnable(GL_TEXTURE_2D);
	glColor3f(1.0f,1.0f,1.0f);
	for(i = 0; i < liste_items_necro[creature_en_cours][0]; i++)
	{
	    glColor3f(1.0f,1.0f,1.0f);
	    draw_item((liste_items_necro[creature_en_cours][i + 1 + liste_items_necro[creature_en_cours][0]]), grille_x + i * 32 , grille_y + 1, 32);
	    safe_snprintf((char *)nombre, sizeof(nombre), "%i", (liste_items_necro[creature_en_cours][i + 1 + liste_items_necro[creature_en_cours][0]*2]));
	    if(objet_recette[i][1] >= (liste_items_necro[creature_en_cours][i + 1 + liste_items_necro[creature_en_cours][0]*2])) draw_string_small_shadowed(grille_x + i * 32, grille_y + 1, nombre, 1, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f);
	    else draw_string_small_shadowed(grille_x + i * 32, grille_y + 1, nombre, 1, 0.8f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	}
	if(double_invoc)
	{
	    glColor3f(1.0f,1.0f,1.0f);
	    draw_item(60, grille_x + (liste_items_necro[creature_en_cours][0]) * 32, grille_y + 1, 32);
	    safe_snprintf((char *)nombre, sizeof(nombre), "%i", 1);
	    if(objet_recette[total-1][1] >= 1) draw_string_small_shadowed(grille_x + (liste_items_necro[creature_en_cours][0]) * 32, grille_y + 1, nombre, 1, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f);
	    else draw_string_small_shadowed(grille_x + (liste_items_necro[creature_en_cours][0]) * 32, grille_y + 1, nombre, 1, 0.8f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	}
	//glColor3f (1.0f, 1.0f, 1.0f);
	glColor3f(0.77f,0.57f,0.39f);
	draw_string_small(texte_options_x, texte_options_y - 26, (unsigned char*)("Bloquer tuer invocations"), 1);
	draw_string_small(texte_options_x - 10, texte_options_y + 60 - 15, (unsigned char*)("Invocation:"), 1);
	glColor3f (0.2f, 1.0f, 0.2f);
	draw_string_small(texte_options_x - 10, texte_options_y + 75 - 15,(unsigned char*)(nom_bestiole[creature_en_cours]), 1);
	//glColor3f (1.0f, 1.0f, 1.0f);
	glColor3f(0.77f,0.57f,0.39f);
	draw_string_small(texte_options_x, texte_options_y + 95 - 15, (unsigned char*)("Double Invocation"), 1);
    }
#endif //FR_NECRO_RECETTES
#ifdef OPENGL_TRACE
    CHECK_GL_ERRORS();
#endif //OPENGL_TRACE*
    return 1;
}
#ifdef FR_NECRO_RECETTES
int chargement_necro_recettes(int choix)
{
    int encours = 0;
    int k = 0;
    char prov[30] = "";
    int bestiole_en_cours = 0;
    int nb_item_bestiole_en_cours;
    int monstre = -1;
    int prochain_monstre = -1;

    FILE* fichnecro;
    fichnecro = NULL;
    fichnecro = fopen("creatnecro", "r");

    if(fichnecro != NULL)
    {
	while (fgets(prov, 30, fichnecro) != NULL)
	{
	    if(k == 0)
	    {
		monstre = 1;
		prochain_monstre = 1;
		bestiole_en_cours = -1;
	    }
	    else
	    {
		if(k == prochain_monstre)
		{
		    monstre = prochain_monstre;
		    bestiole_en_cours += 1;
		    if(choix)
		    {
			nom_bestiole[bestiole_en_cours] = strdup(prov);
		    }
		}
		else
		{
		    if(k == monstre + 1)
		    {
			nb_item_bestiole_en_cours = atoi(prov);
			prochain_monstre = monstre + (nb_item_bestiole_en_cours*3) + 2;
			if(choix) liste_items_necro[bestiole_en_cours][0] = nb_item_bestiole_en_cours;	//total objets différents
			encours = monstre + 1;
		    }
		    else
		    {
			if(choix) liste_items_necro[bestiole_en_cours][k - encours] = atoi(prov);
		    }
		}
	    }
	    k++;
	}
	return bestiole_en_cours + 1;	//fichier ok
    }
    else
    {
	return -1;	//Pas de fichier
    }
}
#endif //FR_NECRO_RECETTES
#endif //FR_FENETRE_NECRO
//CLICK HANDLERS
int switch_handler(int new_win)
{
    window_info *win;
    int this_win;

    last_win=sigil_win;
    this_win=new_win;

    win=&windows_list.window[last_win];
    windows_list.window[this_win].opaque=windows_list.window[last_win].opaque;
    hide_window(last_win);
#ifdef FR_VERSION
    // positionnement en conservant l'alignement à droite (pour rester sur les switchs)
    move_window(this_win, win->pos_id, win->pos_loc, win->pos_x + win->len_x - windows_list.window[this_win].len_x, win->pos_y);
#else //FR_VERSION
    move_window(this_win, win->pos_id, win->pos_loc, win->pos_x, win->pos_y);
#endif //FR_VERSION
    show_window(this_win);
    select_window(this_win);
    sigil_win=this_win;
#ifdef FR_NECRO_RECETTES
    if(new_win == sigils_win) start_mini_spells = 0;
    else if (new_win == spell_mini_win) start_mini_spells = 1;
    else if (new_win == spell_win) start_mini_spells = 2;
    else if (new_win == necro_win) start_mini_spells = 3;
#else
    start_mini_spells=(sigil_win==spell_mini_win)? 1:0;
#endif //FR_NECRO_RECETTES

    return 1;
}

#ifdef FR_VERSION
int mouseover_switcher_handler(window_info *win, int mx, int my)
{
	if (mx >= win->len_x - ELW_BOX_SIZE && my > ELW_BOX_SIZE)
	{
		if (my < ELW_BOX_SIZE*2)
		{
			show_help("Liste des Runes", win->len_x+5, ELW_BOX_SIZE*1);
		}
		else if (my < ELW_BOX_SIZE*3)
		{
			show_help("Liste des Sorts", win->len_x+5, ELW_BOX_SIZE*2);
		}
		else if (my < ELW_BOX_SIZE*4)
		{
			show_help("Liste détaillée", win->len_x+5, ELW_BOX_SIZE*3);
		}
#ifdef FR_FENETRE_NECRO
        else if (my < ELW_BOX_SIZE*5)
        {
            show_help("Nécromancie", win->len_x+5, ELW_BOX_SIZE*4);
        }
#endif //FR_FENETRE_NECRO
	}
	return 0;
}
#endif //FR_VERSION

int click_switcher_handler(window_info *win, int mx, int my, Uint32 flags){
#ifdef FR_VERSION
#ifdef FR_FENETRE_NECRO
#ifdef FR_NECRO_RECETTES
    int pos = 0;
    if(win->window_id == necro_win)
    {
	//grille_x = 12;
	//grille_y = 94 - 12;
	//rendergrid(6, 1, grille_x, grille_y, 32, 32);
	pos = get_mouse_pos_in_grid(mx, my, 1, 1, 12, 82, 181, 33);
	if(pos == 0)
	{
	    if(flags&ELW_WHEEL_UP)
	    {

		//haut
		if(nb_recettes_necro != -1)
		{
		    if(creature_en_cours == nb_recettes_necro - 1) creature_en_cours = -1;
		    creature_en_cours++;
		}
		return 0;
	    }
	    else if(flags&ELW_WHEEL_DOWN)
	    {
		//bas
		if(nb_recettes_necro != -1)
		{
		    if(creature_en_cours == 0) creature_en_cours = nb_recettes_necro;
		    creature_en_cours--;
		}
		return 0;
	    }
	}
    }
#endif //FR_NECRO_RECETTES
#endif //FR_FENETRE_NECRO
    if (mx >= win->len_x - ELW_BOX_SIZE && my > ELW_BOX_SIZE)
    {
	if (my < ELW_BOX_SIZE*2)
	{
	    if (sigil_win != sigils_win)
	    {
		do_click_sound();
		switch_handler(sigils_win);
	    }
	}
	else if (my < ELW_BOX_SIZE*3)
	{
	    if (sigil_win != spell_mini_win)
	    {
		do_click_sound();
		switch_handler(spell_mini_win);
	    }
	}
	else if (my < ELW_BOX_SIZE*4)
	{
	    if (sigil_win != spell_win)
	    {
		do_click_sound();
		switch_handler(spell_win);
	    }
	}
#ifdef FR_FENETRE_NECRO
	else if (my <  ELW_BOX_SIZE*5)
	{
            if (sigil_win != necro_win)
	    {
		do_click_sound();
		switch_handler(necro_win);
	    }
	}
#endif //FR_FENETRE_NECRO
    }
#else //FR_VERSION
    if (mx>=win->len_x-20&&my>=20&&my<=40) {
	do_click_sound();
	switch_handler((sigil_win==sigils_win) ? (last_win):(sigils_win));
    } else if(sigil_win==spell_win || sigil_win==spell_mini_win){
	if (mx>=win->len_x-20&&my>=40&&my<=60) {
	    do_click_sound();
	    switch_handler((sigil_win==spell_win) ? (spell_mini_win):(spell_win));
	}
    }
#endif //FR_VERSION
	return 0;
}


int click_sigils_handler(window_info *win, int mx, int my, Uint32 flags)
{
	Mqbspell **m = cur_mqb();
	// only handle real clicks, not scroll wheel moves
	if ( (flags & ELW_MOUSE_BUTTON) == 0 ) {
		return 0;
	} else if (mx >= 350 && mx <= 381 && my >= 112 && my <= 143 && m[0] && m[0]->spell_id != -1) {
		add_spell_to_quickbar();
		return 1;
	} else if(mx>0 && mx<NUM_SIGILS_LINE*33 && my>0 && my<NUM_SIGILS_ROW*33) {
		int pos=get_mouse_pos_in_grid(mx,my, NUM_SIGILS_LINE, NUM_SIGILS_ROW, 0, 0, 33, 33);

		if (pos >= 0 && sigils_list[pos].have_sigil) {
			int j;
			int image_id=sigils_list[pos].sigil_img;

			//see if it is already on the list
			for(j=0;j<6;j++) {
				if(on_cast[j]==image_id) {
					return 1;
				}
			}

			for(j=0;j<6;j++) {
				if(on_cast[j]==-1) {
					on_cast[j]=image_id;
					return 1;
				}
			}
			return 1;
		}
	} else if(mx>5 && mx<6*33+5 && my>sigil_y_len-37 && my<sigil_y_len-5) {
		int pos=get_mouse_pos_in_grid(mx, my, 6, 1, 5, sigil_y_len-37, 33, 33);

		if (pos >= 0) {
			on_cast[pos]=-1;
		}
	}
	if (init_ok) click_switcher_handler(win,mx,my,flags);
	return 0;
}
static void remove_spell_from_quickbar(int pos) {
	Mqbspell **m = cur_mqb();
	if (pos < 1 || pos > QUICKSPELLS_MAXSIZE || !m[pos]) {
		return;
	}
	free(m[pos]);
	for (int i = pos; i < QUICKSPELLS_MAXSIZE; ++i) {
		m[i] = m[i + 1];
		if (!m[i]) {
			break;
		}
	}
	m[QUICKSPELLS_MAXSIZE] = 0;
	resize_quickspells(0);
	save_quickspells();
}
static void modify_mqb_with_prepared_spell(Uint32 click_flags) {
	if (click_flags & ELW_CTRL) {
		Mqbspell **m = cur_mqb();
		if (m[0]) {
			for (int i = 1; i <= QUICKSPELLS_MAXSIZE; ++i) {
				if (m[i] && m[i]->spell_id == m[0]->spell_id) {
					remove_spell_from_quickbar(i);
				}
			}
		}
	} else {
		add_spell_to_quickbar();
	}
}
int click_spells_handler(window_info *win, int mx, int my, Uint32 flags){
	int pos,i,the_group=-1,the_spell=-1;
	static int last_clicked=0;
	static int last_pos=-1;

	if (!(flags & ELW_MOUSE_BUTTON)) return 0;

	for(i=0;i<num_groups;i++){
		pos=get_mouse_pos_in_grid(mx,my, SPELLS_ALIGN_X, groups_list[i].spells/(SPELLS_ALIGN_X+1)+1, groups_list[i].x, groups_list[i].y, 33, 33);
		if(pos>=0&&pos<groups_list[i].spells) {
			the_group=i;
			the_spell=pos;
			break;
		}
	}

	if (the_spell!=-1){
		//a spell has been clicked
		int code_pos=(the_group*256+the_spell);
		we_have_spell=groups_list[the_group].spells_id[the_spell];
		if (put_on_cast() && we_have_spell >= 0 && (flags & ELW_RIGHT_MOUSE)) {
			prepare_for_cast();
			modify_mqb_with_prepared_spell(flags);
		}
		//handle double click && cast spell
		if ( ((SDL_GetTicks() - last_clicked) < 400)&&last_pos==code_pos) cast_handler();
		else have_error_message=0; //if not double click, clear server msg
		last_pos=code_pos;
	} else {
		last_pos=-1;
		//check spell icon
		if(we_have_spell>=0&&mx>20&&mx<53&&my>spell_y_len-37&&my<spell_y_len-4) {
			if(flags & ELW_LEFT_MOUSE) {
				if (put_on_cast()) cast_handler();
			} else if (flags & ELW_RIGHT_MOUSE) {
				if (put_on_cast()) {
					prepare_for_cast();
					modify_mqb_with_prepared_spell(flags);
				}
			}
		} else click_switcher_handler(win,mx,my,flags);
	}
	last_clicked = SDL_GetTicks();
	return 0;
}
int click_spells_mini_handler(window_info *win, int mx, int my, Uint32 flags){

	int pos;
	static int last_clicked=0;
	static int last_pos=-1;

	if (!(flags & ELW_MOUSE_BUTTON)) return 0;

	pos=get_mouse_pos_in_grid(mx,my, SPELLS_ALIGN_X, spell_mini_rows, 20, 10, 33, 33);
	if (pos>=0){
		int i,j,cs,cg,the_spell=-1,the_group=-1,the_pos=pos;
		//find the spell clicked
		for (i=0,cs=0,cg=0;i<spell_mini_rows&&the_pos>=0;i++) {
			for (j=0;j<SPELLS_ALIGN_X;j++){
				the_pos--;
				if (the_pos==-1) { the_spell=cs; the_group=cg;}
				else if(the_pos<-1) break;
				if (cs==groups_list[cg].spells-1) {cs=0; cg++; the_pos-=(SPELLS_ALIGN_X-j-1); break;}
				else cs++;
			}
		}
		//put it on the cast bar
		if(the_spell!=-1){
			we_have_spell=groups_list[the_group].spells_id[the_spell];
			put_on_cast();
			//handle double click
			if ( ((SDL_GetTicks() - last_clicked) < 400)&&last_pos==pos) cast_handler();
		}
	} else {
		//check if clicked on the spell icon
		if(we_have_spell>=0&&mx>=20&&mx<=53&&my>=spell_mini_y_len-37&&my<=spell_mini_y_len-4) {
			if(flags & ELW_LEFT_MOUSE) {
				if (put_on_cast()) cast_handler();
			} else if (flags & ELW_RIGHT_MOUSE) {
				//add to quickbar
				if(put_on_cast()){
					prepare_for_cast();
					add_spell_to_quickbar();
				}
			}
		} else click_switcher_handler(win,mx,my,flags);
	}
	last_pos=pos;
	last_clicked = SDL_GetTicks();
	return 0;
}

#ifdef FR_FENETRE_NECRO
int click_necro_handler(window_info *win, int mx, int my, Uint32 flags)
{
    int pos, i, clique_bouton = 0;

    for (i = 0 ; i < NECRO_NO; i++)
    {
	if (i >= 4)
	{
		pos = get_mouse_pos_in_grid(mx, my, 1, 1, NECRO_SPACE_Y+NECRO_SPACE_Y_BETWEEN*(i+6)+i*33, NECRO_SPACE_X, 33, 33);
	}
	else
	{
	    if (i >= 2)
	    {
		pos = get_mouse_pos_in_grid(mx, my, 1, 1, NECRO_SPACE_Y+NECRO_SPACE_Y_BETWEEN*(i+3)+i*33, NECRO_SPACE_X, 33, 33);
	    }
	    else
	    {
		pos = get_mouse_pos_in_grid(mx, my, 1, 1, NECRO_SPACE_Y+NECRO_SPACE_Y_BETWEEN*i+i*33, NECRO_SPACE_X, 33, 33);
	    }
	}

	if (pos == 0)
	{
#ifdef FR_NECRO_RECETTES
	    if(nb_recettes_necro > 0)
	    {
		if((i == 4) && (securite_invoc))
		{
		    LOG_TO_CONSOLE(c_red1, "Impossible de tuer les invocations lorsque le blocage est actif...");
		    set_shown_string(c_red2, "Impossible de tuer les invocations lorsque le blocage est actif...");
		}
		else
		{
		    test_for_console_command (necro_list[i].name, strlen(necro_list[i].name));
		}
		clique_bouton = 1;
		break;
	    }
	    else
	    {
#endif //FR_NECRO_RECETTES
		test_for_console_command (necro_list[i].name, strlen(necro_list[i].name));
		clique_bouton = 1;
		break;
#ifdef FR_NECRO_RECETTES
	    }
#endif //FR_NECRO_RECETTES
	}
    }

    //sort de soin des invoqués
    pos = get_mouse_pos_in_grid(mx, my, 1, 1, pos_x_sdi, pos_y_sdi, 33, 33);
    if(pos == 0)
    {
	we_have_spell=groups_list[1].spells_id[4];
	put_on_cast();
	cast_handler();
    }
#ifdef FR_NECRO_RECETTES
    if(nb_recettes_necro > 0)
    {
	//pos = get_mouse_pos_in_grid(mx, my, 1, 1, double_invoc_x - 5, double_invoc_y - 5, 10, 10);
	pos = get_mouse_pos_in_grid(mx, my, 1, 1, 8, 155, 10, 10);
	if(pos == 0)
	{
	    if(double_invoc == 1) double_invoc = 0;
	    else double_invoc++;
	}
	//pos = get_mouse_pos_in_grid(mx, my, 1, 1, blocage_invoc_x - 5, blocage_invoc_y - 5, 10, 10);
	pos = get_mouse_pos_in_grid(mx, my, 1, 1, 8, 49, 10, 10);
	if(pos == 0)
	{
	    if(securite_invoc == 1) securite_invoc = 0;
	    else securite_invoc++;
	}
	//pos = get_mouse_pos_in_grid(mx, my, 1, 1, place_fleches_x - 1, place_fleches_y, 10, 10);
	pos = get_mouse_pos_in_grid(mx, my, 1, 1, 209, 83, 10, 10);
	if(pos == 0)
	{
	    //haut
	    if(nb_recettes_necro != -1)
	    {
		if(creature_en_cours == nb_recettes_necro - 1) creature_en_cours = -1;
		creature_en_cours++;
	    }
	}
	//pos = get_mouse_pos_in_grid(mx, my, 1, 1, place_fleches_x - 1, place_fleches_y + 21, 10, 10);
	pos = get_mouse_pos_in_grid(mx, my, 1, 1, 209, 104, 10, 10);
	if(pos == 0)
	{
	    //bas
	    if(nb_recettes_necro != -1)
	    {
		if(creature_en_cours == 0) creature_en_cours = nb_recettes_necro;
		creature_en_cours--;
	    }
	}
    }
#endif //FR_NECRO_RECETTES
    if (!clique_bouton)
    {
	click_switcher_handler(win, mx, my, flags);
    }

	return 0;
}
#endif //FR_FENETRE_NECRO


//MOUSEOVER HANDLERS
int mouseover_sigils_handler(window_info *win, int mx, int my)
{
	if(!have_error_message) {
		spell_text[0] = 0;
	}
	Mqbspell **m = cur_mqb();
	if (mx >= 350 && mx <= 381 && my >= 112 && my <= 143 && m[0] && m[0]->spell_name[0]) {
		show_last_spell_help = 1;
	}

	//see if we clicked on any sigil in the main category
	if(mx>0 && mx<NUM_SIGILS_LINE*33 && my>0 && my<NUM_SIGILS_ROW*33) {
		int pos=get_mouse_pos_in_grid(mx,my, NUM_SIGILS_LINE, NUM_SIGILS_ROW, 0, 0, 33, 33);

		if (pos >= 0 && sigils_list[pos].have_sigil)
		{
			my_strcp((char*)spell_text,sigils_list[pos].name);
			have_error_message=0;
		}
		return 0;
	}

	//see if we clicked on any sigil from "on cast"
	if(mx>5 && mx<6*33+5 && my>sigil_y_len-37 && my<sigil_y_len-5) {
		int pos=get_mouse_pos_in_grid(mx, my, 6, 1, 5, sigil_y_len-37, 33, 33);

		if (pos >= 0 && on_cast[pos]!=-1){
			my_strcp((char*)spell_text,sigils_list[on_cast[pos]].name);
			have_error_message=0;
		}
		return 0;
	}
	if (mx >= 350 && mx <= 381 && my >= 112 && my <= 143 && m[0] && m[0]->spell_id != -1) {
		safe_snprintf((char*)spell_text, sizeof(spell_text), click_to_add_str);
		return 0;
	}
	mouseover_switcher_handler(win, mx, my);
	return 0;
}


void set_spell_help_text(int spell){

	char clr[4];

	if (spell<0) {
		spell_help[0]=0;
		return;
	}

	//Set spell name color
	if (spell==we_have_spell) spell_help[0]=127+c_green3;
	else spell_help[0]=127+c_orange2;
	spell_help[1]=0;

	//Set spell name
	safe_strcat((char*)spell_help,spells_list[spell].name,sizeof(spell_help));

	//Set uncastable message
	if(spells_list[spell].uncastable){
		clr[0]=127+c_red2;
		clr[1]=clr[2]=' ';
		clr[3]=0;
		safe_strcat((char*)spell_help,clr,sizeof(spell_help));
		safe_strcat((char*)spell_help,GET_UNCASTABLE_STR(spells_list[spell].uncastable),sizeof(spell_help));
	}
	safe_strcat((char*)spell_help,"\n",sizeof(spell_help));
	clr[0]=127+c_grey1;
	clr[1]=0;
	safe_strcat((char*)spell_help,clr,sizeof(spell_help));
	safe_strcat((char*)spell_help,spells_list[spell].desc,sizeof(spell_help));

}

int mouseover_spells_handler(window_info *win, int mx, int my){
	int i,pos;

	if(!have_error_message) {
		spell_text[0] = 0;
	}

	on_spell=-1;
	for(i=0;i<num_groups;i++){
		pos=get_mouse_pos_in_grid(mx,my, SPELLS_ALIGN_X, groups_list[i].spells/(SPELLS_ALIGN_X+1)+1, groups_list[i].x, groups_list[i].y, 33, 33);
		if(pos>=0&&pos<groups_list[i].spells) {
			on_spell=groups_list[i].spells_id[pos];
			set_spell_help_text(on_spell);
			return 0;
		}
	}
	set_spell_help_text(we_have_spell);
	//check spell icon
	if(mx>20&&mx<53&&my>spell_y_len-37&&my<spell_y_len-4&&we_have_spell>=0) {
#ifdef ENGLISH
		safe_snprintf((char*)spell_text, sizeof(spell_text), "Left click to cast\nRight click to add the spell to the quickbar");
#else //ENGLISH
		safe_snprintf((char*)spell_text, sizeof(spell_text), "Clique gauche pour lancer le sort\nClique droit pour ajouter le sort dans la barre rapide");
#endif //ENGLISH
		have_error_message=0;
	}
#ifdef FR_VERSION
	mouseover_switcher_handler(win, mx, my);
#endif //FR_VERSION
	return 0;
}
int mouseover_spells_mini_handler(window_info *win, int mx, int my){

	int pos=get_mouse_pos_in_grid(mx,my, SPELLS_ALIGN_X, spell_mini_rows, 20, 10, 33, 33);
	on_spell=-1;
	if (pos>=0){
		int i,j,cs,cg,the_spell=-1,the_group=-1,the_pos=pos;
		//find the spell clicked
		for (i=0,cs=0,cg=0;i<spell_mini_rows&&the_pos>=0;i++) {
			for (j=0;j<SPELLS_ALIGN_X;j++){
				the_pos--;
				if (the_pos==-1) { the_spell=cs; the_group=cg;}
				else if(the_pos<-1) break;
				if (cs==groups_list[cg].spells-1) {cs=0; cg++; the_pos-=(SPELLS_ALIGN_X-j-1); break;}
				else cs++;
			}
		}
		if(the_spell!=-1) on_spell=groups_list[the_group].spells_id[the_spell];
	} else if(mx>20&&mx<53&&my>spell_mini_y_len-37&&my<spell_mini_y_len-4&&we_have_spell>=0) {
		//check spell icon
		on_spell=-2; //draw uncastability reason
	}
#ifdef FR_VERSION
	mouseover_switcher_handler(win, mx, my);
#endif //FR_VERSION
	return 0;
}

#ifdef FR_FENETRE_NECRO
int mouseover_necro_handler(window_info *win, int mx, int my)
{
    int i, pos;

    for (i = 0 ; i < NECRO_NO ; i++)
    {
	if (i >= 4)
	{
	    pos = get_mouse_pos_in_grid(mx, my, 1, 1, NECRO_SPACE_Y+NECRO_SPACE_Y_BETWEEN*(i+6)+i*33, NECRO_SPACE_X, 33, 33);
	}
	else
	{
	    if (i >= 2)
	    {
		pos = get_mouse_pos_in_grid(mx, my, 1, 1, NECRO_SPACE_Y+NECRO_SPACE_Y_BETWEEN*(i+3)+i*33, NECRO_SPACE_X, 33, 33);
	    }
	    else
	    {
		pos = get_mouse_pos_in_grid(mx, my, 1, 1, NECRO_SPACE_Y+NECRO_SPACE_Y_BETWEEN*i+i*33, NECRO_SPACE_X, 33, 33);
	    }
	}

	if (pos == 0)
	{
	    mouseover_necro = i;
	    break;
	}
	else
	{
	    mouseover_necro = -1;
	}
    }

    //sort de soin des invoqués
    pos = get_mouse_pos_in_grid(mx, my, 1, 1, pos_x_sdi, pos_y_sdi, 33, 33);
    if(pos == 0) mouseover_necro = 5;

	mouseover_switcher_handler(win, mx, my);
	return 0;
}
#endif //FR_FENETRE_NECRO



//MISC FUNCTIONS
void get_sigils_we_have(Uint32 sigils_we_have, Uint32 sigils2)
{
	int i;
	Uint32 po2=1;

	// the first 32 sigils
	for(i=0;i<32;i++)
		{
			if((sigils_we_have&po2))sigils_list[i].have_sigil=1;
			else sigils_list[i].have_sigil=0;
			po2*=2;
		}

	// the next optional sigils
	po2= 1;
	for(i=32;i<SIGILS_NO;i++)
		{
			if((sigils2&po2))sigils_list[i].have_sigil=1;
			else sigils_list[i].have_sigil=0;
			po2*=2;
		}
	check_castability();
}
int have_spell_name(int spell_id) {
	Mqbspell **m = cur_mqb();
	for (int i = 1; i <= QUICKSPELLS_MAXSIZE; ++i) {
		if (m[i] && m[i]->spell_id == spell_id && m[i]->spell_name[0]) {
			if (m[0]) {
				memcpy(m[0]->spell_name, m[i]->spell_name, sizeof(m[0]->spell_name));
			}
			return 1;
		}
	}
	return 0;
}
void set_spell_name(int id, const char *data, int len) {
	counters_set_spell_name(id, (char *)data, len);
	Mqbspell **m = cur_mqb();
	int s = sizeof(m[0]->spell_name), n = min2i(len, s - 1);
	for (int i = 0; i <= QUICKSPELLS_MAXSIZE; ++i) {
		if (m[i] && m[i]->spell_id == id)  {
			safe_snprintf(m[i]->spell_name, s, "%.*s", n, data);
		}
	}
}
void process_network_spell(const char *d, int len) {
	last_spell_name[0] = '\0';
	switch (d[0]) {
	case S_INVALID:
		spell_result = 0;
		LOG_TO_CONSOLE(c_red1, invalid_spell_str);
		return;
	case S_NAME:
		set_spell_name(d[1], d + 2, len - 2); // Will set the spell name of the given ID
		return;
	case S_SELECT_TARGET:
		spell_result = 3;
		if (selected_spell != -1 && selected_spell_sent) {
			fast_spell_cast(selected_spell_target);
		}
		action_mode = ACTION_WAND;
		break;
	case S_SELECT_TELE_LOCATION:
		spell_result = 2;
		// we're about to teleport, don't let the pathfinder
		// interfere with our destination
		if (pf_follow_path) {
			pf_destroy_path();
		}
		if (selected_spell != -1 && selected_spell_sent) {
			fast_spell_teleport();
		}
		action_mode = ACTION_WAND;
		break;
	case S_SUCCES:
		spell_result = 1;
		if (selected_spell_sent) {
			action_mode = ACTION_ATTACK;
			selected_spell_sent = 0;
		} else {
			action_mode = ACTION_WALK;
		}
		break;
	case S_FAILED:
		spell_result = 0;
		if (selected_spell_sent) {
			selected_spell_sent = 0;
			action_mode = ACTION_ATTACK;
		} else {
			action_mode = ACTION_WALK;
		}
		return;
	}
	Mqbspell **m = cur_mqb();
	if (!m[0]) {
		m[0] = calloc(1, sizeof(**m));
		m[0]->spell_id = -1;
	}
	if (m[0]->spell_id != d[1]) {
		m[0]->spell_id = d[1];
		m[0]->spell_image = d[2];
		if (!have_spell_name(d[1])) {
			Uint8 b[2] = {SPELL_NAME, d[1]};
			my_tcp_send(my_socket, b, 2);
		}
	}
}
/* Retourne l'indice de la case de la barre rapide correspondant aux coordonnées de la souris */
int get_quickspell_from_mouse(int mx, int my)
{
	int pos;
	int width = (quickspells_dim - quickspells_ico) / 2;
	if (quickspells_dir != VERTICAL) { int tmp = mx; mx = my; my = tmp; }

	if (mx < width) return -1;
	if (mx > width + quickspells_ico) return -1;

	for (pos = 0; pos < quickspells_size; pos++)
	{
		int y = 1 + (int)(pos/quickspells_div) * quickspells_sep + (quickspells_dim+1)*pos + width;
		if ((my >= y) && (my <= y + quickspells_ico)) return pos;
	}
	return -1;
}

/* Modification de la barre pour la rendre déplaçable (avec barre de titre) ou non */
void toggle_quickspells_draggable()
{
	Uint32 flags = get_flags(quickspell_win);

	if (! quickspells_draggable)
	{
//		flags &= ~ELW_SHOW_LAST;
		flags |= ELW_TITLE_BAR | ELW_DRAGGABLE | ELW_USE_BACKGROUND | ELW_SWITCHABLE_OPAQUE;
		change_flags(quickspell_win, flags);
		quickspells_draggable = 1;
	}
	else
	{
//		flags |= ELW_SHOW_LAST;
		flags &= ~(ELW_TITLE_BAR | ELW_DRAGGABLE | ELW_USE_BACKGROUND | ELW_SWITCHABLE_OPAQUE);
		change_flags(quickspell_win, flags);
		quickspells_draggable = 0;
		quickspell_x = window_width - windows_list.window[quickspell_win].cur_x;
		quickspell_y = windows_list.window[quickspell_win].cur_y;
	}
}

/* Changement de l'orientation de la barre rapide (vertical/horizontal) */
void flip_quickspells()
{
	int temp = quickspell_x_len;
	quickspell_x_len = quickspell_y_len;
	quickspell_y_len = temp;
	quickspells_dir = (quickspells_dir == VERTICAL) ? HORIZONTAL : VERTICAL;
	resize_window(quickspell_win, quickspell_x_len, quickspell_y_len);
}

/* Redéfinition de la barre rapide dans sa position par défaut */
void reset_quickspells()
{
	quickspells_on_top = 1;
	quickspells_draggable = 0;
	change_flags(quickspell_win, ELW_TITLE_NONE | ELW_CLICK_TRANSPARENT); //|ELW_SHOW|ELW_SHOW_LAST

	quickspell_x = HUD_MARGIN_X;
	quickspell_y = (hud_x) ? HUD_MARGIN_X : 0;
	move_window(quickspell_win, -1, 0, window_width - quickspell_x, quickspell_y);

	quickspells_dir = VERTICAL;
	quickspell_x_len = (1+quickspells_dim)*1 + 1;
	quickspell_y_len = (1+quickspells_dim)*quickspells_nb + (int)((quickspells_nb-1)/quickspells_div)*quickspells_sep + 1;
	resize_window(quickspell_win, quickspell_x_len, quickspell_y_len);
}

/* Mise à jour du nombre d'icones affichées et redimensionnement de la barre si nécessaire */
int resize_quickspells(int nb)
{
	Mqbspell **m = cur_mqb();
	if (nb >= 0)
	{
		if (nb > quickspells_size) nb = quickspells_size;
		while ((nb<quickspells_size) && (m[nb+1])) nb++;
		if (nb < 1) nb = 1; // minimum 1 case, même sans icone
	}
	if (nb != quickspells_nb)
	{
		if (nb > 0) quickspells_nb = nb;
		if (quickspells_dir == VERTICAL)
			quickspell_y_len = (1+quickspells_dim)*quickspells_nb + (int)((quickspells_nb-1)/quickspells_div)*quickspells_sep + 1;
		else
			quickspell_x_len = (1+quickspells_dim)*quickspells_nb + (int)((quickspells_nb-1)/quickspells_div)*quickspells_sep + 1;
		if (quickspell_win >= 0) resize_window(quickspell_win, quickspell_x_len, quickspell_y_len);
	}
	return nb;
}
void add_spell_to_quickbar(void) {
	Mqbspell **m = cur_mqb();
	if (!m[0]) {
		return;
	}
	int i;
	for (i = 1; i <= QUICKSPELLS_MAXSIZE; ++i) {
		if (m[i] && m[0]->spell_id == m[i]->spell_id) {
			if (i > quickspells_size) {
				Mqbspell *t = m[quickspells_size];
				m[quickspells_size] = m[i];
				m[i] = t;
				save_quickspells();
			}
			return;
		}
	}
	for (i = 1; i <= QUICKSPELLS_MAXSIZE; i++) {
		if (!m[i]) {
			m[i] = calloc(1, sizeof(**m));
			resize_quickspells(i);
			break;
		}
	}
	if (i > QUICKSPELLS_MAXSIZE) {
		*m[quickspells_size] = *m[0];
	} else if (i > quickspells_size) {
		*m[i] = *m[quickspells_size];
		*m[quickspells_size] = *m[0];
	} else {
		*m[i] = *m[0];
	}
	save_quickspells();
}

void move_spell_on_quickbar(int pos, int where) {
	Mqbspell **m = cur_mqb(), *t;
	int i = pos, last = 1;
	for (; last <= QUICKSPELLS_MAXSIZE && m[last]; ++last);
	if (i > 0 && i <= --last && (t = m[i])) {
		enum { w_up, w_down, w_first, w_last };
		if (where == w_up && i > 1) {
			m[i--] = m[pos - 1];
		} else if (where == w_down && i < last) {
			m[i++] = m[pos + 1];
		} else if (where == w_first && i > 1) {
			memmove(m + 2, m + 1, sizeof(*m)*(i - 1));
			i = 1;
		} else if (where == w_last && i < last) {
			memmove(m + i, m + i + 1, sizeof(*m)*(last - i));
			i = last;
		}
		m[i] = t;
		save_quickspells();
	}
}

static Mqbspell *build_quickspell_data(Uint32 spell_id) {
	spell_info *s = 0;
	for (spell_info *p = spells_list, *e = p + SPELLS_NO; p < e; ++p) {
		if (p->id == spell_id) {
			s = p;
			break;
		}
	}
	if (!s) {
		LOG_WARNING("Invalid spell id %d", spell_id);
		return 0;
	}
	Mqbspell *r = calloc(1, sizeof(*r));
	if (!r) {
		LOG_WARNING("Can't allocate memory for spell");
		return 0;
	}
	Uint8 *b = r->spell_str;
	b[0] = CAST_SPELL;
	int n = 0;
	for (int i = 0; i < 6; ++i) {
		if (s->sigils[i] != -1) {
			b[n++ + 2] = s->sigils[i];
		}
	}
	b[1] = n;
	r->spell_id = s->id;
	r->spell_image = s->image;
	safe_snprintf(r->spell_name, sizeof(r->spell_name), "%s", s->name);
	return r;
}

static inline void make_mqb_filename(char *b, int n, int i) {
	char s[8] = {i ? '1' + i : 0};
	safe_snprintf(b, n, "spells_%s%s.dat", username, s);
	my_tolower(b);
}
static void load_mqb(int which) {
	char p[128];
	make_mqb_filename(p, sizeof(p), which);
	if (file_exists_config(p) != 1) {
		return;
	}
	FILE *f = open_file_config(p, "rb");
	if (!f) {
		LOG_ERROR("%s: %s \"%s\": %s", reg_error_str, cant_open_file, p, strerror(errno));
		return;
	}
	Uint8 n;
	if (fread(&n, sizeof(n), 1, f) != 1) {
		LOG_ERROR("%s() read failed for [%s]", __FUNCTION__, p);
		fclose(f);
		return;
	}
	if (n) {
		--n;
	}
	if (n > QUICKSPELLS_MAXSIZE) {
		LOG_WARNING("Too many spells (%d), only %d spells allowed", n, QUICKSPELLS_MAXSIZE);
		n = QUICKSPELLS_MAXSIZE;
	}
	Mqbspell **m = mqbars[which];
	for (int i = 0, j = 1; i < n; ++i) {
		Mqbspell t;
		if (fread(&t, sizeof(t), 1, f) == 1) {
			if ((m[j] = build_quickspell_data(t.spell_id))) {
				++j;
			}
		} else {
			LOG_ERROR("Failed reading spell %d from file '%s'", i, p);
		}
	}
	fclose(f);
}
void load_quickspells(void) {
	for (int i = 0; i < 5; ++i) {
		load_mqb(i);
	}
	quickspells_loaded = 1;
	resize_quickspells(0);
}
static void save_mqb(int which) {
	char p[128];
	make_mqb_filename(p, sizeof(p), which);
	FILE *f = open_file_config(p, "wb");
	if (!f) {
		LOG_ERROR("%s: %s \"%s\": %s", reg_error_str, cant_open_file, p, strerror(errno));
		return;
	}
	Mqbspell **m = mqbars[which];
	Uint8 n;
	for (n = 1; n <= QUICKSPELLS_MAXSIZE && m[n]; ++n);
	fwrite(&n, sizeof(n), 1, f);
	for (int i = 1; i < n; ++i) {
		if (fwrite(m[i], sizeof(*m[i]), 1, f) != 1) {
			LOG_ERROR("Failed writing spell '%s' to file '%s'", m[i]->spell_name, p);
			break;
		}
	}
	fclose(f);
}
void save_quickspells(void) {
	if (quickspells_loaded) {
		for (int i = 0; i < 5; ++i) {
			save_mqb(i);
		}
	}
}

// Quickspell window start

int quickspell_over=-1;

#ifdef ENGLISH
// get the quickbar length - it depends on the numbe rof slots active
int get_quickspell_y_len(void)
{
	return num_quickbar_slots*30;
}
#endif //ENGLISH

/*	returns the y coord position of the active base
	of the quickspell window.  If spell slots are unused
	the base is higher */
int get_quickspell_y_base()
{
	if (quickspells_draggable) return 0;
	if (quickspell_x > HUD_MARGIN_X) return 0;
	return quickspell_y + quickspell_y_len;
}


int display_quickspell_handler(window_info *win)
{
	int x,y,width,i;
#ifdef FR_VERSION
	int over = -1;
	width = (quickspells_dim - quickspells_ico) / 2;
#else //FR_VERSION
	static int last_num_quickbar_slots = -1;
	// Check for a change of the number of quickbar slots
	if (last_num_quickbar_slots == -1)
		last_num_quickbar_slots = num_quickbar_slots;
	else if (last_num_quickbar_slots != num_quickbar_slots)
	{
		last_num_quickbar_slots = num_quickbar_slots;
		init_window(win->window_id, -1, 0, win->cur_x, win->cur_y, win->len_x, get_quickspell_y_len());
		cm_update_quickspells();
	}
#endif //FR_VERSION

#ifdef OPENGL_TRACE
CHECK_GL_ERRORS();
#endif //OPENGL_TRACE

#ifdef FR_MORE_MQB
	glDisable(GL_TEXTURE_2D);
	glColor3f(0.77f,0.57f,0.39f);
	if (quickspells_dir != VERTICAL){
        glBegin(GL_QUADS);
        glVertex3i(4, 0, 0); //top
        glVertex3i(0, 10, 0); //left
        glVertex3i(8, 10, 0); //right
        glVertex3i(4, 0, 0); //top
        glEnd();
        glBegin(GL_QUADS);
        glVertex3i(4, 22+10, 0); //top
        glVertex3i(0, 22, 0); //left
        glVertex3i(8, 22, 0); //right
        glVertex3i(4, 22+10, 0); //top
        glEnd();
    } else {
        glBegin(GL_QUADS);
        glVertex3i(0, 4, 0); //top
        glVertex3i(10, 8, 0); //left
        glVertex3i(10, 0, 0); //right
        glVertex3i(0, 4, 0); //top
        glEnd();
        glBegin(GL_QUADS);
        glVertex3i(10+22, 4, 0); //top
        glVertex3i(22, 8, 0); //left
        glVertex3i(22, 0, 0); //right
        glVertex3i(10+22, 4, 0); //top
        glEnd();
    }
#endif //FR_MORE_MQB
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER, 0.20f);
	glEnable(GL_BLEND);	// Turn Blending On
	glBlendFunc(GL_SRC_ALPHA,GL_DST_ALPHA);

	Mqbspell **m = cur_mqb();
	for (i = 1; i <= quickspells_size; i++) {
		if (!m[i] || !m[i]->spell_name[0]) break;
		y = (1+quickspells_dim)*(i-1) + (int)(i/quickspells_div)*quickspells_sep;
		if (quickspell_over == i) {
			over = y;
			glColor4f(1.0f, 1.0f, 1.0f, 1.5f);
		} else {
			glColor4f(1.0f, 1.0f, 1.0f, 0.8f);
		}
		if (quickspells_dir != VERTICAL) { x = y; y = 1; } else { x = 1; }
		draw_spell_icon(m[i]->spell_image, x+width, y+width, quickspells_ico, 0, 0);
	}
	glColor4f(1.0f,1.0f,1.0f,1.0f);
	glDisable(GL_BLEND);	// Turn Blending Off
	glDisable(GL_ALPHA_TEST);
	if (selected_spell != -1) {
		int loc = (1+quickspells_dim)*(selected_spell-1) + (int)(selected_spell/quickspells_div)*quickspells_sep;
		if (quickspells_dir == VERTICAL)
			show_help("*", -5 - 8, loc + (quickspells_dim-SMALL_FONT_Y_LEN)/2);
		else
			show_help("*", loc + width, 5 + quickspells_dim);
	}
	Mqbspell *mo = m[quickspell_over];
	if (quickspell_over > 0 && mo) {
		const char *n = mo->spell_name;
		for (int i = 0; i < SPELLS_NO; ++i) {
			if (spells_list[i].id == mo->spell_id) {
				n = spells_list[i].name;
				break;
			}
		}
		if (quickspells_dir == VERTICAL) {
			show_help(n, -5 - strlen(n)*8, over + (quickspells_dim-SMALL_FONT_Y_LEN)/2);
		} else {
			show_help(n, over + width, 5 + quickspells_dim);
		}
	}
	quickspell_over=-1;
#ifdef OPENGL_TRACE
CHECK_GL_ERRORS();
#endif //OPENGL_TRACE

	return 1;
}

int mouseover_quickspell_handler(window_info *win, int mx, int my) {
	int pos = get_quickspell_from_mouse(mx, my) + 1;
	Mqbspell **m = cur_mqb();
	if (pos > 0 && m[pos] && m[pos]->spell_name[0]) {
		quickspell_over = pos;
		elwin_mouse = CURSOR_WAND;
		return 1;
	}
	return 0;
}

static inline void next_mqb(void) {
	if (++quickspell_mqb_selected > 4) {
		quickspell_mqb_selected = 0;
	}
	resize_quickspells(0);
}
static inline void prev_mqb(void) {
	if (--quickspell_mqb_selected < 0) {
		quickspell_mqb_selected = 4;
	}
	resize_quickspells(0);
}
int click_quickspell_handler(window_info *win, int mx, int my, Uint32 flags)
{
	int pos;
	int ctrl_on = flags & ELW_CTRL;
	int shift_on = flags & ELW_SHIFT;

	// actions sur la barre (clic droit + modifieur)
	if (flags & ELW_RIGHT_MOUSE)
	{
		if (ctrl_on && shift_on)
		{
			reset_quickspells(); return 1;
		}
		else if (ctrl_on)
		{
			toggle_quickspells_draggable(); return 1;
		}
		else if (shift_on)
		{
			flip_quickspells(); return 1;
		}
	}

	pos = get_quickspell_from_mouse(mx, my) + 1;
	Mqbspell **m = cur_mqb();
	// menus contextuels (clic droit simple)
	if (flags & ELW_RIGHT_MOUSE) {
		// clic droit sur un icone : affichage du menu contextuel des icones (général inclus)
		if (pos > 0 && m[pos] && m[pos]->spell_name[0]) {
			cm_show_direct(cm_quickspells_id, quickspell_win, -1);
			return 1;
		}
		// clic droit en dehors des icones : affichage du menu contextuel général
		cm_show_direct(cm_quickspells_win_id, quickspell_win, -1);
		return 1;
	}
	// actions sur un raccourci (clic gauche)
	if (pos > 0 && m[pos] && m[pos]->spell_name[0]) {
		if (!ctrl_on && !shift_on) {
			if (!m[pos]->spell_str[0]) return 0;
			if (set_fast_spell_target) {
				fast_spell_cible(pos);
				set_fast_spell_target = 0;
			} else {
				send_spell(m[pos]->spell_str, m[pos]->spell_str[1]+2);
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
	if(pos==0){
		if(quickspells_dir != VERTICAL){
			if(mx >= 0 && mx <= 8 && my >= 0 && my <= 10){
				next_mqb();
				return 1;
			} else if(mx >=0  && mx <= 8 && my >= 22 && my <= 32){
				prev_mqb();
				return 1;
			}
		}else{
			if(mx >= 0 && mx <= 10 && my >= 0 && my <= 8){
				prev_mqb();
				return 1;
			} else if(mx >=22  && mx <= 32 && my >= 0 && my <= 8){
				next_mqb();
				return 1;
			}
		}
	}
	return 0;
}

// menu contextuel général (barre de titre ou fenêtre hors icones)
static int context_quickspellwin_handler(window_info *win, int widget_id, int mx, int my, int option)
{
	switch (option)
	{
		case 0: quickspells_draggable ^= 1; toggle_quickspells_draggable(); break;
		case 1: flip_quickspells(); break;
		case 2: reset_quickspells(); break;
	}
	return 1;
}

static int context_quickspell_handler(window_info *win, int widget_id, int mx, int my, int option)
{
	Mqbspell **m = cur_mqb();
	int pos = get_quickspell_from_mouse(mx, my) + 1;
	if (pos > 0 && m[pos] && m[pos]->spell_name[0])
	{
		switch (option)
		{
			case 0: move_spell_on_quickbar (pos,0); break;
			case 1: move_spell_on_quickbar (pos,1); break;
#ifdef FR_VERSION
			case 2: move_spell_on_quickbar (pos,2); break;
			case 3: move_spell_on_quickbar (pos,3); break;
			case 4: remove_spell_from_quickbar (pos); break;
#ifdef FR_FAST_SPELL
		case 5: fast_spell_cible(pos); break;
		case 7: quickspells_draggable ^= 1; toggle_quickspells_draggable(); break;
		case 8: flip_quickspells(); break;
		case 9: reset_quickspells(); break;
#else
			case 6: quickspells_draggable ^= 1; toggle_quickspells_draggable(); break;
			case 7: flip_quickspells(); break;
			case 8: reset_quickspells(); break;
#endif
#else //FR_VERSION
			case 2: remove_spell_from_quickbar (pos); break;
#endif //FR_VERSION
		}
	}
	return 1;
}

void init_quickspell()
{
#ifdef FR_VERSION
	// on remonte/descend les barres rapides placées sur le hud sous le logo
	if ((! hud_x) && (! quickspells_draggable) && (quickspells_dir==VERTICAL) && (quickspell_x<=HUD_MARGIN_X))
	{
		if (quickspell_y == HUD_MARGIN_X) quickspell_y = 0;
	}
#endif //FR_VERSION
	if (quickspell_win < 0){
#ifdef FR_VERSION
		quickspell_win = create_window ("Quickspell", -1, 0, window_width - quickspell_x, quickspell_y, quickspell_x_len, quickspell_y_len, ELW_CLICK_TRANSPARENT|ELW_TITLE_NONE|ELW_SHOW_LAST);
#else //FR_VERSION
		quickspell_win = create_window ("Quickspell", -1, 0, window_width - quickspell_x, quickspell_y, quickspell_x_len, get_quickspell_y_len(), ELW_CLICK_TRANSPARENT|ELW_TITLE_NONE|ELW_SHOW_LAST);
#endif //FR_VERSION
		set_window_handler(quickspell_win, ELW_HANDLER_DISPLAY, &display_quickspell_handler);
		set_window_handler(quickspell_win, ELW_HANDLER_CLICK, &click_quickspell_handler);
		set_window_handler(quickspell_win, ELW_HANDLER_MOUSEOVER, &mouseover_quickspell_handler );

#ifdef FR_VERSION
		// menu contextuel général (barre de titre + clic sur fond hors icones)
		cm_quickspells_win_id = cm_create(cm_quickspellwin_menu_str, &context_quickspellwin_handler);
		cm_bool_line(cm_quickspells_win_id, 0, &quickspells_draggable, NULL);
		cm_bool_line(cm_quickspells_win_id, 3, &quickspells_on_top, NULL);
		// menu contextuel des icones (menu général intégré à la fin)
		cm_quickspells_id = cm_create(cm_quickspell_menu_str, &context_quickspell_handler);
		cm_add(cm_quickspells_id, cm_quickspellwin_menu_str, NULL);
#ifdef FR_FAST_SPELL
		cm_bool_line(cm_quickspells_id, 5, &fast_spell_cible_bool, NULL);
		cm_bool_line(cm_quickspells_id, 7, &quickspells_draggable, NULL);
		cm_bool_line(cm_quickspells_id, 10, &quickspells_on_top, NULL);
#else
		cm_bool_line(cm_quickspells_id, 6, &quickspells_draggable, NULL);
		cm_bool_line(cm_quickspells_id, 9, &quickspells_on_top, NULL);
#endif
#else //FR_VERSION
		cm_quickspells_id = cm_create(cm_quickspell_menu_str, &context_quickspell_handler);
		cm_update_quickspells();
#endif //FR_VERSION
	} else {
#ifdef FR_VERSION
		change_flags(quickspell_win, ELW_CLICK_TRANSPARENT|ELW_TITLE_NONE|ELW_SHOW_LAST);
		if (quickspells_draggable)
		{
			show_window(quickspell_win);
		}
		else if (quickspell_y > window_height-10 || quickspell_x < 10)
		{
			move_window(quickspell_win, -1, 0, window_width - HUD_MARGIN_X, HUD_MARGIN_X);
		}
		else
		{
			move_window(quickspell_win, -1, 0, window_width - quickspell_x, quickspell_y);
		}
#else //FR_VERSION
		show_window (quickspell_win);
		move_window (quickspell_win, -1, 0, window_width - quickspell_x, quickspell_y);
#endif //FR_VERSION
	}
#ifdef FR_VERSION
	// on change les flags de la fenêtre draggable seulement maintenant pour éviter
	// qu'elle soit créée avec le menu contextuel par défaut de la barre de titre
	if (quickspells_draggable)
	{
		change_flags(quickspell_win, ELW_CLICK_TRANSPARENT|ELW_SHOW_LAST|ELW_TITLE_BAR|ELW_DRAGGABLE|ELW_USE_BACKGROUND|ELW_SWITCHABLE_OPAQUE);
	}
#endif //FR_VERSION
}
// Quickspells end



//CAST FUNCTIONS
int spell_clear_handler()
{
	int i;

	for(i=0;i<6;i++) {
		on_cast[i]=-1;
	}

	we_have_spell=-1;
	spell_text[0]=0;
	return 1;
}

void send_spell(Uint8 *str, int len)
{
	my_tcp_send(my_socket, str, len);
	memcpy(last_spell_str, str, len);
	last_spell_len = len;
}

void send_mqb_spell(int target_id) {
	Mqbspell **m = cur_mqb();
	send_spell(m[selected_spell]->spell_str, m[selected_spell]->spell_str[1] + 2);
	selected_spell_sent = 1;
	selected_spell_target = target_id;
}

int action_spell_keys(Uint32 key) {
	Uint32 keys[] = {K_SPELL1, K_SPELL2, K_SPELL3, K_SPELL4, K_SPELL5, K_SPELL6, K_SPELL7, K_SPELL8, K_SPELL9, K_SPELL10, K_SPELL11, K_SPELL12};
	Mqbspell **m = cur_mqb();
	for (int i = 0; i < countof(keys); ++i) {
		if (key == keys[i]) {
			if (set_fast_spell_target) {
				fast_spell_cible(i + 1);
				set_fast_spell_target = 0;
				return 1;
			}
			if (m[i+1] && m[i+1]->spell_str[0]) {
				send_spell(m[i+1]->spell_str, m[i+1]->spell_str[1]+2);
			}
			return 1;
		}
		if (key == K_PREVQUICKSPELLBAR) {
			prev_mqb();
			return 1;
		} else if (key == K_NEXTQUICKSPELLBAR) {
			next_mqb();
			return 1;
		}
	}
	return 0;
}

int prepare_for_cast(){
	Uint8 str[20];
	int count=0;
	int sigils_no=0;
	int i;

	for(i=0;i<6;i++) {
		if(on_cast[i]!=-1) {
			count++;
		}
	}

	if(count<2) {
		safe_snprintf((char*)spell_text, sizeof(spell_text), "%c%s",127+c_red2,sig_too_few_sigs);
		have_error_message=1;
		return 0;
	}

	str[0]=CAST_SPELL;
	for(i=0;i<6;i++) {
		if(on_cast[i]!=-1){
			str[sigils_no+2]=on_cast[i];
			sigils_no++;
		}
	}

	str[1]=sigils_no;
	Mqbspell **m = cur_mqb();
	if (!m[0]) {
		m[0] = calloc(1, sizeof(**m));
		m[0]->spell_id = -1;
	}

	if(sigil_win!=sigils_win&&we_have_spell>=0){
		m[0]->spell_id=spells_list[we_have_spell].id;
		m[0]->spell_image=spells_list[we_have_spell].image;
		memcpy(m[0]->spell_name, spells_list[we_have_spell].name, sizeof(m[0]->spell_name));
	}

	memcpy(m[0]->spell_str, str, sigils_no+2);
	return sigils_no;
}

int cast_handler() {
	int sigils_no = prepare_for_cast();
	Mqbspell **m = cur_mqb();
	if (sigils_no) {
		send_spell(m[0]->spell_str, sigils_no + 2);
	}
	return 1;
}

//Calc windows size based on xml data
void calc_spell_windows(){

	int i,gy=0,y;
	//calc spell_win
	for(i=0;i<num_groups;i+=2)
		gy+=MAX(33*(groups_list[i].spells/(SPELLS_ALIGN_X+1)+1)+20,33*(groups_list[i+1].spells/(SPELLS_ALIGN_X+1)+1)+20);
	spell_x_len = SPELLS_ALIGN_X*33*2+33+50;
	spell_y_len = 10+gy+50+15+37;
	spell_y_len_ext = spell_y_len+35;

	y=10;
	for(i=0;i<num_groups;i++){

		groups_list[i].x=20;
		groups_list[i].y=y+15;
		if(i==num_groups-1) groups_list[i].x+=((2*33*SPELLS_ALIGN_X+33)-(33*SPELLS_ALIGN_X))/2; //if groups are odd, last one is drawn in the middle

		i++;
		if(i>=num_groups) break;
		groups_list[i].x+=20+33+33*SPELLS_ALIGN_X;
		groups_list[i].y=y+15;
		y+=20+33*MAX(groups_list[i-1].spells/(SPELLS_ALIGN_X+1)+1,groups_list[i].spells/(SPELLS_ALIGN_X+1)+1);
	}

	//calc spell_mini_win
	spell_mini_rows=0;
	for(i=0;i<num_groups;i++)
		spell_mini_rows+=groups_list[i].spells/(SPELLS_ALIGN_X+1)+1;
	spell_mini_x_len=SPELLS_ALIGN_X*33+50;
	spell_mini_y_len=10+33*spell_mini_rows+20+30+37;
}

#ifdef FR_NECRO_RECETTES
int invocation_depuis_sorts(Uint8 quantity)
{
    if(nb_recettes_necro > 0)
    {
	int i, j, k, l, total;
	int objet_recette[6][3] = {0};
	Uint8 str[32];
	int items_no=0;

	total = liste_items_necro[creature_en_cours][0];
	for(i = 0; i < total; i++)
	{
	    objet_recette[i][0] = liste_items_necro[creature_en_cours][i+1];
	}
	if(double_invoc)
	{
	    total++;
	    objet_recette[total-1][0] = 41;
	}
	for(i = 0; i < 36; i++)
	{
	    if(item_list[i].quantity > 0)
	    {
		for(j = 0; j < total; j++)
		{
		    if(item_list[i].id == objet_recette[j][0])
		    {
			objet_recette[j][1] = item_list[i].quantity;
			objet_recette[j][2] = item_list[i].pos;
		    }
		}
	    }
	}
	if(quantity > 1) l = 2;
	else l = 1;
	k = 0;
	for(i = 0; i < liste_items_necro[creature_en_cours][0]; i++)
	{
	    //printf("Item %i, id: %i, nombre demandé: %i, nombre dans l' inventaire: %i\n", i+1, liste_items_necro[creature_en_cours][i+1], liste_items_necro[creature_en_cours][i + 1 + liste_items_necro[creature_en_cours][0]*2], objet_recette[i][1]);
	    if(objet_recette[i][1] >= (liste_items_necro[creature_en_cours][i + 1 + liste_items_necro[creature_en_cours][0]*2])) k++;
	}
	if(double_invoc)
	{
	    if(objet_recette[total-1][1] >= (1)) k++;
	}
	if(k == total)
	{
	    str[0]=MANUFACTURE_THIS;
	    for(i = 0; i < total; i++)
	    {
		str[items_no*3+2] = objet_recette[i][2];
		if(double_invoc && i == total -1) pack_u16_le(str+items_no*3+2+1, 1);
		else pack_u16_le(str+items_no*3+2+1, liste_items_necro[creature_en_cours][i + 1 + liste_items_necro[creature_en_cours][0]*2]);
		items_no++;
	    }
	    str[1]=items_no;
	    if(items_no)
	    {
		str[items_no*3+2]= quantity;
		my_tcp_send(my_socket,str,items_no*3+3);
	    }
	}
	else
	{
	    if(l == 1) set_shown_string(c_red2, "Il manque des ingrédients pour pouvoir lancer l' invocation !");
	    else set_shown_string(c_red2, "Il manque des ingrédients pour pouvoir lancer plusieurs invocations !");
	}
    }
    return 1;
}
int une_bebete()
{
    if(nb_recettes_necro > 0) return invocation_depuis_sorts(1);
    else return 1;
}
int plusieurs_bebetes()
{
    if(nb_recettes_necro > 0) return invocation_depuis_sorts(255);
    else return 1;
}
#endif //FR_NECRO_RECETTES

//Create and show/hide our windows
void display_sigils_menu()
{
	static int checked_reagents = 0;
	if (!checked_reagents) {
		if (item_info_available()) {
			int i, j;
			// check item ids/uid all give unique items
			for (i = 0; i < SPELLS_NO; i++)
				for(j=0;j<4;j++)
					if (spells_list[i].reagents_id[j] >= 0)
						if (get_item_count(spells_list[i].reagents_uid[j], spells_list[i].reagents_id[j]) != 1)
							LOG_ERROR("Invalid spell.xml reagents spells_list[%d].reagents_uid[%d]=%d spells_list[%d].reagents_id[%d]=%d\n",
								i, j, spells_list[i].reagents_uid[j], i, j, spells_list[i].reagents_id[j]);
		}
		checked_reagents = 1;
	}

	calc_spell_windows();
	if(sigils_win < 0){
		//create sigil win
		static int cast_button_id=100;
		static int clear_button_id=101;
		widget_list *w_cast = NULL;
		widget_list *w_clear = NULL;
		int but_space = 0;

		int our_root_win = -1;

		if (!windows_on_top) {
			our_root_win = game_root_win;
		}
		sigils_win= create_window(win_sigils, our_root_win, 0, sigil_menu_x, sigil_menu_y, sigil_x_len, sigil_y_len, ELW_WIN_DEFAULT);

		set_window_handler(sigils_win, ELW_HANDLER_DISPLAY, &display_sigils_handler );
		set_window_handler(sigils_win, ELW_HANDLER_CLICK, &click_sigils_handler );
		set_window_handler(sigils_win, ELW_HANDLER_MOUSEOVER, &mouseover_sigils_handler );

		cast_button_id=button_add_extended(sigils_win, cast_button_id, NULL, 0, 0, 0, 0, 0, 1.0f, 0.77f, 0.57f, 0.39f, cast_str);
		widget_set_OnClick(sigils_win, cast_button_id, cast_handler);

		clear_button_id=button_add_extended(sigils_win, clear_button_id, NULL, 0, 0, 0, 0, 0, 1.0f, 0.77f, 0.57f, 0.39f, clear_str);
		widget_set_OnClick(sigils_win, clear_button_id, spell_clear_handler);

		w_cast = widget_find(sigils_win, cast_button_id);
		w_clear = widget_find(sigils_win, clear_button_id);
		but_space = (sigil_x_len - (33*6+5) - w_cast->len_x - w_clear->len_x)/3;
		widget_move(sigils_win, cast_button_id, 33*6+5 + but_space+5, sigil_y_len - w_cast->len_y - 4);
		widget_move(sigils_win, clear_button_id, w_cast->pos_x + w_cast->len_x + but_space, sigil_y_len - w_clear->len_y - 4);
		hide_window(sigils_win);
	}

	if(spell_win < 0){
		//create spell win
		static int cast2_button_id=102;
		widget_list *w_cast = NULL;
		int our_root_win = -1;

		if (!windows_on_top) {
			our_root_win = game_root_win;
		}
#ifdef ENGLISH
		spell_win= create_window("Spells", our_root_win, 0, sigil_menu_x, sigil_menu_y, spell_x_len, spell_y_len_ext, ELW_WIN_DEFAULT);
#else
		//@tosh : traduction de la fenêtre de sorts
		spell_win= create_window("Grimoire - Recettes", our_root_win, 0, sigil_menu_x, sigil_menu_y, spell_x_len, spell_y_len_ext, ELW_WIN_DEFAULT);
#endif

		set_window_handler(spell_win, ELW_HANDLER_DISPLAY, &display_spells_handler );
		set_window_handler(spell_win, ELW_HANDLER_CLICK, &click_spells_handler );
		set_window_handler(spell_win, ELW_HANDLER_MOUSEOVER, &mouseover_spells_handler );


		cast2_button_id=button_add_extended(spell_win, cast2_button_id, NULL, 0, 0, 0, 0, 0, 1.0f, 0.77f, 0.57f, 0.39f, cast_str);
		widget_set_OnClick(spell_win, cast2_button_id, cast_handler);
		w_cast = widget_find(spell_win, cast2_button_id);
		widget_move(spell_win, cast2_button_id, spell_x_len-20-10-w_cast->len_x , spell_y_len_ext - w_cast->len_y - 4);

		hide_window(spell_win);
#ifndef FR_NECRO_RECETTES
		if(!start_mini_spells) sigil_win=spell_win;
#endif //FR_NECRO_RECETTES
	}

	if(spell_mini_win < 0){
		//create mini spell win
		int our_root_win = -1;

		if (!windows_on_top) {
			our_root_win = game_root_win;
		}
#ifdef ENGLISH
		spell_mini_win= create_window("Spells", our_root_win, 0, sigil_menu_x, sigil_menu_y, spell_mini_x_len, spell_mini_y_len, ELW_WIN_DEFAULT);
#else
		//@tosh : Traduction du titre de la fenêtre
		spell_mini_win= create_window("Grimoire - Sorts", our_root_win, 0, sigil_menu_x, sigil_menu_y, spell_mini_x_len, spell_mini_y_len, ELW_WIN_DEFAULT);
#endif

		set_window_handler(spell_mini_win, ELW_HANDLER_DISPLAY, &display_spells_mini_handler );
		set_window_handler(spell_mini_win, ELW_HANDLER_CLICK, &click_spells_mini_handler );
		set_window_handler(spell_mini_win, ELW_HANDLER_MOUSEOVER, &mouseover_spells_mini_handler );

		hide_window(spell_mini_win);
#ifndef FR_NECRO_RECETTES
		if(start_mini_spells) sigil_win=spell_mini_win;
#endif //FR_NECRO_RECETTES
	}
#ifdef FR_FENETRE_NECRO
    if(necro_win < 0)
	{
		int our_root_win = -1;
#ifdef FR_NECRO_RECETTES
		int i;
		int necro_mixone_button_id=1001;
		int necro_mixall_button_id=1002;
		int placement_x = 25;
		int placement_y = 190 - 15;

		nb_recettes_necro = chargement_necro_recettes(0);
		if(nb_recettes_necro == -1)
		{
		    LOG_ERROR("Le fichier creatnecro est absent...");
		}
		else
		{
		    nom_bestiole = (char **) malloc(nb_recettes_necro * sizeof(char *));
		    liste_items_necro = malloc(nb_recettes_necro * sizeof(int*));
		    for (i = 0 ; i < nb_recettes_necro ; i++)
		    {
			liste_items_necro[i] = malloc(20 * sizeof(int));
		    }
		    nb_recettes_necro = chargement_necro_recettes(1);
		}
#endif //FR_NECRO_RECETTES
		if (!windows_on_top)
		{
			our_root_win = game_root_win;
		}
        	necro_win= create_window("Nécromancie", our_root_win, 0, sigil_menu_x, sigil_menu_y, necro_x_len, necro_y_len, ELW_WIN_DEFAULT);
		set_window_handler(necro_win, ELW_HANDLER_DISPLAY, &display_necro_handler );
		set_window_handler(necro_win, ELW_HANDLER_CLICK, &click_necro_handler );
		set_window_handler(necro_win, ELW_HANDLER_MOUSEOVER, &mouseover_necro_handler );
#ifdef FR_NECRO_RECETTES
		if(nb_recettes_necro > 0)
		{
		    necro_mixone_button_id=button_add_extended(necro_win, necro_mixone_button_id, NULL, placement_x, placement_y, 43, 0, 0, 1.0f, 0.77f, 0.57f, 0.39f, ">");
		    widget_set_OnClick(necro_win, necro_mixone_button_id, une_bebete);
		    necro_mixall_button_id=button_add_extended(necro_win, necro_mixall_button_id, NULL, placement_x + 85, placement_y, 66, 0, 0, 1.0f, 0.77f, 0.57f, 0.39f, ">>");
		    widget_set_OnClick(necro_win, necro_mixall_button_id, plusieurs_bebetes);
		}
#endif //FR_NECRO_RECETTES
		hide_window(necro_win);
#ifndef FR_NECRO_RECETTES
		if(start_mini_spells) sigil_win=necro_win;
#endif //FR_NECRO_RECETTES
	}
#endif //FR_FENETRE_NECRO
	check_castability();
#ifdef FR_NECRO_RECETTES
	if(start_mini_spells == 0) sigil_win = sigils_win;
    	else if (start_mini_spells == 1) sigil_win = spell_mini_win;
   	else if (start_mini_spells == 2) sigil_win = spell_win;
    	else if (start_mini_spells == 3) sigil_win = necro_win;
#endif //FR_NECRO_RECETTES
	switch_handler((init_ok) ? (sigil_win):(sigils_win));
}

void init_sigils(){
	int i;

	i=0;

	// TODO: load this data from a file
	sigils_list[i].sigil_img=0;
	my_strcp(sigils_list[i].name,(char*)sig_change.str);
	my_strcp(sigils_list[i].description,(char*)sig_change.desc);
	sigils_list[i].have_sigil=1;

	i++;
	sigils_list[i].sigil_img=1;
	my_strcp(sigils_list[i].name,(char*)sig_restore.str);
	my_strcp(sigils_list[i].description,(char*)sig_restore.desc);
	sigils_list[i].have_sigil=1;

	i++;
	sigils_list[i].sigil_img=2;
	my_strcp(sigils_list[i].name,(char*)sig_space.str);
	my_strcp(sigils_list[i].description,(char*)sig_space.desc);
	sigils_list[i].have_sigil=1;

	i++;
	sigils_list[i].sigil_img=3;
	my_strcp(sigils_list[i].name,(char*)sig_increase.str);
	my_strcp(sigils_list[i].description,(char*)sig_increase.desc);
	sigils_list[i].have_sigil=1;

	i++;
	sigils_list[i].sigil_img=4;
	my_strcp(sigils_list[i].name,(char*)sig_decrease.str);
	my_strcp(sigils_list[i].description,(char*)sig_decrease.desc);
	sigils_list[i].have_sigil=1;

	i++;
	sigils_list[i].sigil_img=5;
	my_strcp(sigils_list[i].name,(char*)sig_temp.str);
	my_strcp(sigils_list[i].description,(char*)sig_temp.desc);
	sigils_list[i].have_sigil=1;

	i++;
	sigils_list[i].sigil_img=6;
	my_strcp(sigils_list[i].name,(char*)sig_perm.str);
	my_strcp(sigils_list[i].description,(char*)sig_perm.desc);
	sigils_list[i].have_sigil=1;

	i++;
	sigils_list[i].sigil_img=7;
	my_strcp(sigils_list[i].name,(char*)sig_move.str);
	my_strcp(sigils_list[i].description,(char*)sig_move.desc);
	sigils_list[i].have_sigil=1;

	i++;
	sigils_list[i].sigil_img=8;
	my_strcp(sigils_list[i].name,(char*)sig_local.str);
	my_strcp(sigils_list[i].description,(char*)sig_local.desc);
	sigils_list[i].have_sigil=1;

	i++;
	sigils_list[i].sigil_img=9;
	my_strcp(sigils_list[i].name,(char*)sig_global.str);
	my_strcp(sigils_list[i].description,(char*)sig_global.desc);
	sigils_list[i].have_sigil=1;

	i++;
	sigils_list[i].sigil_img=10;
	my_strcp(sigils_list[i].name,(char*)sig_fire.str);
	my_strcp(sigils_list[i].description,(char*)sig_fire.desc);
	sigils_list[i].have_sigil=1;

	i++;
	sigils_list[i].sigil_img=11;
	my_strcp(sigils_list[i].name,(char*)sig_water.str);
	my_strcp(sigils_list[i].description,(char*)sig_water.desc);
	sigils_list[i].have_sigil=1;

	i++;
	sigils_list[i].sigil_img=12;
	my_strcp(sigils_list[i].name,(char*)sig_air.str);
	my_strcp(sigils_list[i].description,(char*)sig_air.desc);
	sigils_list[i].have_sigil=1;

	i++;
	sigils_list[i].sigil_img=13;
	my_strcp(sigils_list[i].name,(char*)sig_earth.str);
	my_strcp(sigils_list[i].description,(char*)sig_earth.desc);
	sigils_list[i].have_sigil=1;

	i++;
	sigils_list[i].sigil_img=14;
	my_strcp(sigils_list[i].name,(char*)sig_spirit.str);
	my_strcp(sigils_list[i].description,(char*)sig_spirit.desc);
	sigils_list[i].have_sigil=1;

	i++;
	sigils_list[i].sigil_img=15;
	my_strcp(sigils_list[i].name,(char*)sig_matter.str);
	my_strcp(sigils_list[i].description,(char*)sig_matter.desc);
	sigils_list[i].have_sigil=1;

	i++;
	sigils_list[i].sigil_img=16;
	my_strcp(sigils_list[i].name,(char*)sig_energy.str);
	my_strcp(sigils_list[i].description,(char*)sig_energy.desc);
	sigils_list[i].have_sigil=1;

	i++;
	sigils_list[i].sigil_img=17;
	my_strcp(sigils_list[i].name,(char*)sig_magic.str);
	my_strcp(sigils_list[i].description,(char*)sig_magic.desc);
	sigils_list[i].have_sigil=1;

	i++;
	sigils_list[i].sigil_img=18;
	my_strcp(sigils_list[i].name,(char*)sig_destroy.str);
	my_strcp(sigils_list[i].description,(char*)sig_destroy.desc);
	sigils_list[i].have_sigil=1;

	i++;
	sigils_list[i].sigil_img=19;
	my_strcp(sigils_list[i].name,(char*)sig_create.str);
	my_strcp(sigils_list[i].description,(char*)sig_create.desc);
	sigils_list[i].have_sigil=1;

	i++;
	sigils_list[i].sigil_img=20;
	my_strcp(sigils_list[i].name,(char*)sig_knowledge.str);
	my_strcp(sigils_list[i].description,(char*)sig_knowledge.desc);
	sigils_list[i].have_sigil=1;

	i++;
	sigils_list[i].sigil_img=21;
	my_strcp(sigils_list[i].name,(char*)sig_protection.str);
	my_strcp(sigils_list[i].description,(char*)sig_protection.desc);
	sigils_list[i].have_sigil=1;

	i++;
	sigils_list[i].sigil_img=22;
	my_strcp(sigils_list[i].name,(char*)sig_remove.str);
	my_strcp(sigils_list[i].description,(char*)sig_remove.desc);
	sigils_list[i].have_sigil=1;

	i++;
	sigils_list[i].sigil_img=23;
	my_strcp(sigils_list[i].name,(char*)sig_health.str);
	my_strcp(sigils_list[i].description,(char*)sig_health.desc);
	sigils_list[i].have_sigil=1;

	i++;
	sigils_list[i].sigil_img=24;
	my_strcp(sigils_list[i].name,(char*)sig_life.str);
	my_strcp(sigils_list[i].description,(char*)sig_life.desc);
	sigils_list[i].have_sigil=1;

	i++;
	sigils_list[i].sigil_img=25;
	my_strcp(sigils_list[i].name,(char*)sig_death.str);
	my_strcp(sigils_list[i].description,(char*)sig_death.desc);
	sigils_list[i].have_sigil=1;

#ifndef ENGLISH
	i++;
	sigils_list[i].sigil_img=26;
	my_strcp(sigils_list[i].name,(char*)sig_chant.str);
	my_strcp(sigils_list[i].description,(char*)sig_chant.desc);
	sigils_list[i].have_sigil=1;
#endif //ENGLISH
}

#ifdef FR_RCM_MAGIE

/**
    Ouvre la fenêtre d'acceptation du sort type "sort bénéfique avec acceptation"
**/
void spell_you_answer( int player_id ){

    int sort_win = 0;

    int our_root_win = -1;
    if (!windows_on_top) {
        our_root_win = game_root_win;
    }
    sort_win= create_window(sort_win, our_root_win, 0, sort_menu_x, sort_menu_y, sort_menu_x_len, sort_menu_y_len, (ELW_WIN_DEFAULT& ~ELW_CLOSE_BOX));

    set_window_handler(sort_win, ELW_HANDLER_DISPLAY, &display_sort_handler );
    ///set_window_handler(sort_win, ELW_HANDLER_CLICK, &click_trade_handler );
    ///set_window_handler(sort_win, ELW_HANDLER_MOUSEOVER, &mouseover_trade_handler );

}

int display_sort_handler(window_info *win) {
	int x=10+33;
	int i;
	char str[20];

	//Draw the names in the accept boxes

	/*if(trade_you_accepted==1){
		glColor3f(1.0f,1.0f,0.0f);
	} else if(trade_you_accepted==2){
		glColor3f(0.0f,1.0f,0.0f);
	} else {
		glColor3f(1.0f,0.0f,0.0f);
	}*/

	draw_string_small(x+33-strlen(accept_str)*4, button_y_top+2, (unsigned char*)accept_str, 1);

	/*if(trade_other_accepted<=0){    // RED
		glColor3f(1.0f,0.0f,0.0f);
	} else if(trade_other_accepted==1){ // YELLOW
		glColor3f(1.0f,1.0f,0.0f);
	} else {    // all others default to GREEN
		glColor3f(0.0f,1.0f,0.0f);
	}*/

	draw_string_small(x+6*33-strlen(accept_str)*4, button_y_top+2, (unsigned char*)accept_str, 1);

	glColor3f(0.77f,0.57f,0.39f);

	//Draw the trade session names
	draw_string_small(10+2*33-strlen(you_str)*4,11,(unsigned char*)you_str,1);
	///draw_string_small(10+7*33-strlen(other_player_trade_name)*4,11,(unsigned char*)other_player_trade_name,1);

	//Draw the X for aborting the trade
	draw_string(win->len_x-(ELW_BOX_SIZE-4), 2, (unsigned char*)"X", 1);

	glColor3f(1.0f,1.0f,1.0f);

	//Now let's draw the goods on trade...


	glDisable(GL_TEXTURE_2D);

	glColor3f(0.77f,0.57f,0.39f);
	// grids for goods on trade
	rendergrid (4, 4, 10, 30, 33, 33);
	rendergrid (4, 4, 10+5*33, 30, 33, 33);

	// Accept buttons
	x=10+33;

	glBegin (GL_LINE_LOOP);
		glVertex3i(x,button_y_top,0);
		glVertex3i(x+66,button_y_top,0);
		glVertex3i(x+66,button_y_bot,0);
		glVertex3i(x,button_y_bot,0);
	glEnd ();

	x+=5*33;

	glBegin (GL_LINE_LOOP);
		glVertex3i(x,button_y_top,0);
		glVertex3i(x+66,button_y_top,0);
		glVertex3i(x+66,button_y_bot,0);
		glVertex3i(x,button_y_bot,0);
	glEnd ();


	//Draw the border for the "X"
	glBegin(GL_LINE_STRIP);
		glVertex3i(win->len_x, ELW_BOX_SIZE, 0);
		glVertex3i(win->len_x-ELW_BOX_SIZE, ELW_BOX_SIZE, 0);
		glVertex3i(win->len_x-ELW_BOX_SIZE, 0, 0);
	glEnd();

	//Draw the help text
	///if(show_help_text && show_abort_help) show_help(abort_str, win->len_x-(ELW_BOX_SIZE-4)/2-strlen(abort_str)*4, 4+ELW_BOX_SIZE);

	glEnable(GL_TEXTURE_2D);

	//now, draw the inventory text, if any.


#ifdef OPENGL_TRACE
CHECK_GL_ERRORS();
#endif //OPENGL_TRACE

	return 1;
}
#endif
void fast_spell_cible(int pos) {
	char str[256];
	if (selected_spell == -1) {
		fast_spell_cible_bool = 1;
		selected_spell = pos;
	} else if (selected_spell == pos) {
		fast_spell_cible_bool = 0;
		selected_spell = -1;
	} else {
		fast_spell_cible_bool = 1;
		selected_spell = pos;
	}
	Mqbspell **m = cur_mqb();
	if (m[pos]) {
		if (fast_spell_cible_bool) {
			safe_snprintf(str, sizeof(str), "Tu as sélectionné \"%s\" !", m[pos]->spell_name);
			LOG_TO_CONSOLE(c_green1, str);
		} else {
			LOG_TO_CONSOLE(c_green1, "Tu ne séléctionne plus aucuns sorts.");
		}
	}
}
void fast_spell_cast(int actor_id) {
	if (actor_id >= 0) {
		actor *a = get_actor_ptr_from_id(actor_id);
		if (a) {
			Uint8 b[8] = {TOUCH_PLAYER};
			pack_u32_le(b+1, actor_id);
			my_tcp_send(my_socket, b, 5);
			add_highlight(a->x_tile_pos, a->y_tile_pos, HIGHLIGHT_TYPE_SPELL_TARGET);
		}
	}
}
void fast_spell_teleport(void) {
	if (selected_spell_target >= 0) {
		Uint8 b[8] = {ATTACK_SOMEONE};
		pack_u32_le(b+1, selected_spell_target);
		my_tcp_send(my_socket, b, 5);
	}
}
void fast_spell_decible(void) {
    fast_spell_cible_bool = 0;
    selected_spell = -1;
    LOG_TO_CONSOLE(c_orange1, "Tu ne séléctionne plus aucuns sorts.");
}
