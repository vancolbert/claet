#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include "manufacture.h"
#include "asc.h"
#include "context_menu.h"
#include "cursors.h"
#include "elconfig.h"
#include "elwindows.h"
#include "gamewin.h"
#include "hud.h"
#include "init.h"
#include "interface.h"
#include "item_info.h"
#include "multiplayer.h"
#include "textures.h"
#include "translate.h"
#include "sound.h"
#include "io/elpathwrapper.h"
#include "errors.h"
#include "context_menu.h"

#define MAX_RECIPE 20
#define NUM_MIX_SLOTS 6
#define MIX_SLOT_OFFSET 36
#define SHOW_MAX_RECIPE (MAX_RECIPE)

int recipe_status[MAX_RECIPE];
item recipes[MAX_RECIPE][6];
int cur_recipe = -1; // aucune recette sélectionnée par défaut
int cur_valids[6];
item manufacture_list[ITEM_WEAR_START+6];
int cm_listrecipe_enabled = 0;
int cm_manurecipe_invertwheel = 0;
int cm_manurecipe_addnolimit = 0;
int cm_manurecipe_wheelaffect = 0;
static size_t cm_listrecipe_id = CM_INIT_VALUE;
int recipes_shown=0;
static int recipes_loaded=0;
static int mouse_over_recipe = -1;
int manufacture_menu_x_len=12*33+20;
int manufacture_menu_y_len=6*33;
int manufacture_win= -1;
int recipe_win= -1;
int manufacture_menu_x=10;
int manufacture_menu_y=20;
static int last_changed_slot=-1;

static char items_string[350]={0};
static size_t last_items_string_id = 0;

/* vars for saving recipe names */
static char *recipe_name[SHOW_MAX_RECIPE];
static item last_mix[NUM_MIX_SLOTS];
static int recipe_names_changed = 0;
static int initialised_recipe_names = 0;

static char *current_recipe_name = NULL;
static int current_recipe_changed = 0;

/* change (ou simplement vide) le nom associé à la recette courante */
void change_current_name(const char *name)
{
	if (current_recipe_name != NULL)
	{
		free(current_recipe_name);
		current_recipe_name = NULL;
	}
	if (name != NULL)
	{
		size_t len = strlen(name);
		current_recipe_name = (char *)malloc(len+1);
		safe_strncpy(current_recipe_name, name, len+1);
	}
	current_recipe_changed = 1;
}

/* called on client exit to free memory and clean up */
void cleanup_manufacture(void)
{
	size_t i;
	for (i=0; i<SHOW_MAX_RECIPE; i++)
		if (recipe_name[i] != NULL)
		{
			free(recipe_name[i]);
			recipe_name[i] = NULL;
		}
}


/* initialse recipe name vars */
static void init_recipe_names(void)
{
	size_t i;
	if (initialised_recipe_names)
		return;
	for (i=0; i<SHOW_MAX_RECIPE; i++)
		recipe_name[i] = NULL;
	for (i=0; i<NUM_MIX_SLOTS; i++)
		last_mix[i].quantity = 0;
	initialised_recipe_names = 1;
}

/* create a new recipe name entry */
static void new_recipe_name(size_t recipe_no, const char *name)
{
	size_t len = strlen(name);
	if ((recipe_no >= SHOW_MAX_RECIPE) || (recipe_name[recipe_no] != NULL))
		return;
	recipe_name[recipe_no] = (char *)malloc(len+1);
	safe_strncpy(recipe_name[recipe_no], name, len+1);
	recipe_names_changed = 1;
}


/* save recipe names to file if any have changed */
static void save_recipe_names(void)
{
	char fname[128];
	FILE *fp;
	size_t i;
	int errorflag = 0;

	if (!recipe_names_changed)
		return;

	safe_snprintf(fname, sizeof(fname), "recipes_%s.names",username_str);
	my_tolower(fname);
	fp = open_file_config(fname,"w");
	if(fp == NULL)
	{
		LOG_ERROR("%s() %s \"%s\": %s\n", __FUNCTION__, cant_open_file, fname, strerror(errno));
		return;
	}

	for (i=0; i<SHOW_MAX_RECIPE; i++)
	{
		if (recipe_name[i] != NULL)
		{
			if (fputs(recipe_name[i], fp) < 0)
			{
				errorflag = 1;
				break;
			}
		}
		if (fputc('\n', fp) != '\n')
		{
			errorflag = 1;
			break;
		}
	}
	if (errorflag)
		LOG_ERROR("%s() %s \"%s\": %s\n", __FUNCTION__, cant_open_file, fname, strerror(errno));

	fclose(fp);
}


/* load saved recipe names from file */
static void load_recipe_names(void)
{
	char fname[128];
	FILE *fp;
	char line [128];
	size_t recipe_no;

	recipe_names_changed = 0;

	safe_snprintf(fname, sizeof(fname), "recipes_%s.names",username_str);
	my_tolower(fname);

	/* sliently ignore non existing file */
	if (file_exists_config(fname)!=1)
		return;

	fp = open_file_config(fname,"r");
	if(fp == NULL)
	{
		LOG_ERROR("%s() %s \"%s\": %s\n", __FUNCTION__, cant_open_file, fname, strerror(errno));
		return;
	}

	recipe_no = 0;
 	while (fgets(line, sizeof(line), fp) != NULL)
	{
		size_t len = strlen(line);
		while ((len > 0) && ((line[len-1] == '\r') || (line[len-1] == '\n') || (line[len-1] == ' ')))
		{
			line[len-1] = '\0';
			len--;
		}
		if (len > 0)
			new_recipe_name(recipe_no, line);
		recipe_no++;
	}

	fclose(fp);
}


/* each time a mix is called, save the ingredients - so we can compare later */
static void save_last_mix(void)
{
	size_t i;
	for (i=MIX_SLOT_OFFSET;i<MIX_SLOT_OFFSET+NUM_MIX_SLOTS;i++)
		if (manufacture_list[i].quantity > 0)
			last_mix[i-MIX_SLOT_OFFSET] = manufacture_list[i];
}


/* if the name is set, delete it */
static void clear_recipe_name(size_t recipe_no)
{
	if ((recipe_no < SHOW_MAX_RECIPE) && (recipe_name[cur_recipe] != NULL))
	{
		free(recipe_name[cur_recipe]);
		recipe_name[cur_recipe] = NULL;
		recipe_names_changed = 1;
	}
}


/*	Compare the last mixed ingredients to all the recipes without a name.
 *	If one matches then set the name to the provided string.
 */
void check_for_recipe_name(const char *name)
{
	size_t i, recipe_index;
	int num_last_ing = 0;

	// exit now if no last recipe
	for (i=0; i<NUM_MIX_SLOTS; i++)
		if (last_mix[i].quantity > 0)
			num_last_ing++;
	if (!num_last_ing)
		return;

	// on affecte le nom à la recette courante si elle n'a pas bougé entre deux
	if (! current_recipe_changed) change_current_name(name);

	// check any not-set recipe name to see if the recipe matches the last_mix
	for (recipe_index=0; recipe_index<SHOW_MAX_RECIPE; recipe_index++)
	{
		int num_recipe_ing = 0;
		int num_match_ing = 0;
		size_t recipe_slot_index;
		item last_mix_cpy[NUM_MIX_SLOTS];
		item *recipe = recipes[recipe_index];

		// move on if already have name, no recipe or if the ingredient counts don't match
		if (recipe_name[recipe_index] != NULL)
			continue;
		for (i=0; i<NUM_MIX_SLOTS; i++)
			if (recipe[i].quantity > 0)
				num_recipe_ing++;
		if (!num_recipe_ing || (num_recipe_ing!=num_last_ing))
			continue;

		// allowing for any order, check if ingredients are the same
		memcpy(last_mix_cpy, last_mix, sizeof(item)*NUM_MIX_SLOTS);
		for (recipe_slot_index=0; recipe_slot_index<NUM_MIX_SLOTS; recipe_slot_index++)
		{
			size_t last_mix_slot_index;
			if (recipe[recipe_slot_index].quantity < 1)
				continue;
			for (last_mix_slot_index=0; last_mix_slot_index<NUM_MIX_SLOTS; last_mix_slot_index++)
				if ((last_mix_cpy[last_mix_slot_index].quantity > 0) &&
					(recipe[recipe_slot_index].quantity == last_mix_cpy[last_mix_slot_index].quantity) &&
					(recipe[recipe_slot_index].image_id == last_mix_cpy[last_mix_slot_index].image_id) &&
					(recipe[recipe_slot_index].id == last_mix_cpy[last_mix_slot_index].id))
				{
					last_mix_cpy[last_mix_slot_index].quantity = 0;
					num_match_ing++;
					break;
				}
		}

		// if ingredients are the same, eureka!
		if (num_match_ing == num_recipe_ing)
		{
			new_recipe_name(recipe_index, name);
			break;
		}
	}

	/* clear the last mix in all cases */
	for (i=0; i<NUM_MIX_SLOTS; i++)
		last_mix[i].quantity = 0;
}



/* load recipes, done just after login when we have the player name */
void load_recipes (){
	char fname[128];
	FILE *fp;

	init_recipe_names();
		/*
		 * cette précaution est totalement inutile puisque le fichier est sauvé dès
		 * qu'il est modifié, autant s'en passer (un save au lieu d'un load est trompeur)
		 */

	recipes_loaded=1;
	memset (recipes, 0, sizeof (recipes));

	safe_snprintf(fname, sizeof(fname), "recipes_%s.dat",username_str);
	my_tolower(fname);

	/* sliently ignore non existing file */
	if (file_exists_config(fname)!=1)
		return;

	fp = open_file_config(fname,"rb");
	if(fp == NULL){
		LOG_ERROR("%s: %s \"%s\": %s\n", reg_error_str, cant_open_file, fname, strerror(errno));
		return;
	}
	// controle que le fichier contient le bon nombre d'items (à la bonne taille) sinon abandon
	if (fread(recipes, sizeof(item)*6, MAX_RECIPE, fp) != MAX_RECIPE)
	{
		int k, i;
		for (k=0; k<MAX_RECIPE; k++) for(i=0; i<6; i++) recipes[k][i].quantity = -1;
		LOG_ERROR("%s: %s \"%s\"\n", reg_error_str, cant_open_file, fname);
	}
	else
	{
		build_manufacture_list();
	}
	fclose (fp);

	load_recipe_names();
}


/* save the recipe names, done on exit and after each change */
void save_recipes(){
	char fname[128];
	FILE *fp;

	if (!recipes_loaded)
		return;

	save_recipe_names();

	safe_snprintf(fname, sizeof(fname), "recipes_%s.dat",username_str);
	my_tolower(fname);
	// si le fichier n'a pas été lu, alors il ne faut pas l'écraser !
	// (par exemple en fermant le client avant de se connecter)
	if (! recipes_loaded && file_exists_config(fname)) return;
	fp=open_file_config(fname,"wb");
	if(fp == NULL){
		LOG_ERROR("%s: %s \"%s\": %s\n", reg_error_str, cant_open_file, fname, strerror(errno));
		return;
	}

	fwrite(recipes, sizeof(recipes), 1, fp);

	fclose(fp);
}


void build_manufacture_list()
{
	int i,j,l,k;


	/* on passe les cases vierges à -1, pour les différencier des quantités réduites à 0
	   par la recette courante qui pourront alors être affichées grisées */
	for (i=0; i<ITEM_WEAR_START; i++) manufacture_list[i].quantity = -1;
//	for (i=0; i<6; i++) manufacture_list[ITEM_WEAR_START+i].quantity = 0;

	//ok, now see which items are resources
	j=0;
	for(i=0;i<ITEM_WEAR_START;i++) {
		if(item_list[i].quantity && item_list[i].is_resource) {
			manufacture_list[j].quantity=item_list[i].quantity;
			manufacture_list[j].image_id=item_list[i].image_id;
			manufacture_list[j].id=item_list[i].id;
			manufacture_list[j].pos=item_list[i].pos;
			j++;
		}
	}
	//now check for all items in all recipes
	for(k=0;k<MAX_RECIPE;k++){
		l = -1; // -1 nous indiquera que la recette est totalement vide
		for (i=0; i<6 && l; i++) if (recipes[k][i].quantity > 0)
		{
			l = 1;
			for (j=0; j<ITEM_WEAR_START; j++)
			{
				if (manufacture_list[j].quantity < 0) continue;
				if (manufacture_list[j].id != unset_item_uid)
				{
					if (manufacture_list[j].id != recipes[k][i].id) continue;
				}
				else
				if (recipes[k][i].image_id != manufacture_list[j].image_id) continue;
				if (recipes[k][i].quantity > manufacture_list[j].quantity) l = 0;  // can't make
				break;
			}
			if (j >= ITEM_WEAR_START) l = 0;  // watch for the item missing
		}
		//updates recipe_status
		recipe_status[k]=l;
	}

	for (i=ITEM_WEAR_START; i<ITEM_WEAR_START+6; i++)
	{
		cur_valids[i-ITEM_WEAR_START] = 0;
		if (manufacture_list[i].quantity <= 0) continue;

		// on recherche une ressource du sac correspondant à l'ingrédient de la recette
		manufacture_list[i].pos = -1;
		l = -1;
		for (j=0; j<ITEM_WEAR_START; j++)
		{
			if (manufacture_list[j].quantity < 0) continue;
			if (manufacture_list[j].id != unset_item_uid)
			{
				if (manufacture_list[j].id != manufacture_list[i].id) continue;
			}
			else
			if (manufacture_list[j].image_id != manufacture_list[i].image_id) continue;
			// si la ressource trouvée a sa quantité réduite à 0, on continue la recherche
			// dans l'éventualité d'en trouver une autre dispo (ingrédient non empilable)
			if (manufacture_list[j].quantity > 0) { l = j; break; }
			if (l < 0) l = j;
		}
		// si une ressource correspondante est trouvée, on assigne la position et on règle les quantités
		if (l >= 0)
		{
			manufacture_list[i].pos = manufacture_list[l].pos;
			if (manufacture_list[l].quantity < manufacture_list[i].quantity)
			{
				// la ressource n'est pas suffisante, son stock est réduit à 0
				manufacture_list[l].quantity = 0;
			}
			else
			{
				// la ressource est disponible en quantité, on la décrémente et l'ingrédient devient valide
				manufacture_list[l].quantity -= manufacture_list[i].quantity;
				cur_valids[i-ITEM_WEAR_START] = 1;
			}
		}
	}
}


//DRAWING FUNCTIONS

static void draw_recipe_controls(){
	int wpx=33*6+2;
	int wpy=manufacture_menu_y_len-37;
	int lpx=18;
	int lpy=33;

	if (recipes_shown){
	/* Up arrow */
		glBegin(GL_QUADS);
			glVertex3i(wpx+lpx/2, wpy+lpy-10, 0); //top
			glVertex3i(wpx+5, wpy+lpy, 0); //left
			glVertex3i(wpx+lpx-5, wpy+lpy, 0); //right
			glVertex3i(wpx+lpx/2, wpy+lpy-10, 0); //top
		glEnd();
	} else {
		/* Dn arrow */
		glBegin(GL_QUADS);
			glVertex3i(wpx+lpx/2, wpy+lpy, 0); //top
			glVertex3i(wpx+5, wpy+lpy-10, 0); //left
			glVertex3i(wpx+lpx-5, wpy+lpy-10, 0); //right
			glVertex3i(wpx+lpx/2, wpy+lpy, 0); //top
		glEnd();
	}

	/* Add btn */
	glEnable(GL_TEXTURE_2D);
	draw_string_zoomed(wpx+3, wpy-2, (unsigned char *)"+", 1, 1);
}

//draws a 6x1 grid of items+grid
static int draw_production_pipe(int x, int y, int recipe_num){
	int i,ofs,valid;
	Uint8 str[80];
	item *the_list;

	//if recipe_num is negative we draw the current manufacture_list, else a recipe
	if (recipe_num<0) {
		the_list=manufacture_list;
		ofs=36;
		/* Cas particulier pour la recette courante, affichable même si non réalisable.
		 * La fonction build doit déterminer son état, plutot qu'une simple variable globale,
		 * (un tableau contient l'indication pour chaque ingrédient)
		 */
		valid=2;
	} else {
		the_list=recipes[recipe_num];
		ofs=0;
		valid=recipe_status[recipe_num];
	}

	glEnable(GL_TEXTURE_2D);
	//ok, now let's draw the mixed objects
	if (valid >= 0) // on peut ajouter ce test puisqu'on sait avec -1 que la recette est vide
	for(i=ofs;i<6+ofs;i++) {
		glColor3f(1.0f,1.0f,1.0f);
		if(the_list[i].quantity > 0){
			int x_start,y_start;

			//get the x and y
			x_start=x+33*(i%6)+1;
			y_start=y;

			draw_item(the_list[i].image_id,x_start,y_start,33);
			safe_snprintf((char *)str, sizeof(str), "%i", the_list[i].quantity);
			if ((valid == 2) && (!cur_valids[i - ofs]))
			{
				// Pour la recette courante, on affiche en rouge la quantité des ingrédients manquants
				draw_string_small_shadowed(x_start,y_start+17,str,1,0.8f,0.0f,0.0f,0.0f,0.0f,0.0f);
			}
			else
			{
				draw_string_small_shadowed(x_start,y_start+17,str,1,1.0f,1.0f,1.0f,0.0f,0.0f,0.0f);
				// Tandis que pour les recettes mémorisées dans la liste, on les grise si non réalisables
				if (!valid) gray_out(x_start,y_start,33);
			}
		}
	}
	glDisable(GL_TEXTURE_2D);

	//draw the grid, in red if selected
	if (recipe_num>=0 && recipe_num==cur_recipe) glColor3f(1.0f,0.0f,0.0f);
	else glColor3f(0.77f,0.57f,0.39f);
	rendergrid(6,1,x, y, 33, 33);

	glEnable(GL_TEXTURE_2D);
	return 1;
}

static int	display_manufacture_handler(window_info *win)
{
	Uint8 str[80];
	int i;

	// keep the main and recipe windows opacity in sync
	if (recipe_win >= 0)
	{
		static int last_recipes_opaque = -1, last_main_opaque = -1;
		if (win->opaque != last_main_opaque)
			last_recipes_opaque = last_main_opaque = windows_list.window[recipe_win].opaque = win->opaque;
		else if (windows_list.window[recipe_win].opaque != last_recipes_opaque)
			last_recipes_opaque = last_main_opaque = win->opaque = windows_list.window[recipe_win].opaque;
	}

	glColor3f(0.77f,0.57f,0.39f);
	glEnable(GL_TEXTURE_2D);

	glColor3f(1.0f,1.0f,1.0f);
	//ok, now let's draw the objects...
	for(i=35;i>=0;i--){
		// on affiche aussi les case dont la quantité est réduite à 0 par la recette courante
		if(manufacture_list[i].quantity >= 0) {
			int x_start,y_start;

			//get the x and y
			x_start=33*(i%12)+1;
			y_start=33*(i/12);

			draw_item(manufacture_list[i].image_id,x_start,y_start,33);

			// Affichage "grisé" de la case si la quantité est réduite à 0 par la recette courante
			if (manufacture_list[i].quantity == 0)
			{
				glDisable(GL_TEXTURE_2D);
				glEnable(GL_BLEND);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				glBegin(GL_QUADS);
					glColor4f(0.8f, 0.0f, 0.0f, 0.4f);
					glVertex2f(x_start, y_start);
					glVertex2f(x_start, y_start+33);
					glVertex2f(x_start+33, y_start+33);
					glVertex2f(x_start+33, y_start);
				glEnd();
				glDisable(GL_BLEND);
				glEnable(GL_TEXTURE_2D);
			}
			safe_snprintf((char *)str, sizeof(str), "%i",manufacture_list[i].quantity);
			draw_string_small_shadowed(x_start, (i&1)?(y_start+17):(y_start+7),
				(unsigned char*)str, 1,1.0f,1.0f,1.0f, 0.0f, 0.0f, 0.0f);
		}
	}

	//ok, now let's draw the mixed objects
	draw_production_pipe(2,manufacture_menu_y_len-37, -1);

	//now, draw the inventory text, if any.
	if (last_items_string_id != inventory_item_string_id)
	{
		put_small_text_in_box((unsigned char*)inventory_item_string, strlen(inventory_item_string), win->len_x-8, items_string);
		last_items_string_id = inventory_item_string_id;
	}
	// si le texte n'occupe pas toutes les lignes on ajoute le nom de la recette courante
	if ((draw_string_small(4,manufacture_menu_y_len-88,(unsigned char *)items_string,4) < 4) && (current_recipe_name != NULL))
	{
		glColor3f(0.8f,0.8f,0.8f);
		draw_string_small(4, manufacture_menu_y_len-52, (unsigned char *)current_recipe_name, 1);
	}

	// Render the grid *after* the images. It seems impossible to code
	// it such that images are rendered exactly within the boxes on all
	// cards
	glDisable(GL_TEXTURE_2D);
	glColor3f(0.77f,0.57f,0.39f);

	//draw the grid
	rendergrid(12,3,0,0,33,33);

	//Draw the bottom grid - NOT NEEDED, DONE IN draw_production_pipe
	//rendergrid(NUM_MIX_SLOTS,1,0, manufacture_menu_y_len-37, SLOT_SIZE, SLOT_SIZE);

	//Draw recipe control buttons
	draw_recipe_controls();
	glEnable(GL_TEXTURE_2D);

	/* Affichage des noms (et messages d'aide) sur la liste des recettes.
	 * Déplacé ici puisque la fenêtre des recettes étant scrollable,
	 * il lui est impossible d'afficher les bulles à côté d'elle !
	 */
	if (recipes_shown)
	{
		if ((cur_recipe < SHOW_MAX_RECIPE) && (recipe_name[cur_recipe] != NULL))
		{
			i = cur_recipe*33 - get_window_scroll_pos(recipe_win);
			if (i>=0) show_help(recipe_name[cur_recipe], 33*6+30, win->len_y + i + (33-SMALL_FONT_Y_LEN)/2);
		}
		if (mouse_over_recipe != -1)
		{
			i = mouse_over_recipe*33 - get_window_scroll_pos(recipe_win);
			if ((i>=0) && (mouse_over_recipe != cur_recipe) && (mouse_over_recipe < SHOW_MAX_RECIPE) &&
				(recipe_name[mouse_over_recipe] != NULL))
				show_help_coloured(recipe_name[mouse_over_recipe], 33*6+30, win->len_y + i + (33-SMALL_FONT_Y_LEN)/2, 1.0f,1.0f,1.0f);

			if (show_help_text) show_help(recipe_select_str, 0, win->len_y + windows_list.window[recipe_win].len_y + 10);
		}
		mouse_over_recipe = -1;
	}

	return 1;
}


/* draw the recipes - finishing any resize to snap to a reasonable size */
static int recipe_dropdown_draw(window_info *win){
	int i;
	static Uint8 resizing = 0;

	// attendre la fin du redimensionnement...
	if (win->resized) resizing = 1;
	// ...pour adapter la nouvelle taille à la grille
	else if (resizing)
	{
		resizing = 0;
		resize_window (win->window_id, win->len_x, floor(win->len_y/33.0f + 0.5f)*33);
	}

	for (i=0;i<SHOW_MAX_RECIPE;i++){
		draw_production_pipe(0,33*i,i);
	}

	if (cur_recipe >= 0) draw_production_pipe(0,33*cur_recipe,cur_recipe);

	/* Les bulles d'aide avec le nom des recettes ne peuvent être créees ici !
	 * La fenêtre étant scrollable : impossible d'afficher du contenu en dehors
	 * Leur traitement est donc déporté dans la fonction display_manufacture_handler
	 */
	mouse_over_recipe = -1;

	return 1;
}


int recipe_dropdown_scroll_selection(Uint32 flags)
{
	if (flags & ELW_WHEEL_UP)
	{
/*
		// boucle entre le premier et le dernier
		if (cur_recipe < 0) cur_recipe = 0;
		cur_recipe = (cur_recipe-1+MAX_RECIPE)%MAX_RECIPE;
*/
		cur_recipe -= 1;
		if (cur_recipe < 0) cur_recipe = 0;
	}
	else if (flags & ELW_WHEEL_DOWN)
	{
/*
		// boucle entre le dernier et le premier
		cur_recipe = (cur_recipe+1)%MAX_RECIPE;
*/
		cur_recipe += 1;
		if (cur_recipe >= MAX_RECIPE) cur_recipe = MAX_RECIPE - 1;
	}
	else return 0;

	// déplacer l'ascenceur pour s'assurer que la recette sélectionnée est visible
	if (cur_recipe*33 < get_window_scroll_pos(recipe_win))
	{
		set_window_scroll_pos(recipe_win, cur_recipe*33);
	}
	else if ((cur_recipe+1)*33 > get_window_scroll_pos(recipe_win) + windows_list.window[recipe_win].len_y)
	{
		set_window_scroll_pos(recipe_win, (cur_recipe+1)*33 - windows_list.window[recipe_win].len_y);
	}

	// si le changement de recette sélectionnée à la molette entraine son affectation
	if (cm_manurecipe_wheelaffect)
	{
//		if (recipe_status[cur_recipe] >= 0)
		{
			int i;
			for (i=0; i<6; i++) manufacture_list[ITEM_WEAR_START+i] = recipes[cur_recipe][i];
			change_current_name(recipe_name[cur_recipe]);
			set_shown_string(c_green2, "Recette chargée depuis la liste.");
			build_manufacture_list();
		}
	}

	do_click_sound();
	return 1;
}


//CLICK HANDLERS
static int recipe_dropdown_click_handler(window_info *win, int mx, int my, Uint32 flags){

	static int last_clicked=0;
	static int last_recipe=0;
	int recipe_clicked = (my + get_window_scroll_pos(win->window_id))/33;

	// est-ce un changement de la recette sélectionnée à la molette ?
	if (recipe_dropdown_scroll_selection(flags)) return 1;

	// sinon considérer un clic normal sur le bouton
	if (flags & ELW_RIGHT_MOUSE)
	{
		// sélectionne ou désélectionne une recette de la liste
		cur_recipe = (cur_recipe == recipe_clicked) ? -1 : recipe_clicked;
		if (cm_listrecipe_enabled)
		{
			// affiche le menu contextuel en sélectionnant la ligne
			cm_show_direct(cm_listrecipe_id, recipe_win, -1);
			cur_recipe = recipe_clicked;
		}
		return 1;
	}

	if (recipe_status[recipe_clicked] >= 0)
	{
		int i;
		for (i=0; i<6; i++) manufacture_list[ITEM_WEAR_START+i] = recipes[recipe_clicked][i];
		change_current_name(recipe_name[recipe_clicked]);
		set_shown_string(c_green2, "Recette chargée depuis la liste");
		build_manufacture_list();

		// un double-clic ferme la liste (seulement sur le clic gauche)
		if ( ((SDL_GetTicks()-last_clicked) < 400) && last_recipe==recipe_clicked)
		{
			recipes_shown = 0;
			hide_window(recipe_win);
		}
		last_recipe = recipe_clicked;
		last_clicked = SDL_GetTicks();
	}
	do_click_sound();
	return 1;
}

static int recipe_controls_click_handler(int mx, int my, Uint32 flags){

	int i;
	int wpx=33*6+2;
	int wpy=manufacture_menu_y_len-37;
	int lpx=18;
	int lpy=33;
	if (!recipes_loaded)
		return 0;

	if (mx>wpx && mx<wpx+lpx && my>wpy+lpy-10 && my<wpy+lpy){
		//arrow
		// est-ce un changement de la recette sélectionnée à la molette ?
		if (recipe_dropdown_scroll_selection(flags)) return 1;

		// sinon considérer un clic normal sur le bouton
		recipes_shown = !recipes_shown;
		if (recipes_shown) show_window(recipe_win);
		else hide_window(recipe_win);
		do_click_sound();
	} else
	if (mx>wpx+3 && mx<wpx+lpx-3 && my>wpy && my<wpy+15){
		//+ button
		// est-ce un changement de la recette sélectionnée à la molette ?
		if (recipe_dropdown_scroll_selection(flags)) return 1;

		if (! (flags&ELW_LEFT_MOUSE)) return 1;

		// si aucune recette n'est sélectionnée on cherche la 1ère ligne vide
		if (cur_recipe < 0)
		{
			for (i=0; i<MAX_RECIPE; i++) if (recipe_status[i]<0) break;
			if (i<MAX_RECIPE) cur_recipe = i;
			else
			{
				// aucune ligne sélectionnée et aucune n'est vide : abandon de la mémorisation
				set_shown_string(c_red2, "Aucun emplacement libre - sélectionner d'abord l'emplacement à écraser (clic droit)");
				return 0;
			}
		}

		// protection contre l'écrasement d'une recette si la liste est fermée : ouverture de la liste d'abord
		if (recipes_shown || recipe_status[cur_recipe]<0)
		{
			// mémorisation de la recette courante dans la liste
			for (i=0; i<6; i++) recipes[cur_recipe][i] = manufacture_list[ITEM_WEAR_START+i];
			clear_recipe_name(cur_recipe);
			if (current_recipe_name != NULL) new_recipe_name(cur_recipe, current_recipe_name);
			set_shown_string(c_green2, "Recette mémorisée dans la liste");
			build_manufacture_list();
		} else set_shown_string(c_yellow2, "Valider l'enregistrement à cet emplacement ou en sélectionner un autre (clic droit)");

		recipes_shown = 1;
		show_window(recipe_win);
		do_click_sound();
		// save recipes to disk to avoid loss on disconnects/crashes
		save_recipes();
	}
	return 0;
}

static int click_manufacture_handler(window_info *win, int mx, int my, Uint32 flags)
{
	int pos;
	Uint8 str[100];

	int quantitytomove=1;

	if ((flags & ELW_CTRL) || (flags & ELW_SHIFT) || (flags & ELW_ALT))
		quantitytomove = 10;

	/* Et si au lieu de faire l'action "oeil" si le curseur principal était sur "oeil"
	 * (alors qu'on ne le voit pas quand on est sur cette fenêtre)
	 * le clic gauche était uniquement considéré pour sélectionner un ingrédient,
	 * laissant uniquement le clic droit pour avoir la description de l'objet,
	 * et sans aller modifier l'action du curseur principal !
	 */

	//see if we clicked on any item in the main category
	pos=get_mouse_pos_in_grid(mx, my, 12, 3, 0, 0, 33, 33);

	/* on traite aussi les quantités réduites à 0 par la recette courante
	   pour permettre l'action de la molette et réaugmenter la quantité dispo */
	if (pos >= 0 && manufacture_list[pos].quantity >= 0)
	{
		/* peu importe l'état du curseur principal (qu'on ne voit même pas !)
		   la description de l'objet est demandé uniquement sur clic droit */
		if (flags&ELW_RIGHT_MOUSE)
		{
			str[0]=LOOK_AT_INVENTORY_ITEM;
			str[1]=manufacture_list[pos].pos;
			my_tcp_send(my_socket,str,2);
			return 1;
		} else	{
			int j;

			last_changed_slot=-1;

			for (j=ITEM_WEAR_START; j<ITEM_WEAR_START+6; j++)
				if (manufacture_list[j].pos==manufacture_list[pos].pos && manufacture_list[j].quantity > 0)
				{
					// transfert bas vers haut (retrait de quantité d'un ingrédient de la recette)
					if (flags & ELW_WHEEL_UP)
					{
						// le retrait dans la recette va au pire jusqu'à 0 (suppression de l'ingrédient)
						if (manufacture_list[j].quantity < quantitytomove) manufacture_list[j].quantity = 0;
						else manufacture_list[j].quantity -= quantitytomove;
						// la restitution dans la liste des ressources est limitée cpar la quantité dans le sac
						manufacture_list[pos].quantity = item_list[manufacture_list[pos].pos].quantity - manufacture_list[j].quantity;
/*
						manufacture_list[pos].quantity = item_list[manufacture_list[pos].pos].quantity - manufacture_list[j].quantity;
						if (manufacture_list[pos].quantity < 0) manufacture_list[pos].quantity = 0;
*/
						if (item_list[manufacture_list[pos].pos].quantity < manufacture_list[j].quantity) manufacture_list[pos].quantity = 0;
						else manufacture_list[pos].quantity = item_list[manufacture_list[pos].pos].quantity - manufacture_list[j].quantity;
					}
					// transfert haut vers bas (ajout de quantité dans un ingrédient de la recette)
					else
					{
						if (cm_manurecipe_addnolimit)
						{
							// on autorise l'ajout en bas même si quantité dispo en haut insuffisante
							manufacture_list[j].quantity += quantitytomove;
							if (manufacture_list[j].quantity > 100) manufacture_list[j].quantity = 100;
							// par contre la quantité en haut ne descendra pas en dessous de 0
							if (manufacture_list[pos].quantity < quantitytomove) manufacture_list[pos].quantity = 0;
							else manufacture_list[pos].quantity -= quantitytomove;
						}
						else
						{
							// on limite aussi la quantité ajoutée en bas avec la quantité dispo en haut
							if (manufacture_list[pos].quantity < quantitytomove) quantitytomove = manufacture_list[pos].quantity;
							manufacture_list[pos].quantity -= quantitytomove;
							manufacture_list[j].quantity += quantitytomove;
						}
					}
//					manufacture_list[j].pos=manufacture_list[pos].pos;  // inutile (cf test)
					manufacture_list[j].image_id=manufacture_list[pos].image_id;
					manufacture_list[j].id=manufacture_list[pos].id;

					cur_valids[j - ITEM_WEAR_START] = (manufacture_list[j].quantity > item_list[manufacture_list[j].pos].quantity) ? 0 : 1;

					change_current_name(NULL);
					do_click_sound();
					return 1;
				}


			if (flags & ELW_WHEEL_UP) return 1; //quantity already 0 in production pipeline
			for (j=ITEM_WEAR_START; j<ITEM_WEAR_START+6; j++)
				if (manufacture_list[j].quantity == 0)
				{
					// on ajoute l'ingrédient à la quantité voulue dans une case libre de la recette courante
					manufacture_list[j].image_id=manufacture_list[pos].image_id;
					manufacture_list[j].pos=manufacture_list[pos].pos;
					manufacture_list[j].quantity = quantitytomove;
					manufacture_list[j].id=manufacture_list[pos].id;
					// on décrémente la quantité de la ressource dispo, limitée à 0
					if (manufacture_list[pos].quantity < quantitytomove) manufacture_list[pos].quantity = 0;
					else manufacture_list[pos].quantity -= quantitytomove;

					cur_valids[j - ITEM_WEAR_START] = (manufacture_list[j].quantity > item_list[manufacture_list[j].pos].quantity) ? 0 : 1;

					change_current_name(NULL);
					do_click_sound();
					return 1;
				}
		}
	} else
	if (pos>=0) last_changed_slot=-1;
	// Si on avait (pos >= 0), le clic était sur une des 36 cases, alors pourquoi continuer ?
	if (pos >= 0) return 0;

	//see if we clicked on any item from the "production pipe"
	pos=get_mouse_pos_in_grid(mx, my, 6, 1, 5, manufacture_menu_y_len-37, 33, 33);

	if (pos >= 0 && manufacture_list[36+pos].quantity > 0)
	{
		/* peu importe l'état du curseur principal (qu'on ne voit même pas !)
		   la description de l'objet est demandé uniquement sur clic droit */
		if (flags & ELW_RIGHT_MOUSE)
		{
			if (manufacture_list[ITEM_WEAR_START+pos].pos < 0)
			{
				set_shown_string(c_yellow2, "Objet non disponible");
				return 1;
			}
			str[0] = LOOK_AT_INVENTORY_ITEM;
			str[1] = manufacture_list[ITEM_WEAR_START+pos].pos;
			my_tcp_send(my_socket,str,2);
			return 1;
		}
		else {
			int j;

			last_changed_slot=pos;
			for (j=0; j<ITEM_WEAR_START; j++)
				if ((manufacture_list[j].quantity>=0) && (manufacture_list[j].pos==manufacture_list[ITEM_WEAR_START+pos].pos))
				{
					// transfert haut vers bas (ajout de quantité dans un ingrédient de la recette)
					if ((flags&ELW_WHEEL_DOWN && !cm_manurecipe_invertwheel) || (flags&ELW_WHEEL_UP && cm_manurecipe_invertwheel))
					{
						// on autorise l'ajout en bas même si quantité dispo en haut insuffisante
						manufacture_list[ITEM_WEAR_START+pos].quantity += quantitytomove;
						if (manufacture_list[ITEM_WEAR_START+pos].quantity > 100) manufacture_list[ITEM_WEAR_START+pos].quantity = 100;
						// par contre la quantité en haut ne descendra pas en dessous de 0
						if (manufacture_list[j].quantity < quantitytomove) manufacture_list[j].quantity = 0;
						else manufacture_list[j].quantity -= quantitytomove;
					}
					// transfert bas vers haut (retrait de quantité d'un ingrédient de la recette)
					else
					{
						// le retrait dans la recette va au pire jusqu'à 0 (suppression de l'ingrédient)
						if (manufacture_list[ITEM_WEAR_START+pos].quantity < quantitytomove) manufacture_list[ITEM_WEAR_START+pos].quantity = 0;
						else manufacture_list[ITEM_WEAR_START+pos].quantity -= quantitytomove;
						// la restitution dans la liste des ressources est limitée par la quantité dans le sac
/*
						manufacture_list[j].quantity = item_list[manufacture_list[j].pos].quantity - manufacture_list[ITEM_WEAR_START+pos].quantity;
						if (manufacture_list[j].quantity < 0) manufacture_list[j].quantity = 0;
*/
						if (item_list[manufacture_list[j].pos].quantity < manufacture_list[ITEM_WEAR_START+pos].quantity) manufacture_list[j].quantity = 0;
						else manufacture_list[j].quantity = item_list[manufacture_list[j].pos].quantity - manufacture_list[ITEM_WEAR_START+pos].quantity;
					}
//					manufacture_list[j].pos=manufacture_list[36+pos].pos;  // inutile (cf test)
//					manufacture_list[ITEM_WEAR_START+pos].image_id=manufacture_list[36+pos].image_id;  // bof, pourquoi dans ce sens et pas l'autre ?

					if (manufacture_list[ITEM_WEAR_START+pos].pos < 0) cur_valids[pos] = 0;
					else cur_valids[pos] = (manufacture_list[ITEM_WEAR_START+pos].quantity > item_list[manufacture_list[ITEM_WEAR_START+pos].pos].quantity) ? 0 : 1;
					change_current_name(NULL);
					do_click_sound();
					return 1;
				}


			/* En permettant l'affichage d'une recette courante non réalisable (ressource non dispo dans le sac),
			 * il est possible de cliquer sur un ingrédient qui n'a alors pas été traité précédement.
			 * Il n'y a pas de restitution de quantité à effectuer, on se contente de mettre à jour la quantité dans la recette.
			 */
			if ((flags&ELW_WHEEL_UP && !cm_manurecipe_invertwheel) || (flags&ELW_WHEEL_DOWN && cm_manurecipe_invertwheel))
			{
				if (manufacture_list[ITEM_WEAR_START+pos].quantity < quantitytomove) manufacture_list[ITEM_WEAR_START+pos].quantity = 0;
				else manufacture_list[ITEM_WEAR_START+pos].quantity -= quantitytomove;
			}
			else
			{
				// pour rester cohérent avec les ingreds dispo, le clic décrémente
				manufacture_list[ITEM_WEAR_START+pos].quantity -= quantitytomove;
				if (manufacture_list[ITEM_WEAR_START+pos].quantity < 0) manufacture_list[ITEM_WEAR_START+pos].quantity = 0;
			}
			cur_valids[pos] = 0;
			change_current_name(NULL);
					do_click_sound();
			return 1;
		}
	} else
	if (pos>=0) {
		//click on an empty slot
		//handle the mouse wheel
		if (recipes_loaded && (pos!=last_changed_slot))
		{
			if ((flags&ELW_WHEEL_UP)||(flags&ELW_WHEEL_DOWN)) {
			//simulate a click on the dropdown
				last_changed_slot=-1;
			recipe_dropdown_scroll_selection(flags);
			do_click_sound();
			return 0;
			}
		}
	} else last_changed_slot=-1;
	// Si on avait (pos >= 0), le clic était sur une des 6 cases, alors pourquoi continuer ?
	if (pos >= 0) return 0;
	//see if we clicked on the recipe handler
	recipe_controls_click_handler(mx,my,flags);

	// clear the message area if double-clicked
	if ((my > manufacture_menu_y_len-85) && my < (manufacture_menu_y_len-37)) {
		static Uint32 last_click = 0;
		if (safe_button_click(&last_click)) {
			set_shown_string(0,"");
			return 1;
		}
	}
	return 0;
}



int mix_handler(Uint8 quantity, const char* empty_error_str)
{
	Uint8 str[32];
	int items_no=0;
	int i;
	// franchement ce test est suffisant... les quantités sont testées ensuite de toute façon
	if (manufacture_win < 0)
	{
		set_shown_string(c_red2, empty_error_str);
		return 1;
	}

	str[0]=MANUFACTURE_THIS;
	for (i=ITEM_WEAR_START; i<ITEM_WEAR_START+6; i++) if (manufacture_list[i].quantity > 0)
	{
		if ((cur_valids[i-ITEM_WEAR_START]) && (manufacture_list[i].pos >= 0))
		{
			str[items_no*3+2] = manufacture_list[i].pos;
			*((Uint16 *)(str+items_no*3+2+1))=SDL_SwapLE16(manufacture_list[i].quantity);
			items_no++;
		}
		else
		{
			set_shown_string(c_red2, "Il manque des ingrédients pour pouvoir lancer la fabrication !");
			return 1;
		}
	}

	str[1]=items_no;
	if(items_no){
		//don't send an empty string
		current_recipe_changed = 0;
		save_last_mix();
		str[items_no*3+2]= quantity;
		my_tcp_send(my_socket,str,items_no*3+3);
	}
	else set_shown_string(c_red2, empty_error_str);

	return 1;
}

static int clear_handler()
{
	int i;
	static Uint32 last_click = 0;

	if (safe_button_click(&last_click))
	{
		for (i=ITEM_WEAR_START; i<ITEM_WEAR_START+6; i++) manufacture_list[i].quantity = 0;
		change_current_name(NULL);
		build_manufacture_list();
		set_shown_string(c_yellow2, "Ingrédients retirés");
	}
	else set_shown_string(c_orange2, dc_warning_str);
	last_changed_slot = -1;
	return 1;
}

/* ">" button clicked - manufacture just one item */
static int mixone_handler()
{
  return mix_handler(1, mix_empty_str);
}

/* ">>" button clicked - manufacture the maxiumum nuber of items */
int mixall_handler()
{
  return mix_handler(255, mix_empty_str);
}


/* show help for recipe window */
int recipe_dropdown_mouseover_handler(window_info *win, int mx, int my)
{
	if (mx <= 6*33) mouse_over_recipe = my/(33+1);
	return 0;
}


//MOUSEOVER HANDLERS
static int recipe_controls_mouseover_handler(int mx, int my){
	int wpx=33*6+2;
	int wpy=manufacture_menu_y_len-37;
	int lpx=18;
	int lpy=33;

	if (!show_help_text)
		return 0;

	if (mx>wpx && mx<wpx+lpx && my>wpy+lpy-10 && my<wpy+lpy){
		//on arrow
		show_help(recipe_show_hide_str, manufacture_menu_x_len-strlen(recipe_show_hide_str)*SMALL_FONT_X_LEN, manufacture_menu_y_len+10);
	} else
	if (mx>wpx+3 && mx<wpx+lpx-3 && my>wpy && my<wpy+15){
		//on + button
		show_help(recipe_save_str, manufacture_menu_x_len-strlen(recipe_save_str)*SMALL_FONT_X_LEN, manufacture_menu_y_len+10);
	}
	return 0;
}

/* mouse over slots - show tool tips */
static int mouseover_manufacture_slot_handler(window_info *win, int mx, int my)
{
	int pos;
	int check_for_eye = 0;
	const char *descp_str = NULL;
	/* Do nothing when mouse over title bar */
	if (my<0)
		return 0;

	/* See if we're over a message - and offer clear help if so */
	if (show_help_text && *inventory_item_string && (my > manufacture_menu_y_len-85) && my < (manufacture_menu_y_len-37)) {
		show_help((disable_double_click)?click_clear_str :double_click_clear_str, 0, win->len_y+10);
	}

	/* see if we're over an item in the main category */
	pos=get_mouse_pos_in_grid(mx, my, 12, 3, 0, 0, 33, 33);
	if (pos >= 0 && manufacture_list[pos].quantity > 0){
		if (show_help_text)
		show_help(manu_add_str, 0, manufacture_menu_y_len+10);
		if (show_item_desc_text && item_info_available() && (get_item_count(manufacture_list[pos].id, manufacture_list[pos].image_id) == 1))
			descp_str = get_item_description(manufacture_list[pos].id, manufacture_list[pos].image_id);
		check_for_eye = 1;
	}

	/* see if we're over an item from the "production pipe" */
	pos=get_mouse_pos_in_grid(mx, my, 6, 1, 5, manufacture_menu_y_len-37, 33, 33);
	if (pos >= 0 && manufacture_list[36+pos].quantity > 0){
		if (show_help_text)
		show_help(manu_remove_str, 0, manufacture_menu_y_len+10);
		if (show_item_desc_text && item_info_available() && (get_item_count(manufacture_list[pos].id, manufacture_list[pos].image_id) == 1))
			descp_str = get_item_description(manufacture_list[pos].id, manufacture_list[pos].image_id);
		check_for_eye = 1;
	}

	/*check recipe controls*/
	/* if set, show the description last */
	if (descp_str != NULL)
		show_help(descp_str, 0, manufacture_menu_y_len+30);
	if (show_help_text)
	recipe_controls_mouseover_handler(mx,my);

	/* if we're over an occupied slot and the eye cursor function is active, show the eye cursor */
	if (check_for_eye){
	if (action_mode == ACTION_LOOK){
		elwin_mouse = CURSOR_EYE;
		return 1;
	}
	}

	return 0;
}

/* mouse over ">" button - show tool tip */
static int mouseover_mixone_handler(widget_list *widget, int mx, int my)
{
	if (show_help_text)
		show_help(mix_str, 33*6+25, manufacture_menu_y_len+10);
	return 0;
}

/* mouse over ">>" button - show tool tip */
static int mouseover_mixall_handler(widget_list *widget, int mx, int my)
{
	if (show_help_text)
		show_help(mixall_str, manufacture_menu_x_len - strlen(mixall_str)*SMALL_FONT_X_LEN, manufacture_menu_y_len+10);
	return 0;
}


int mouseover_clear_handler(window_info *win, int mx, int my)
{
	if (show_help_text)
	{
		if (disable_double_click)
			show_help(mixclear_help_str, manufacture_menu_x_len - strlen(mixclear_help_str)*SMALL_FONT_X_LEN, manufacture_menu_y_len+10);
		else
			show_help(dc_mixclear_help_str, manufacture_menu_x_len - strlen(dc_mixclear_help_str)*SMALL_FONT_X_LEN, manufacture_menu_y_len+10);
	}
	return 0;
}

static int context_listrecipe_handler(window_info *win, int widget_id, int mx, int my, int option)
{
	int i, p;
	item tmp_ingr;
	char *tmp_name;
	switch (option)
	{
		case 0: // affectation de la recette sélectionnée vers la recette courante
			if (cur_recipe < 0) return 1;
			for (i=0; i<6; i++) manufacture_list[ITEM_WEAR_START+i] = recipes[cur_recipe][i];
			change_current_name(recipe_name[cur_recipe]);
			set_shown_string(c_green2, "Recette chargée depuis la liste");
		break;

		case 2: // enregistrement de la recette courante à l'emplacement sélectionné
			if (cur_recipe < 0) return 1;
			for (i=0; i<6; i++) recipes[cur_recipe][i] = manufacture_list[ITEM_WEAR_START+i];
			clear_recipe_name(cur_recipe);
			if (current_recipe_name != NULL) new_recipe_name(cur_recipe, current_recipe_name);
			set_shown_string(c_green2, "Recette mémorisée dans la liste");
		break;

		case 3: // suppression de la recette mémorisée à l'emplacement sélectionné
			if (cur_recipe < 0) return 1;
			for (i=0; i<6; i++) recipes[cur_recipe][i].quantity = 0;
			clear_recipe_name(cur_recipe);
			set_shown_string(c_green2, "Recette supprimée dans la liste");
		break;

		case 4: // désélection de l'emplacement sélectionné
			cur_recipe = -1;
		break;

		case 6: // monter d'un cran la recette mémorisée dans la liste
			if (cur_recipe < 0) return 1;
			p = (cur_recipe-1+MAX_RECIPE)%MAX_RECIPE;
			for (i=0; i<6; i++)
			{
				tmp_ingr = recipes[p][i];
				recipes[p][i] = recipes[cur_recipe][i];
				recipes[cur_recipe][i] = tmp_ingr;
			}
			tmp_name = recipe_name[p];
			recipe_name[p] = recipe_name[cur_recipe];
			recipe_name[cur_recipe] = tmp_name;
			cur_recipe = p;
		break;

		case 7: // descendre d'un cran la recette mémorisée dans la liste
			if (cur_recipe < 0) return 1;
			p = (cur_recipe+1)%MAX_RECIPE;
			for (i=0; i<6; i++)
			{
				tmp_ingr = recipes[p][i];
				recipes[p][i] = recipes[cur_recipe][i];
				recipes[cur_recipe][i] = tmp_ingr;
			}
			tmp_name = recipe_name[p];
			recipe_name[p] = recipe_name[cur_recipe];
			recipe_name[cur_recipe] = tmp_name;
			cur_recipe = p;
		break;
	}

	build_manufacture_list();
	do_click_sound();
	// save recipes to disk to avoid loss on disconnects/crashes
	save_recipes();
	return 1;
}

static int recipe_dropdown_resize_handler(window_info *win)
{
	// la fenêtre reste fixe en largeur (6 cases + scrollbar)
	win->len_x = 33*6 + 22;

	// la fenêtre ne sera pas plus haute que le nombre de lignes max
	if (win->len_y > MAX_RECIPE*33) win->len_y = MAX_RECIPE*33;

	// redimensionnement et positionnement des barres de défilement
	set_window_scroll_len(win->window_id, MAX_RECIPE*33 - win->len_y);

	return 0;
}


void display_manufacture_menu()
{
	if(manufacture_win < 0){
		int clear_button_id=100;
		int mixone_button_id=101;
		int mixall_button_id=102;

		int our_root_win = -1;

		if (!windows_on_top) {
			our_root_win = game_root_win;
		}
		manufacture_win= create_window(win_manufacture, our_root_win, 0, manufacture_menu_x, manufacture_menu_y, manufacture_menu_x_len, manufacture_menu_y_len, ELW_WIN_DEFAULT);
		set_window_handler(manufacture_win, ELW_HANDLER_DISPLAY, &display_manufacture_handler );
		set_window_handler(manufacture_win, ELW_HANDLER_CLICK, &click_manufacture_handler );
		set_window_handler(manufacture_win, ELW_HANDLER_MOUSEOVER, &mouseover_manufacture_slot_handler );
		mixall_button_id=button_add_extended(manufacture_win, mixall_button_id,
			NULL, manufacture_menu_x_len - ELW_BOX_SIZE - 50, manufacture_menu_y_len-36, 50, 0, 0, 1.0f, 0.77f, 0.57f, 0.39f, ">>");
		widget_set_OnClick(manufacture_win, mixall_button_id, mixall_handler);
		widget_set_OnMouseover(manufacture_win, mixall_button_id, mouseover_mixall_handler);

		mixone_button_id=button_add_extended(manufacture_win, mixone_button_id,
			NULL, manufacture_menu_x_len - ELW_BOX_SIZE -105, manufacture_menu_y_len-36, 50, 0, 0, 1.0f, 0.77f, 0.57f, 0.39f, ">");
		widget_set_OnClick(manufacture_win, mixone_button_id, mixone_handler);
		widget_set_OnMouseover(manufacture_win, mixone_button_id, mouseover_mixone_handler);

		clear_button_id=button_add_extended(manufacture_win, clear_button_id,
			NULL, manufacture_menu_x_len - ELW_BOX_SIZE -170, manufacture_menu_y_len-36, 55, 0, 0, 1.0f, 0.77f, 0.57f, 0.39f, clear_str);
		widget_set_OnClick(manufacture_win, clear_button_id, clear_handler);
		widget_set_OnMouseover(manufacture_win, clear_button_id, mouseover_clear_handler);

		cm_add(windows_list.window[manufacture_win].cm_id, cm_manufacture_menu_str, NULL);
		cm_bool_line(windows_list.window[manufacture_win].cm_id, ELW_CM_MENU_LEN+1, &cm_manurecipe_invertwheel, NULL);
		cm_bool_line(windows_list.window[manufacture_win].cm_id, ELW_CM_MENU_LEN+2, &cm_manurecipe_addnolimit, NULL);
		cm_bool_line(windows_list.window[manufacture_win].cm_id, ELW_CM_MENU_LEN+3, &cm_manurecipe_wheelaffect, NULL);
		cm_bool_line(windows_list.window[manufacture_win].cm_id, ELW_CM_MENU_LEN+4, &cm_listrecipe_enabled, NULL);

		//Create a child window to show recipes in a dropdown panel
		recipe_win= create_window("w_recipe", manufacture_win, 0, 2, manufacture_menu_y_len-2, 33*6+22, 33*6,
			ELW_SCROLLABLE|ELW_RESIZEABLE|ELW_TITLE_NONE|ELW_SHOW|ELW_USE_BACKGROUND|ELW_ALPHA_BORDER|ELW_SWITCHABLE_OPAQUE|ELW_USE_BORDER);
		set_window_handler(recipe_win, ELW_HANDLER_DISPLAY, &recipe_dropdown_draw);
		set_window_handler(recipe_win, ELW_HANDLER_CLICK, &recipe_dropdown_click_handler);
		set_window_handler(recipe_win, ELW_HANDLER_MOUSEOVER, &recipe_dropdown_mouseover_handler);
		set_window_handler(recipe_win, ELW_HANDLER_RESIZE, &recipe_dropdown_resize_handler);
		set_window_min_size(recipe_win, 33*6 + 22, 33*2);        // mini 6 cases + bar et 2 lignes
		set_window_scroll_yoffset(recipe_win, 0);                // scrollbar partant du haut
		set_window_scroll_inc(recipe_win, 33);                   // scroll ligne par ligne
		set_window_scroll_len(recipe_win, 33*MAX_RECIPE); // hauteur totale des lignes

		cm_listrecipe_id = cm_create(cm_listrecipe_menu_str, context_listrecipe_handler);
		cm_bool_line(cm_listrecipe_id, 9, &cm_listrecipe_enabled, NULL);
		hide_window(recipe_win); //start hidden
	} else {
		show_window(manufacture_win);
		if (!recipes_shown) hide_window(recipe_win);
		else show_window(recipe_win);
		select_window(manufacture_win);
	}
}
