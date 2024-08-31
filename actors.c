#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <SDL.h>
#include "actors.h"
#include "actor_scripts.h"
#include "asc.h"
#include "bbox_tree.h"
#include "buffs.h"
#include "cal.h"
#include "cursors.h"
#include "draw_scene.h"
#include "errors.h"
#include "gl_init.h"
#include "global.h"
#include "interface.h"
#include "load_gl_extensions.h"
#include "map.h"
#include "new_actors.h"
#include "platform.h"
#include "shadows.h"
#include "textures.h"
#include "translate.h"
#include "vmath.h"
#include "cluster.h"
#include "eye_candy_wrapper.h"
#include "minimap.h"
#include "actor_init.h"
#include "fsaa/fsaa.h"
#include "themes.h"

typedef struct
{
	char *texte;
	float pcmin;  // limite inf barre de vie
	float pcmax;  // limite sup barre de vie
	float pcmoy;  // moyenne pour la couleur
} translation_niveau_vie;

const translation_niveau_vie niveau_vie[] =
{
	{"Mort",        0.00f, 0.00f, 0.00f},
	{"Mourant",     0.01f, 0.04f, 0.03f},
	{"Souffrant",   0.05f, 0.24f, 0.15f},
	{"Entaillé",    0.25f, 0.49f, 0.38f},
	{"Contusionné", 0.50f, 0.74f, 0.63f},
	{"Pantelant",   0.75f, 0.94f, 0.85f},
	{"Indemne",     1.00f, 1.00f, 1.00f}  // normalement 95,100
};

#define DRAW_ORTHO_INGAME_NORMAL(x, y, z, our_string, max_lines)	draw_ortho_ingame_string(x, y, z, (const Uint8*)our_string, max_lines, INGAME_FONT_X_LEN*10.0, INGAME_FONT_Y_LEN*10.0)
#define DRAW_INGAME_NORMAL(x, y, our_string, max_lines)	draw_ingame_string(x, y, (const Uint8*)our_string, max_lines, INGAME_FONT_X_LEN, INGAME_FONT_Y_LEN)
#define DRAW_INGAME_SMALL(x, y, our_string, max_lines)	draw_ingame_string(x, y, (const Uint8*)our_string, max_lines, SMALL_INGAME_FONT_X_LEN, SMALL_INGAME_FONT_Y_LEN)
#define DRAW_INGAME_ALT(x, y, our_string, max_lines)	draw_ingame_string(x, y, (const Uint8*)our_string, max_lines, ALT_INGAME_FONT_X_LEN, ALT_INGAME_FONT_Y_LEN)

actor *actors_list[MAX_ACTORS];
int max_actors=0;
SDL_mutex *actors_lists_mutex = NULL;	//used for locking between the timer and main threads
actor *your_actor = NULL;

actor_types actors_defs[MAX_ACTOR_DEFS];

attached_actors_types attached_actors_defs[MAX_ACTOR_DEFS];

void draw_actor_overtext( actor* actor_ptr ); /* forward declaration */

int no_near_actors=0;
int no_near_enhanced_actors = 0;
float distanceSq_to_near_enhanced_actors;
near_actor near_actors[MAX_ACTORS];


int cm_mouse_over_banner = 0;		/* use to trigger banner context menu */

int etat_sante(float percentage)
{
	int i;
	for (i=5; i>=0; i--)
	{
		if (percentage > niveau_vie[i].pcmax) return i+1;
	}
	return 0;
}

//Threading support for actors_lists
void init_actors_lists()
{
	int	i;

	actors_lists_mutex=SDL_CreateMutex();
	LOCK_ACTORS_LISTS();	//lock it to avoid timing issues
	for (i=0; i < MAX_ACTORS; i++)
		actors_list[i] = NULL;
	UNLOCK_ACTORS_LISTS();	// release now that we are done
}

//return the ID (number in the actors_list[]) of the new allocated actor
int add_actor (int actor_type, char * skin_name, float x_pos, float y_pos, float z_pos, float z_rot, float scale, char remappable, short skin_color, short hair_color, short shirt_color, short pants_color, short boots_color, int actor_id)
{
	int texture_id;
	int i;
	int k;
	actor *our_actor;
	int x, y;


	if (actors_defs[actor_type].ghost)
	{
		texture_id = load_texture_cached(skin_name, tt_mesh);
	}
	else
	{
		if (!remappable)
		{
			texture_id = load_texture_cached(skin_name, tt_mesh);
		}
		else
		{
			LOG_ERROR("remapped skin for %s", skin_name);
			exit(-1);
		}
	}

	our_actor = calloc(1, sizeof(actor));
	memset(our_actor->current_displayed_text, 0, 160);
	our_actor->current_displayed_text_time_left =  0;

	our_actor->is_enhanced_model=0;
	our_actor->remapped_colors=remappable;
	our_actor->actor_id=actor_id;
	our_actor->cur_anim_sound_cookie = 0;


	our_actor->x_pos=x_pos;
	our_actor->y_pos=y_pos;
	our_actor->z_pos=z_pos;
	our_actor->scale=scale;

	our_actor->x_speed=0;
	our_actor->y_speed=0;
	our_actor->z_speed=0;

	our_actor->x_rot=0;
	our_actor->y_rot=0;
	our_actor->z_rot=z_rot;

	our_actor->last_range_attacker_id = -1;

	//reset the script related things
	our_actor->move_x_speed=0;
	our_actor->move_y_speed=0;
	our_actor->move_z_speed=0;
	our_actor->rotate_x_speed=0;
	our_actor->rotate_y_speed=0;
	our_actor->rotate_z_speed=0;
	our_actor->movement_time_left=0;
	our_actor->moving=0;
	our_actor->rotating=0;
	our_actor->busy=0;
	our_actor->last_command=nothing;

    /* load the texture in case it's not already loaded and look if it has
     * an alpha map */
	our_actor->has_alpha = get_texture_alpha(texture_id);

	//clear the que
	for(k=0;k<MAX_CMD_QUEUE;k++)	our_actor->que[k]=nothing;

	our_actor->texture_id=texture_id;
	our_actor->skin=skin_color;
	our_actor->hair=hair_color;
	our_actor->pants=pants_color;
	our_actor->boots=boots_color;
	our_actor->shirt=shirt_color;
	our_actor->stand_idle=0;
	our_actor->sit_idle=0;

	our_actor->attached_actor = -1;
	our_actor->attachment_shift[0] = our_actor->attachment_shift[1] = our_actor->attachment_shift[2] = 0.0;

	for (i = 0; i < NUM_BUFFS; i++)
	{
		our_actor->ec_buff_reference[i] = NULL;
	}

	x = (int) (our_actor->x_pos / 0.5f);
	y = (int) (our_actor->y_pos / 0.5f);
	our_actor->cluster = get_cluster (x, y);

	//find a free spot, in the actors_list
	LOCK_ACTORS_LISTS();

	for(i=0;i<max_actors;i++)
		{
			if(!actors_list[i])break;
		}

	if(actor_id == yourself)
		set_our_actor (our_actor);

	actors_list[i]=our_actor;
	if(i>=max_actors)max_actors=i+1;

	//It's unlocked later

	ec_add_actor_obstruction(our_actor, 3.0);
	return i;
}

void add_actor_attachment(int actor_id, int attachment_type)
{
	int i;
	actor *parent = NULL;

	for (i = 0; i < max_actors; ++i)
		if (actors_list[i]->actor_id == actor_id)
		{
			parent = actors_list[i];
			break;
		}

	if (!parent)
		LOG_ERROR("unable to add an attached actor: actor with id %d doesn't exist!", actor_id);
	else if(attachment_type < 0 || attachment_type >= MAX_ACTOR_DEFS || (attachment_type > 0 && actors_defs[attachment_type].actor_type != attachment_type) )
		LOG_ERROR("unable to add an attached actor: illegal/missing actor definition %d", attachment_type);
	else
	{
		int id = add_actor(attachment_type, actors_defs[attachment_type].skin_name,
						   parent->x_pos, parent->y_pos, parent->z_pos, parent->z_rot, get_actor_scale(parent),
						   0, 0, 0, 0, 0, 0, -1);
		actors_list[id]->attached_actor = i;
		parent->attached_actor = id;

		actors_list[id]->async_fighting = 0;
		actors_list[id]->async_x_tile_pos = parent->async_x_tile_pos;
		actors_list[id]->async_y_tile_pos = parent->async_y_tile_pos;
		actors_list[id]->async_z_rot = parent->async_z_rot;

		actors_list[id]->x_tile_pos=parent->x_tile_pos;
		actors_list[id]->y_tile_pos=parent->y_tile_pos;
		actors_list[id]->buffs=parent->buffs & BUFF_DOUBLE_SPEED; // the attachment can only have this buff
		actors_list[id]->actor_type=attachment_type;
		actors_list[id]->damage=0;
		actors_list[id]->damage_ms=0;
		actors_list[id]->sitting=0;
		actors_list[id]->fighting=0;
		//test only
		actors_list[id]->max_health=0;
		actors_list[id]->cur_health=0;
		actors_list[id]->ghost=actors_defs[attachment_type].ghost;
		actors_list[id]->dead=0;
		actors_list[id]->stop_animation=1;//helps when the actor is dead...
		actors_list[id]->kind_of_actor=0;


		actors_list[id]->z_pos = get_actor_z(actors_list[id]);

		//printf("attached actor n°%d of type %d to actor n°%d with id %d\n", id, attachment_type, i, actor_id);

		if (actors_defs[attachment_type].coremodel!=NULL) {
			//Setup cal3d model
			actors_list[id]->calmodel = model_new(actors_defs[attachment_type].coremodel);
			//Attach meshes
			if(actors_list[id]->calmodel) {
				model_attach_mesh(actors_list[id], actors_defs[attachment_type].shirt[0].mesh_index);
				set_on_idle(id);

				build_actor_bounding_box(actors_list[id]);
			}
		}
		else
			actors_list[id]->calmodel=NULL;

		UNLOCK_ACTORS_LISTS();
	}
}

void remove_actor_attachment(int actor_id)
{
	int i;

	LOCK_ACTORS_LISTS();

	for (i = 0; i < max_actors; ++i)
		if (actors_list[i]->actor_id == actor_id)
		{
			int att = actors_list[i]->attached_actor;
			actors_list[i]->attached_actor = -1;
			actors_list[i]->attachment_shift[0] = 0.0;
			actors_list[i]->attachment_shift[1] = 0.0;
			actors_list[i]->attachment_shift[2] = 0.0;
			free_actor_data(att);
			free(actors_list[att]);
			actors_list[att]=NULL;
			if(att==max_actors-1)max_actors--;
			else {
				//copy the last one down and fill in the hole
				max_actors--;
				actors_list[att]=actors_list[max_actors];
				actors_list[max_actors]=NULL;
				if (actors_list[att] && actors_list[att]->attached_actor >= 0)
					actors_list[actors_list[att]->attached_actor]->attached_actor = att;
			}
			break;
		}

	UNLOCK_ACTORS_LISTS();
}

void set_health_color(float percent, float multiplier, float a)
{
	float r,g;

	r = (1.0f-percent)*2.5f;
	g = (percent/3.0f)*5.0f;

	if(r<0.0f)r=0.0f;
	else if(r>1.0f)r=1.0f;
	if(g<0.0f)g=0.0f;
	else if(g>1.0f)g=1.0f;

	glColor4f(r*multiplier,g*multiplier,0.0f, a);
}


static int get_under_mouse_type(const actor *a) {
       int k, e, h, p;
       k = a->kind_of_actor;
       if (k == NPC)
          return UNDER_MOUSE_NPC;
       e = a->is_enhanced_model;
       h = k == HUMAN || k == COMPUTER_CONTROLLED_HUMAN;
       p = k == PKABLE_HUMAN || k == PKABLE_COMPUTER_CONTROLLED;
       if (h || (e && p))
          return UNDER_MOUSE_PLAYER;
       return UNDER_MOUSE_ANIMAL;
}

void draw_actor_banner(actor * actor_id, float offset_z)
{
	GLdouble hx, hy, hz;
	GLdouble model[16], proj[16];
	GLint view[4];
	float banner_width = 0.0f;
	float banner_height = 0.0f;
	float font_scale = 1.0f / ALT_INGAME_FONT_X_LEN;

	//if first person, dont draw banner
	actor *me = get_our_actor();
	if (first_person && me && me->actor_id==actor_id->actor_id) return;

	//Figure out where the point just above the actor's head is in the viewport
	//See if Projection and viewport can be saved elsewhere to prevent doing this so often
	//MODELVIEW is hopeless
	glGetDoublev(GL_MODELVIEW_MATRIX, model);
	glGetDoublev(GL_PROJECTION_MATRIX, proj);
	glGetIntegerv(GL_VIEWPORT, view);
	// Input adjusted healthbar_y value to scale hy according to actor scale
	gluProject(0.0f, 0.0f, (offset_z + 0.1f) * actor_id->scale * actors_defs[actor_id->actor_type].actor_scale + 0.02, model, proj, view, &hx, &hy, &hz);
	//Save World-view and Projection matrices to allow precise raster placement of quads
	glPushMatrix();
	glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	//Don't forget that the viewport is expressed in X+W,Y+H, or point-displacement,
	//versus the Ortho projection which expects x1,x2,y1,y2, or absolute coordinates
	glOrtho(view[0], view[2]+view[0], view[1], view[3]+view[1], 0.0f, -1.0f);

	//------------------------------------------------------------------- DAMAGE

	glDepthFunc(GL_ALWAYS);

	if (actor_id->damage_ms) {
		unsigned char str[60];
		if (floatingmessages_enabled) {
			float font_scale2 = font_scale * powf(1.0f+((float)abs(actor_id->damage)/2.0f)/1000.0f, 4.0);
			float a = (float)(cur_time - actor_id->last_health_loss);
			float aa = a*a / 4000000.0f;
			GLdouble a_bounce = 0;

			if (actor_id->damage > 0) {
				sprintf((char*)str, "%i", actor_id->damage);
				glColor4f(1.0f, 0.1f, 0.2f, 1.0-aa);
			} else {
				sprintf((char*)str, "%i", -actor_id->damage);
				glColor4f(0.3f, 1.0f, 0.3f, 1.0-aa);
			}

			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE);

			//Make damage numbers bounce on the actor's head. Owie!
			if (a < 500) {
				a_bounce = 50.0 - 0.0002 * powf(a,2);
			} else if (a <  950.0) {
				a_bounce = 0.0900*(a- 500.0) - 0.0002 * powf((a- 500.0), 2);
			} else if (a < 1355.0) {
				a_bounce = 0.0810*(a- 950.0) - 0.0002 * powf((a- 950.0), 2);
			} else if (a < 1720.0) {
				a_bounce = 0.0730*(a-1355.0) - 0.0002 * powf((a-1355.0), 2);
			} else {
				a_bounce = 0.0640*(a-1720.0) - 0.0002 * powf((a-1720.0), 2);
			}
            if((view_mp || view_mana_bar) && !actor_id->dead && actor_id->kind_of_actor!=NPC && me->actor_id==actor_id->actor_id){
                draw_ortho_ingame_string(hx-(((float)get_string_width(str) * (font_scale2*0.17*name_zoom)))*0.5f, a_bounce+hy+14+10.0f, 0, str, 1, font_scale2*.14, font_scale2*.21);
            }else{
                draw_ortho_ingame_string(hx-(((float)get_string_width(str) * (font_scale2*0.17*name_zoom)))*0.5f, a_bounce+hy+10.0f, 0, str, 1, font_scale2*.14, font_scale2*.21);
            }
			glDisable(GL_BLEND);
		}
		else //No floating messages
		{
			sprintf((char*)str, "%i", actor_id->damage);
			glColor3f(1.0f, 0.3f, 0.3f);

			DRAW_ORTHO_INGAME_NORMAL(-0.1f, (offset_z + 0.1f)/2.0f, 0, str, 1.0f);
		}
	}

	glDepthFunc(GL_LESS);



	set_font(0); // back to fixed pitch

	if ((view_mp || view_mana_bar) && me &&!actor_id->dead && me->actor_id==actor_id->actor_id)
	{
        unsigned char mana_point[200];
        double largeur_mana=0;
        double largeur_mana_bar=0;
        double largeur_total_mana=0;
        double hauteur_mana = ALT_INGAME_FONT_Y_LEN * 12.0 * name_zoom * font_scale;
        float percentage_mana=0;

		// calcul la largeur de la barre equivalente a "000/000" (necessaire pour positionner l'etat)
		if (view_mana_bar)
		{
            sprintf((char*)mana_point, "%03u/%03u", 0, 0);
			largeur_mana_bar = (float)get_string_width(mana_point) * ALT_INGAME_FONT_X_LEN * name_zoom * font_scale;
			largeur_total_mana = largeur_mana_bar;
			if((float)your_info.ethereal_points.base != 0)
                percentage_mana = (float)your_info.ethereal_points.cur / (float)your_info.ethereal_points.base;

		}
        //affichage des points de mana
        if (view_mp)
        {
            if(me && me->actor_id==actor_id->actor_id){
                sprintf((char*)mana_point,"%u/%u", your_info.ethereal_points.cur, your_info.ethereal_points.base);
                // calcul la largeur des pdv/etat de sante puis la largeur totale (barre incluse)
                largeur_mana = (float)get_string_width(mana_point) * ALT_INGAME_FONT_X_LEN * name_zoom * font_scale;
                largeur_total_mana+= (view_mana_bar) ? 5.0f + largeur_mana : largeur_mana;

                glColor4f(0.5f,0.5f,1.0f,1.0f);
                draw_ortho_ingame_string(hx+(largeur_total_mana/2.0f)-largeur_mana, hy, hz, mana_point, 1, ALT_INGAME_FONT_X_LEN*font_scale, ALT_INGAME_FONT_Y_LEN*font_scale);
            }
        }

        if (view_mana_bar)
        {
            if(me && me->actor_id==actor_id->actor_id)
            {
                double manabar_conv = 0;
                double hauteur_barre_mana = hauteur_mana*0.7f; // la barre ne remplit pas toute la hauteur
                double starty = hy + hauteur_mana*0.2f;   // du coup on ne la place pas au ras du sol
                double startx = hx - largeur_total_mana*0.5f;

                if (percentage_mana > 110.0f){
                    percentage_mana = 110.0f;
                }

                glDisable(GL_TEXTURE_2D);
                //Mana bar
                manabar_conv = largeur_mana_bar * percentage_mana;
                glBegin(GL_QUADS);
                    glColor4f(0.1f,0.1f,1.0f,1.0f);
                    glVertex3d(startx,                starty,               hz);
                    glVertex3d(startx+manabar_conv,   starty,               hz);
                    glColor4f(0.4f,0.4f,1.0f,1.0f);
                    glVertex3d(startx+manabar_conv,   starty+hauteur_barre_mana, hz);
                    glVertex3d(startx,                starty+hauteur_barre_mana, hz);
                glEnd();

                //Mana frame
                glDepthFunc(GL_LEQUAL);
                glBegin(GL_LINE_LOOP);
                    glColor3f(0.0f, 0.0f, 0.0f);
                    glVertex3f(startx,                  starty,               hz);
                    glVertex3f(startx+largeur_mana_bar, starty,               hz);
                    glVertex3f(startx+largeur_mana_bar, starty+hauteur_barre_mana, hz);
                    glVertex3f(startx,                  starty+hauteur_barre_mana, hz);
                glEnd();

                glEnable(GL_TEXTURE_2D);
            }
        }
        if (largeur_total_mana > banner_width) banner_width = largeur_total_mana;

        hy+=hauteur_mana;
	}

    // ------------------------------------------------------------------- SANTE

	if ((view_hp || view_health_bar) && !actor_id->dead && actor_id->kind_of_actor!=NPC)
	{
		unsigned char hp[200];
		double largeur_sante=0, largeur_barre=0, largeur_total=0;
		double hauteur_sante = ALT_INGAME_FONT_Y_LEN * 12.0 * name_zoom * font_scale;
		float percentage;

		// calcul du pourcentage de vie (utile pour couleur et si besoin determination de l'état)
		if (actor_id->max_health) {
			percentage = (float)actor_id->cur_health / (float)actor_id->max_health;
		} else {
			// précaution si jamais le serveur envoie des infos de santé farfelues :p
			if (actor_id->cur_health > 6)
			{
				unsigned char errstr[200];
				sprintf((char*)errstr, "Etat de santé reçu par le serveur (%i) non valide !", actor_id->cur_health);
				LOG_TO_CONSOLE(c_red2, errstr);
				actor_id->cur_health = 6;
			}
			percentage = niveau_vie[actor_id->cur_health].pcmoy;
		}

		// calcul la largeur de la barre equivalente a "000/000" (necessaire pour positionner l'etat)
		if (view_health_bar)
		{
			sprintf((char*)hp, "%03u/%03u", 0, 0);
			largeur_barre = (float)get_string_width(hp) * ALT_INGAME_FONT_X_LEN * name_zoom * font_scale;
			largeur_total = largeur_barre;
		}

		// affichage du niveau de santé
		if (view_hp)
		{
			if (! actor_id->max_health) {
				sprintf((char*)hp, "%s", niveau_vie[actor_id->cur_health].texte);
			} else if (! voir_pdv) {
				sprintf((char*)hp, "%s", niveau_vie[etat_sante(percentage)].texte);
			} else {
				sprintf((char*)hp, "%d/%d", actor_id->cur_health, actor_id->max_health);
			}

			// calcul la largeur des pdv/etat de sante puis la largeur totale (barre incluse)
			largeur_sante = (float)get_string_width(hp) * ALT_INGAME_FONT_X_LEN * name_zoom * font_scale;
			largeur_total+= (view_health_bar) ? 5.0f + largeur_sante : largeur_sante;

			set_health_color(percentage, 1.0f, 1.0f);
			draw_ortho_ingame_string(hx+(largeur_total/2.0f)-largeur_sante, hy, hz, hp, 1, ALT_INGAME_FONT_X_LEN*font_scale, ALT_INGAME_FONT_Y_LEN*font_scale);
		}

		// affichage de la barre de santé
		if (view_health_bar)
		{
			double hauteur_barre = hauteur_sante*0.7f; // la barre ne remplit pas toute la hauteur
			double starty = hy + hauteur_sante*0.2f;   // du coup on ne la place pas au ras du sol
			double startx = hx - largeur_total*0.5f;
			double healthbar_conv = 0;
			double healthbar_loss = 0;
			double healthbar_fade = 1.0f;
			if (percentage > 110.0f) percentage = 110.0f; //deal with massive bars by trimming at 110%

			if (actor_id->max_health)
			{
				if (actor_id->last_health_loss && cur_time-actor_id->last_health_loss<1000)
				{
					//only when using floatingmessages
					if (actor_id->damage > 0)
					{
						healthbar_conv = largeur_barre * percentage;
						healthbar_loss = largeur_barre * (float)((float)actor_id->damage/(float)actor_id->max_health);
						healthbar_fade = 1.0f - ((float)(cur_time-actor_id->last_health_loss) / 1000.0f);
					}
					else
					{
						healthbar_conv = largeur_barre * (float)((float)(actor_id->cur_health+actor_id->damage)/(float)actor_id->max_health);
						healthbar_loss = largeur_barre * (float)((float)(-actor_id->damage)/(float)actor_id->max_health);
						healthbar_fade = (float)(cur_time-actor_id->last_health_loss) / 1000.0f;
					}
				}
				else
				{
					healthbar_conv = largeur_barre * percentage;
					actor_id->last_health_loss = 0;
				}
			}
			else
			{
				// Quand les PVs sont inconnus, on affiche le pourcentage min de l'etat en utilisant le "loss" pour le pourcentage max
				healthbar_conv = largeur_barre * niveau_vie[actor_id->cur_health].pcmin;
				healthbar_loss = largeur_barre * (niveau_vie[actor_id->cur_health].pcmax - niveau_vie[actor_id->cur_health].pcmin);
				healthbar_fade = 0.4f;
			}

			glDisable(GL_TEXTURE_2D);

			glBegin(GL_QUADS);
				set_health_color(percentage, 0.5f, 1.0f);
				glVertex3d(startx,                starty,               hz);
				glVertex3d(startx+healthbar_conv, starty,               hz);
				set_health_color(percentage, 1.0f, 1.0f);
				glVertex3d(startx+healthbar_conv, starty+hauteur_barre, hz);
				glVertex3d(startx,                starty+hauteur_barre, hz);
			glEnd();

			if (healthbar_loss)
			{
				glEnable(GL_BLEND);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE);
				glBegin(GL_QUADS);
					set_health_color(percentage, 0.5f, healthbar_fade);
					glVertex3d(startx+healthbar_conv,                starty,               hz);
					glVertex3d(startx+healthbar_conv+healthbar_loss, starty,               hz);
					set_health_color(percentage, 1.0f, healthbar_fade);
					glVertex3d(startx+healthbar_conv+healthbar_loss, starty+hauteur_barre, hz);
					glVertex3d(startx+healthbar_conv,                starty+hauteur_barre, hz);
				glEnd();
				glDisable(GL_BLEND);
			}

			glDepthFunc(GL_LEQUAL);
			glBegin(GL_LINE_LOOP);
				glColor3f(0.0f, 0.0f, 0.0f);
				glVertex3f(startx,               starty,               hz);
				glVertex3f(startx+largeur_barre, starty,               hz);
				glVertex3f(startx+largeur_barre, starty+hauteur_barre, hz);
				glVertex3f(startx,               starty+hauteur_barre, hz);
			glEnd();

			glEnable(GL_TEXTURE_2D);
		}

		if (largeur_total > banner_width) banner_width = largeur_total;
		banner_height+= hauteur_sante;
	}

	// -------------------------------------------------------------------- NOMS

	if (view_names && actor_id->actor_name[0])
	{
		unsigned char name[255];
		float largeur_nom = 0;
		float hauteur_nom = 0;
		float font_size_x = font_scale*SMALL_INGAME_FONT_X_LEN;
		float font_size_y = font_scale*SMALL_INGAME_FONT_Y_LEN;

		// affichage du titre
		if ((actor_id->titre[0] != 0) && (actor_id->titre[0] != 48))
		{
			float font_scale = titre_zoom / name_zoom;
			set_font(police_titre);
			glColor3f(couleur_titre.rouge, couleur_titre.vert, couleur_titre.bleu);
			largeur_nom = (float)get_string_width((unsigned char*)actor_id->titre) * font_size_x * titre_zoom;
			hauteur_nom = font_size_y * titre_zoom * 12;
			draw_ortho_ingame_string(hx-(largeur_nom/2.0f), hy+banner_height, hz, (unsigned char*)actor_id->titre, 1, font_size_x*font_scale, font_size_y*font_scale);
			if (largeur_nom > banner_width) banner_width = largeur_nom;
			banner_height+= hauteur_nom;
		}

		// couleur du nom
		if (actor_id->kind_of_actor==NPC) {
			glColor3f(0.3f,0.8f,1.0f);
		} else if (actor_id->kind_of_actor==HUMAN) {
			glColor3f(1.0f,1.0f,1.0f);
		} else if (actor_id->is_enhanced_model && actor_id->kind_of_actor==6) {
			glColor3f(0.8f,0.4f,1.0f);
		} else if (actor_id->is_enhanced_model && (actor_id->kind_of_actor==PKABLE_HUMAN || actor_id->kind_of_actor==PKABLE_COMPUTER_CONTROLLED)) {
			glColor3f(1.0f,0.0f,0.0f);
		} else {
			glColor3f(1.0f,1.0f,0.0f);
		}

		// affichage du nom
		set_font(name_font);
		largeur_nom = (float)get_string_width((unsigned char*)actor_id->actor_name) * font_size_x * name_zoom;
		hauteur_nom = font_size_y * name_zoom * 12;
		safe_snprintf((char*)name, sizeof(name), "%s", actor_id->actor_name);
		draw_ortho_ingame_string(hx-(largeur_nom/2.0f), hy+banner_height, hz, name, 1, font_size_x, font_size_y);
		if (largeur_nom > banner_width) banner_width = largeur_nom;
		banner_height+= hauteur_nom;
		set_font(0);


		if (view_buffs) draw_buffs(actor_id->actor_id, hx, hy, hz);
	}

	// -------------------------------------------------------------------- FOND

    if((view_mp || view_mana_bar) && me && me->actor_id==actor_id->actor_id){
        hy-=ALT_INGAME_FONT_Y_LEN * 12.0 * name_zoom * font_scale;
        banner_height+=ALT_INGAME_FONT_Y_LEN * 12.0 * name_zoom * font_scale;
    }
	if (use_alpha_banner && banner_width > 0)
	{
		glDisable(GL_TEXTURE_2D);
		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE, GL_SRC_ALPHA);
		glColor4f(0.0f, 0.0f, 0.0f, 0.6f);
		glBegin(GL_QUADS);
			glVertex3f(hx-(banner_width/2.0f), hy, hz+0.0001);
			glVertex3f(hx+(banner_width/2.0f), hy, hz+0.0001);
			glVertex3f(hx+(banner_width/2.0f), hy+banner_height, hz+0.0001);
			glVertex3f(hx-(banner_width/2.0f), hy+banner_height, hz+0.0001);
		glEnd();
		glDisable(GL_BLEND);
		glEnable(GL_TEXTURE_2D);
	}

	// -------------------------------------------------------------------- MISC

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

	if ((actor_id->current_displayed_text_time_left>0) && (actor_id->current_displayed_text[0]!=0))
	{
		draw_actor_overtext(actor_id);
	}

	if (floatingmessages_enabled)
	{
		drawactor_floatingmessages(actor_id->actor_id, offset_z+0.1f);
	}

	// -------------------------------------------------------------------- MENU

	/* set cm_mouse_over_banner true if the mouse is over your banner, or a box where it might be */
	if (actor_id->actor_id == yourself)
	{
		if ((mouse_x > hx-(banner_width/2.0f)) && (mouse_x < hx+(banner_width/2.0f))
		 && (window_height-mouse_y > hy) && (window_height-mouse_y < hy+banner_height))
			cm_mouse_over_banner = 1;
		else
			cm_mouse_over_banner = 0;
	}
	else {
        if ((mouse_x > hx-(banner_width/2.0f))
           && (mouse_x < hx+(banner_width/2.0f))
           && (window_height-mouse_y > hy)
           && (window_height-mouse_y < hy+banner_height)
           &&  select_with_mouse_on_banner==1 ) {
               actor_under_mouse = actor_id;
               object_under_mouse = actor_id->actor_id;
               thing_under_the_mouse = get_under_mouse_type(actor_id);
        }
	}

    if((view_mp || view_mana_bar) && me && me->actor_id==actor_id->actor_id){
        hy+=ALT_INGAME_FONT_Y_LEN * 12.0 * name_zoom * font_scale;
        banner_height-=ALT_INGAME_FONT_Y_LEN * 12.0 * name_zoom * font_scale;
    }

	// -------------------------------------------------------------------------
	glColor3f(1,1,1);
}


void draw_bubble(float x_left, float x_right, float x_leg_left, float x_leg_right, float y_top, float y_bottom, float y_actor)
{
	const float r=zoom_level/20;
	const float mul=M_PI/180.0f;
	int angle;

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	glColor4f(1.0f, 1.0f, 1.0f, 0.75f);
	glBegin(GL_POLYGON);

	for(angle=90;angle<180;angle+=10){
		float rad=-mul*angle;
		glVertex3f(x_left+cos(rad)*r-r, 0.01f, y_bottom+r+sin(rad)*r);
	}

	for(angle=180;angle<270;angle+=10){
		float rad=-mul*angle;
		glVertex3f(x_left+cos(rad)*r-r, 0.01f, y_top-r+sin(rad)*r);
	}

	for(angle=270;angle<360;angle+=10){
		float rad=-mul*angle;
		glVertex3f(x_right+cos(rad)*r+r, 0.01f, y_top-r+sin(rad)*r);
	}

	for(angle=0;angle<90;angle+=10){
		float rad=-mul*angle;
		glVertex3f(x_right+cos(rad)*r+r, 0.01f, y_bottom+sin(rad)*r+r);
	}

	glEnd();

	glBegin(GL_POLYGON);
		glVertex3f(x_leg_right, 0.01f, y_bottom+0.02);
		glVertex3f(x_leg_right, 0.01f, y_actor);
		glVertex3f(x_leg_left, 0.01f, y_bottom+0.02);
	glEnd();

	glDisable(GL_BLEND);
}

//-- Logan Dugenoux [5/26/2004]
void draw_actor_overtext( actor* actor_ptr )
{
	float z, w, h;
	float x_left, x_right, x_leg_left, x_leg_right, y_top, y_bottom, y_actor;
	float textwidth;
	float textheight;
	float margin;
	float maxwidth = 0;
	float nb_ligne_bulle = 0;
	int index, index2;
	char * position;
	char ligne1[MAX_CURRENT_DISPLAYED_TEXT_LEN+1] = "";
	char ligne2[MAX_CURRENT_DISPLAYED_TEXT_LEN+1] = "";
	char ligne3[MAX_CURRENT_DISPLAYED_TEXT_LEN+1] = "";

	//-- decrease display time
	actor_ptr->current_displayed_text_time_left -= (cur_time-last_time);
	if(!(SDL_GetAppState()&SDL_APPACTIVE)) return;	// not actually drawing, fake it

	set_font(chat_font);
	safe_strncpy((char *)ligne1, (char *)actor_ptr->current_displayed_text, sizeof(ligne1));
	// si tout ne rentre pas sur la ligne 1 : recherche d'un espace dans la ligne pour tronquer
	if (strlen((char *)actor_ptr->current_displayed_text) > MAX_CURRENT_DISPLAYED_TEXT_LEN) {
		if (actor_ptr->current_displayed_text[MAX_CURRENT_DISPLAYED_TEXT_LEN] != ' ') {
			position = strrchr((char *)ligne1, ' ');
			if (position != NULL) {
				index = strlen((char *)ligne1) - strlen((char *)position) + 1;
				safe_strncpy((char *)ligne1, (char *)ligne1, index);
			} else index = MAX_CURRENT_DISPLAYED_TEXT_LEN;
		} else index = MAX_CURRENT_DISPLAYED_TEXT_LEN + 1;
		safe_strncpy((char *)ligne2, (char *)actor_ptr->current_displayed_text + index, sizeof(ligne2));
        nb_ligne_bulle = 1;
		// si la ligne 2 n'a pas pu contenir tout le reste : on recommence pour une 3eme
		if (strlen((char *)actor_ptr->current_displayed_text) > MAX_CURRENT_DISPLAYED_TEXT_LEN + index) {
			if (actor_ptr->current_displayed_text[MAX_CURRENT_DISPLAYED_TEXT_LEN + index] != ' ') {
				position = strrchr((char *)ligne2, ' ');
				if (position != NULL) {
					index2 = strlen((char *)ligne2) - strlen((char *)position) + 1;
					if (strlen((char *)actor_ptr->current_displayed_text) > MAX_CURRENT_DISPLAYED_TEXT_LEN + index + index2) {
						safe_strncpy((char *)ligne2, (char *)ligne2, index2);
					} else index2 = MAX_CURRENT_DISPLAYED_TEXT_LEN;
				} else index2 = MAX_CURRENT_DISPLAYED_TEXT_LEN;
			} else index2 = MAX_CURRENT_DISPLAYED_TEXT_LEN + 1;
			safe_strncpy((char *)ligne3, (char *)actor_ptr->current_displayed_text+index+index2, sizeof(ligne3));
        nb_ligne_bulle = 2;
        }
    }
	maxwidth = (float)get_string_width((unsigned char*)(ligne1));
	w = (float)get_string_width((unsigned char*)(ligne2)); if (w > maxwidth) maxwidth = w;
	w = (float)get_string_width((unsigned char*)(ligne3)); if (w > maxwidth) maxwidth = w;

	margin = zoom_level * 0.04f;
	textwidth = (maxwidth * SMALL_INGAME_FONT_X_LEN * chat_zoom * zoom_level / 3.0f) / 12.0f;
	textheight = zoom_level * chat_zoom * 0.05f + margin;
	z = (actor_ptr->sitting) ? 0.6f : 1.4f; // close if he's sitting
	h = margin + textheight * (nb_ligne_bulle+1);
	w = margin + textwidth + margin;
	if (w < zoom_level * 0.25f) w = zoom_level * 0.25f; // largeur minimale de la bulle

	x_left  =-w * 0.5f;
	x_right = w * 0.5f;
	x_leg_left = -zoom_level * 0.1f;
	x_leg_right= 0.0f;

	y_bottom = z + 0.3f + zoom_level * 0.2f;
	y_top = y_bottom + h;
	y_actor = z + 0.2f;

	glDisable(GL_TEXTURE_2D);

	draw_bubble(x_left+0.01f, x_right-0.01f, x_leg_left, x_leg_right, y_top-0.01f, y_bottom+0.01f, y_actor+0.01f);

	glEnable(GL_TEXTURE_2D);

	//---
	// Draw text
	glColor3f(0.0f,0.0f,0.0f);

    if (nb_ligne_bulle == 0)
    {
       	DRAW_INGAME_SMALL(x_left+margin, y_bottom+margin, ligne1, 1);
    }
    else if (nb_ligne_bulle == 1)
    {
       	DRAW_INGAME_SMALL(x_left+margin, y_bottom+margin+textheight, ligne1, 1);
      	DRAW_INGAME_SMALL(x_left+margin, y_bottom+margin, ligne2, 1);
    }
    else if (nb_ligne_bulle == 2)
    {
       	DRAW_INGAME_SMALL(x_left+margin, y_bottom+margin+textheight*2, ligne1, 1);
       	DRAW_INGAME_SMALL(x_left+margin, y_bottom+margin+textheight, ligne2, 1);
       	DRAW_INGAME_SMALL(x_left+margin, y_bottom+margin, ligne3, 1);
    }
	set_font(0);

	//glDepthFunc(GL_LESS);
	if (actor_ptr->current_displayed_text_time_left<=0)
	{	// clear if needed
		actor_ptr->current_displayed_text_time_left = 0;
		actor_ptr->current_displayed_text[0] = 0;
	}
}

void draw_actor_without_banner(actor * actor_id, Uint32 use_lightning, Uint32 use_textures, Uint32 use_glow)
{
	double x_pos,y_pos,z_pos;
	float x_rot,y_rot,z_rot;
	//if first person, dont draw actor
	actor *me = get_our_actor();
	if (me&&me->actor_id==actor_id->actor_id&&first_person) return;
	if (use_textures)
	{
		if (actor_id->is_enhanced_model)
		{
			if (bind_actor_texture(actor_id->texture_id, &actor_id->has_alpha) == 0)
			{
				return;
			}
		}
		else
		{
			if (!actor_id->remapped_colors)
			{
				bind_texture(actor_id->texture_id);
			}
			else
			{
				if (bind_actor_texture(actor_id->texture_id, &actor_id->has_alpha) == 0)
				{
					return;
				}
			}
		}
	}

	glPushMatrix();//we don't want to affect the rest of the scene

	x_pos = actor_id->x_pos;
	y_pos = actor_id->y_pos;
	z_pos = actor_id->z_pos;

	if (z_pos == 0.0f)
	{
		//actor is walking, as opposed to flying, get the height underneath
		z_pos = get_tile_height(actor_id->x_tile_pos, actor_id->y_tile_pos);
	}

	x_rot = actor_id->x_rot;
	y_rot = actor_id->y_rot;
	z_rot = 180 - actor_id->z_rot;

	glTranslatef(x_pos + 0.25f, y_pos + 0.25f, z_pos);

	glRotatef(z_rot, 0.0f, 0.0f, 1.0f);
	glRotatef(x_rot, 1.0f, 0.0f, 0.0f);
	glRotatef(y_rot, 0.0f, 1.0f, 0.0f);

	if (actor_id->attached_actor >= 0)
		glTranslatef(actor_id->attachment_shift[0], actor_id->attachment_shift[1], actor_id->attachment_shift[2]);

	cal_render_actor(actor_id, use_lightning, use_textures, use_glow);

	//now, draw their damage & nametag
	glPopMatrix();  // restore the matrix

}

static __inline__ void draw_actor_banner_new(actor * actor_id)
{
	float x_pos, y_pos, z_pos;
	float healthbar_z;

	healthbar_z = actor_id->max_z + 0.2;

	glPushMatrix();//we don't want to affect the rest of the scene

	x_pos = actor_id->x_pos;
	y_pos = actor_id->y_pos;
	z_pos = actor_id->z_pos;

	if (z_pos == 0.0f)
	{
		//actor is walking, as opposed to flying, get the height underneath
		z_pos = get_tile_height(actor_id->x_tile_pos, actor_id->y_tile_pos);
	}

	glTranslatef(x_pos + 0.25f, y_pos + 0.25f, z_pos);

	if (actor_id->attached_actor >= 0)
	{
		glRotatef(180 - actor_id->z_rot, 0.0f, 0.0f, 1.0f);
		glTranslatef(actor_id->attachment_shift[0], actor_id->attachment_shift[1], actor_id->attachment_shift[2]);
		glRotatef(180 - actor_id->z_rot, 0.0f, 0.0f, -1.0f);
	}

	glRotatef(-rz, 0.0f, 0.0f, 1.0f);

	draw_actor_banner(actor_id, healthbar_z);

	glPopMatrix();	//we don't want to affect the rest of the scene
}

static int comp_actors(const void *in_a, const void *in_b)
{
	near_actor *a, *b;
	int at, bt;

	a = (near_actor *)in_a;
	b = (near_actor *)in_b;

	at = a->type;
	bt = b->type;

	if (at < bt)
	{
		return (-1);
	}
	else
	{
		if (at == bt)
		{
			return (0);
		}
		else
		{
			return (1);
		}
	}
}

void get_actors_in_range()
{
	VECTOR3 pos;
	unsigned int i;
	unsigned int tmp_nr_enh_act;		// Use temp variables to stop crowd sound interference during count
	float tmp_dist_to_nr_enh_act;
	actor *me;
	AABBOX bbox;

	me = get_our_actor ();

	if (!me) return;

	no_near_actors = 0;
	tmp_nr_enh_act = 0;
	tmp_dist_to_nr_enh_act = 0;

	set_current_frustum(get_cur_intersect_type(main_bbox_tree));

	for (i = 0; i < max_actors; i++)
	{
		if(actors_list[i]
		   && (actors_list[i]->cluster == me->cluster || actors_list[i]->cluster == 0)
		)
		{
			// if we have an attached actor, we maybe have to modify the position of the current actor
			if (actors_list[i]->attached_actor >= 0)
			{
				actor *att = actors_list[actors_list[i]->attached_actor];
				attachment_props *att_props;
				float loc_pos[3];
				float att_pos[3];
				float loc_scale = get_actor_scale(actors_list[i]);
				float att_scale = get_actor_scale(att);
				if (actors_list[i]->actor_id < 0) // we are on a attached actor
				{
					att_props = &attached_actors_defs[actors_list[i]->actor_type].actor_type[att->actor_type];
					if (!att_props->is_holder) // the attachment is not a holder so we have to move it
					{
						cal_get_actor_bone_local_position(att, att_props->parent_bone_id, NULL, att_pos);
						cal_get_actor_bone_local_position(actors_list[i], att_props->local_bone_id, NULL, loc_pos);
						actors_list[i]->attachment_shift[0] = att_pos[0] * att_scale - (loc_pos[0] - att_props->shift[0]) * loc_scale;
						actors_list[i]->attachment_shift[1] = att_pos[1] * att_scale - (loc_pos[1] - att_props->shift[1]) * loc_scale;
						actors_list[i]->attachment_shift[2] = att_pos[2] * att_scale - (loc_pos[2] - att_props->shift[2]) * loc_scale;
					}
				}
				else if (actors_list[i]->actor_id >= 0) // we are on a standard actor
				{
					att_props = &attached_actors_defs[att->actor_type].actor_type[actors_list[i]->actor_type];
					if (att_props->is_holder) // the attachment is an holder, we have to move the current actor
					{
						cal_get_actor_bone_local_position(att, att_props->local_bone_id, NULL, att_pos);
						cal_get_actor_bone_local_position(actors_list[i], att_props->parent_bone_id, NULL, loc_pos);
						actors_list[i]->attachment_shift[0] = att_pos[0] * att_scale - (loc_pos[0] - att_props->shift[0]) * loc_scale;
						actors_list[i]->attachment_shift[1] = att_pos[1] * att_scale - (loc_pos[1] - att_props->shift[1]) * loc_scale;
						actors_list[i]->attachment_shift[2] = att_pos[2] * att_scale - (loc_pos[2] - att_props->shift[2]) * loc_scale;
					}
				}
			}
			pos[X] = actors_list[i]->x_pos + actors_list[i]->attachment_shift[X];
			pos[Y] = actors_list[i]->y_pos + actors_list[i]->attachment_shift[Y];
			pos[Z] = actors_list[i]->z_pos + actors_list[i]->attachment_shift[Z];
			if (pos[Z] == 0.0f)
			{
				//actor is walking, as opposed to flying, get the height underneath
				pos[Z] = get_tile_height(actors_list[i]->x_tile_pos, actors_list[i]->y_tile_pos);
			}

			if (actors_list[i]->calmodel == NULL) continue;

			memcpy(&bbox, &actors_list[i]->bbox, sizeof(AABBOX));
			rotate_aabb(&bbox, actors_list[i]->x_rot, actors_list[i]->y_rot, 180.0f-actors_list[i]->z_rot);

			VAddEq(bbox.bbmin, pos);
			VAddEq(bbox.bbmax, pos);

			if (aabb_in_frustum(bbox))
			{
				near_actors[no_near_actors].actor = i;
				near_actors[no_near_actors].ghost = actors_list[i]->ghost;
				near_actors[no_near_actors].buffs = actors_list[i]->buffs;
				near_actors[no_near_actors].select = 0;
				near_actors[no_near_actors].type = actors_list[i]->actor_type;
				if (actors_list[i]->ghost)
				{
					near_actors[no_near_actors].alpha = 0;
				}
				else
				{
					near_actors[no_near_actors].alpha =
						actors_list[i]->has_alpha;
				}

				actors_list[i]->max_z = actors_list[i]->bbox.bbmax[Z];

				if (read_mouse_now && (get_cur_intersect_type(main_bbox_tree) == INTERSECTION_TYPE_DEFAULT))
				{
					near_actors[no_near_actors].select = 1;
				}
				no_near_actors++;
				if (actors_list[i]->is_enhanced_model && actors_list[i]->actor_id != me->actor_id)
				{
					tmp_nr_enh_act++;
					tmp_dist_to_nr_enh_act += ((me->x_pos - actors_list[i]->x_pos) *
														(me->x_pos - actors_list[i]->x_pos)) +
														((me->y_pos - actors_list[i]->y_pos) *
														(me->y_pos - actors_list[i]->y_pos));
				}
			}
		}
	}
	if (tmp_nr_enh_act > 0)
		tmp_dist_to_nr_enh_act = tmp_dist_to_nr_enh_act / tmp_nr_enh_act;
	no_near_enhanced_actors = tmp_nr_enh_act;
	distanceSq_to_near_enhanced_actors = tmp_dist_to_nr_enh_act;
	qsort(near_actors, no_near_actors, sizeof(near_actor), comp_actors);
}

void display_actors(int banner, int render_pass)
{
	Sint32 i, has_alpha, has_ghosts;
	Uint32 use_lightning = 0, use_textures = 0;

	get_actors_in_range();



	switch (render_pass)
	{
		case DEFAULT_RENDER_PASS:
		case SHADOW_RENDER_PASS:
		case REFLECTION_RENDER_PASS:
			use_lightning = 1;
			use_textures = 1;
			break;
		case DEPTH_RENDER_PASS:
			use_lightning = 0;
			use_textures = 1;
			break;
	}

	has_alpha = 0;
	has_ghosts = 0;

	if (fsaa > 1)
	{
		glEnable(GL_MULTISAMPLE);
	}
	for (i = 0; i < no_near_actors; i++)
	{
		if (near_actors[i].ghost || (near_actors[i].buffs & BUFF_INVISIBILITY))
			{
				if ((render_pass == DEFAULT_RENDER_PASS) ||
					(render_pass == SHADOW_RENDER_PASS))
				{
					has_ghosts = 1;
				}
			}
		else if (near_actors[i].alpha)
		{
			has_alpha = 1;
		}
		else
		{
			actor *cur_actor = actors_list[near_actors[i].actor];
			if (cur_actor)
			{
				draw_actor_without_banner(cur_actor, use_lightning, use_textures, 1);
				if (near_actors[i].select)
				{
					if (cur_actor->kind_of_actor == NPC)
					{
						anything_under_the_mouse(near_actors[i].actor, UNDER_MOUSE_NPC);
					}
					else
					{
						if ((cur_actor->kind_of_actor == HUMAN) ||
							(cur_actor->kind_of_actor == COMPUTER_CONTROLLED_HUMAN) ||
							(cur_actor->is_enhanced_model &&
							((cur_actor->kind_of_actor == PKABLE_HUMAN) ||
							(cur_actor->kind_of_actor == PKABLE_COMPUTER_CONTROLLED))))
						{
							anything_under_the_mouse(near_actors[i].actor, UNDER_MOUSE_PLAYER);
						}
						else
						{
							anything_under_the_mouse(near_actors[i].actor, UNDER_MOUSE_ANIMAL);
						}
					}
				}
			}
		}
	}
	if (has_alpha)
	{
		glEnable(GL_ALPHA_TEST);
		glAlphaFunc(GL_GREATER, 0.4f);
		for (i = 0; i < no_near_actors; i++)
		{

			if (near_actors[i].alpha && !(near_actors[i].ghost || (near_actors[i].buffs & BUFF_INVISIBILITY)))
			{

				actor *cur_actor = actors_list[near_actors[i].actor];
				if (cur_actor)
				{
					draw_actor_without_banner(cur_actor, use_lightning, 1, 1);

					if (near_actors[i].select)
					{
						if (cur_actor->kind_of_actor == NPC)
						{
							anything_under_the_mouse(near_actors[i].actor, UNDER_MOUSE_NPC);
						}
						else
						{
							if ((cur_actor->kind_of_actor == HUMAN) ||
								(cur_actor->kind_of_actor == COMPUTER_CONTROLLED_HUMAN) ||
								(cur_actor->is_enhanced_model &&
								 ((cur_actor->kind_of_actor == PKABLE_HUMAN) ||
								 (cur_actor->kind_of_actor == PKABLE_COMPUTER_CONTROLLED))))
							{
								anything_under_the_mouse(near_actors[i].actor, UNDER_MOUSE_PLAYER);
							}
							else
							{
								anything_under_the_mouse(near_actors[i].actor, UNDER_MOUSE_ANIMAL);
							}
						}
					}
				}
			}
		}
		glDisable(GL_ALPHA_TEST);
	}
	if (has_ghosts)
	{
		glEnable(GL_BLEND);
		glDisable(GL_LIGHTING);

		for (i = 0; i < no_near_actors; i++)
		{

			if (near_actors[i].ghost || (near_actors[i].buffs & BUFF_INVISIBILITY))
			{

				actor *cur_actor = actors_list[near_actors[i].actor];
				if (cur_actor)
				{
					//if any ghost has a glowing weapon, we need to reset the blend function each ghost actor.
					glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

						if ((near_actors[i].buffs & BUFF_INVISIBILITY))
						{
							glColor4f(1.0f, 1.0f, 1.0f, 0.25f);
						}
						else
						{
							glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
						}

					draw_actor_without_banner(cur_actor, use_lightning, use_textures, 1);

					if (near_actors[i].select)
					{
						if (cur_actor->kind_of_actor == NPC)
						{
							anything_under_the_mouse(near_actors[i].actor, UNDER_MOUSE_NPC);
						}
						else
						{
							if ((cur_actor->kind_of_actor == HUMAN) ||
								(cur_actor->kind_of_actor == COMPUTER_CONTROLLED_HUMAN) ||
								(cur_actor->is_enhanced_model &&
								 ((cur_actor->kind_of_actor == PKABLE_HUMAN) ||
								 (cur_actor->kind_of_actor == PKABLE_COMPUTER_CONTROLLED))))
							{
								anything_under_the_mouse(near_actors[i].actor, UNDER_MOUSE_PLAYER);
							}
							else
							{
								anything_under_the_mouse(near_actors[i].actor, UNDER_MOUSE_ANIMAL);
							}
						}
					}
				}
			}
		}
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
		glDisable(GL_BLEND);
		glEnable(GL_LIGHTING);
	}
	if (fsaa > 1)
	{
		glDisable(GL_MULTISAMPLE);
	}


	if (banner && (SDL_GetAppState() & SDL_APPACTIVE))
	{
		if (use_shadow_mapping)
		{
			glPushAttrib(GL_TEXTURE_BIT|GL_ENABLE_BIT);
			glDisable(GL_TEXTURE_2D);
			ELglActiveTextureARB(shadow_unit);
			glDisable(depth_texture_target);
			disable_texgen();
			ELglActiveTextureARB(GL_TEXTURE0);
			glEnable(GL_TEXTURE_2D);
			glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		}

		glDisable(GL_LIGHTING);
		glDisable(GL_BLEND);

		for (i = 0; i < no_near_actors; i++)
		{
			actor *cur_actor = actors_list[near_actors[i].actor];
			if (cur_actor
				&& cur_actor->actor_id >= 0
				)
			{
				draw_actor_banner_new(cur_actor);
			}
		}

		if (use_shadow_mapping)
		{
			last_texture = -1;
			glPopAttrib();
		}

		glDisable(GL_BLEND);
		glEnable(GL_LIGHTING);
	}
}


void add_actor_from_server (const char *in_data, int len)
{
	short actor_id;
	Uint32 buffs = 0;
	short x_pos;
	short y_pos;
	short z_rot;
	short max_health;
	short cur_health;
	Uint8 actor_type;
	Uint8 frame;
	int i;
	int dead=0;
	int kind_of_actor;

	double f_x_pos,f_y_pos,f_z_rot;
	float scale= 1.0f;
	int attachment_type = -1;

	actor_id=SDL_SwapLE16(*((short *)(in_data)));
	buffs=((*((char *)(in_data+3))>>3)&0x1F) | (((*((char*)(in_data+5))>>3)&0x1F)<<5);	// Strip the last 5 bits of the X and Y coords for the buffs
	x_pos=*((short *)(in_data+2)) & 0x7FF;
	y_pos=*((short *)(in_data+4)) & 0x7FF;
	buffs |= (SDL_SwapLE16(*((short *)(in_data+6))) & 0xFF80) << 3; // we get the 9 MSB for the buffs and leave the 7 LSB for a further use
	z_rot=SDL_SwapLE16(*((short *)(in_data+8)));
	actor_type=*(in_data+10);

	frame=*(in_data+11);
	max_health=SDL_SwapLE16(*((short *)(in_data+12)));
	cur_health=SDL_SwapLE16(*((short *)(in_data+14)));
	kind_of_actor=*(in_data+16);
	if(len > 17+(int)strlen(in_data+17)+2){
		scale=((float)SDL_SwapLE16(*((short *)(in_data+17+strlen(in_data+17)+1)))/((float)ACTOR_SCALE_BASE));

		if(len > 17+(int)strlen(in_data+17)+3)
			attachment_type = (unsigned char)in_data[17+strlen(in_data+17)+3];
	}

	if(actor_type < 0 || actor_type >= MAX_ACTOR_DEFS || (actor_type > 0 && actors_defs[actor_type].actor_type != actor_type) ){
		LOG_ERROR("Illegal/missing actor definition %d", actor_type);
	}

	//translate from tile to world
	f_x_pos=x_pos*0.5;
	f_y_pos=y_pos*0.5;
	f_z_rot=z_rot;

	//get the current frame
	switch(frame) {
	case frame_walk:
	case frame_run:
		break;
	case frame_die1:
	case frame_die2:
		dead=1;
		break;
	case frame_pain1:
	case frame_pain2:
	case frame_pick:
	case frame_drop:
	case frame_idle:
	case frame_sit_idle:
	case frame_harvest:
	case frame_cast:
	case frame_attack_up_1:
	case frame_attack_up_2:
	case frame_attack_up_3:
	case frame_attack_up_4:
	case frame_attack_up_5:
	case frame_attack_up_6:
	case frame_attack_up_7:
	case frame_attack_up_8:
	case frame_attack_up_9:
	case frame_attack_up_10:
	case frame_attack_down_1:
	case frame_attack_down_2:
	case frame_attack_down_3:
	case frame_attack_down_4:
	case frame_attack_down_5:
	case frame_attack_down_6:
	case frame_attack_down_7:
	case frame_attack_down_8:
	case frame_attack_down_9:
	case frame_attack_down_10:
	case frame_combat_idle:
    case frame_salut:
		break;
	default:
		{

			LOG_ERROR("%s %d - %s\n", unknown_frame, frame, &in_data[17]);
		}
	}


	//find out if there is another actor with that ID
	//ideally this shouldn't happen, but just in case

	for(i=0;i<max_actors;i++)
		{
			if(actors_list[i])
				if(actors_list[i]->actor_id==actor_id)
					{
						LOG_ERROR(duplicate_actors_str,actor_id, actors_list[i]->actor_name, &in_data[17]);
						destroy_actor(actors_list[i]->actor_id);//we don't want two actors with the same ID
						i--;// last actor was put here, he needs to be checked too
					}
		}

	i= add_actor(actor_type, actors_defs[actor_type].skin_name, f_x_pos, f_y_pos, 0.0, f_z_rot, scale, 0, 0, 0, 0, 0, 0, actor_id);

	if(i==-1) return;//A nasty error occured and we couldn't add the actor. Ignore it.

	//The actors list is locked when we get here...

	actors_list[i]->async_fighting = 0;
	actors_list[i]->async_x_tile_pos = x_pos;
	actors_list[i]->async_y_tile_pos = y_pos;
	actors_list[i]->async_z_rot = z_rot;

	actors_list[i]->x_tile_pos=x_pos;
	actors_list[i]->y_tile_pos=y_pos;
	actors_list[i]->buffs=buffs;
	actors_list[i]->actor_type=actor_type;
	actors_list[i]->damage=0;
	actors_list[i]->damage_ms=0;
	actors_list[i]->sitting=0;
	actors_list[i]->fighting=0;
	//test only
	actors_list[i]->max_health=max_health;
	actors_list[i]->cur_health=cur_health;


	actors_list[i]->z_pos = get_actor_z(actors_list[i]);
	if(frame==frame_sit_idle)actors_list[i]->sitting=1;
	else
		if(frame==frame_combat_idle)actors_list[i]->fighting=1;
	//ghost or not?
	actors_list[i]->ghost=actors_defs[actor_type].ghost;

	actors_list[i]->dead=dead;
	actors_list[i]->stop_animation=1;//helps when the actor is dead...
	actors_list[i]->kind_of_actor=kind_of_actor;
	if(strlen(&in_data[17]) >= 30)
		{
			LOG_ERROR("%s (%d): %s/%d\n", bad_actor_name_length, actors_list[i]->actor_type,&in_data[17], (int)strlen(&in_data[17]));
		}
	else my_strncp(actors_list[i]->actor_name,&in_data[17],30);

	if (attachment_type >= 0)
		add_actor_attachment(actor_id, attachment_type);

	if (actors_defs[actor_type].coremodel!=NULL) {
		//Setup cal3d model
		actors_list[i]->calmodel = model_new(actors_defs[actor_type].coremodel);
		//Attach meshes
		if(actors_list[i]->calmodel){
			model_attach_mesh(actors_list[i], actors_defs[actor_type].shirt[0].mesh_index);
			if(dead){
				cal_actor_set_anim(i, actors_defs[actors_list[i]->actor_type].cal_frames[cal_actor_die1_frame]);
				actors_list[i]->stop_animation=1;
				CalModel_Update(actors_list[i]->calmodel,1000);
			}
            else {
                /* Schmurk: we explicitly go on idle here to avoid weird
                 * flickering when actors appear */
                set_on_idle(i);
                /* CalModel_Update(actors_list[i]->calmodel,0); */
            }
			build_actor_bounding_box(actors_list[i]);
            /* lines commented by Schmurk: we've set an animation just before
             * so we don't want do screw it up */
			/* actors_list[i]->cur_anim.anim_index=-1; */
			/* actors_list[i]->cur_anim_sound_cookie=0; */
			/* actors_list[i]->IsOnIdle=0; */
		}
	}
	else
	{
		actors_list[i]->calmodel=NULL;
	}
	update_actor_buffs(actor_id, buffs);
	UNLOCK_ACTORS_LISTS();	//unlock it

}

//--- LoganDugenoux [5/25/2004]
#define MS_PER_CHAR	200
#define MINI_BUBBLE_MS	500
void	add_displayed_text_to_actor( actor * actor_ptr, const char* text )
{
	int len_to_add;
	len_to_add = strlen(text);
	safe_snprintf(actor_ptr->current_displayed_text, sizeof(actor_ptr->current_displayed_text), "%s", text);
	actor_ptr->current_displayed_text_time_left = len_to_add*MS_PER_CHAR;

	actor_ptr->current_displayed_text_time_left += MINI_BUBBLE_MS;
}

//--- LoganDugenoux [5/25/2004]
actor *	get_actor_ptr_from_id( int actor_id )
{
	int i;
	for (i = 0; i < max_actors; i++)
	{
		if (actors_list[i]->actor_id == actor_id)
			return actors_list[i];
	}
	return NULL;
}

void end_actors_lists()
{
	SDL_DestroyMutex(actors_lists_mutex);
	actors_lists_mutex=NULL;
}


int on_the_move (const actor *act){
	if (act == NULL) return 0;
	return act->moving || (act->que[0] >= move_n && act->que[0] <= move_nw);
}

void get_actor_rotation_matrix(actor *in_act, float *out_rot)
{
	float tmp_rot1[9], tmp_rot2[9];

	MAT3_ROT_Z(out_rot, (180.0 - in_act->z_rot) * (M_PI / 180.0));
	MAT3_ROT_X(tmp_rot1, in_act->x_rot * (M_PI / 180.0));
	MAT3_MULT(tmp_rot2, out_rot, tmp_rot1);
	MAT3_ROT_Y(tmp_rot1, in_act->y_rot * (M_PI / 180.0));
	MAT3_MULT(out_rot, tmp_rot2, tmp_rot1);
}

void transform_actor_local_position_to_absolute(actor *in_act, float *in_local_pos, float *in_act_rot, float *out_pos)
{
	float scale = get_actor_scale(in_act);
	float rot[9];

	if (!in_act_rot)
	{
		get_actor_rotation_matrix(in_act, rot);
		in_act_rot = rot;
	}

	MAT3_VECT3_MULT(out_pos, in_act_rot, in_local_pos);

	out_pos[0] = out_pos[0] * scale + in_act->x_pos + 0.25;
	out_pos[1] = out_pos[1] * scale + in_act->y_pos + 0.25;
	out_pos[2] = out_pos[2] * scale + get_actor_z(in_act);

	if (in_act->attached_actor >= 0)
	{
		float shift[3];
		MAT3_VECT3_MULT(shift, in_act_rot, in_act->attachment_shift);
		out_pos[0] += shift[0];
		out_pos[1] += shift[1];
		out_pos[2] += shift[2];
	}
}
