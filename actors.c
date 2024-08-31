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
#ifdef MISSILES
#include "missiles.h"
#endif // MISSILES
#include "new_actors.h"
#include "platform.h"
#include "shadows.h"
#include "textures.h"
#include "translate.h"
#include "vmath.h"
#ifdef CLUSTER_INSIDES
#include "cluster.h"
#endif
#include "eye_candy_wrapper.h"
#include "minimap.h"
#include "actor_init.h"
#ifdef	FSAA
#include "fsaa/fsaa.h"
#endif	/* FSAA */
#ifdef FR_VERSION
#include "themes.h"
#endif //FR_VERSION

#ifdef FR_VERSION
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
#endif //FR_VERSION

#ifdef ELC
#define DRAW_ORTHO_INGAME_NORMAL(x, y, z, our_string, max_lines)	draw_ortho_ingame_string(x, y, z, (const Uint8*)our_string, max_lines, INGAME_FONT_X_LEN*10.0, INGAME_FONT_Y_LEN*10.0)
#define DRAW_INGAME_NORMAL(x, y, our_string, max_lines)	draw_ingame_string(x, y, (const Uint8*)our_string, max_lines, INGAME_FONT_X_LEN, INGAME_FONT_Y_LEN)
#define DRAW_INGAME_SMALL(x, y, our_string, max_lines)	draw_ingame_string(x, y, (const Uint8*)our_string, max_lines, SMALL_INGAME_FONT_X_LEN, SMALL_INGAME_FONT_Y_LEN)
#define DRAW_INGAME_ALT(x, y, our_string, max_lines)	draw_ingame_string(x, y, (const Uint8*)our_string, max_lines, ALT_INGAME_FONT_X_LEN, ALT_INGAME_FONT_Y_LEN)
#endif

actor *actors_list[MAX_ACTORS];
int max_actors=0;
SDL_mutex *actors_lists_mutex = NULL;	//used for locking between the timer and main threads
actor *your_actor = NULL;

actor_types actors_defs[MAX_ACTOR_DEFS];

#ifdef ATTACHED_ACTORS
attached_actors_types attached_actors_defs[MAX_ACTOR_DEFS];
#endif // ATTACHED_ACTORS

void draw_actor_overtext( actor* actor_ptr ); /* forward declaration */

int no_near_actors=0;
#ifdef NEW_SOUND
int no_near_enhanced_actors = 0;
float distanceSq_to_near_enhanced_actors;
#endif // NEW_SOUND
near_actor near_actors[MAX_ACTORS];

#ifdef MUTEX_DEBUG
Uint32 have_actors_lock = 0;
#endif

int cm_mouse_over_banner = 0;		/* use to trigger banner context menu */

#ifdef FR_VERSION
int etat_sante(float percentage)
{
	int i;
	for (i=5; i>=0; i--)
	{
		if (percentage > niveau_vie[i].pcmax) return i+1;
	}
	return 0;
}
#endif //FR_VERSION

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
#ifdef NEW_EYES
int add_actor (int actor_type, char * skin_name, float x_pos, float y_pos, float z_pos, float z_rot, float scale, char remappable, short skin_color, short hair_color, short eyes_color, short shirt_color, short pants_color, short boots_color, int actor_id)
#else //NEW_EYES
int add_actor (int actor_type, char * skin_name, float x_pos, float y_pos, float z_pos, float z_rot, float scale, char remappable, short skin_color, short hair_color, short shirt_color, short pants_color, short boots_color, int actor_id)
#endif //NEW_EYES
{
	int texture_id;
	int i;
	int k;
	actor *our_actor;
#ifdef CLUSTER_INSIDES
	int x, y;
#endif

#ifdef EXTRA_DEBUG
	ERR();
#endif

#ifdef	NEW_TEXTURES
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
#else	/* NEW_TEXTURES */
	if(actors_defs[actor_type].ghost)	texture_id= load_texture_cache_deferred(skin_name, 150);
	else if(!remappable)texture_id= load_texture_cache_deferred(skin_name, -1);
	else
		{
			LOG_ERROR("remapped skin for %s", skin_name);
			//texture_id=load_bmp8_remapped_skin(skin_name,150,skin_color,hair_color,eyes_color,shirt_color,pants_color,boots_color);
			exit(-1);
		}
#endif	/* NEW_TEXTURES */

	our_actor = calloc(1, sizeof(actor));
#ifdef FR_VERSION
	memset(our_actor->current_displayed_text, 0, 160);
#else //FR_VERSION
	memset(our_actor->current_displayed_text, 0, MAX_CURRENT_DISPLAYED_TEXT_LEN);
#endif //FR_VERSION
	our_actor->current_displayed_text_time_left =  0;

	our_actor->is_enhanced_model=0;
	our_actor->remapped_colors=remappable;
	our_actor->actor_id=actor_id;
	our_actor->cur_anim_sound_cookie = 0;

#ifdef MISSILES
	our_actor->cal_h_rot_start = 0.0;
	our_actor->cal_h_rot_end = 0.0;
	our_actor->cal_v_rot_start = 0.0;
	our_actor->cal_v_rot_end = 0.0;
	our_actor->cal_rotation_blend = -1.0;
	our_actor->cal_rotation_speed = 0.0;
	our_actor->are_bones_rotating = 0;
	our_actor->in_aim_mode = 0;
	our_actor->range_actions_count = 0;
	our_actor->delayed_item_changes_count = 0;
#endif // MISSILES

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
#ifdef	ANIMATION_SCALING
	our_actor->animation_scale = 1.0f;
#endif	/* ANIMATION_SCALING*/

    /* load the texture in case it's not already loaded and look if it has
     * an alpha map */
#ifdef	NEW_TEXTURES
	our_actor->has_alpha = get_texture_alpha(texture_id);
#else
    get_texture_id(texture_id);
	our_actor->has_alpha=texture_cache[texture_id].has_alpha;
#endif

	//clear the que
	for(k=0;k<MAX_CMD_QUEUE;k++)	our_actor->que[k]=nothing;
#ifdef EMOTES
	//clear emotes
	for(k=0;k<MAX_EMOTE_QUEUE;k++)	{
		our_actor->emote_que[k].emote=NULL;
		our_actor->emote_que[k].origin=NO_EMOTE;
		our_actor->emote_que[k].create_time=0;
		}
	memset(&our_actor->cur_emote,0,sizeof(emote_anim));
	memset(&our_actor->poses,0,sizeof(emote_data*)*4);
	for(k=0;k<MAX_EMOTE_FRAME;k++) our_actor->cur_emote.frames[k].anim_index=-1;
	our_actor->cur_emote.idle.anim_index=-1;
	our_actor->cur_emote_sound_cookie=0;



#endif

	our_actor->texture_id=texture_id;
	our_actor->skin=skin_color;
	our_actor->hair=hair_color;
#ifdef NEW_EYES
	our_actor->eyes=eyes_color;
#endif //NEW_EYES
	our_actor->pants=pants_color;
	our_actor->boots=boots_color;
	our_actor->shirt=shirt_color;
	our_actor->stand_idle=0;
	our_actor->sit_idle=0;

#ifdef ATTACHED_ACTORS
	our_actor->attached_actor = -1;
	our_actor->attachment_shift[0] = our_actor->attachment_shift[1] = our_actor->attachment_shift[2] = 0.0;
#endif // ATTACHED_ACTORS

	for (i = 0; i < NUM_BUFFS; i++)
	{
		our_actor->ec_buff_reference[i] = NULL;
	}

#ifdef CLUSTER_INSIDES
	x = (int) (our_actor->x_pos / 0.5f);
	y = (int) (our_actor->y_pos / 0.5f);
	our_actor->cluster = get_cluster (x, y);
#endif

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

#ifdef ATTACHED_ACTORS
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
#ifdef NEW_EYES
		int id = add_actor(attachment_type, actors_defs[attachment_type].skin_name,
						   parent->x_pos, parent->y_pos, parent->z_pos, parent->z_rot, get_actor_scale(parent),
						   0, 0, 0, 0, 0, 0, 0, -1);
#else //NEW_EYES
		int id = add_actor(attachment_type, actors_defs[attachment_type].skin_name,
						   parent->x_pos, parent->y_pos, parent->z_pos, parent->z_rot, get_actor_scale(parent),
						   0, 0, 0, 0, 0, 0, -1);
#endif //NEW_EYES
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

#ifdef VARIABLE_SPEED
		if (attached_actors_defs[attachment_type].actor_type[parent->actor_type].is_holder)
			actors_list[id]->step_duration = actors_defs[attachment_type].step_duration;
		else
			actors_list[id]->step_duration = parent->step_duration;

		if (actors_list[id]->buffs & BUFF_DOUBLE_SPEED)
			actors_list[id]->step_duration /= 2;
#endif // VARIABLE_SPEED

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
#ifdef ENGLISH
				if (use_animation_program)
					set_transformation_buffers(actors_list[id]);
#endif //ENGLISH
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
#endif // ATTACHED_ACTORS

void set_health_color(float percent, float multiplier, float a)
{
	float r,g;

#ifdef FR_VERSION
	r = (1.0f-percent)*2.5f;
	g = (percent/3.0f)*5.0f;
#endif //FR_VERSION

	if(r<0.0f)r=0.0f;
	else if(r>1.0f)r=1.0f;
	if(g<0.0f)g=0.0f;
	else if(g>1.0f)g=1.0f;

	glColor4f(r*multiplier,g*multiplier,0.0f, a);
}

#ifdef ENGLISH
void set_mana_color(float percent, float multiplier, float a)
{
	float c;

	c=0.6f - percent*0.6f;

	if(c<0.0f)c=0.0f;
	else if(c>1.0f)c=1.0f;

	glColor4f(c,c,2.0f, a);
}
#endif //ENGLISH

#ifdef SELECT_WITH_MOUSE_ON_BANNER
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
#endif //SELECT_WITH_MOUSE_ON_BANNER

void draw_actor_banner(actor * actor_id, float offset_z)
{
#ifdef FR_VERSION
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
#ifdef DISPLAY_MANAPOINT
            if((view_mp || view_mana_bar) && !actor_id->dead && actor_id->kind_of_actor!=NPC && me->actor_id==actor_id->actor_id){
                draw_ortho_ingame_string(hx-(((float)get_string_width(str) * (font_scale2*0.17*name_zoom)))*0.5f, a_bounce+hy+14+10.0f, 0, str, 1, font_scale2*.14, font_scale2*.21);
            }else{
                draw_ortho_ingame_string(hx-(((float)get_string_width(str) * (font_scale2*0.17*name_zoom)))*0.5f, a_bounce+hy+10.0f, 0, str, 1, font_scale2*.14, font_scale2*.21);
            }
#else //DISPLAY_MANAPOINT
			draw_ortho_ingame_string(hx-(((float)get_string_width(str) * (font_scale2*0.17*name_zoom)))*0.5f, a_bounce+hy+10.0f, 0, str, 1, font_scale2*.14, font_scale2*.21);
#endif //DISPLAY_MANAPOINT
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

#ifdef DISPLAY_MANAPOINT
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
#endif //DISPLAY_MANAPOINT

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

#ifdef FR_VERSION
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
#endif //FR_VERSION

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
#ifdef FR_AFFICHE_NOM
		safe_snprintf((char*)name, sizeof(name), "%s", actor_id->nom_acteur_affiche);
#else //FR_AFFICHE_NOM
		safe_snprintf((char*)name, sizeof(name), "%s", actor_id->actor_name);
#endif //FR_AFFICHE_NOM
		draw_ortho_ingame_string(hx-(largeur_nom/2.0f), hy+banner_height, hz, name, 1, font_size_x, font_size_y);
		if (largeur_nom > banner_width) banner_width = largeur_nom;
		banner_height+= hauteur_nom;
		set_font(0);


		if (view_buffs) draw_buffs(actor_id->actor_id, hx, hy, hz);
	}

	// -------------------------------------------------------------------- FOND

#ifdef DISPLAY_MANAPOINT
    if((view_mp || view_mana_bar) && me && me->actor_id==actor_id->actor_id){
        hy-=ALT_INGAME_FONT_Y_LEN * 12.0 * name_zoom * font_scale;
        banner_height+=ALT_INGAME_FONT_Y_LEN * 12.0 * name_zoom * font_scale;
    }
#endif //DISPLAY_MANAPOINT
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
#ifdef SELECT_WITH_MOUSE_ON_BANNER
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
#endif //SELECT_WITH_MOUSE_ON_BANNER

#ifdef DISPLAY_MANAPOINT
    if((view_mp || view_mana_bar) && me && me->actor_id==actor_id->actor_id){
        hy+=ALT_INGAME_FONT_Y_LEN * 12.0 * name_zoom * font_scale;
        banner_height-=ALT_INGAME_FONT_Y_LEN * 12.0 * name_zoom * font_scale;
    }
#endif //DISPLAY_MANAPOINT

	// -------------------------------------------------------------------------
#else //FR_VERSION
	unsigned char str[60];
	unsigned char temp[255];
	GLdouble model[16],proj[16];
	GLint view[4];

	GLdouble hx,hy,hz,a_bounce;
	float font_scale = 1.0f/ALT_INGAME_FONT_X_LEN;
	double healthbar_x=0.0f;
	double healthbar_y=0.0f;
	double healthbar_z=offset_z+0.1;
	double health_str_x_len=ALT_INGAME_FONT_X_LEN*12.0*name_zoom*3*font_scale;
	double healthbar_x_len_converted=0;
	double healthbar_x_len_loss=0;
	double healthbar_x_loss_fade=1.0f;

	//we use health bar variables if possible, all the extras we need for ether bar are:
	double ether_str_x_len = 0;
	double etherbar_x_len_converted=0;
	GLdouble ey;

	//some general values valid for whole banner
	double bar_x_len = 0;
	double bar_y_len=ALT_INGAME_FONT_Y_LEN*12.0*name_zoom*font_scale;
	float banner_width = 0.0f;
	int num_lines;

	//define inner display_xxxxx variables to have more control over displaying inside this function
	//necesary to implement instance mode makes code a bit more easy to understand imho
	int display_hp = view_hp;
	int display_names = view_names;
	int display_health_bar = view_health_bar;
	int display_ether_bar = view_ether_bar;
	int display_ether = view_ether;
	int display_banner_alpha = use_alpha_banner;

	//some general info about "what's going on" - allows not to repeat complex conditions later
	int displaying_me = 0;
	int displaying_other_player = 0;
	int display_health_line = 0;
	int display_ether_line = 0;

	//if first person, dont draw banner
	actor *me = get_our_actor();
	if (me && me->actor_id==actor_id->actor_id) {
		displaying_me = 1;
	};
	if (displaying_me && first_person) return;

	//if not drawing me, can't display ether and ether bar
	if (!displaying_me) {
		display_ether_bar = 0;
		display_ether = 0;
	}

	//if instance mode enabled, overwrite default view banner view options according to it
	if (view_mode_instance) {
		//for my banner - use standard banner settings
		if (!actor_id->is_enhanced_model) {
			//creatures
			display_hp = im_creature_view_hp;
			display_names = im_creature_view_names;
			display_health_bar = im_creature_view_hp_bar;
			display_banner_alpha = im_creature_banner_bg;
		//TODO: it shows healthbar above mule & summons too - probably no way to solve this issue
		} else if (!displaying_me && actor_id->is_enhanced_model){
			//other players
			displaying_other_player = (actor_id->kind_of_actor != NPC);
			display_hp = im_other_player_view_hp;
			display_names = im_other_player_view_names;
			display_health_bar = im_other_player_view_hp_bar;
			display_banner_alpha = im_other_player_banner_bg;
		}
	}

	//Figure out where the point just above the actor's head is in the viewport
	//See if Projection and viewport can be saved elsewhere to prevent doing this so often
	//MODELVIEW is hopeless
	glGetDoublev(GL_MODELVIEW_MATRIX, model);
	glGetDoublev(GL_PROJECTION_MATRIX, proj);
	glGetIntegerv(GL_VIEWPORT, view);
	// Input adjusted healthbar_y value to scale hy according to actor scale
	gluProject(healthbar_x, healthbar_y, healthbar_z * actor_id->scale * actors_defs[actor_id->actor_type].actor_scale + 0.02, model, proj, view, &hx, &hy, &hz);

	//Save World-view and Projection matrices to allow precise raster placement of quads
	glPushMatrix();
	glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	//Don't forget that the viewport is expressed in X+W,Y+H, or point-displacement,
	//versus the Ortho projection which expects x1,x2,y1,y2, or absolute coordinates
	glOrtho(view[0],view[2]+view[0],view[1],view[3]+view[1],0.0f,-1.0f);

	glColor3f (1.0f, 0.0f, 0.0f);

	glDepthFunc(GL_ALWAYS);
	if(actor_id->damage_ms){
		if(floatingmessages_enabled){
			float a=(float)(cur_time-actor_id->last_health_loss)/2000.0f;
			if(actor_id->damage>0){
				sprintf((char*)str,"%i",actor_id->damage);
				glColor4f(1.0f, 0.1f, 0.2f, 1.0-(a*a));
			} else {
				sprintf((char*)str,"%i",-actor_id->damage);
				glColor4f(0.3f, 1.0f, 0.3f, 1.0-(a*a));
			}

			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE);

			//Make damage numbers bounce on the actor's head. Owie!
			a*=2000;
			a_bounce=0;
			if (a < 500){
				a_bounce = 50.0 - 0.0002 * powf(a,2);
			} else if ( a < 950.0){
				a_bounce = 0.09*(a-500.0) - .0002 * powf((a-500.0), 2);
			} else if ( a < 1355.0 ){
				a_bounce = 0.081*(a-950.0) - .0002 * powf((a-950.0), 2);
			} else if ( a < 1720 ){
				a_bounce = 0.0730*(a-1355.0) - .0002 * powf((a-1355.0), 2);
			} else {
				a_bounce = 0.0640*(a-1720.0) - .0002 * powf((a-1720.0), 2);
			}
			/* Schmurk: actually we never reach this code as long as there's
             * an exit condition at the beginning of the function */
			if ((first_person)&&(actor_id->actor_id==yourself)){
				float x,y;
				x = window_width/2.0 -(((float)get_string_width(str) * (font_scale*0.17*name_zoom)))*0.5f;
				y = a_bounce + window_height/2.0-40.0;
				draw_ortho_ingame_string(x, y, 0, str, 1, font_scale*.14, font_scale*.21);
			}
			else
			{
				float font_scale2 = font_scale*powf(1.0f+((float)abs(actor_id->damage)/2.0f)/1000.0f, 4.0);
				draw_ortho_ingame_string(hx-(((float)get_string_width(str) * (font_scale2*0.17*name_zoom)))*0.5f, a_bounce+hy+10.0f, 0, str, 1, font_scale2*.14, font_scale2*.21);
			}			glDisable(GL_BLEND);
		}
		else
		{	//No floating messages
			sprintf((char*)str,"%i",actor_id->damage);
			glColor3f (1.0f, 0.3f, 0.3f);
			DRAW_ORTHO_INGAME_NORMAL(-0.1f,healthbar_z/2.0f,0,str,1.0f);
		}
		if (view_mode_instance && im_other_player_show_banner_on_damage && displaying_other_player && !display_hp && !display_health_bar && actor_id->damage>0) {
			display_hp = 1;
			display_names = 1;
		}
	}

	glDepthFunc(GL_LESS);

	//figure out which lines should we display and how many lines total do we show
	display_health_line = (actor_id->kind_of_actor != NPC && (display_hp || display_health_bar) && actor_id->cur_health > 0 && actor_id->max_health > 0);
	display_ether_line = ((display_ether || display_ether_bar) && displaying_me && your_info.ethereal_points.base > 0 );
	num_lines = display_names + display_health_line + display_ether_line;
	if (view_mode_instance && displaying_me) {
		//make your bar a bit more above everything else so you can see it good enough
		//and got no problems with attacking mobs
		hy += view_mode_instance_banner_height*bar_y_len;
	} else if (displaying_me && display_health_line && display_ether_line) {
		hy += 1.5*bar_y_len;
	}

	//calculate "y" positions of ether lines
	ey = hy -(display_health_line * bar_y_len);

	// Schmurk: same here, we actually never reach this code
	if (!((first_person)&&(actor_id->actor_id==yourself)))
	{
		if(actor_id->actor_name[0] && (display_names || display_health_line || display_ether_line)){
			set_font(name_font);	// to variable length

			if(display_names){
				float font_size_x=font_scale*SMALL_INGAME_FONT_X_LEN;
				float font_size_y=font_scale*SMALL_INGAME_FONT_Y_LEN;

				if(actor_id->kind_of_actor==NPC){
					glColor3f(0.3f,0.8f,1.0f);
				} else if(actor_id->kind_of_actor==HUMAN || actor_id->kind_of_actor==COMPUTER_CONTROLLED_HUMAN){
					if(map_type == 2){
						glColor3f(0.6f,0.9f,0.9f);
					} else {
						glColor3f(1.0f,1.0f,1.0f);
					}
				} else if(actor_id->is_enhanced_model && (actor_id->kind_of_actor==PKABLE_HUMAN || actor_id->kind_of_actor==PKABLE_COMPUTER_CONTROLLED)){
					glColor3f(1.0f,0.0f,0.0f);
				} else {
					glColor3f(1.0f,1.0f,0.0f);
				}
				safe_snprintf ((char*)temp, sizeof (temp), "%s", actor_id->actor_name);
				banner_width = ((float)get_string_width((unsigned char*)actor_id->actor_name)*(font_size_x*name_zoom))/2.0;
				draw_ortho_ingame_string(hx-banner_width, hy+bar_y_len/2.0f, hz, temp, 1, font_size_x, font_size_y);
			}
			if (view_buffs)
			{
				draw_buffs(actor_id->actor_id, hx, hy, hz);
			}

			if(  (!actor_id->dead) && (actor_id->kind_of_actor != NPC) && (display_health_line || display_ether_line)){
				unsigned char hp[200];
				unsigned char mana[200];

				// make the heath bar the same length as the the health text so they are balanced
				// use the same length health bar, even if not displaying the health text
				sprintf((char*)hp,"%u/%u", actor_id->cur_health, actor_id->max_health);
				health_str_x_len = (float)get_string_width(hp)*(ALT_INGAME_FONT_X_LEN*name_zoom*font_scale);
				//do the same with mana if we want to display it
				if (display_ether || display_ether_bar) {
					sprintf((char*)mana,"%u/%u", your_info.ethereal_points.cur, your_info.ethereal_points.base);
					ether_str_x_len=(float)get_string_width(mana)*(ALT_INGAME_FONT_X_LEN*name_zoom*font_scale);
				}
				//set bar length to longer one (mana or health) - not really clean solution
				if (ether_str_x_len > health_str_x_len) {
					bar_x_len = ether_str_x_len;
				} else {
					bar_x_len = health_str_x_len;
				}

				if (display_hp || display_ether) {
					float hp_off=(bar_x_len - health_str_x_len)/2.0;
					float eth_off=(bar_x_len - ether_str_x_len)/2.0;
					float disp;
					disp=(bar_x_len/2.0);

					if(display_health_bar){
						hp_off+=5.0+disp;
                    }
					if(display_ether_bar){
						eth_off+=5.0+disp;
                    }
					if (display_hp && (disp+hp_off > banner_width)) {
						banner_width = disp + hp_off;
                    }
					if (display_ether && (disp+eth_off > banner_width)) {
						banner_width = disp + eth_off;
                    }

					if (display_hp) {
						//choose color for the health
						set_health_color((float)actor_id->cur_health/(float)actor_id->max_health, 1.0f, 1.0f);
						draw_ortho_ingame_string(hx-disp+hp_off, hy-bar_y_len/3.0f, hz, hp, 1, ALT_INGAME_FONT_X_LEN*font_scale, ALT_INGAME_FONT_Y_LEN*font_scale);
                    }

					if (display_ether) {
						set_mana_color((float)your_info.ethereal_points.cur / (float)your_info.ethereal_points.base, 1.0f, 1.0f);
						draw_ortho_ingame_string(hx-disp+eth_off, ey-bar_y_len/3.0f, hz, mana, 1, ALT_INGAME_FONT_X_LEN*font_scale, ALT_INGAME_FONT_Y_LEN*font_scale);
                    }
				}
			}

			set_font(0);	// back to fixed pitch
		}
	}

	//draw the health bar
	glDisable(GL_TEXTURE_2D);

	if(display_health_bar && display_health_line && (!actor_id->dead) && (actor_id->kind_of_actor != NPC)){
		float percentage = (float)actor_id->cur_health/(float)actor_id->max_health;
		float off;

		if(percentage>110.0f) //deal with massive bars by trimming at 110%
			percentage = 110.0f;
		if (display_hp){
			off = bar_x_len + 5.0f;
		} else {
			off = bar_x_len / 2.0f;
		}

		if(actor_id->last_health_loss && cur_time-actor_id->last_health_loss<1000){//only when using floatingmessages
			if(actor_id->damage>0){
				healthbar_x_len_converted=bar_x_len*percentage;
				healthbar_x_len_loss=bar_x_len*(float)((float)actor_id->damage/(float)actor_id->max_health);
				healthbar_x_loss_fade=1.0f-((float)(cur_time-actor_id->last_health_loss)/1000.0f);
			} else {
				healthbar_x_len_converted=bar_x_len*(float)((float)(actor_id->cur_health+actor_id->damage)/(float)actor_id->max_health);
				healthbar_x_len_loss=bar_x_len*(float)((float)(-actor_id->damage)/(float)actor_id->max_health);
				healthbar_x_loss_fade=((float)(cur_time-actor_id->last_health_loss)/1000.0f);
			}
		} else {
			healthbar_x_len_converted=bar_x_len*percentage;
			actor_id->last_health_loss=0;
		}

		if (bar_x_len / 2.0f > banner_width) {
			banner_width = bar_x_len / 2.0f;
		}

		hx-=off;

		//choose tint color
		set_health_color(percentage, 0.5f, 1.0f);
		glBegin(GL_QUADS);
			glVertex3d(hx,hy,hz);
			glVertex3d(hx+healthbar_x_len_converted,hy,hz);

		set_health_color(percentage, 1.0f, 1.0f);

			glVertex3d(hx+healthbar_x_len_converted,hy+bar_y_len/3.0,hz);
			glVertex3d(hx,hy+bar_y_len/3.0,hz);
		glEnd();

		if(healthbar_x_len_loss){
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE);

			set_health_color(percentage, 0.5f, healthbar_x_loss_fade);

			glBegin(GL_QUADS);
				glVertex3d(hx+healthbar_x_len_converted, hy, hz);
				glVertex3d(hx+healthbar_x_len_converted+healthbar_x_len_loss, hy, hz);

			set_health_color(percentage, 1.0f, healthbar_x_loss_fade);

				glVertex3d(hx+healthbar_x_len_converted+healthbar_x_len_loss, hy+bar_y_len/3.0,hz);
				glVertex3d(hx+healthbar_x_len_converted, hy+bar_y_len/3.0,hz);
			glEnd();

			glDisable(GL_BLEND);
		}


		//draw the frame
		glDepthFunc(GL_LEQUAL);
		glColor3f (0.0f, 0.0f, 0.0f);
		glBegin(GL_LINE_LOOP);
			glVertex3f (hx-1.0, hy-1.0, hz);
			glVertex3f (hx+bar_x_len+1.0, hy-1.0,hz);
			glVertex3f (hx+bar_x_len+1.0, hy+bar_y_len/3.0+1.0,hz);
			glVertex3f (hx-1.0, hy+bar_y_len/3.0+1.0,hz);
		glEnd();

		hx+=off;
	}

	if (display_ether_bar && display_ether_line) {
		float percentage = (float)your_info.ethereal_points.cur / (float)your_info.ethereal_points.base;
		float off;
		if(percentage>110.0f) //deal with massive bars by trimming at 110%
			percentage = 110.0f;
		if (display_ether){
			off = bar_x_len + 5.0f;
		} else {
			off = bar_x_len / 2.0f;
		}
		if (bar_x_len / 2.0f > banner_width) {
			banner_width = bar_x_len / 2.0f;
		}
		hx-=off;

		set_mana_color(percentage, 0.5f, 1.0f);
		etherbar_x_len_converted = percentage * bar_x_len;
		glBegin(GL_QUADS);
			glVertex3d(hx,ey,hz);
			glVertex3d(hx+etherbar_x_len_converted,ey,hz);

		set_mana_color(percentage, 1.0f, 1.0f);

			glVertex3d(hx+etherbar_x_len_converted,ey+bar_y_len/3.0,hz);
			glVertex3d(hx,ey+bar_y_len/3.0,hz);
		glEnd();
		set_health_color(percentage, 1.0f, 1.0f);
		glDepthFunc(GL_LEQUAL);
		glColor3f (0.0f, 0.0f, 0.0f);
		glBegin(GL_LINE_LOOP);
			glVertex3f (hx-1.0, ey-1.0 , hz);
			glVertex3f (hx+bar_x_len+1.0, ey-1.0,hz);
			glVertex3f (hx+bar_x_len+1.0, ey+bar_y_len/3.0+1.0,hz);
			glVertex3f (hx-1.0, ey+bar_y_len/3.0+1.0,hz);
		glEnd();
		hx+=off;
	}

	// draw the alpha background (if ness)
	if (display_banner_alpha && banner_width > 0) {
		//if banner width > 0 there MUST be something displayed in the banner
		float start_y = hy;
		start_y  += ((!display_health_line && !display_ether_line && display_names) ?bar_y_len-6.0 :-5.0);
		start_y  -= (num_lines == 3 || (num_lines==2 && !display_names)) ? bar_y_len:0.0;
		banner_width += 3;
		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE, GL_SRC_ALPHA);
		glColor4f(0.0f, 0.0f, 0.0f, 0.6f);
		glBegin(GL_QUADS);
			glVertex3f (hx-banner_width, start_y, hz + 0.0001);
			glVertex3f (hx+banner_width, start_y, hz + 0.0001);
			glVertex3f (hx+banner_width, start_y+bar_y_len*num_lines+2, hz + 0.0001);
			glVertex3f (hx-banner_width, start_y+bar_y_len*num_lines+2, hz + 0.0001);
		glEnd();
		glDisable(GL_BLEND);
	}

	glEnable(GL_TEXTURE_2D);

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

	if ((actor_id->current_displayed_text_time_left>0)&&(actor_id->current_displayed_text[0] != 0)){
		draw_actor_overtext( actor_id );
	}

	if(floatingmessages_enabled)drawactor_floatingmessages(actor_id->actor_id, healthbar_z);

	/* set cm_mouse_over_banner true if the mouse is over your banner, or a box where it might be */
	if (actor_id->actor_id == yourself)
	{
		/* use the same calculation as for the alpha background but have a fallback if no banner shown */
		int xoff = (banner_width > 0) ?banner_width: 60;
		float start_y = hy;
		start_y  += ((!display_health_line && !display_ether_line && display_names) ?bar_y_len-6.0 :-5.0);
		start_y  -= (num_lines == 3 || (num_lines==2 && !display_names)) ? bar_y_len:0.0;
		if ((mouse_x > hx-xoff) && (mouse_x < hx+xoff) &&
			(window_height-mouse_y > start_y) && (window_height-mouse_y < start_y+bar_y_len*((num_lines>0)?num_lines:3)))
			cm_mouse_over_banner = 1;
		else
			cm_mouse_over_banner = 0;
	}

#endif //FR_VERSION
	glColor3f(1,1,1);
#ifdef OPENGL_TRACE
CHECK_GL_ERRORS();
#endif //OPENGL_TRACE
}


void draw_bubble(float x_left, float x_right, float x_leg_left, float x_leg_right, float y_top, float y_bottom, float y_actor)
{
#ifdef FR_VERSION
	const float r=zoom_level/20;
#else //FR_VERSION
	const float r=0.1f;
#endif //FR_VERSION
	const float mul=M_PI/180.0f;
	int angle;

	glEnable(GL_BLEND);
#ifdef FR_VERSION
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	glColor4f(1.0f, 1.0f, 1.0f, 0.75f);
#else //FR_VERSION
	glColor4f(1.0f, 1.0f, 1.0f, 0.5f);
	glBlendFunc(GL_NONE, GL_SRC_ALPHA);
#endif //FR_VERSION
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
#ifdef OPENGL_TRACE
CHECK_GL_ERRORS();
#endif //OPENGL_TRACE
}

//-- Logan Dugenoux [5/26/2004]
void draw_actor_overtext( actor* actor_ptr )
{
	float z, w, h;
	float x_left, x_right, x_leg_left, x_leg_right, y_top, y_bottom, y_actor;
	float textwidth;
	float textheight;
	float margin;
#ifdef FR_VERSION
	float maxwidth = 0;
	float nb_ligne_bulle = 0;
	int index, index2;
	char * position;
	char ligne1[MAX_CURRENT_DISPLAYED_TEXT_LEN+1] = "";
	char ligne2[MAX_CURRENT_DISPLAYED_TEXT_LEN+1] = "";
	char ligne3[MAX_CURRENT_DISPLAYED_TEXT_LEN+1] = "";
#endif //FR_VERSION

	//-- decrease display time
	actor_ptr->current_displayed_text_time_left -= (cur_time-last_time);
	if(!(SDL_GetAppState()&SDL_APPACTIVE)) return;	// not actually drawing, fake it

#ifdef FR_VERSION
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
#else //FR_VERSION
	textwidth = ((float)get_string_width((unsigned char*)(actor_ptr->current_displayed_text))*(SMALL_INGAME_FONT_X_LEN*zoom_level*name_zoom/3.0))/12.0;
	textheight = (0.06f*zoom_level/3.0)*4;
	margin = 0.02f*zoom_level;
	z = 1.2f;// distance over the player
	if (actor_ptr->sitting)		z = 0.8f; // close if he's sitting
	w = textwidth+margin*2;
	h = textheight+margin*2;

	x_left=-w/2.0f;
	x_right=w/2.0f;
	x_leg_left=-0.3f;
	x_leg_right=0.0f;

	y_top=z+0.7f+h;
	y_bottom=z+0.7f;
	y_actor=z+0.2f;
#endif //FR_VERSION

	glDisable(GL_TEXTURE_2D);

	draw_bubble(x_left+0.01f, x_right-0.01f, x_leg_left, x_leg_right, y_top-0.01f, y_bottom+0.01f, y_actor+0.01f);

	glEnable(GL_TEXTURE_2D);

	//---
	// Draw text
#ifdef FR_VERSION
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
#else //FR_VERSION
	glColor3f(0.77f,0.57f,0.39f);

	DRAW_INGAME_SMALL(x_left+margin, y_bottom+margin,actor_ptr->current_displayed_text,1);
#endif //FR_VERSION

	//glDepthFunc(GL_LESS);
	if (actor_ptr->current_displayed_text_time_left<=0)
	{	// clear if needed
		actor_ptr->current_displayed_text_time_left = 0;
		actor_ptr->current_displayed_text[0] = 0;
	}
#ifdef OPENGL_TRACE
CHECK_GL_ERRORS();
#endif //OPENGL_TRACE
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
#ifdef	NEW_TEXTURES
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
#else	/* NEW_TEXTURES */
		if (actor_id->is_enhanced_model)
		{
			bind_texture_id(actor_id->texture_id);
		}
		else
		{
			if (!actor_id->remapped_colors)
			{
				get_and_set_texture_id(actor_id->texture_id);
			}
			else
			{
				bind_texture_id(actor_id->texture_id);
			}
		}
#endif	/* NEW_TEXTURES */
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

#ifdef ATTACHED_ACTORS
	if (actor_id->attached_actor >= 0)
		glTranslatef(actor_id->attachment_shift[0], actor_id->attachment_shift[1], actor_id->attachment_shift[2]);
#endif // ATTACHED_ACTORS

#ifdef ENGLISH
	if (use_animation_program)
	{
		cal_render_actor_shader(actor_id, use_lightning, use_textures, use_glow);
	}
	else
	{
		cal_render_actor(actor_id, use_lightning, use_textures, use_glow);
	}
#else //ENGLISH
	cal_render_actor(actor_id, use_lightning, use_textures, use_glow);
#endif //ENGLISH

	//now, draw their damage & nametag
	glPopMatrix();  // restore the matrix

#ifdef OPENGL_TRACE
CHECK_GL_ERRORS();
#endif //OPENGL_TRACE
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

#ifdef ATTACHED_ACTORS
	if (actor_id->attached_actor >= 0)
	{
		glRotatef(180 - actor_id->z_rot, 0.0f, 0.0f, 1.0f);
		glTranslatef(actor_id->attachment_shift[0], actor_id->attachment_shift[1], actor_id->attachment_shift[2]);
		glRotatef(180 - actor_id->z_rot, 0.0f, 0.0f, -1.0f);
	}
#endif // ATTACHED_ACTORS

	glRotatef(-rz, 0.0f, 0.0f, 1.0f);

	draw_actor_banner(actor_id, healthbar_z);

	glPopMatrix();	//we don't want to affect the rest of the scene
#ifdef OPENGL_TRACE
CHECK_GL_ERRORS();
#endif //OPENGL_TRACE
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
#ifdef NEW_SOUND
	unsigned int tmp_nr_enh_act;		// Use temp variables to stop crowd sound interference during count
	float tmp_dist_to_nr_enh_act;
#endif // NEW_SOUND
	actor *me;
	AABBOX bbox;

	me = get_our_actor ();

	if (!me) return;

	no_near_actors = 0;
#ifdef NEW_SOUND
	tmp_nr_enh_act = 0;
	tmp_dist_to_nr_enh_act = 0;
#endif // NEW_SOUND

	set_current_frustum(get_cur_intersect_type(main_bbox_tree));

	for (i = 0; i < max_actors; i++)
	{
		if(actors_list[i]
#ifdef CLUSTER_INSIDES
		   && (actors_list[i]->cluster == me->cluster || actors_list[i]->cluster == 0)
#endif
		)
		{
#ifdef ATTACHED_ACTORS
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
#else // ATTACHED_ACTORS
			pos[X] = actors_list[i]->x_pos;
			pos[Y] = actors_list[i]->y_pos;
			pos[Z] = actors_list[i]->z_pos;
#endif // ATTACHED_ACTORS
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

#ifdef	VERTEX_PROGRAM_ACTOR_ANIMATION_DEBUG
			if (aabb_in_frustum(bbox) || (!use_actor_bbox_check))
#else	/* VERTEX_PROGRAM_ACTOR_ANIMATION_DEBUG */
			if (aabb_in_frustum(bbox))
#endif	/* VERTEX_PROGRAM_ACTOR_ANIMATION_DEBUG */
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
#ifdef NEW_SOUND
				if (actors_list[i]->is_enhanced_model && actors_list[i]->actor_id != me->actor_id)
				{
					tmp_nr_enh_act++;
					tmp_dist_to_nr_enh_act += ((me->x_pos - actors_list[i]->x_pos) *
														(me->x_pos - actors_list[i]->x_pos)) +
														((me->y_pos - actors_list[i]->y_pos) *
														(me->y_pos - actors_list[i]->y_pos));
				}
#endif // NEW_SOUND
			}
		}
	}
#ifdef NEW_SOUND
	if (tmp_nr_enh_act > 0)
		tmp_dist_to_nr_enh_act = tmp_dist_to_nr_enh_act / tmp_nr_enh_act;
	no_near_enhanced_actors = tmp_nr_enh_act;
	distanceSq_to_near_enhanced_actors = tmp_dist_to_nr_enh_act;
#endif // NEW_SOUND
	qsort(near_actors, no_near_actors, sizeof(near_actor), comp_actors);
}

void display_actors(int banner, int render_pass)
{
	Sint32 i, has_alpha, has_ghosts;
	Uint32 use_lightning = 0, use_textures = 0;

	get_actors_in_range();

#ifdef ENGLISH
    // Ackak : si on active cette fonction, vu que l'on utilise
    // d'ancien modele, les capes par exemple vont avoir un
    // cote invisible
	glEnable(GL_CULL_FACE);
#endif //ENGLISH

#ifdef	VERTEX_PROGRAM_ACTOR_ANIMATION_DEBUG
	if (!use_display_actors)
	{
		return;
	}
#endif	/* VERTEX_PROGRAM_ACTOR_ANIMATION_DEBUG */
#ifdef ENGLISH
	if (use_animation_program)
	{
		set_actor_animation_program(render_pass, 0);
	}
#endif //ENGLISH

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

#ifdef	FSAA
	if (fsaa > 1)
	{
		glEnable(GL_MULTISAMPLE);
	}
#endif	/* FSAA */
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
#ifdef ENGLISH
		if (use_animation_program)
		{
			set_actor_animation_program(render_pass, 1);
		}
#endif //ENGLISH

		for (i = 0; i < no_near_actors; i++)
		{

			if (near_actors[i].ghost || (near_actors[i].buffs & BUFF_INVISIBILITY))
			{

				actor *cur_actor = actors_list[near_actors[i].actor];
				if (cur_actor)
				{
					//if any ghost has a glowing weapon, we need to reset the blend function each ghost actor.
					glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

#ifdef ENGLISH
					if (!use_animation_program)
					{
#endif //ENGLISH
						if ((near_actors[i].buffs & BUFF_INVISIBILITY))
						{
							glColor4f(1.0f, 1.0f, 1.0f, 0.25f);
						}
						else
						{
							glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
						}
#ifdef ENGLISH
					}
#endif //ENGLISH

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
#ifdef	FSAA
	if (fsaa > 1)
	{
		glDisable(GL_MULTISAMPLE);
	}
#endif	/* FSAA */

#ifdef ENGLISH
	if (use_animation_program)
	{
		disable_actor_animation_program();
	}
#endif //ENGLISH

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
#ifdef ATTACHED_ACTORS
				&& cur_actor->actor_id >= 0
#endif // ATTACHED_ACTORS
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
#ifdef OPENGL_TRACE
CHECK_GL_ERRORS();
#endif //OPENGL_TRACE
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
#ifdef FR_VERSION
	Uint8 actor_type;
#else //FR_VERSION
	short actor_type;
#endif //FR_VERSION
	Uint8 frame;
	int i;
	int dead=0;
	int kind_of_actor;

	double f_x_pos,f_y_pos,f_z_rot;
	float scale= 1.0f;
#ifdef EMOTES
	emote_data *pose=NULL;
#endif
#ifdef ATTACHED_ACTORS
	int attachment_type = -1;
#endif // ATTACHED_ACTORS

	actor_id=SDL_SwapLE16(*((short *)(in_data)));
#ifndef EL_BIG_ENDIAN
	buffs=((*((char *)(in_data+3))>>3)&0x1F) | (((*((char*)(in_data+5))>>3)&0x1F)<<5);	// Strip the last 5 bits of the X and Y coords for the buffs
	x_pos=*((short *)(in_data+2)) & 0x7FF;
	y_pos=*((short *)(in_data+4)) & 0x7FF;
#else
	buffs=((SDL_SwapLE16(*((char*)(in_data+3)))>>3)&0x1F | (SDL_SwapLE16(((*((char*)(in_data+5)))>>3)&0x1F)<<5));	// Strip the last 5 bits of the X and Y coords for the buffs
	x_pos=SDL_SwapLE16(*((short *)(in_data+2))) & 0x7FF;
	y_pos=SDL_SwapLE16(*((short *)(in_data+4))) & 0x7FF;
#endif //EL_BIG_ENDIAN
	buffs |= (SDL_SwapLE16(*((short *)(in_data+6))) & 0xFF80) << 3; // we get the 9 MSB for the buffs and leave the 7 LSB for a further use
	z_rot=SDL_SwapLE16(*((short *)(in_data+8)));
	actor_type=*(in_data+10);

	frame=*(in_data+11);
	max_health=SDL_SwapLE16(*((short *)(in_data+12)));
	cur_health=SDL_SwapLE16(*((short *)(in_data+14)));
	kind_of_actor=*(in_data+16);
	if(len > 17+(int)strlen(in_data+17)+2){
		scale=((float)SDL_SwapLE16(*((short *)(in_data+17+strlen(in_data+17)+1)))/((float)ACTOR_SCALE_BASE));

#ifdef ATTACHED_ACTORS
		if(len > 17+(int)strlen(in_data+17)+3)
			attachment_type = (unsigned char)in_data[17+strlen(in_data+17)+3];
#endif // ATTACHED_ACTORS
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
#ifdef FR_VERSION
    case frame_salut:
#endif //FR_VERSION
		break;
	default:
		{
#ifdef EMOTES
		if(frame>=frame_poses_start&&frame<=frame_poses_end) {
			//we have a pose, get it! (frame is the emote_id)
			hash_entry *he;
			he=hash_get(emotes,(void*)(NULL+frame));
			if(!he) LOG_ERROR("unknown pose %d", frame);
			else pose = he->item;
			break;
		}
#endif

			LOG_ERROR("%s %d - %s\n", unknown_frame, frame, &in_data[17]);
		}
	}

#ifdef EXTRA_DEBUG
	ERR();
#endif

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

#ifdef NEW_EYES
	i= add_actor(actor_type, actors_defs[actor_type].skin_name, f_x_pos, f_y_pos, 0.0, f_z_rot, scale, 0, 0, 0, 0, 0, 0, 0, actor_id);
#else //NEW_EYES
	i= add_actor(actor_type, actors_defs[actor_type].skin_name, f_x_pos, f_y_pos, 0.0, f_z_rot, scale, 0, 0, 0, 0, 0, 0, actor_id);
#endif //NEW_EYES

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

#ifdef VARIABLE_SPEED
    actors_list[i]->step_duration = actors_defs[actor_type].step_duration;
	if (actors_list[i]->buffs & BUFF_DOUBLE_SPEED)
		actors_list[i]->step_duration /= 2;
#endif // VARIABLE_SPEED

	actors_list[i]->z_pos = get_actor_z(actors_list[i]);
#ifdef EMOTES
	if(frame==frame_sit_idle||(pose!=NULL&&pose->pose==EMOTE_SITTING)){ //sitting pose sent by the server
			actors_list[i]->poses[EMOTE_SITTING]=pose;
			actors_list[i]->sitting=1;
		}
	else if(frame==frame_stand||(pose!=NULL&&pose->pose==EMOTE_STANDING)){//standing pose sent by server
			actors_list[i]->poses[EMOTE_STANDING]=pose;
			actors_list[i]->sitting=0;
		}
	else if(frame==frame_walk||(pose!=NULL&&pose->pose==EMOTE_WALKING)){//walking pose sent by server
			actors_list[i]->poses[EMOTE_WALKING]=pose;
		}
	else if(frame==frame_run||(pose!=NULL&&pose->pose==EMOTE_RUNNING)){//running pose sent by server
			actors_list[i]->poses[EMOTE_RUNNING]=pose;
		}
	else
		{
			if(frame==frame_combat_idle)
				actors_list[i]->fighting=1;
			else if (frame == frame_ranged)
				actors_list[i]->in_aim_mode = 1;
		}
#else
	if(frame==frame_sit_idle)actors_list[i]->sitting=1;
	else
		if(frame==frame_combat_idle)actors_list[i]->fighting=1;
#endif
	//ghost or not?
	actors_list[i]->ghost=actors_defs[actor_type].ghost;

	actors_list[i]->dead=dead;
	actors_list[i]->stop_animation=1;//helps when the actor is dead...
	actors_list[i]->kind_of_actor=kind_of_actor;
	if(strlen(&in_data[17]) >= 30)
		{
			LOG_ERROR("%s (%d): %s/%d\n", bad_actor_name_length, actors_list[i]->actor_type,&in_data[17], (int)strlen(&in_data[17]));
		}
#ifdef FR_AFFICHE_NOM
	else
    {
        my_strncp(actors_list[i]->actor_name,&in_data[17],30);
        my_strncp(actors_list[i]->nom_acteur_affiche, &in_data[17],30);
    }
#else //FR_AFFICHE_NOM
	else my_strncp(actors_list[i]->actor_name,&in_data[17],30);
#endif //FR_AFFICHE_NOM

#ifdef ATTACHED_ACTORS
	if (attachment_type >= 0)
		add_actor_attachment(actor_id, attachment_type);
#endif // ATTACHED_ACTORS

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
#ifdef ENGLISH
			if (use_animation_program)
			{
				set_transformation_buffers(actors_list[i]);
			}
#endif //ENGLISH
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
#ifdef EXTRA_DEBUG
	ERR();
#endif

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

#ifdef ATTACHED_ACTORS
	if (in_act->attached_actor >= 0)
	{
		float shift[3];
		MAT3_VECT3_MULT(shift, in_act_rot, in_act->attachment_shift);
		out_pos[0] += shift[0];
		out_pos[1] += shift[1];
		out_pos[2] += shift[2];
	}
#endif // ATTACHED_ACTORS
}
