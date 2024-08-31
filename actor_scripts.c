#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "actor_scripts.h"
#include "actors.h"
#include "asc.h"
#include "cal.h"
#include "cal3d_wrapper.h"
#include "counters.h"
#include "cursors.h"
#include "draw_scene.h"
#include "errors.h"
#include "global.h"
#include "hud.h"
#include "init.h"
#include "interface.h"
#include "new_actors.h"
#include "multiplayer.h"
#include "new_character.h"
#include "particles.h"
#include "pathfinder.h"
#include "platform.h"
#include "skeletons.h"
#include "sound.h"
#include "spells.h"
#include "text.h"
#include "tiles.h"
#include "timers.h"
#include "translate.h"
#include "eye_candy_wrapper.h"
#include "minimap.h"
#include "io/elfilewrapper.h"
#include "io/cal3d_io_wrapper.h"
#include "actor_init.h"
#include "textures.h"
#include "client_serv.h"

const dict_elem skin_color_dict[] =
	{ { "brown"	, SKIN_BROWN	},
	  { "normal", SKIN_NORMAL	},
	  { "pale"	, SKIN_PALE		},
	  { "tan"	, SKIN_TAN		},
	  { "darkblue", SKIN_DARK_BLUE },	// Elf's only
      { "clair", SKIN_EN_CLAIR },
      { "fonce", SKIN_EN_FONCE },
      { "gris", SKIN_EN_GRIS },
      { "medium", SKIN_EN_MEDIUM },
      { "masque_pnj", SKIN_MASQUE_PNJ },
	  { NULL	, -1			}
	};

const dict_elem glow_mode_dict[] =
	{ { "none"   , GLOW_NONE    },
	  { "fire"   , GLOW_FIRE    },
	  { "ice"    , GLOW_COLD    },
	  { "thermal", GLOW_THERMAL },
	  { "magic"  , GLOW_MAGIC   },
	  { NULL     , -1           }
	};

const dict_elem head_number_dict[] =
	{ { "1" ,  HEAD_1 },
	  { "2" ,  HEAD_2 },
	  { "3" ,  HEAD_3 },
	  { "4" ,  HEAD_4 },
	  { "5" ,  HEAD_5 },
	  { NULL, -1      }
	};
int actor_part_sizes[ACTOR_NUM_PARTS] = {10, 40, 100, 100, 240, 100, 10, 20, 40, 80, 20};		// Elements according to actor_parts_enum

//Forward declarations
int cal_load_weapon_mesh (actor_types *act, const char *fn, const char *kind);
int cal_load_mesh(actor_types *act, const char *fn, const char *kind);
void unqueue_cmd(int i);
int parse_actor_sounds(actor_types *act, const xmlNode *cfg);






void cal_actor_set_random_idle(int id)
{
	struct CalMixer *mixer;
	int i;
	int random_anim;
	int random_anim_index;

	if (actors_list[id]->calmodel==NULL) return;
	//LOG_TO_CONSOLE(c_green2,"Randomizing");
	//if (actors_list[id]->cur_anim.anim_index==anim.anim_index) return;
	srand( (unsigned)time( NULL ) );
	mixer=CalModel_GetMixer(actors_list[id]->calmodel);
	//Stop previous animation if needed
	if (actors_list[id]->IsOnIdle!=1){
		if ((actors_list[id]->cur_anim.anim_index!=-1)&&(actors_list[id]->cur_anim.kind==0)) {
			CalMixer_ClearCycle(mixer,actors_list[id]->cur_anim.anim_index,0.05);
		}
		if ((actors_list[id]->cur_anim.anim_index!=-1)&&(actors_list[id]->cur_anim.kind==1)) {
			CalMixer_RemoveAction(mixer,actors_list[id]->cur_anim.anim_index);
		}
	}

	for (i=0;i<actors_defs[actors_list[id]->actor_type].group_count;++i) {
		random_anim=rand()%(actors_defs[actors_list[id]->actor_type].idle_group[i].count+1);
		if (random_anim<actors_defs[actors_list[id]->actor_type].idle_group[i].count) random_anim_index=actors_defs[actors_list[id]->actor_type].idle_group[i].anim[random_anim].anim_index;
		else random_anim_index=-1;
		if (actors_list[id]->IsOnIdle==1) {
			if (actors_list[id]->cur_idle_anims[i].anim_index!=random_anim_index)
			CalMixer_ClearCycle(mixer,actors_list[id]->cur_idle_anims[i].anim_index,2.0);
		}
		if (actors_list[id]->cur_idle_anims[i].anim_index!=random_anim_index)
			if (random_anim_index>=0) CalMixer_BlendCycle(mixer,random_anim_index,0.5,0.05);
		//safe_snprintf(str, sizeof(str),"%d",random_anim);
		//LOG_TO_CONSOLE(c_green2,str);
		actors_list[id]->cur_idle_anims[i].anim_index=random_anim_index;
		//anim.anim_index,1.0,0.05);else
	}

	//if (anim.kind==0) CalMixer_BlendCycle(mixer,anim.anim_index,1.0,0.05);else
	//CalMixer_ExecuteAction(mixer,anim.anim_index,0.0,0.0);
	//actors_list[id]->cur_anim=anim;
	//actors_list[id]->anim_time=0.0;
	CalModel_Update(actors_list[id]->calmodel,0.0001);//Make changes take effect now
	build_actor_bounding_box(actors_list[id]);
	actors_list[id]->IsOnIdle= 1;
	actors_list[id]->cur_anim.duration= 0;
	actors_list[id]->anim_time= 0.0;
	actors_list[id]->last_anim_update= cur_time;
	actors_list[id]->cur_anim.anim_index= -1;
	if (check_sound_loops(actors_list[id]->cur_anim_sound_cookie))
		stop_sound(actors_list[id]->cur_anim_sound_cookie);
	actors_list[id]->cur_anim_sound_cookie= 0;
	//if (actors_list[id]->cur_anim.anim_index==-1) actors_list[id]->busy=0;
}


float unwindAngle_Degrees( float fAngle )
{
	fAngle -= 360.0f * (int)( fAngle / 360.0f );
	if( fAngle < 0.0f )
		{
			fAngle += 360.0f;
		}
	return fAngle;
}


float get_rotation_vector( float fStartAngle, float fEndAngle )
{
	float ccw = unwindAngle_Degrees( fStartAngle - fEndAngle );
	float cw = unwindAngle_Degrees( fEndAngle - fStartAngle );
	if(cw<ccw)return cw;
	else return -ccw;
}

int get_motion_vector(int move_cmd, int *dx, int *dy)
{
	int result = 1;
    switch(move_cmd) {
    case move_n:
    case run_n:
        *dx = 0;
        *dy = 1;
        break;
    case move_s:
    case run_s:
        *dx = 0;
        *dy = -1;
        break;
    case move_e:
    case run_e:
        *dx = 1;
        *dy = 0;
        break;
    case move_w:
    case run_w:
        *dx = -1;
        *dy = 0;
        break;
    case move_ne:
    case run_ne:
        *dx = 1;
        *dy = 1;
        break;
    case move_se:
    case run_se:
        *dx = 1;
        *dy = -1;
        break;
    case move_sw:
    case run_sw:
        *dx = -1;
        *dy = -1;
        break;
    case move_nw:
    case run_nw:
        *dx = -1;
        *dy = 1;
        break;

    default:
        *dx = 0;
        *dy = 0;
		result = 0;
        break;
    }
	return result;
}


void animate_actors()
{
	int i;
	static int last_update= 0;
    int time_diff = cur_time-last_update;
    int tmp_time_diff;

	// lock the actors_list so that nothing can interere with this look
	LOCK_ACTORS_LISTS();	//lock it to avoid timing issues
	for(i=0; i<max_actors; i++) {
		if(actors_list[i]) {
			if(actors_list[i]->moving) {
				if (time_diff <= actors_list[i]->movement_time_left+40) {
					actors_list[i]->x_pos += actors_list[i]->move_x_speed*time_diff;
					actors_list[i]->y_pos += actors_list[i]->move_y_speed*time_diff;
					actors_list[i]->z_pos += actors_list[i]->move_z_speed*time_diff;
				}
				else {
					actors_list[i]->x_pos += actors_list[i]->move_x_speed*actors_list[i]->movement_time_left;
					actors_list[i]->y_pos += actors_list[i]->move_y_speed*actors_list[i]->movement_time_left;
					actors_list[i]->z_pos += actors_list[i]->move_z_speed*actors_list[i]->movement_time_left;
				}
                actors_list[i]->movement_time_left -= time_diff;
				if(actors_list[i]->movement_time_left <= 0){	//we moved all the way
					Uint8 last_command;
                    int dx, dy;

					actors_list[i]->moving= 0;	//don't move next time, ok?
					//now, we need to update the x/y_tile_pos, and round off
					//the x/y_pos according to x/y_tile_pos
					last_command= actors_list[i]->last_command;
					//if(HAS_HORSE(i)) {MY_HORSE(i)->busy=0; if(actors_list[i]->actor_id==yourself) printf("%i, %s wakes up Horse\n",thecount, ACTOR(i)->actor_name);}
                    if (get_motion_vector(last_command, &dx, &dy)) {
						actors_list[i]->x_tile_pos += dx;
						actors_list[i]->y_tile_pos += dy;

						actors_list[i]->busy = 0;
						//if(actors_list[i]->actor_id==yourself) printf("%i, unbusy(moved)\n", thecount);
						//if(actors_list[i]->actor_id<0) printf("%i, unbusy horse(moved)\n", thecount);

						if (actors_list[i]->que[0] >= move_n &&
							actors_list[i]->que[0] <= move_nw) {
							next_command();
						}
						else {
							actors_list[i]->x_pos= actors_list[i]->x_tile_pos*0.5;
							actors_list[i]->y_pos= actors_list[i]->y_tile_pos*0.5;
							actors_list[i]->z_pos= get_actor_z(actors_list[i]);
						}
					} else {
						actors_list[i]->busy = 0;
						//if(actors_list[i]->actor_id==yourself) printf("%i, unbusy(moved2)\n", thecount);
						//if(actors_list[i]->actor_id<0) printf("%i, unbusy horse(moved2)\n", thecount);

					}
				}
			} //moving

			if(actors_list[i]->rotating) {
				actors_list[i]->rotate_time_left -= time_diff;
				if (actors_list[i]->rotate_time_left <= 0) { //we rotated all the way
					actors_list[i]->rotating= 0;//don't rotate next time, ok?
                    tmp_time_diff = time_diff + actors_list[i]->rotate_time_left;
/*
#ifdef MORE_ATTACHED_ACTORS
					if(actors_list[i]->actor_id==yourself) printf("%i, rot: %i\n",thecount,actors_list[i]->rotating);
					if(actors_list[i]->actor_id<0) printf("%i, (horse) rot: %i\n",thecount,actors_list[i]->rotating);
#endif
*/
                }
                else {
                    tmp_time_diff = time_diff;
                }
				actors_list[i]->x_rot+= actors_list[i]->rotate_x_speed*tmp_time_diff;
				actors_list[i]->y_rot+= actors_list[i]->rotate_y_speed*tmp_time_diff;
				actors_list[i]->z_rot+= actors_list[i]->rotate_z_speed*tmp_time_diff;
				if(actors_list[i]->z_rot >= 360) {
					actors_list[i]->z_rot -= 360;
				} else if (actors_list[i]->z_rot <= 0) {
					actors_list[i]->z_rot += 360;
				}
				//if(actors_list[i]->actor_id==yourself) printf("%i, rotating: z_rot %f,  status %i-%i\n",thecount,actors_list[i]->z_rot,actors_list[i]->rotating,actors_list[i]->moving);
				//if(actors_list[i]->actor_id<0) printf("%i, rotating (horse): z_rot %f,  status %i-%i\n",thecount,actors_list[i]->z_rot,actors_list[i]->rotating,actors_list[i]->moving);
			}//rotating

			actors_list[i]->anim_time += ((cur_time-last_update)*actors_list[i]->cur_anim.duration_scale)/1000.0;
			/*if(ACTOR(i)->anim_time>=ACTOR(i)->cur_anim.duration) {
				if (HAS_HORSE(i)||IS_HORSE(i)) {
						if(MY_HORSE(i)->anim_time<MY_HORSE(i)->cur_anim.duration) {
							MY_HORSE(i)->anim_time=MY_HORSE(i)->cur_anim.duration;
							printf("%i, ANIMATION FORCED\n",thecount);
						}
				}
			}*/
			if (actors_list[i]->calmodel!=NULL){

				CalModel_Update(actors_list[i]->calmodel, (((cur_time-last_update)*actors_list[i]->cur_anim.duration_scale)/1000.0));
				build_actor_bounding_box(actors_list[i]);
			}
		}
	}
	// unlock the actors_list since we are done now
	UNLOCK_ACTORS_LISTS();

	last_update = cur_time;
}

void unqueue_cmd(int i){
	int k;
	int max_queue=0;
	//move que down with one command
	for(k=0;k<MAX_CMD_QUEUE-1;k++) {
		if(k>max_queue && actors_list[i]->que[k]!=nothing)max_queue=k;
			actors_list[i]->que[k]=actors_list[i]->que[k+1];
		}
	actors_list[i]->que[k]=nothing;
}


void print_queue(actor *act) {
	int k;

	printf("   Actor %s queue:",act->actor_name);
	printf(" -->");
	for(k=0; k<MAX_CMD_QUEUE; k++){
			if(act->que[k]==enter_combat) printf("IC");
			if(act->que[k]==leave_combat) printf("LC");
			if(act->que[k]>=move_n&&act->que[k]<=move_nw) printf("M");
			if(act->que[k]>=turn_n&&act->que[k]<=turn_nw) printf("R");
			printf("%2i|",act->que[k]);
	}
	printf("\n");
	/*for(k=0; k<MAX_RANGE_ACTION_QUEUE; k++){
			printf("%2i-%2i|",act->range_actions[k].shot_type,act->range_actions[k].state);
	}
	printf("\n");
	*/



}


void flush_delayed_item_changes(actor *a)
{
	int item;
	for (item = 0; item < a->delayed_item_changes_count; ++item) {
		if (a->delayed_item_changes[item] < 0) {
			unwear_item_from_actor(a->actor_id,
								   a->delayed_item_type_changes[item]);
		}
		else {
			actor_wear_item(a->actor_id,
							a->delayed_item_type_changes[item],
							a->delayed_item_changes[item]);
		}
	}
	a->delayed_item_changes_count = 0;
}

int coun= 0;
void move_to_next_frame()
{
	int i;
	//int numFrames=0;
	//char frame_exists;
	//struct CalMixer *mixer;
	//char str[255];

	LOCK_ACTORS_LISTS();
	for(i=0;i<max_actors;i++) {
		if(actors_list[i]!=NULL) {
			if (actors_list[i]->calmodel!=NULL) {
			if ((ACTOR(i)->stop_animation==1)&&(ACTOR(i)->anim_time>=ACTOR(i)->cur_anim.duration)){

					actors_list[i]->busy=0;
				}
			}

				if (actors_list[i]->is_enhanced_model != 0)
				{
				if (get_actor_texture_ready(actors_list[i]->texture_id))
				{
					use_ready_actor_texture(actors_list[i]->texture_id);
				}
				}

				if (actors_list[i]->delayed_item_changes_count > 0)
				{
					// we really leave the aim mode only when the animation is finished
					actors_list[i]->delay_texture_item_changes = 0;

					// then we do all the item changes that have been delayed
					flush_delayed_item_changes(actors_list[i]);

					actors_list[i]->delay_texture_item_changes = 1;
				}

			// we change the idle animation only when the previous one is finished
			if (actors_list[i]->stand_idle && actors_list[i]->anim_time >= actors_list[i]->cur_anim.duration - 0.2)
			{
				if (!is_actor_held(actors_list[i]))
				{
					// 1% chance to do idle2
					if (actors_defs[actors_list[i]->actor_type].cal_frames[cal_actor_idle2_frame].anim_index != -1 && RAND(0, 99) == 0)
						cal_actor_set_anim(i, actors_defs[actors_list[i]->actor_type].cal_frames[cal_actor_idle2_frame]); // normal idle
					else
						cal_actor_set_anim(i, actors_defs[actors_list[i]->actor_type].cal_frames[cal_actor_idle1_frame]); // normal idle
				}
			}

			if (actors_list[i]->cur_anim.anim_index==-1) {
				actors_list[i]->busy=0;
				//if(actors_list[i]->actor_id==yourself) printf("%i, unbusy(-1)\n", thecount);
				//if(actors_list[i]->actor_id<0) printf("%i, unbusy horse(-1)\n", thecount);
			}

			//first thing, decrease the damage time, so we will see the damage splash only for 2 seconds
			if(actors_list[i]->damage_ms) {
				actors_list[i]->damage_ms-=80;
				if(actors_list[i]->damage_ms<0)actors_list[i]->damage_ms=0;
			}

			//9 frames, not moving, and another command is queued farther on (based on how long we've done this action)
			if(!actors_list[i]->moving && !actors_list[i]->rotating){

				/*	actors_list[i]->stop_animation=1;	//force stopping, not looping
					actors_list[i]->busy=0;	//ok, take the next command
					LOG_TO_CONSOLE(c_green2,"FREE");
					//Idle here?
				*/
			}

			if(actors_list[i]->stop_animation) {

				//we are done with this guy
				//Should we go into idle here?
			}
		}
	}
	UNLOCK_ACTORS_LISTS();
}


void set_on_idle(int actor_idx)
{
    actor *a = actors_list[actor_idx];
    if(!a->dead) {
        a->stop_animation=0;

        if(a->fighting){
            cal_actor_set_anim(actor_idx,actors_defs[a->actor_type].cal_frames[cal_actor_combat_idle_frame]);
        }
        else if(!a->sitting) {
            // we are standing, see if we can activate a stand idle
            if(!a->stand_idle){
                if (actors_defs[a->actor_type].group_count == 0)
                {
					attachment_props *att_props = get_attachment_props_if_held(a);
					if (att_props)
						cal_actor_set_anim(actor_idx, att_props->cal_frames[cal_attached_idle_frame]);
					else
                    // 75% chance to do idle1
                    if (actors_defs[a->actor_type].cal_frames[cal_actor_idle2_frame].anim_index != -1 && RAND(0, 3) == 0){
                        cal_actor_set_anim(actor_idx, actors_defs[a->actor_type].cal_frames[cal_actor_idle2_frame]); // normal idle
                    } else {
                        cal_actor_set_anim(actor_idx, actors_defs[a->actor_type].cal_frames[cal_actor_idle1_frame]); // normal idle
                    }
                }
                else
                {
                    cal_actor_set_random_idle(actor_idx);
                    a->IsOnIdle=1;
                }

                a->stand_idle=1;
            }
        } else	{
            // we are sitting, see if we can activate the sit idle
            if(!a->sit_idle) {
                cal_actor_set_anim(actor_idx,actors_defs[a->actor_type].cal_frames[cal_actor_idle_sit_frame]);
                a->sit_idle=1;
            }
        }
    }
}










//in case the actor is not busy, and has commands in it's que, execute them
void next_command()
{
	int i, index;



	for(i=0;i<max_actors;i++){
		if(!actors_list[i])continue;//actor exists?
		if(!actors_list[i]->busy){//Are we busy?
			if(actors_list[i]->que[0]==nothing){//Is the queue empty?
				//if que is empty, set on idle
                set_on_idle(i);
				//synch_attachment(i);
				actors_list[i]->last_command=nothing;//prevents us from not updating the walk/run animation
			} else {
				int actor_type;
				int last_command=actors_list[i]->last_command;
				float z_rot=actors_list[i]->z_rot;
				float targeted_z_rot;
				int no_action = 0;

				actors_list[i]->sit_idle=0;
				actors_list[i]->stand_idle=0;


				actor_type=actors_list[i]->actor_type;
				switch(actors_list[i]->que[0]) {
					case kill_me:
/*						if(actors_list[i]->remapped_colors)
						glDeleteTextures(1,&actors_list[i]->texture_id);
						ec_actor_delete(actors_list[i]);
						free(actors_list[i]);
						actors_list[i]=0;*/ //Obsolete
						break;
					case die1:
						cal_actor_set_anim(i,actors_defs[actor_type].cal_frames[cal_actor_die1_frame]);
						actors_list[i]->stop_animation=1;
						actors_list[i]->dead=1;
						break;
					case die2:
						cal_actor_set_anim(i,actors_defs[actor_type].cal_frames[cal_actor_die2_frame]);
						actors_list[i]->stop_animation=1;
						actors_list[i]->dead=1;
						break;
                    case pain1:
					case pain2: {
						int painframe = (actors_list[i]->que[0]==pain1) ? (cal_actor_pain1_frame):(cal_actor_pain2_frame);
						attachment_props *att_props = get_attachment_props_if_held(actors_list[i]);
						if (att_props) {
							cal_actor_set_anim(i, att_props->cal_frames[cal_attached_pain_frame]);
						} else
							cal_actor_set_anim(i,actors_defs[actor_type].cal_frames[painframe]);
						actors_list[i]->stop_animation=1;
						break;
					}
/*					case pain2: {
#ifdef ATTACHED_ACTORS
						attachment_props *att_props = get_attachment_props_if_held(actors_list[i]);
						if (att_props)
							cal_actor_set_anim(i, att_props->cal_frames[cal_attached_pain_frame]);
						else
#endif // ATTACHED_ACTORS
							cal_actor_set_anim(i,actors_defs[actor_type].cal_frames[cal_actor_pain2_frame]);
						actors_list[i]->stop_animation=1;
						break;
					}
*/					case pick:
						cal_actor_set_anim(i,actors_defs[actor_type].cal_frames[cal_actor_pick_frame]);
						actors_list[i]->stop_animation=1;
						break;
					case drop:
						cal_actor_set_anim(i,actors_defs[actor_type].cal_frames[cal_actor_drop_frame]);
						actors_list[i]->stop_animation=1;
						break;
					case harvest:
						cal_actor_set_anim(i,actors_defs[actor_type].cal_frames[cal_actor_harvest_frame]);
						actors_list[i]->stop_animation=1;
						LOG_TO_CONSOLE(c_green2,"Harvesting!");
						break;
					case cast:
						cal_actor_set_anim(i,actors_defs[actor_type].cal_frames[cal_actor_attack_cast_frame]);
						actors_list[i]->stop_animation=1;
						break;
					case ranged:
						cal_actor_set_anim(i,actors_defs[actor_type].cal_frames[cal_actor_attack_ranged_frame]);
						actors_list[i]->stop_animation=1;
						break;
					case sit_down:
						cal_actor_set_anim(i,actors_defs[actor_type].cal_frames[cal_actor_sit_down_frame]);
						actors_list[i]->stop_animation=1;
						actors_list[i]->sitting=1;
						if(actors_list[i]->actor_id==yourself)
							you_sit=1;
						break;
					case stand_up:
						//LOG_TO_CONSOLE(c_green2,"stand_up");
						cal_actor_set_anim(i,actors_defs[actor_type].cal_frames[cal_actor_stand_up_frame]);
						actors_list[i]->stop_animation=1;
						actors_list[i]->sitting=0;
						if(actors_list[i]->actor_id==yourself)
							you_sit=0;
						break;
					case enter_combat:
					case leave_combat:
						{
						int fight_k = (actors_list[i]->que[0]==enter_combat) ? (1):(0);
						int combat_frame = (fight_k) ? (cal_actor_in_combat_frame):(cal_actor_out_combat_frame);
						cal_actor_set_anim(i,actors_defs[actor_type].cal_frames[combat_frame]);

						actors_list[i]->stop_animation=1;
						actors_list[i]->fighting=fight_k;
						}
						break;
/*					case leave_combat:
#ifdef MORE_ATTACHED_ACTORS
						if(HAS_HORSE(i)){
							cal_actor_set_anim(i,actors_defs[actor_type].cal_frames[cal_actor_out_combat_held_frame]);
							//rotate counterclowwise horse and actor
							add_rotation_to_actor(i,HORSE_FIGHT_ROTATION,HORSE_FIGHT_TIME);
							rotate_actor(MY_HORSE_ID(i),HORSE_FIGHT_ROTATION,HORSE_FIGHT_TIME);

							cal_actor_set_anim(MY_HORSE_ID(i),actors_defs[MY_HORSE(i)->actor_type].cal_frames[cal_actor_out_combat_frame]);
							MY_HORSE(i)->stop_animation=1;
							MY_HORSE(i)->fighting=0;
						} else
#endif
						cal_actor_set_anim(i,actors_defs[actor_type].cal_frames[cal_actor_out_combat_frame]);
						actors_list[i]->stop_animation=1;
						actors_list[i]->fighting=0;
						break;
*/
					case attack_up_1:
					case attack_up_2:
					case attack_up_3:
					case attack_up_4:
					case attack_up_5:
					case attack_up_6:
					case attack_up_7:
					case attack_up_8:
					case attack_up_9:
					case attack_up_10:
					case attack_down_1:
					case attack_down_2:
					case attack_down_3:
					case attack_down_4:
					case attack_down_5:
					case attack_down_6:
					case attack_down_7:
					case attack_down_8:
					case attack_down_9:
					case attack_down_10:
						index = -1;
						switch (actors_list[i]->que[0])
						{
							case attack_down_10:
								index++;
							case attack_down_9:
								index++;
							case attack_down_8:
								index++;
							case attack_down_7:
								index++;
							case attack_down_6:
								index++;
							case attack_down_5:
								index++;
							case attack_down_4:
								index++;
							case attack_down_3:
								index++;
							case attack_down_2:
								index++;
							case attack_down_1:
								index++;
							case attack_up_10:
								index++;
							case attack_up_9:
								index++;
							case attack_up_8:
								index++;
							case attack_up_7:
								index++;
							case attack_up_6:
								index++;
							case attack_up_5:
								index++;
							case attack_up_4:
								index++;
							case attack_up_3:
								index++;
							case attack_up_2:
								index++;
							case attack_up_1:
								index++;
						break;
							default:
						break;
						}
						if (actors_list[i]->is_enhanced_model) {
							cal_actor_set_anim(i,actors_defs[actor_type].weapon[actors_list[i]->cur_weapon].cal_frames[index]);
						} else {
							//non enhanced models
						 {
							//select normal actor att frames
							index +=cal_actor_attack_up_1_frame;
							cal_actor_set_anim(i,actors_defs[actor_type].cal_frames[index]);
						}
						}
						actors_list[i]->stop_animation=1;
						actors_list[i]->fighting=1;

						// Maybe play a battlecry sound
						add_battlecry_sound(actors_list[i]);
						break;
					case salut:
						cal_actor_set_anim(i,actors_defs[actor_type].cal_frames[cal_actor_salut_frame]);
						actors_list[i]->stop_animation=1;
						actors_list[i]->sitting=0;
						break;
                    case danse:
						cal_actor_set_anim(i,actors_defs[actor_type].cal_frames[cal_actor_danse_frame]);
						actors_list[i]->stop_animation=1;
						actors_list[i]->sitting=0;
                    break;

                    case salto:
						cal_actor_set_anim(i,actors_defs[actor_type].cal_frames[cal_actor_salto_frame]);
						actors_list[i]->stop_animation=1;
						actors_list[i]->sitting=0;
                    break;

                    case roue:
						cal_actor_set_anim(i,actors_defs[actor_type].cal_frames[cal_actor_roue_frame]);
						actors_list[i]->stop_animation=1;
						actors_list[i]->sitting=0;
                    break;
					case turn_left:
					case turn_right:
					{
						int mul= (actors_list[i]->que[0]==turn_left) ? (1):(-1);

						//LOG_TO_CONSOLE(c_green2,"turn left");
						actors_list[i]->rotate_z_speed=mul*45.0/540.0;
						actors_list[i]->rotate_time_left=540;
						actors_list[i]->rotating=1;
						//generate a fake movement, so we will know when to make the actor
						//not busy
						actors_list[i]->move_x_speed=0;
						actors_list[i]->move_y_speed=0;
						actors_list[i]->move_z_speed=0;
						actors_list[i]->movement_time_left=540;
						actors_list[i]->moving=1;
						//test
						if(!actors_list[i]->fighting){
							attachment_props *att_props = get_attachment_props_if_held(actors_list[i]);
							if (att_props)
								cal_actor_set_anim(i, att_props->cal_frames[get_held_actor_motion_frame(actors_list[i])]);
							else
							cal_actor_set_anim(i,actors_defs[actor_type].cal_frames[get_actor_motion_frame(actors_list[i])]);
						}
						actors_list[i]->stop_animation=0;
						break;
					}

					//ok, now the movement, this is the tricky part
					default:
						if(actors_list[i]->que[0]>=move_n && actors_list[i]->que[0]<=move_nw) {
							float rotation_angle;
							int dx, dy;
							int step_duration = DEFAULT_STEP_DURATION;
							struct cal_anim *walk_anim;

							attachment_props *att_props = get_attachment_props_if_held(actors_list[i]);
							if (att_props)
								walk_anim = &att_props->cal_frames[get_held_actor_motion_frame(actors_list[i])];
							else
							walk_anim = &actors_defs[actor_type].cal_frames[get_actor_motion_frame(actors_list[i])];


							actors_list[i]->moving=1;
							actors_list[i]->fighting=0;
							if(last_command<move_n || last_command>move_nw){//update the frame name too
								cal_actor_set_anim(i,*walk_anim);
								actors_list[i]->stop_animation=0;
							}

							if(last_command!=actors_list[i]->que[0]){ //Calculate the rotation
								targeted_z_rot=(actors_list[i]->que[0]-move_n)*45.0f;
								rotation_angle=get_rotation_vector(z_rot,targeted_z_rot);
								actors_list[i]->rotate_z_speed=rotation_angle/360.0;
								if(auto_camera && actors_list[i]->actor_id==yourself){
									camera_rotation_speed=rotation_angle/1000.0;
									camera_rotation_duration=1000;
								}

								actors_list[i]->rotate_time_left=360;
								actors_list[i]->rotating=1;
							}
                            get_motion_vector(actors_list[i]->que[0], &dx, &dy);

							/* if other move commands are waiting in the queue,
							 * we walk at a speed that is close to the server speed
							 * else we walk at a slightly slower speed to wait next
							 * incoming walking commands */
                            if (actors_list[i]->que[1] >= move_n &&
                                actors_list[i]->que[1] <= move_nw) {
                                if (actors_list[i]->que[2] >= move_n &&
                                    actors_list[i]->que[2] <= move_nw) {
									if (actors_list[i]->que[3] >= move_n &&
										actors_list[i]->que[3] <= move_nw)
										actors_list[i]->movement_time_left = (int)(step_duration*0.9); // 3 moves
									else
										actors_list[i]->movement_time_left = step_duration; // 2 moves
								}
                                else
                                    actors_list[i]->movement_time_left = (int)(step_duration*1.1); // 1 move
                            }
                            else {
                                actors_list[i]->movement_time_left = (int)(step_duration*1.2); // 0 move
                            }
							// if we have a diagonal motion, we slow down the animation a bit
							if (dx != 0 && dy != 0)
								actors_list[i]->movement_time_left = (int)(actors_list[i]->movement_time_left*1.2+0.5);

                            // we compute the moving speeds in x, y and z directions
							actors_list[i]->move_x_speed = 0.5*(dx+actors_list[i]->x_tile_pos)-actors_list[i]->x_pos;
							actors_list[i]->move_y_speed = 0.5*(dy+actors_list[i]->y_tile_pos)-actors_list[i]->y_pos;
							actors_list[i]->move_z_speed = get_tile_height(actors_list[i]->x_tile_pos+dx, actors_list[i]->y_tile_pos+dy) - actors_list[i]->z_pos;
							actors_list[i]->move_x_speed /= (float)actors_list[i]->movement_time_left;
							actors_list[i]->move_y_speed /= (float)actors_list[i]->movement_time_left;
							actors_list[i]->move_z_speed /= (float)actors_list[i]->movement_time_left;

							/* we change the speed of the walking animation according to the walking speed and to the size of the actor
							 * we suppose here that the normal speed of the walking animation is 2 meters per second (1 tile in 250ms) */
							actors_list[i]->cur_anim.duration_scale = walk_anim->duration_scale;
							actors_list[i]->cur_anim.duration_scale *= (float)DEFAULT_STEP_DURATION/(actors_list[i]->movement_time_left*actors_list[i]->scale);
							if (actors_defs[actor_type].actor_scale != 1.0)
								actors_list[i]->cur_anim.duration_scale /= actors_defs[actor_type].actor_scale;
							else
								actors_list[i]->cur_anim.duration_scale /= actors_defs[actor_type].scale;
							if (dx != 0 && dy != 0)
								actors_list[i]->cur_anim.duration_scale *= 1.4142315;
						} else if(actors_list[i]->que[0]>=turn_n && actors_list[i]->que[0]<=turn_nw) {
							float rotation_angle;
							targeted_z_rot=(actors_list[i]->que[0]-turn_n)*45.0f;
							rotation_angle=get_rotation_vector(z_rot,targeted_z_rot);
							actors_list[i]->rotate_z_speed=rotation_angle/360.0f;
							actors_list[i]->rotate_time_left=360;
							actors_list[i]->rotating=1;
							actors_list[i]->stop_animation=1;
						}
					}

					//mark the actor as being busy
					if (!no_action)
						actors_list[i]->busy=1;
					//if (actors_list[i]->actor_id==yourself) LOG_TO_CONSOLE(c_green2,"Busy");
					//save the last command. It is especially good for run and walk
					actors_list[i]->last_command=actors_list[i]->que[0];

					unqueue_cmd(i);
				}
			}
		}
}

void free_actor_data(int actor_index)
{
	actor *act = actors_list[actor_index];
    if(act->calmodel!=NULL)
        model_delete(act->calmodel);
	if(act->remapped_colors)
	{
		free_actor_texture(act->texture_id);
	}
	if (act->is_enhanced_model)
	{
		free_actor_texture(act->texture_id);

	        if (act->body_parts)
		{
			free(act->body_parts);
		}
	}
    stop_sound(act->cur_anim_sound_cookie);
    act->cur_anim_sound_cookie = 0;
    ec_actor_delete(act);
}

void destroy_actor(int actor_id)
{
	int i;
    int attached_actor = -1;

	for(i=0;i<max_actors;i++){
		if(actors_list[i])//The timer thread doesn't free memory
			if(actors_list[i]->actor_id==actor_id){
				LOCK_ACTORS_LISTS();
                attached_actor = actors_list[i]->attached_actor;

				if (actor_id == yourself)
					set_our_actor (NULL);
                free_actor_data(i);
				free(actors_list[i]);
				actors_list[i]=NULL;
				if(i==max_actors-1)max_actors--;
				else {
					//copy the last one down and fill in the hole
					max_actors--;
					actors_list[i]=actors_list[max_actors];
					actors_list[max_actors]=NULL;
                    if (attached_actor == max_actors) attached_actor = i;
					if (actors_list[i] && actors_list[i]->attached_actor >= 0)
						actors_list[actors_list[i]->attached_actor]->attached_actor = i;
				}

                if (attached_actor >= 0)
                {
                    free_actor_data(attached_actor);
                    free(actors_list[attached_actor]);
                    actors_list[attached_actor]=NULL;
                    if(attached_actor==max_actors-1)max_actors--;
                    else {
                        //copy the last one down and fill in the hole
                        max_actors--;
                        actors_list[attached_actor]=actors_list[max_actors];
                        actors_list[max_actors]=NULL;
						if (actors_list[attached_actor] && actors_list[attached_actor]->attached_actor >= 0)
							actors_list[actors_list[attached_actor]->attached_actor]->attached_actor = attached_actor;
                    }
                }

				actor_under_mouse = NULL;
				UNLOCK_ACTORS_LISTS();
				break;
			}
	}
}

void destroy_all_actors()
{
	int i=0;
	LOCK_ACTORS_LISTS();	//lock it to avoid timing issues
	set_our_actor (NULL);
	for(i=0;i<max_actors;i++) {
		if(actors_list[i]){
            free_actor_data(i);
			free(actors_list[i]);
			actors_list[i]=NULL;
		}
	}
	max_actors= 0;
	actor_under_mouse = NULL;
	my_timer_adjust= 0;
	harvesting_effect_reference = NULL;
	UNLOCK_ACTORS_LISTS();	//unlock it since we are done
}




void update_all_actors()
{
 	Uint8 str[40];

	//we got a nasty error, log it
	LOG_TO_CONSOLE(c_red2,resync_server);

	destroy_all_actors();
	str[0]=SEND_ME_MY_ACTORS;
	my_tcp_send(my_socket,str,1);
}

int push_command_in_actor_queue(unsigned int command, actor *act)
{
	int k;
	for(k=0;k<MAX_CMD_QUEUE;k++){
		if(act->que[k]==nothing){
			//if we are SEVERLY behind, just update all the actors in range
			if(k>MAX_CMD_QUEUE-2) break;
			else if(k>MAX_CMD_QUEUE-8){
				// is the front a sit/stand spam?
				if((act->que[0]==stand_up||act->que[0]==sit_down)
				   &&(act->que[1]==stand_up||act->que[1]==sit_down)){
					int j;
					//move que down with one command
					for(j=0;j<=k;j++){
						act->que[j]=act->que[j+1];
					}
					act->que[j]=nothing;
					//backup one entry
					k--;
				}

				// is the end a sit/stand spam?
				else if((command==stand_up||command==sit_down)
						&& (act->que[k-1]==stand_up||act->que[k-1]==sit_down)) {
					act->que[k-1]=command;
					break;
				}

			}

			act->que[k]=command;
			break;
		}
	}
	return k;
}

void sanitize_cmd_queue(actor *act){
	int k,j;
	for(k=0,j=0;k<MAX_CMD_QUEUE-1-j;k++){
		if(act->que[k]==nothing) j++;
		act->que[k]=act->que[k+j];
	}
	for(k=MAX_CMD_QUEUE-1;k>0&&j>0;k--,j--) act->que[k]=nothing;
}

void add_command_to_actor(int actor_id, unsigned char command)
{
	//int i=0;
	int k=0;
	int k2 = 0;
	//int have_actor=0;
//if ((actor_id==yourself)&&(command==enter_combat)) LOG_TO_CONSOLE(c_green2,"FIGHT!");
	actor * act;
	int isme = 0;
	act= get_actor_ptr_from_id(actor_id);

	if(!act){
		//Resync
		//if we got here, it means we don't have this actor, so get it from the server...
		LOG_ERROR("%s %d - %d\n", cant_add_command, command, actor_id);
	} else {
		LOCK_ACTORS_LISTS();


		if(command==leave_combat||command==enter_combat||command==die1||command==die2)
		{
			int j= 0;

			//Strip the queue for attack messages
			for(k=0; k<MAX_CMD_QUEUE; k++){
				switch(act->que[k]){
					case pain1:
					case pain2:
					case attack_up_1:
					case attack_up_2:
					case attack_up_3:
					case attack_up_4:
					case attack_down_1:
					case attack_down_2:
						act->que[k]= nothing;
						break;

					default:
						act->que[j]= act->que[k];
						j++;
						if(j<=k){
							act->que[k]= nothing;
						}
						break;
				}
			}




			if(act->last_command == nothing)
			{
				//We may be on idle, update the actor so we can reduce the rendering lag
				CalModel_Update(act->calmodel, 5.0f);
				build_actor_bounding_box(act);
			}
		}

		k = push_command_in_actor_queue(command, act);
		if (act->attached_actor >= 0){
			k2 = push_command_in_actor_queue(command, actors_list[act->attached_actor]);
		}
		else
			k2 = k;

		{
			actor * me = get_our_actor();
			if (me!=NULL)
				isme = act->actor_id == me->actor_id;
		}


		//if(act->actor_id==yourself) printf("COMMAND: %i at pos %i (and %i)\n",command,k,k2);
		//if(act->actor_id==yourself) print_queue(act);
		//Reduce resync in invasions
		if(command==enter_combat) {
			// we received an enter_combat, look back in the queue
			// if a leave_combat is found and all the commands in between
			// are turning commands, just ignore the leave and the enter combat commands

			int j=k-1;
			while(act->que[j]>=turn_n&&act->que[j]<=turn_nw&&j>=0) j--; //skip rotations
			if(j>=0&&act->que[j]==leave_combat) {
				//remove leave_combat and enter_combat
				act->que[j]=nothing;
				act->que[k]=nothing;
				sanitize_cmd_queue(act);
				//if(act->actor_id==yourself) printf("   actor %s: skipped %i and %i\n",act->actor_name,j,k);
			}

			//if(act->actor_id==yourself) printf("   ***Skip Done***\n");
			//if(act->actor_id==yourself) print_queue(act);
		}




		switch(command) {
		case enter_combat:
			act->async_fighting= 1;
			break;
		case leave_combat:
			act->async_fighting= 0;
			break;
		case move_n:
		case run_n:
			act->async_y_tile_pos++;
			act->async_z_rot= 0;
			if(isme && pf_follow_path)
			{
                if(checkvisitedlist(act->async_x_tile_pos,act->async_y_tile_pos))
                    pf_destroy_path();
            }
			break;
		case move_ne:
		case run_ne:
			act->async_x_tile_pos++;
			act->async_y_tile_pos++;
			act->async_z_rot= 45;
			if(isme && pf_follow_path)
			{
                if(checkvisitedlist(act->async_x_tile_pos,act->async_y_tile_pos))
                    pf_destroy_path();
            }
			break;
		case move_e:
		case run_e:
			act->async_x_tile_pos++;
			act->async_z_rot= 90;
			if(isme && pf_follow_path)
			{
                if(checkvisitedlist(act->async_x_tile_pos,act->async_y_tile_pos))
                    pf_destroy_path();
            }
			break;
		case move_se:
		case run_se:
			act->async_x_tile_pos++;
			act->async_y_tile_pos--;
			act->async_z_rot= 135;
			if(isme && pf_follow_path)
			{
                if(checkvisitedlist(act->async_x_tile_pos,act->async_y_tile_pos))
                    pf_destroy_path();
            }
			break;
		case move_s:
		case run_s:
			act->async_y_tile_pos--;
			act->async_z_rot= 180;
			if(isme && pf_follow_path)
			{
                if(checkvisitedlist(act->async_x_tile_pos,act->async_y_tile_pos))
                    pf_destroy_path();
            }
			break;
		case move_sw:
		case run_sw:
			act->async_x_tile_pos--;
			act->async_y_tile_pos--;
			act->async_z_rot= 225;
			if(isme && pf_follow_path)
			{
                if(checkvisitedlist(act->async_x_tile_pos,act->async_y_tile_pos))
                    pf_destroy_path();
            }
			break;
		case move_w:
		case run_w:
			act->async_x_tile_pos--;
			act->async_z_rot= 270;
			if(isme && pf_follow_path)
			{
                if(checkvisitedlist(act->async_x_tile_pos,act->async_y_tile_pos))
                    pf_destroy_path();
            }
			break;
		case move_nw:
		case run_nw:
			act->async_x_tile_pos--;
			act->async_y_tile_pos++;
			act->async_z_rot= 315;
			if(isme && pf_follow_path)
			{
                if(checkvisitedlist(act->async_x_tile_pos,act->async_y_tile_pos))
                    pf_destroy_path();
            }
			break;
		case turn_n:
		case turn_ne:
		case turn_e:
		case turn_se:
		case turn_s:
		case turn_sw:
		case turn_w:
		case turn_nw:
			 act->async_z_rot= (command-turn_n)*45;
			 break;
		}
		UNLOCK_ACTORS_LISTS();

		if (k != k2) {
			LOG_ERROR("Inconsistency between queues of attached actors %s (%d) and %s (%d)!",
					  act->actor_name,
					  act->actor_id,
					  actors_list[act->attached_actor]->actor_name,
					  actors_list[act->attached_actor]->actor_id);
		}
		else
		if(k>MAX_CMD_QUEUE-2){
			int i;
			LOG_ERROR("Too much commands in the queue for actor %d (%s) => resync!\n",
					  act->actor_id, act->actor_name);
			for (i = 0; i < MAX_CMD_QUEUE; ++i)
				LOG_ERROR("%dth command in the queue: %d\n", i, (int)act->que[i]);
			update_all_actors();
		}
	}
}



void get_actor_damage(int actor_id, int damage, int sante)
{
	//int i=0;
	actor * act;
	float blood_level;
        float bone_list[1024][3];
        int total_bones;
        int bone;
        float bone_x, bone_y, bone_z;

	act = get_actor_ptr_from_id(actor_id);

	if(!act){
		//if we got here, it means we don't have this actor, so get it from the server...
	} else {
		if(floatingmessages_enabled){
			act->last_health_loss=cur_time;
		}

		if (actor_id == yourself)
			set_last_damage(damage);

		act->damage=damage;
		act->damage_ms=2000;
        if ((sante == PAS_ETAT_SANTE) && (act->max_health))
        {
		    act->cur_health-=damage;
        }
        else
        {
            act->cur_health = sante;
        }

		if (act->cur_health <= 0) {
			add_death_sound(act);
			increment_death_counter(act);
		}
		act->last_range_attacker_id = -1;

		if (use_eye_candy && enable_blood)
		{
			if (strcmp(act->actor_name, "Gargoyle") && strcmp(act->actor_name, "Skeleton") && strcmp(act->actor_name, "Phantom Warrior"))	//Ideally, we'd also check to see if it was a player or not, but since this is just cosmetic...
			{
				blood_level=(act->max_health) ? (int)powf(damage/powf(act->max_health, 0.5), 0.75)+0.5 : (int)powf(damage/8, 0.75)+0.5;
				total_bones = CalSkeleton_GetBonePoints(CalModel_GetSkeleton(act->calmodel), &bone_list[0][0]);
				bone = rand() % total_bones;
				bone_x = bone_list[bone][0] + act->x_pos + 0.25;
				bone_y = bone_list[bone][1] + act->y_pos + 0.25;
				bone_z = bone_list[bone][2] + ec_get_z(act);
//				printf("ec_create_impact_blood((%f %f, %f), (%f, %f, %f), %d, %f);", bone_x, bone_y, bone_z, ((float)rand()) * blood_level / RAND_MAX / 13.0, ((float)rand()) * blood_level / RAND_MAX / 13.0, ((float)rand()) * blood_level / RAND_MAX / 13.0, (poor_man ? 6 : 10), blood_level);
				ec_create_impact_blood(bone_x, bone_y, bone_z, ((float)rand()) * blood_level / RAND_MAX / 13.0, ((float)rand()) * blood_level / RAND_MAX / 13.0, ((float)rand()) * blood_level / RAND_MAX / 13.0, (poor_man ? 6 : 10), blood_level);
			}
		}
	}
}

void get_actor_heal(int actor_id, int quantity, int sante)
{
	//int i=0;
	actor *act;
	act = get_actor_ptr_from_id(actor_id);

	if(!act){
		//if we got here, it means we don't have this actor, so get it from the server...
	} else {

		if (actor_id == yourself)
			set_last_heal(quantity);

		if(floatingmessages_enabled){
			act->damage=-quantity;
			act->damage_ms=2000;
			act->last_health_loss=cur_time;
		}

        if ((sante == PAS_ETAT_SANTE) && (act->max_health))
        {
		    act->cur_health+=quantity;
        }
        else
        {
		    act->cur_health = sante;
        }
	}
	//if we got here, it means we don't have this actor, so get it from the server...
}

void get_actor_health(int actor_id, int quantity, int sante)
{
	//int i=0;
	actor *act;
	act = get_actor_ptr_from_id(actor_id);

	if(!act){
		//if we got here, it means we don't have this actor, so get it from the server...
	} else {
//		if(floatingmessages_enabled){
			//act->damage=-quantity;
			//act->damage_ms=2000;
			//act->last_health_loss=cur_time;
//		}

        if (sante == PAS_ETAT_SANTE)
		{
		    act->max_health = quantity;
		}
		else
		{
		    act->cur_health = sante;
		}
	}
	//if we got here, it means we don't have this actor, so get it from the server...
}

void move_self_forward()
{
	int x,y,rot,tx,ty;

	actor *me = get_our_actor ();

	if(!me)return;//Wtf!?

	x=me->x_tile_pos;
	y=me->y_tile_pos;
	rot=(int)rint(me->z_rot/45.0f);
	if (rot < 0) rot += 8;
	switch(rot) {
		case 8: //360
		case 0: //0
			tx=x;
			ty=y+1;
			break;
		case 1: //45
			tx=x+1;
			ty=y+1;
			break;
		case 2: //90
			tx=x+1;
			ty=y;
			break;
		case 3: //135
			tx=x+1;
			ty=y-1;
			break;
		case 4: //180
			tx=x;
			ty=y-1;
			break;
		case 5: //225
			tx=x-1;
			ty=y-1;
			break;
		case 6: //270
			tx=x-1;
			ty=y;
			break;
		case 7: //315
			tx=x-1;
			ty=y+1;
			break;
		default:
			tx=x;
			ty=y;
	}

	//check to see if the coordinates are OUTSIDE the map
	if(ty<0 || tx<0 || tx>=tile_map_size_x*6 || ty>=tile_map_size_y*6) {
		return;
	}
	if (pf_follow_path) {
		pf_destroy_path();
	}

	move_to (tx, ty, 0);
}


void actor_check_string(actor_types *act, const char *section, const char *type, const char *value)
{
	if (value == NULL || *value=='\0')
	{
	}
}

void actor_check_int(actor_types *act, const char *section, const char *type, int value)
{
	if (value < 0)
	{
	}
}


const xmlNode *get_default_node(const xmlNode *cfg, const xmlNode *defaults)
{
	const xmlNode *item;
	const char *group;

	// first, check for errors
	if(defaults == NULL || cfg == NULL)
        return NULL;

	//lets find out what group to look for
	group = get_string_property(cfg, "group");

	// look for defaul entries with the same name
	for(item=defaults->children; item; item=item->next){
		if(item->type == XML_ELEMENT_NODE) {
			if(xmlStrcasecmp(item->name, cfg->name) == 0){
				const char *item_group;

				item_group = get_string_property(item, "group");
				// either both have no group, or both groups match
				if(xmlStrcasecmp((xmlChar*)item_group, (xmlChar*)group) == 0){
					// this is the default entry we want then!
					return item;
				}
			}
		}
	}

	// if we got here, there is no default node that matches
	return NULL;
}

int parse_actor_shirt(actor_types *act, const xmlNode *cfg, const xmlNode *defaults)
{
	const xmlNode *item;
	int ok, col_idx;
	shirt_part *shirt;

	if(cfg == NULL || cfg->children == NULL) return 0;

	col_idx= get_int_property(cfg, "id");
/*	if(col_idx < 0){
		col_idx= get_property(cfg, "color", "shirt color", shirt_color_dict);
	}
*/
	if(col_idx < 0 || col_idx >= actor_part_sizes[ACTOR_SHIRT_SIZE]){
		LOG_ERROR("Unable to find id/property node %s\n", cfg->name);
		return 0;
	}

	if (act->shirt == NULL) {
		int i;
		act->shirt = (shirt_part*)calloc(actor_part_sizes[ACTOR_SHIRT_SIZE], sizeof(shirt_part));
		for (i = actor_part_sizes[ACTOR_SHIRT_SIZE]; i--;) act->shirt[i].mesh_index= -1;
	}

	shirt= &(act->shirt[col_idx]);
	ok= 1;
	for(item=cfg->children; item; item=item->next) {
		if(item->type == XML_ELEMENT_NODE) {
			if(xmlStrcasecmp (item->name, (xmlChar*)"arms") == 0) {
				get_string_value(shirt->arms_name, sizeof(shirt->arms_name), item);
			} else if(xmlStrcasecmp(item->name, (xmlChar*)"mesh") == 0) {
				get_string_value(shirt->model_name, sizeof(shirt->model_name), item);
				shirt->mesh_index= cal_load_mesh(act, shirt->model_name, "shirt");
			} else if(xmlStrcasecmp(item->name, (xmlChar*)"torso") == 0) {
				get_string_value(shirt->torso_name, sizeof(shirt->torso_name), item);
			} else if(xmlStrcasecmp(item->name, (xmlChar*)"armsmask") == 0) {
				get_string_value(shirt->arms_mask, sizeof(shirt->arms_mask), item);
			} else if(xmlStrcasecmp(item->name, (xmlChar*)"torsomask") == 0) {
				get_string_value(shirt->torso_mask, sizeof(shirt->torso_mask), item);
			} else {
				LOG_ERROR("unknown shirt property \"%s\"", item->name);
				ok= 0;
			}
		}
	}

	// check for default entries, if found, use them to fill in missing data
	if(defaults){
		const xmlNode *default_node = get_default_node(cfg, defaults);

		if(default_node){
			if (!*shirt->arms_name)
				get_item_string_value(shirt->arms_name, sizeof(shirt->arms_name), default_node, (xmlChar*)"arms");
			if (!*shirt->model_name){
				get_item_string_value(shirt->model_name, sizeof(shirt->model_name), default_node, (xmlChar*)"mesh");
				shirt->mesh_index= cal_load_mesh(act, shirt->model_name, "shirt");
			}
			if (!*shirt->torso_name)
				get_item_string_value(shirt->torso_name, sizeof(shirt->torso_name), default_node, (xmlChar*)"torso");
		}
	}

	// check the critical information
	actor_check_string(act, "shirt", "arms", shirt->arms_name);
	actor_check_string(act, "shirt", "model", shirt->model_name);
	actor_check_int(act, "shirt", "mesh", shirt->mesh_index);
	actor_check_string(act, "shirt", "torso", shirt->torso_name);

	return ok;
}

int parse_actor_skin (actor_types *act, const xmlNode *cfg, const xmlNode *defaults)
{
	const xmlNode *item;
	int ok, col_idx;
	skin_part *skin;

	if (cfg == NULL || cfg->children == NULL) return 0;

	col_idx= get_int_property(cfg, "id");
	if(col_idx < 0){
		col_idx= get_property(cfg, "color", "skin color", skin_color_dict);
	}

	if(col_idx < 0 || col_idx >= actor_part_sizes[ACTOR_SKIN_SIZE]){
		LOG_ERROR("Unable to find id/property node %s\n", cfg->name);
		return 0;
	}

	if (act->skin == NULL) {
		int i;
		act->skin = (skin_part*)calloc(actor_part_sizes[ACTOR_SKIN_SIZE], sizeof(skin_part));
		for (i = actor_part_sizes[ACTOR_SKIN_SIZE]; i--;) act->skin[i].mesh_index= -1;
	}

	skin = &(act->skin[col_idx]);
	ok = 1;
	for (item = cfg->children; item; item = item->next) {
		if (item->type == XML_ELEMENT_NODE) {
			if (xmlStrcasecmp (item->name, (xmlChar*)"hands") == 0) {
				get_string_value (skin->hands_name, sizeof (skin->hands_name), item);
			} else if (xmlStrcasecmp (item->name, (xmlChar*)"head") == 0) {
				get_string_value (skin->head_name, sizeof (skin->head_name), item);
			} else if (xmlStrcasecmp (item->name, (xmlChar*)"torso") == 0) {
				get_string_value (skin->body_name, sizeof (skin->body_name), item);
			} else if (xmlStrcasecmp (item->name, (xmlChar*)"arms") == 0) {
				get_string_value (skin->arms_name, sizeof (skin->arms_name), item);
			} else if (xmlStrcasecmp (item->name, (xmlChar*)"legs") == 0) {
				get_string_value (skin->legs_name, sizeof (skin->legs_name), item);
			} else if (xmlStrcasecmp (item->name, (xmlChar*)"feet") == 0) {
				get_string_value (skin->feet_name, sizeof (skin->feet_name), item);
			} else {
				LOG_ERROR("unknown skin property \"%s\"", item->name);
				ok = 0;
			}
		}
	}

	// check for default entries, if found, use them to fill in missing data
	if(defaults){
		const xmlNode *default_node= get_default_node(cfg, defaults);

		if(default_node){
			if (!*skin->hands_name)
				get_item_string_value(skin->hands_name, sizeof(skin->hands_name), default_node, (xmlChar*)"hands");
			if (!*skin->head_name)
				get_item_string_value(skin->head_name, sizeof(skin->head_name), default_node, (xmlChar*)"head");
		}
	}

	// check the critical information
	actor_check_string(act, "skin", "hands", skin->hands_name);
	actor_check_string(act, "skin", "head", skin->head_name);


	return ok;
}

int parse_actor_legs (actor_types *act, const xmlNode *cfg, const xmlNode *defaults)
{
	const xmlNode *item;
	int ok, col_idx;
	legs_part *legs;

	if (cfg == NULL || cfg->children == NULL) return 0;

	col_idx= get_int_property(cfg, "id");
/*	if(col_idx < 0){
		col_idx= get_property(cfg, "color", "legs color", legs_color_dict);
	}
*/
	if(col_idx < 0 || col_idx >= actor_part_sizes[ACTOR_LEGS_SIZE]){
		LOG_ERROR("Unable to find id/property node %s\n", cfg->name);
		return 0;
	}

	if (act->legs == NULL) {
		int i;
		act->legs = (legs_part*)calloc(actor_part_sizes[ACTOR_LEGS_SIZE], sizeof(legs_part));
		for (i = actor_part_sizes[ACTOR_LEGS_SIZE]; i--;) act->legs[i].mesh_index= -1;
	}

	legs = &(act->legs[col_idx]);
	ok = 1;
	for (item = cfg->children; item; item = item->next) {
		if (item->type == XML_ELEMENT_NODE) {
			if (xmlStrcasecmp (item->name, (xmlChar*)"skin") == 0) {
				get_string_value (legs->legs_name, sizeof (legs->legs_name), item);
			} else if (xmlStrcasecmp (item->name, (xmlChar*)"mesh") == 0) {
				get_string_value (legs->model_name, sizeof (legs->model_name), item);
				legs->mesh_index = cal_load_mesh (act, legs->model_name, "legs");
			} else if (xmlStrcasecmp (item->name, (xmlChar*)"legsmask") == 0) {
				get_string_value (legs->legs_mask, sizeof (legs->legs_mask), item);
			} else if (xmlStrcasecmp (item->name, (xmlChar*)"glow") == 0) {
				int mode = find_description_index (glow_mode_dict, (char*)item->children->content, "glow mode");
				if (mode < 0) mode = GLOW_NONE;
				legs->glow = mode;
			} else {
				LOG_ERROR("unknown legs property \"%s\"", item->name);
				ok = 0;
			}
		}
	}

	// check for default entries, if found, use them to fill in missing data
	if(defaults){
		const xmlNode *default_node = get_default_node(cfg, defaults);

		if(default_node){
			if (!*legs->legs_name)
				get_item_string_value(legs->legs_name, sizeof(legs->legs_name), default_node, (xmlChar*)"skin");
			if (!*legs->model_name){
				get_item_string_value(legs->model_name, sizeof(legs->model_name), default_node, (xmlChar*)"mesh");
				legs->mesh_index= cal_load_mesh(act, legs->model_name, "legs");
			}
		}
	}

	// check the critical information
	actor_check_string(act, "legs", "skin", legs->legs_name);
	actor_check_string(act, "legs", "model", legs->model_name);
	actor_check_int(act, "legs", "mesh", legs->mesh_index);


	return ok;
}

int parse_actor_weapon_detail (actor_types *act, weapon_part *weapon, const xmlNode *cfg, const xmlNode *defaults)
{
	const xmlNode *item;
	char str[255];
	char name[256];
	int ok, index;

	if (cfg == NULL || cfg->children == NULL) return 0;

	ok = 1;
	for (item = cfg->children; item; item = item->next)
	{
		if (item->type == XML_ELEMENT_NODE)
		{
			safe_strncpy(name, (const char*)item->name, sizeof(name));
			my_tolower(name);

			if (!strcmp(name, "mesh")) {
				get_string_value (weapon->model_name, sizeof (weapon->model_name), item);
				weapon->mesh_index = cal_load_weapon_mesh (act, weapon->model_name, "weapon");
			} else if (!strcmp(name, "skin")) {
				get_string_value (weapon->skin_name, sizeof (weapon->skin_name), item);
			} else if (!strcmp(name, "skinmask")) {
				get_string_value (weapon->skin_mask, sizeof (weapon->skin_mask), item);
			} else if (!strcmp(name, "glow")) {
				int mode = find_description_index(glow_mode_dict, (char*)item->children->content, "glow mode");
				if (mode < 0) mode = GLOW_NONE;
				weapon->glow = mode;
			} else if (!strcmp(name, "snd_attack_up1")) {
				get_string_value (str,sizeof(str),item);
     			cal_set_anim_sound(&weapon->cal_frames[cal_weapon_attack_up_1_frame], str, get_string_property(item, "sound_scale"));
			} else if (!strcmp(name, "snd_attack_up2")) {
				get_string_value (str,sizeof(str),item);
     			cal_set_anim_sound(&weapon->cal_frames[cal_weapon_attack_up_2_frame], str, get_string_property(item, "sound_scale"));
			} else if (!strcmp(name, "snd_attack_up3")) {
				get_string_value (str,sizeof(str),item);
     			cal_set_anim_sound(&weapon->cal_frames[cal_weapon_attack_up_3_frame], str, get_string_property(item, "sound_scale"));
			} else if (!strcmp(name, "snd_attack_up4")) {
				get_string_value (str,sizeof(str),item);
     			cal_set_anim_sound(&weapon->cal_frames[cal_weapon_attack_up_4_frame], str, get_string_property(item, "sound_scale"));
			} else if (!strcmp(name, "snd_attack_up5")) {
				get_string_value (str,sizeof(str),item);
     			cal_set_anim_sound(&weapon->cal_frames[cal_weapon_attack_up_5_frame], str, get_string_property(item, "sound_scale"));
			} else if (!strcmp(name, "snd_attack_up6")) {
				get_string_value (str,sizeof(str),item);
     			cal_set_anim_sound(&weapon->cal_frames[cal_weapon_attack_up_6_frame], str, get_string_property(item, "sound_scale"));
			} else if (!strcmp(name, "snd_attack_up7")) {
				get_string_value (str,sizeof(str),item);
     			cal_set_anim_sound(&weapon->cal_frames[cal_weapon_attack_up_7_frame], str, get_string_property(item, "sound_scale"));
			} else if (!strcmp(name, "snd_attack_up8")) {
				get_string_value (str,sizeof(str),item);
     			cal_set_anim_sound(&weapon->cal_frames[cal_weapon_attack_up_8_frame], str, get_string_property(item, "sound_scale"));
			} else if (!strcmp(name, "snd_attack_up9")) {
				get_string_value (str,sizeof(str),item);
     			cal_set_anim_sound(&weapon->cal_frames[cal_weapon_attack_up_9_frame], str, get_string_property(item, "sound_scale"));
			} else if (!strcmp(name, "snd_attack_up10")) {
				get_string_value (str,sizeof(str),item);
     			cal_set_anim_sound(&weapon->cal_frames[cal_weapon_attack_up_10_frame], str, get_string_property(item, "sound_scale"));
			} else if (!strcmp(name, "snd_attack_down1")) {
				get_string_value (str,sizeof(str),item);
     			cal_set_anim_sound(&weapon->cal_frames[cal_weapon_attack_down_1_frame], str, get_string_property(item, "sound_scale"));
			} else if (!strcmp(name, "snd_attack_down2")) {
				get_string_value (str,sizeof(str),item);
     			cal_set_anim_sound(&weapon->cal_frames[cal_weapon_attack_down_2_frame], str, get_string_property(item, "sound_scale"));
			} else if (!strcmp(name, "snd_attack_down3")) {
				get_string_value (str,sizeof(str),item);
     			cal_set_anim_sound(&weapon->cal_frames[cal_weapon_attack_down_3_frame], str, get_string_property(item, "sound_scale"));
			} else if (!strcmp(name, "snd_attack_down4")) {
				get_string_value (str,sizeof(str),item);
     			cal_set_anim_sound(&weapon->cal_frames[cal_weapon_attack_down_4_frame], str, get_string_property(item, "sound_scale"));
			} else if (!strcmp(name, "snd_attack_down5")) {
				get_string_value (str,sizeof(str),item);
     			cal_set_anim_sound(&weapon->cal_frames[cal_weapon_attack_down_5_frame], str, get_string_property(item, "sound_scale"));
			} else if (!strcmp(name, "snd_attack_down6")) {
				get_string_value (str,sizeof(str),item);
     			cal_set_anim_sound(&weapon->cal_frames[cal_weapon_attack_down_6_frame], str, get_string_property(item, "sound_scale"));
			} else if (!strcmp(name, "snd_attack_down7")) {
				get_string_value (str,sizeof(str),item);
     			cal_set_anim_sound(&weapon->cal_frames[cal_weapon_attack_down_7_frame], str, get_string_property(item, "sound_scale"));
			} else if (!strcmp(name, "snd_attack_down8")) {
				get_string_value (str,sizeof(str),item);
     			cal_set_anim_sound(&weapon->cal_frames[cal_weapon_attack_down_8_frame], str, get_string_property(item, "sound_scale"));
			} else if (!strcmp(name, "snd_attack_down9")) {
				get_string_value (str,sizeof(str),item);
     			cal_set_anim_sound(&weapon->cal_frames[cal_weapon_attack_down_9_frame], str, get_string_property(item, "sound_scale"));
			} else if (!strcmp(name, "snd_attack_down10")) {
				get_string_value (str,sizeof(str),item);
     			cal_set_anim_sound(&weapon->cal_frames[cal_weapon_attack_down_10_frame], str, get_string_property(item, "sound_scale"));
			} else {
				index = -1;
				if (!strcmp(name, "cal_attack_up1")) {
					index = cal_weapon_attack_up_1_frame;
				} else if (!strcmp(name, "cal_attack_up2")) {
					index = cal_weapon_attack_up_2_frame;
				} else if (!strcmp(name, "cal_attack_up3")) {
					index = cal_weapon_attack_up_3_frame;
				} else if (!strcmp(name, "cal_attack_up4")) {
					index = cal_weapon_attack_up_4_frame;
				} else if (!strcmp(name, "cal_attack_up5")) {
					index = cal_weapon_attack_up_5_frame;
				} else if (!strcmp(name, "cal_attack_up6")) {
					index = cal_weapon_attack_up_6_frame;
				} else if (!strcmp(name, "cal_attack_up7")) {
					index = cal_weapon_attack_up_7_frame;
				} else if (!strcmp(name, "cal_attack_up8")) {
					index = cal_weapon_attack_up_8_frame;
				} else if (!strcmp(name, "cal_attack_up9")) {
					index = cal_weapon_attack_up_9_frame;
				} else if (!strcmp(name, "cal_attack_up10")) {
					index = cal_weapon_attack_up_10_frame;
				} else if (!strcmp(name, "cal_attack_down1")) {
					index = cal_weapon_attack_down_1_frame;
				} else if (!strcmp(name, "cal_attack_down2")) {
					index = cal_weapon_attack_down_2_frame;
				} else if (!strcmp(name, "cal_attack_down3")) {
					index = cal_weapon_attack_down_3_frame;
				} else if (!strcmp(name, "cal_attack_down4")) {
					index = cal_weapon_attack_down_4_frame;
				} else if (!strcmp(name, "cal_attack_down5")) {
					index = cal_weapon_attack_down_5_frame;
				} else if (!strcmp(name, "cal_attack_down6")) {
					index = cal_weapon_attack_down_6_frame;
				} else if (!strcmp(name, "cal_attack_down7")) {
					index = cal_weapon_attack_down_7_frame;
				} else if (!strcmp(name, "cal_attack_down8")) {
					index = cal_weapon_attack_down_8_frame;
				} else if (!strcmp(name, "cal_attack_down9")) {
					index = cal_weapon_attack_down_9_frame;
				} else if (!strcmp(name, "cal_attack_down10")) {
					index = cal_weapon_attack_down_10_frame;
				} else if (!strcmp(name, "cal_range_fire")) {
					index = cal_weapon_range_fire_frame;
				} else if (!strcmp(name, "cal_range_fire_out")) {
					index = cal_weapon_range_fire_out_frame;
				} else if (!strcmp(name, "cal_range_idle")) {
					index = cal_weapon_range_idle_frame;
				} else if (!strcmp(name, "cal_range_in")) {
					index = cal_weapon_range_in_frame;
				} else if (!strcmp(name, "cal_range_out")) {
					index = cal_weapon_range_out_frame;
				}
				if (index > -1)
				{
				get_string_value (str,sizeof(str),item);
					weapon->cal_frames[index] = cal_load_anim(act, str
					, get_string_property(item, "sound")
					, get_string_property(item, "sound_scale")
					, get_int_property(item, "duration")
					);
			}
				else
				{
					if(strstr((const char *)(item->name),"held")!=NULL) {
						//do not log this error, it's due to def files with more_attached_actors frames
					} else
					LOG_ERROR("unknown weapon property \"%s\"", item->name);
					ok = 0;
				}
			}
		}
		else if (item->type == XML_ENTITY_REF_NODE)
		{
			ok &= parse_actor_weapon_detail(act, weapon, item->children, defaults);
		}
	}


	return ok;
}

int parse_actor_weapon(actor_types *act, const xmlNode *cfg, const xmlNode *defaults)
{
	int ok, type_idx;
	weapon_part *weapon;

	if (cfg == NULL || cfg->children == NULL) return 0;

	type_idx= get_int_property(cfg, "id");
/*	if(type_idx < 0){
		type_idx= get_property(cfg, "type", "weapon type", weapon_type_dict);
	}
*/
	if(type_idx < 0 || type_idx >= actor_part_sizes[ACTOR_WEAPON_SIZE]){
		LOG_ERROR("Unable to find id/property node %s\n", cfg->name);
		return 0;
	}

	if (act->weapon == NULL) {
		int i, j;
		act->weapon = (weapon_part*)calloc(actor_part_sizes[ACTOR_WEAPON_SIZE], sizeof(weapon_part));
		for (i = actor_part_sizes[ACTOR_WEAPON_SIZE]; i--;) {
			act->weapon[i].mesh_index = -1;
			for (j = 0; j < NUM_WEAPON_FRAMES; j++) {
				act->weapon[i].cal_frames[j].anim_index = -1;
				act->weapon[i].cal_frames[j].sound = -1;
			}
		}
	}

	weapon = &(act->weapon[type_idx]);
	ok= parse_actor_weapon_detail(act, weapon, cfg, defaults);

	// check for default entries, if found, use them to fill in missing data
	if(defaults){
		const xmlNode *default_node= get_default_node(cfg, defaults);

		if(default_node){
			if (!*weapon->skin_name)
				get_item_string_value(weapon->skin_name, sizeof(weapon->skin_name), default_node, (xmlChar*)"skin");
			if(type_idx!=GLOVE_FUR && type_idx!=GLOVE_LEATHER && type_idx!=GANTS_CUIR_NOIR && type_idx!=GLOVE_FUR_LEO && type_idx!=GLOVE_LEATHER_3){ // these dont have meshes
				if (!*weapon->model_name){
					get_item_string_value(weapon->model_name, sizeof(weapon->model_name), default_node, (xmlChar*)"mesh");
					weapon->mesh_index= cal_load_weapon_mesh(act, weapon->model_name, "weapon");
				}
			}
			// TODO: combat animations
		}
	}

	// check the critical information
	if(type_idx!=WEAPON_NONE){   // no weapon doesn't have a skin/model
		actor_check_string(act, "weapon", "skin", weapon->skin_name);
		if(type_idx!=GLOVE_FUR && type_idx!=GLOVE_LEATHER && type_idx!=GANTS_CUIR_NOIR && type_idx!=GLOVE_FUR_LEO && type_idx!=GLOVE_LEATHER_3){ // these dont have meshes
			actor_check_string(act, "weapon", "model", weapon->model_name);
			actor_check_int(act, "weapon.mesh", weapon->model_name, weapon->mesh_index);
		}
		// TODO: check combat animations
	}


	return ok;
}

int parse_actor_body_part (actor_types *act, body_part *part, const xmlNode *cfg, const char *part_name, const xmlNode *default_node)
{
	const xmlNode *item;
	int ok = 1;

	if(cfg == NULL) return 0;

	for(item=cfg; item; item=item->next) {
		if(item->type == XML_ELEMENT_NODE) {
			if(xmlStrcasecmp(item->name, (xmlChar*)"mesh") == 0) {
				get_string_value (part->model_name, sizeof (part->model_name), item);
				if(strcmp("shield",part_name)==0)
					part->mesh_index = cal_load_weapon_mesh (act, part->model_name, part_name);
				else
					part->mesh_index = cal_load_mesh (act, part->model_name, part_name);
			} else if(xmlStrcasecmp(item->name, (xmlChar*)"skin") == 0) {
				get_string_value (part->skin_name, sizeof (part->skin_name), item);
			} else if(xmlStrcasecmp(item->name, (xmlChar*)"skinmask") == 0) {
				get_string_value (part->skin_mask, sizeof (part->skin_mask), item);
			} else if(xmlStrcasecmp(item->name, (xmlChar*)"glow") == 0) {
				int mode = find_description_index (glow_mode_dict, (char*)item->children->content, "glow mode");
				if(mode < 0) mode = GLOW_NONE;
				part->glow= mode;
			} else {
				LOG_ERROR("unknown %s property \"%s\"", part_name, item->name);
				ok = 0;
			}
		}
	}

	// check for default entries, if found, use them to fill in missing data
	if(default_node){
		if (!*part->skin_name)
			if(strcmp(part_name, "head")){ // heads don't have separate skins here
				get_item_string_value(part->skin_name, sizeof(part->skin_name), default_node, (xmlChar*)"skin");
			}
		if (!*part->model_name){
			get_item_string_value(part->model_name, sizeof(part->model_name), default_node, (xmlChar*)"mesh");
			if(strcmp("shield",part_name)==0)
				part->mesh_index= cal_load_weapon_mesh(act, part->model_name, part_name);
			else
				part->mesh_index= cal_load_mesh(act, part->model_name, part_name);
		}
	}

	// check the critical information
	if(strcmp(part_name, "head")){ // heads don't have separate skins here
		actor_check_string(act, part_name, "skin", part->skin_name);
	}
	actor_check_string(act, part_name, "model", part->model_name);
	actor_check_int(act, part_name, "mesh", part->mesh_index);


	return ok;
}

int parse_actor_helmet (actor_types *act, const xmlNode *cfg, const xmlNode *defaults)
{
	const xmlNode *default_node= get_default_node(cfg, defaults);
	int type_idx;
	body_part *helmet;

	if(cfg == NULL || cfg->children == NULL) return 0;

	type_idx= get_int_property(cfg, "id");
/*	if(type_idx < 0){
		type_idx= get_property(cfg, "type", "helmet type", helmet_type_dict);
	}
*/
	if(type_idx < 0 || type_idx >= actor_part_sizes[ACTOR_HELMET_SIZE]){
		LOG_ERROR("Unable to find id/property node %s\n", cfg->name);
		return 0;
	}

	if (act->helmet == NULL) {
		int i;
		act->helmet = (body_part*)calloc(actor_part_sizes[ACTOR_HELMET_SIZE], sizeof(body_part));
		for (i = actor_part_sizes[ACTOR_HELMET_SIZE]; i--;) act->helmet[i].mesh_index= -1;
	}

	helmet= &(act->helmet[type_idx]);


	return parse_actor_body_part(act,helmet, cfg->children, "helmet", default_node);
}

int parse_actor_neck (actor_types *act, const xmlNode *cfg, const xmlNode *defaults)
{
	const xmlNode *default_node = get_default_node(cfg, defaults);
	int type_idx;
	body_part *neck;

	if(cfg == NULL || cfg->children == NULL) return 0;

	type_idx= get_int_property(cfg, "id");

	if(type_idx < 0 || type_idx >= actor_part_sizes[ACTOR_NECK_SIZE]){
		LOG_ERROR("Unable to find id/property node %s\n", cfg->name);
		return 0;
	}

	if (act->neck == NULL) {
		int i;
		act->neck = (body_part*)calloc(actor_part_sizes[ACTOR_NECK_SIZE], sizeof(body_part));
		for (i = actor_part_sizes[ACTOR_NECK_SIZE]; i--;) act->neck[i].mesh_index= -1;
	}

	neck= &(act->neck[type_idx]);

	return parse_actor_body_part(act,neck, cfg->children, "neck", default_node);
}

int parse_actor_sounds(actor_types *act, const xmlNode *cfg)
{
	const xmlNode *item;
	char str[255];
	int ok;
	int i;

	if (cfg == NULL) return 0;
	if (!have_sound_config) return 0;

	ok = 1;
	for (item = cfg; item; item = item->next) {
		if (item->type == XML_ELEMENT_NODE) {
			get_string_value (str,sizeof(str),item);
			if (xmlStrcasecmp (item->name, (xmlChar*)"walk") == 0) {
				cal_set_anim_sound(&act->cal_frames[cal_actor_walk_frame], str, get_string_property(item, "sound_scale"));
			} else if (xmlStrcasecmp (item->name, (xmlChar*)"run") == 0) {
				cal_set_anim_sound(&act->cal_frames[cal_actor_run_frame], str, get_string_property(item, "sound_scale"));
			} else if (xmlStrcasecmp (item->name, (xmlChar*)"die1") == 0) {
				cal_set_anim_sound(&act->cal_frames[cal_actor_die1_frame], str, get_string_property(item, "sound_scale"));
			} else if (xmlStrcasecmp (item->name, (xmlChar*)"die2") == 0) {
				cal_set_anim_sound(&act->cal_frames[cal_actor_die2_frame], str, get_string_property(item, "sound_scale"));
			} else if (xmlStrcasecmp (item->name, (xmlChar*)"pain1") == 0) {
				cal_set_anim_sound(&act->cal_frames[cal_actor_pain1_frame], str, get_string_property(item, "sound_scale"));
			} else if (xmlStrcasecmp (item->name, (xmlChar*)"pain2") == 0) {
				cal_set_anim_sound(&act->cal_frames[cal_actor_pain2_frame], str, get_string_property(item, "sound_scale"));
			} else if (xmlStrcasecmp (item->name, (xmlChar*)"pick") == 0) {
				cal_set_anim_sound(&act->cal_frames[cal_actor_pick_frame], str, get_string_property(item, "sound_scale"));
			} else if (xmlStrcasecmp (item->name, (xmlChar*)"drop") == 0) {
				cal_set_anim_sound(&act->cal_frames[cal_actor_drop_frame], str, get_string_property(item, "sound_scale"));
			} else if (xmlStrcasecmp (item->name, (xmlChar*)"harvest") == 0) {
				cal_set_anim_sound(&act->cal_frames[cal_actor_harvest_frame], str, get_string_property(item, "sound_scale"));
			} else if (xmlStrcasecmp (item->name, (xmlChar*)"attack_cast") == 0) {
				cal_set_anim_sound(&act->cal_frames[cal_actor_attack_cast_frame], str, get_string_property(item, "sound_scale"));
			} else if (xmlStrcasecmp (item->name, (xmlChar*)"attack_ranged") == 0) {
				cal_set_anim_sound(&act->cal_frames[cal_actor_attack_ranged_frame], str, get_string_property(item, "sound_scale"));
			} else if (xmlStrcasecmp (item->name, (xmlChar*)"sit_down") == 0) {
				cal_set_anim_sound(&act->cal_frames[cal_actor_sit_down_frame], str, get_string_property(item, "sound_scale"));
			} else if (xmlStrcasecmp (item->name, (xmlChar*)"stand_up") == 0) {
				cal_set_anim_sound(&act->cal_frames[cal_actor_stand_up_frame], str, get_string_property(item, "sound_scale"));
			// These sounds are only found in the <sounds> block as they aren't tied to an animation
			} else if (xmlStrcasecmp (item->name, (xmlChar*)"battlecry") == 0) {
				i = get_index_for_sound_type_name(str);
				if (i == -1)
					LOG_ERROR("Unknown battlecry sound (%s) in actor def: %s", str, act->actor_name);
				else
				{
					act->battlecry.sound = i;
					safe_strncpy(str, get_string_property(item, "sound_scale"), sizeof(str));
					if (strcasecmp(str, ""))
						act->battlecry.scale = atof(str);
					else
						act->battlecry.scale = 1.0f;
				}
			} else {
				LOG_ERROR("Unknown sound \"%s\"", item->name);
				ok = 0;
			}
		}
	}

	return ok;
}

int parse_actor_cape (actor_types *act, const xmlNode *cfg, const xmlNode *defaults)
{
	const xmlNode *default_node = get_default_node(cfg, defaults);
	int type_idx;
	body_part *cape;

	if(cfg == NULL || cfg->children == NULL) return 0;

	type_idx= get_int_property(cfg, "id");
/*	if(type_idx < 0){
		type_idx= get_property(cfg, "color", "cape color", cape_color_dict);
	}
*/
	if(type_idx < 0 || type_idx >= actor_part_sizes[ACTOR_CAPE_SIZE]){
		LOG_ERROR("Unable to find id/property node %s\n", cfg->name);
		return 0;
	}

	if (act->cape == NULL) {
		int i;
		act->cape = (body_part*)calloc(actor_part_sizes[ACTOR_CAPE_SIZE], sizeof(body_part));
		for (i = actor_part_sizes[ACTOR_CAPE_SIZE]; i--;) act->cape[i].mesh_index= -1;
	}

	cape= &(act->cape[type_idx]);


	return parse_actor_body_part(act,cape, cfg->children, "cape", default_node);
}

int parse_actor_head (actor_types *act, const xmlNode *cfg, const xmlNode *defaults)
{
	const xmlNode *default_node= get_default_node(cfg, defaults);
	int type_idx;
	body_part *head;

	if(cfg == NULL || cfg->children == NULL) return 0;

	type_idx= get_int_property(cfg, "id");
	if(type_idx < 0){
		type_idx= get_property(cfg, "number", "head number", head_number_dict);
	}

	if(type_idx < 0 || type_idx >= actor_part_sizes[ACTOR_HEAD_SIZE]){
		LOG_ERROR("Unable to find id/property node %s\n", cfg->name);
		return 0;
	}

	if (act->head == NULL) {
		int i;
		act->head = (body_part*)calloc(actor_part_sizes[ACTOR_HEAD_SIZE], sizeof(body_part));
		for (i = actor_part_sizes[ACTOR_HEAD_SIZE]; i--;) act->head[i].mesh_index= -1;
	}

	head= &(act->head[type_idx]);



	return parse_actor_body_part(act, head, cfg->children, "head", default_node);
}

int parse_actor_shield_part (actor_types *act, shield_part *part, const xmlNode *cfg, const xmlNode *default_node)
{
	const xmlNode *item;
	int ok = 1;

	if(cfg == NULL) return 0;

	for(item=cfg; item; item=item->next) {
		if(item->type == XML_ELEMENT_NODE) {
			if(xmlStrcasecmp(item->name, (xmlChar*)"mesh") == 0) {
				get_string_value (part->model_name, sizeof (part->model_name), item);
				part->mesh_index = cal_load_weapon_mesh (act, part->model_name, "shield");
			} else if(xmlStrcasecmp(item->name, (xmlChar*)"skin") == 0) {
				get_string_value (part->skin_name, sizeof (part->skin_name), item);
			} else if(xmlStrcasecmp(item->name, (xmlChar*)"skinmask") == 0) {
				get_string_value (part->skin_mask, sizeof (part->skin_mask), item);
			} else if(xmlStrcasecmp(item->name, (xmlChar*)"glow") == 0) {
				int mode = find_description_index (glow_mode_dict, (char*)item->children->content, "glow mode");
				if(mode < 0) mode = GLOW_NONE;
				part->glow= mode;
			} else {
				LOG_ERROR("unknown shield property \"%s\"", item->name);
				ok = 0;
			}
		}
	}

	// check for default entries, if found, use them to fill in missing data
	if(default_node){
		if (!*part->model_name){
			get_item_string_value(part->model_name, sizeof(part->model_name), default_node, (xmlChar*)"mesh");
			part->mesh_index= cal_load_weapon_mesh(act, part->model_name, "shield");
		}
	}

	// check the critical information
	actor_check_string(act, "shield", "model", part->model_name);
	actor_check_int(act, "shield", "mesh", part->mesh_index);


	return ok;
}

int parse_actor_shield (actor_types *act, const xmlNode *cfg, const xmlNode *defaults)
{
	const xmlNode *default_node = get_default_node(cfg, defaults);
	int type_idx;
	shield_part *shield;

	if(cfg == NULL || cfg->children == NULL) return 0;

	type_idx= get_int_property(cfg, "id");
/*	if(type_idx < 0){
		type_idx= get_property(cfg, "type", "shield type", shield_type_dict);
	}
*/
	if(type_idx < 0 || type_idx >= actor_part_sizes[ACTOR_SHIELD_SIZE]){
		LOG_ERROR("Unable to find id/property node %s\n", cfg->name);
		return 0;
	}

	if (act->shield == NULL) {
		int i;
		act->shield = (shield_part*)calloc(actor_part_sizes[ACTOR_SHIELD_SIZE], sizeof(shield_part));
		for (i = actor_part_sizes[ACTOR_SHIELD_SIZE]; i--;) {
			act->shield[i].mesh_index = -1;
		}
	}

	shield= &(act->shield[type_idx]);


	return parse_actor_shield_part(act, shield, cfg->children, default_node);
}

int parse_actor_hair (actor_types *act, const xmlNode *cfg, const xmlNode *defaults)
{
	int col_idx;
	size_t len;
	char *buf;

	if(cfg == NULL || cfg->children == NULL) return 0;

	col_idx= get_int_property(cfg, "id");
/*	if(col_idx < 0){
		col_idx= get_property(cfg, "color", "hair color", hair_color_dict);
	}
*/
	if(col_idx < 0 || col_idx >= actor_part_sizes[ACTOR_HAIR_SIZE]){
		LOG_ERROR("Unable to find id/property node %s\n", cfg->name);
		return 0;
	}

	if (act->hair == NULL) {
		int i;
		act->hair = (hair_part*)calloc(actor_part_sizes[ACTOR_HAIR_SIZE], sizeof(hair_part));
		for (i = actor_part_sizes[ACTOR_HAIR_SIZE]; i--;) act->hair[i].mesh_index= -1;
	}

	buf= act->hair[col_idx].hair_name;
	len= sizeof (act->hair[col_idx].hair_name);
	get_string_value(buf, len, cfg);


	return 1;
}


int cal_get_idle_group(actor_types *act,char *name)
{
	int i;
	int res=-1;

	for (i=0;i<act->group_count;++i) {
		if (strcmp(name,act->idle_group[i].name)==0) res=i;
	}

	if (res>=0) return res;//Found it, return

	//Create a new named group
	res=act->group_count;
	safe_strncpy(act->idle_group[res].name, name, sizeof(act->idle_group[res].name));
	++act->group_count;

	return res;
}

struct cal_anim cal_load_idle(actor_types *act, char *str)
{
	struct cal_anim res={-1,0,0,0.0f
	,-1
	,0.0f
	};
	struct CalCoreAnimation *coreanim;

	res.anim_index=CalCoreModel_ELLoadCoreAnimation(act->coremodel,str,act->scale);
	if(res.anim_index == -1) {
		LOG_ERROR("cal3d erreur : %s : %s\n", str, CalError_GetLastErrorDescription());
		return res;
	}
	coreanim=CalCoreModel_GetCoreAnimation(act->coremodel,res.anim_index);

	if (coreanim) {
		res.duration=CalCoreAnimation_GetDuration(coreanim);
	} else {
		LOG_ERROR("No Anim: %s\n",str);
	}

	return res;
}

void cal_group_addanim(actor_types *act,int gindex, char *fanim)
{
	int i;

	i=act->idle_group[gindex].count;
	act->idle_group[gindex].anim[i]=cal_load_idle(act,fanim);
	//LOG_TO_CONSOLE(c_green2,fanim);
	++act->idle_group[gindex].count;
}

void parse_idle_group (actor_types *act, const char *str)
{
	char gname[255]={0};
	char fname[255]={0};
	//char temp[255];
	int gindex;

	if (sscanf (str, "%254s %254s", gname, fname) != 2) return;

	gindex=cal_get_idle_group(act,gname);
	cal_group_addanim(act,gindex,fname);
	//safe_snprintf(temp, sizeof(temp), "%d",gindex);
	//LOG_TO_CONSOLE(c_green2,gname);
	//LOG_TO_CONSOLE(c_green2,fname);
	//LOG_TO_CONSOLE(c_green2,temp);
}

int parse_actor_frames (actor_types *act, const xmlNode *cfg, const xmlNode *defaults)
{
	const xmlNode *item;
	char str[255];
	int ok = 1, index;

	if (cfg == NULL) return 0;

	for (item = cfg; item; item = item->next) {
		if (item->type == XML_ELEMENT_NODE) {
			index = -1;
			if (xmlStrcasecmp (item->name, (xmlChar*)"CAL_IDLE_GROUP") == 0) {
				get_string_value (str,sizeof(str),item);
     				//act->cal_walk_frame=cal_load_anim(act,str);
				//LOG_TO_CONSOLE(c_green2,str);
				parse_idle_group(act,str);
				//Not functional!
				index = -2;
			} else if (xmlStrcasecmp (item->name, (xmlChar*)"CAL_walk") == 0) {
				index = cal_actor_walk_frame;
			} else if (xmlStrcasecmp (item->name, (xmlChar*)"CAL_run") == 0) {
				index = cal_actor_run_frame;
			} else if (xmlStrcasecmp (item->name, (xmlChar*)"CAL_die1") == 0) {
				index = cal_actor_die1_frame;
			} else if (xmlStrcasecmp (item->name, (xmlChar*)"CAL_die2") == 0) {
				index = cal_actor_die2_frame;
			} else if (xmlStrcasecmp (item->name, (xmlChar*)"CAL_pain1") == 0) {
				index = cal_actor_pain1_frame;
			} else if (xmlStrcasecmp (item->name, (xmlChar*)"CAL_pain2") == 0) {
				index = cal_actor_pain2_frame;
			} else if (xmlStrcasecmp (item->name, (xmlChar*)"CAL_pick") == 0) {
				index = cal_actor_pick_frame;
			} else if (xmlStrcasecmp (item->name, (xmlChar*)"CAL_drop") == 0) {
				index = cal_actor_drop_frame;
			} else if (xmlStrcasecmp (item->name, (xmlChar*)"CAL_idle") == 0) {
				index = cal_actor_idle1_frame;
			} else if (xmlStrcasecmp (item->name, (xmlChar*)"CAL_idle2") == 0) {
				index = cal_actor_idle2_frame;
			} else if (xmlStrcasecmp (item->name, (xmlChar*)"CAL_idle_sit") == 0) {
				index = cal_actor_idle_sit_frame;
 			} else if (xmlStrcasecmp (item->name, (xmlChar*)"CAL_harvest") == 0) {
				index = cal_actor_harvest_frame;
			} else if (xmlStrcasecmp (item->name, (xmlChar*)"CAL_attack_cast") == 0) {
				index = cal_actor_attack_cast_frame;
			} else if (xmlStrcasecmp (item->name, (xmlChar*)"CAL_sit_down") == 0) {
				index = cal_actor_sit_down_frame;
			} else if (xmlStrcasecmp (item->name, (xmlChar*)"CAL_stand_up") == 0) {
				index = cal_actor_stand_up_frame;
			} else if (xmlStrcasecmp (item->name, (xmlChar*)"CAL_in_combat") == 0) {
				index = cal_actor_in_combat_frame;
			} else if (xmlStrcasecmp (item->name, (xmlChar*)"CAL_out_combat") == 0) {
				index = cal_actor_out_combat_frame;
			} else if (xmlStrcasecmp (item->name, (xmlChar*)"CAL_combat_idle") == 0) {
				index = cal_actor_combat_idle_frame;
			} else if (xmlStrcasecmp (item->name, (xmlChar*)"CAL_attack_up_1") == 0) {
				index = cal_actor_attack_up_1_frame;
			} else if (xmlStrcasecmp (item->name, (xmlChar*)"CAL_attack_up_2") == 0) {
				index = cal_actor_attack_up_2_frame;
			} else if (xmlStrcasecmp (item->name, (xmlChar*)"CAL_attack_up_3") == 0) {
				index = cal_actor_attack_up_3_frame;
			} else if (xmlStrcasecmp (item->name, (xmlChar*)"CAL_attack_up_4") == 0) {
				index = cal_actor_attack_up_4_frame;
			} else if (xmlStrcasecmp (item->name, (xmlChar*)"CAL_attack_up_5") == 0) {
				index = cal_actor_attack_up_5_frame;
			} else if (xmlStrcasecmp (item->name, (xmlChar*)"CAL_attack_up_6") == 0) {
				index = cal_actor_attack_up_6_frame;
			} else if (xmlStrcasecmp (item->name, (xmlChar*)"CAL_attack_up_7") == 0) {
				index = cal_actor_attack_up_7_frame;
			} else if (xmlStrcasecmp (item->name, (xmlChar*)"CAL_attack_up_8") == 0) {
				index = cal_actor_attack_up_8_frame;
			} else if (xmlStrcasecmp (item->name, (xmlChar*)"CAL_attack_up_9") == 0) {
				index = cal_actor_attack_up_9_frame;
			} else if (xmlStrcasecmp (item->name, (xmlChar*)"CAL_attack_up_10") == 0) {
				index = cal_actor_attack_up_10_frame;
			} else if (xmlStrcasecmp (item->name, (xmlChar*)"CAL_attack_down_1") == 0) {
				index = cal_actor_attack_down_1_frame;
			} else if (xmlStrcasecmp (item->name, (xmlChar*)"CAL_attack_down_2") == 0) {
				index = cal_actor_attack_down_2_frame;
			} else if (xmlStrcasecmp (item->name, (xmlChar*)"CAL_attack_down_3") == 0) {
				index = cal_actor_attack_down_3_frame;
			} else if (xmlStrcasecmp (item->name, (xmlChar*)"CAL_attack_down_4") == 0) {
				index = cal_actor_attack_down_4_frame;
			} else if (xmlStrcasecmp (item->name, (xmlChar*)"CAL_attack_down_5") == 0) {
				index = cal_actor_attack_down_5_frame;
			} else if (xmlStrcasecmp (item->name, (xmlChar*)"CAL_attack_down_6") == 0) {
				index = cal_actor_attack_down_6_frame;
			} else if (xmlStrcasecmp (item->name, (xmlChar*)"CAL_attack_down_7") == 0) {
				index = cal_actor_attack_down_7_frame;
			} else if (xmlStrcasecmp (item->name, (xmlChar*)"CAL_attack_down_8") == 0) {
				index = cal_actor_attack_down_8_frame;
			} else if (xmlStrcasecmp (item->name, (xmlChar*)"CAL_attack_down_9") == 0) {
				index = cal_actor_attack_down_9_frame;
			} else if (xmlStrcasecmp (item->name, (xmlChar*)"CAL_attack_down_10") == 0) {
				index = cal_actor_attack_down_10_frame;
            } else if (xmlStrcasecmp (item->name, (xmlChar*)"CAL_salut") == 0) {
                index = cal_actor_salut_frame;
            } else if (xmlStrcasecmp (item->name, (xmlChar*)"CAL_danse") == 0) {
                index = cal_actor_danse_frame;
            } else if (xmlStrcasecmp (item->name, (xmlChar*)"CAL_salto") == 0) {
                index = cal_actor_salto_frame;
            } else if (xmlStrcasecmp (item->name, (xmlChar*)"CAL_roue") == 0) {
                index = cal_actor_roue_frame;
			}

			if (index >= 0)
			{
				get_string_value(str, sizeof(str), item);
     			act->cal_frames[index] = cal_load_anim(act, str
					, get_string_property(item, "sound")
					, get_string_property(item, "sound_scale")
					, get_int_property(item, "duration")
					);




			}
			else if (index != -2)
			{
				LOG_ERROR("unknown frame property \"%s\"", item->name);
				ok = 0;
			}
		}
	}

	return ok;
}

int parse_actor_attachment (actor_types *act, const xmlNode *cfg, int actor_type)
{
	const xmlNode *item;
	int ok = 1;
	attached_actors_types *att = &attached_actors_defs[act->actor_type];
	actor_types *held_act = NULL;
	char str[256];
	struct CalCoreSkeleton *skel;

	if (cfg == NULL || cfg->children == NULL) return 0;

	for (item = cfg->children; item; item = item->next) {
		if (item->type == XML_ELEMENT_NODE) {
			if (xmlStrcasecmp (item->name, (xmlChar*)"holder") == 0) {
				att->actor_type[actor_type].is_holder = get_bool_value(item);
				if (att->actor_type[actor_type].is_holder)
					held_act = &actors_defs[actor_type];
				else
					held_act = act;
			} else if (xmlStrcasecmp (item->name, (xmlChar*)"parent_bone") == 0) {
				get_string_value (str, sizeof (str), item);
				skel = CalCoreModel_GetCoreSkeleton(actors_defs[actor_type].coremodel);
				if (skel) {
					att->actor_type[actor_type].parent_bone_id = find_core_bone_id(skel, str);
					if (att->actor_type[actor_type].parent_bone_id < 0) {
						LOG_ERROR("bone %s was not found in skeleton of actor type %d", str, actor_type);
						ok = 0;
					}
				}
				else {
					LOG_ERROR("the skeleton for actor type %d doesn't exist!", actor_type);
					ok = 0;
				}
			} else if (xmlStrcasecmp (item->name, (xmlChar*)"local_bone") == 0) {
				get_string_value (str, sizeof (str), item);
				skel = CalCoreModel_GetCoreSkeleton(act->coremodel);
				if (skel) {
					att->actor_type[actor_type].local_bone_id = find_core_bone_id(skel, str);
					if (att->actor_type[actor_type].local_bone_id < 0) {
						LOG_ERROR("bone %s was not found in skeleton of actor type %d", str, act->actor_type);
						ok = 0;
					}
				}
				else {
					LOG_ERROR("the skeleton for actor type %d doesn't exist!", act->actor_type);
					ok = 0;
				}
			} else if (xmlStrcasecmp (item->name, (xmlChar*)"held_shift") == 0) {
				xmlAttr *attr;
				for (attr = item->properties; attr; attr = attr->next)
					if (attr->type == XML_ATTRIBUTE_NODE)
					{
						if (xmlStrcasecmp (attr->name, (xmlChar*)"x") == 0)
							att->actor_type[actor_type].shift[0] = atof((char*)attr->children->content);
						else if (xmlStrcasecmp (attr->name, (xmlChar*)"y") == 0)
							att->actor_type[actor_type].shift[1] = atof((char*)attr->children->content);
						else if (xmlStrcasecmp (attr->name, (xmlChar*)"z") == 0)
							att->actor_type[actor_type].shift[2] = atof((char*)attr->children->content);
						else {
							LOG_ERROR("unknown attachment shift attribute \"%s\"", attr->name);
							ok = 0;
						}
					}
			} else if (xmlStrcasecmp (item->name, (xmlChar*)"CAL_held_walk") == 0) {
				get_string_value (str, sizeof(str), item);
     			att->actor_type[actor_type].cal_frames[cal_attached_walk_frame] = cal_load_anim(held_act, str
					, get_string_property(item, "sound")
					, get_string_property(item, "sound_scale")
					, get_int_property(item, "duration")
					);
			} else if (xmlStrcasecmp (item->name, (xmlChar*)"CAL_held_run") == 0) {
				get_string_value (str, sizeof(str), item);
     			att->actor_type[actor_type].cal_frames[cal_attached_run_frame] = cal_load_anim(held_act, str
					, get_string_property(item, "sound")
					, get_string_property(item, "sound_scale")
					, get_int_property(item, "duration")
					);
			} else if (xmlStrcasecmp (item->name, (xmlChar*)"CAL_held_idle") == 0) {
				get_string_value (str,sizeof(str),item);
     			att->actor_type[actor_type].cal_frames[cal_attached_idle_frame] = cal_load_anim(held_act, str
					, get_string_property(item, "sound")
					, get_string_property(item, "sound_scale")
					, get_int_property(item, "duration")
					);
			} else if (xmlStrcasecmp (item->name, (xmlChar*)"CAL_held_pain") == 0) {
				get_string_value (str,sizeof(str),item);
     			att->actor_type[actor_type].cal_frames[cal_attached_pain_frame] = cal_load_anim(held_act, str
					, get_string_property(item, "sound")
					, get_string_property(item, "sound_scale")
					, get_int_property(item, "duration")
					);
			} else {
				LOG_ERROR("unknown attachment property \"%s\"", item->name);
				ok = 0;
			}
		} else if (item->type == XML_ENTITY_REF_NODE) {
			ok &= parse_actor_attachment(act, item->children, actor_type);
		}
	}

	return ok;
}

int parse_actor_boots (actor_types *act, const xmlNode *cfg, const xmlNode *defaults)
{
	const xmlNode *item;
	int ok, col_idx;
	boots_part *boots;

	if (cfg == NULL || cfg->children == NULL) return 0;

	col_idx= get_int_property(cfg, "id");
/*	if(col_idx < 0){
		col_idx = get_property (cfg, "color", "boots color", boots_color_dict);
	}
*/
	if(col_idx < 0 || col_idx >= actor_part_sizes[ACTOR_BOOTS_SIZE]){
		LOG_ERROR("Unable to find id/property node %s\n", cfg->name);
		return 0;
	}

	if (act->boots == NULL) {
		int i;
		act->boots = (boots_part*)calloc(actor_part_sizes[ACTOR_BOOTS_SIZE], sizeof(boots_part));
		for (i = actor_part_sizes[ACTOR_BOOTS_SIZE]; i--;) act->boots[i].mesh_index= -1;
	}

	boots = &(act->boots[col_idx]);
	ok = 1;
	for (item = cfg->children; item; item = item->next) {
		if (item->type == XML_ELEMENT_NODE) {
			if (xmlStrcasecmp (item->name, (xmlChar*)"skin") == 0) {
				get_string_value (boots->boots_name, sizeof (boots->boots_name), item);
			} else if (xmlStrcasecmp (item->name, (xmlChar*)"mesh") == 0) {
				get_string_value (boots->model_name, sizeof (boots->model_name), item);
				boots->mesh_index = cal_load_mesh (act, boots->model_name, "boots");
			} else if (xmlStrcasecmp (item->name, (xmlChar*)"bootsmask") == 0) {
				get_string_value (boots->boots_mask, sizeof (boots->boots_mask), item);
			} else if (xmlStrcasecmp (item->name, (xmlChar*)"glow") == 0) {
				int mode = find_description_index (glow_mode_dict, (char*)item->children->content, "glow mode");
				if (mode < 0) mode = GLOW_NONE;
				boots->glow = mode;
			} else {
				LOG_ERROR("unknown legs property \"%s\"", item->name);
				ok = 0;
			}
		}
	}

	// check for default entries, if found, use them to fill in missing data
	if(defaults){
		const xmlNode *default_node = get_default_node(cfg, defaults);

		if(default_node){
			if (!*boots->boots_name)
				get_item_string_value(boots->boots_name, sizeof(boots->boots_name), default_node, (xmlChar*)"skin");
			if (!*boots->model_name){
				get_item_string_value(boots->model_name, sizeof(boots->model_name), default_node, (xmlChar*)"mesh");
				boots->mesh_index= cal_load_mesh(act, boots->model_name, "boots");
			}
		}
	}

	// check the critical information
	actor_check_string(act, "boots", "boots", boots->boots_name);
	actor_check_string(act, "boots", "model", boots->model_name);
	actor_check_int(act, "boots", "mesh", boots->mesh_index);

	return ok;
}

//Searches if a mesh is already loaded- TODO:MAKE THIS BETTER
int cal_search_mesh (actor_types *act, const char *fn, const char *kind)
{
	int i;

	if (kind == NULL)
	{
		return -1;
	}
	else if (act->head && strcmp (kind, "head") == 0)
	{
		for (i = 0; i < actor_part_sizes[ACTOR_HEAD_SIZE]; i++)
			if (strcmp (fn, act->head[i].model_name) == 0 && act->head[i].mesh_index != -1)
				return act->head[i].mesh_index;
	}
	else if (act->shirt && strcmp (kind, "shirt") == 0)
	{
		for (i = 0; i < actor_part_sizes[ACTOR_SHIRT_SIZE]; i++)
		{
			if (strcmp (fn, act->shirt[i].model_name) == 0 && act->shirt[i].mesh_index != -1)
				return act->shirt[i].mesh_index;
		}
	}
	else if (act->legs && strcmp (kind, "legs") == 0)
	{
		for (i = 0; i < actor_part_sizes[ACTOR_LEGS_SIZE]; i++)
		{
			if (strcmp (fn, act->legs[i].model_name) == 0 && act->legs[i].mesh_index != -1)
				return act->legs[i].mesh_index;
		}
	}
	else if (act->boots && strcmp (kind, "boots") == 0)
	{
		for (i = 0; i < actor_part_sizes[ACTOR_BOOTS_SIZE]; i++)
		{
			if (strcmp (fn, act->boots[i].model_name) == 0 && act->boots[i].mesh_index != -1)
				return act->boots[i].mesh_index;
		}
	}
	else if (act->cape && strcmp (kind, "cape") == 0)
	{
		for (i = 0; i < actor_part_sizes[ACTOR_CAPE_SIZE]; i++)
		{
			if (strcmp (fn, act->cape[i].model_name) == 0 && act->cape[i].mesh_index != -1)
				return act->cape[i].mesh_index;
		}
	}
	else if (act->helmet && strcmp (kind, "helmet") == 0)
	{
		for (i = 0; i < actor_part_sizes[ACTOR_HELMET_SIZE]; i++)
		{
			if (strcmp (fn, act->helmet[i].model_name) == 0 && act->helmet[i].mesh_index != -1)
				return act->helmet[i].mesh_index;
		}
	}
	else if (act->neck && strcmp (kind, "neck") == 0)
	{
		for (i = 0; i < actor_part_sizes[ACTOR_NECK_SIZE]; i++)
		{
			if (strcmp (fn, act->neck[i].model_name) == 0 && act->neck[i].mesh_index != -1)
				return act->neck[i].mesh_index;
		}
	}
	else if (act->shield && strcmp (kind, "shield") == 0)
	{
		for (i = 0; i < actor_part_sizes[ACTOR_SHIELD_SIZE]; i++)
		{
			if (strcmp (fn, act->shield[i].model_name) == 0 && act->shield[i].mesh_index != -1)
				return act->shield[i].mesh_index;
		}
	}
	else if (act->weapon && strcmp (kind, "weapon") == 0)
	{
		for (i = 0; i < actor_part_sizes[ACTOR_WEAPON_SIZE]; i++)
		{
			if (strcmp (fn, act->weapon[i].model_name) == 0 && act->weapon[i].mesh_index != -1)
				return act->weapon[i].mesh_index;
		}
	}

	return -1;
}

//Loads a Cal3D mesh
int cal_load_mesh(actor_types *act, const char *fn, const char *kind)
{
	int res;
	struct CalCoreMesh *mesh;

	if (fn==0) return -1;
	if (strlen(fn)==0) return -1;
	if (act->coremodel==NULL) return -1;
	if (kind != NULL)
	{
		res = cal_search_mesh (act, fn, kind);
		if (res != -1) return res;
	}

	//Load coremesh
	res=CalCoreModel_ELLoadCoreMesh(act->coremodel,fn);

	//Scale coremesh
	if (res >= 0) {
		mesh=CalCoreModel_GetCoreMesh(act->coremodel,res);
		if ((mesh)&&(act->mesh_scale!=1.0)) CalCoreMesh_Scale(mesh,act->mesh_scale);
	} else {
		LOG_ERROR("cal3d erreur : %s : %s\n", fn, CalError_GetLastErrorDescription());
	}

	return res;
}

int cal_load_weapon_mesh (actor_types *act, const char *fn, const char *kind)
{
	int res;
	struct CalCoreMesh *mesh;

	if (fn==0) return -1;
	if (strlen(fn)==0) return -1;
	if (act->coremodel==NULL) return -1;

	if (kind != NULL)
	{
		res = cal_search_mesh (act, fn, kind);
		if (res!=-1) return res;
	}

	//Load coremesh
	res=CalCoreModel_ELLoadCoreMesh(act->coremodel,fn);

	//Scale coremesh
	if (res>=0) {
		mesh=CalCoreModel_GetCoreMesh(act->coremodel,res);
		if ((mesh)&&(act->skel_scale!=1.0)) CalCoreMesh_Scale(mesh,act->skel_scale);
	} else {
		LOG_ERROR("cal3d erreur : %s : %s\n", fn, CalError_GetLastErrorDescription());
	}

	return res;
}

int parse_actor_nodes(actor_types *act, const xmlNode *cfg, const xmlNode *defaults)
{
	char name[256];
	const xmlNode *item;
	int	ok= 1;

	for (item=cfg->children; item; item=item->next)
	{
		if(item->type == XML_ELEMENT_NODE)
		{
			safe_strncpy(name, (const char*)item->name, sizeof(name));
			my_tolower(name);

			if (!strcmp(name, "ghost")) {
				act->ghost= get_bool_value(item);
			} else if (!strcmp(name, "skin")) {
				get_string_value(act->skin_name, sizeof (act->skin_name), item);
			} else if (!strcmp(name, "mesh")) {
				get_string_value(act->file_name, sizeof (act->file_name), item);
			} else if (!strcmp(name, "actor_scale")) {
				act->actor_scale= get_float_value(item);
			} else if (!strcmp(name, "scale")) {
				act->scale= get_float_value(item);
			} else if (!strcmp(name, "mesh_scale")) {
				act->mesh_scale= get_float_value(item);
			} else if (!strcmp(name, "bone_scale")) {
				act->skel_scale= get_float_value(item);
			} else if (!strcmp(name, "skeleton")) {
				char skeleton_name[MAX_FILE_PATH];
				get_string_value(skeleton_name, sizeof(skeleton_name), item);
				act->coremodel= CalCoreModel_New("Model");
				if(!CalCoreModel_ELLoadCoreSkeleton(act->coremodel, skeleton_name)) {
					LOG_ERROR("cal3d erreur : %s : %s\n", skeleton_name, CalError_GetLastErrorDescription());
					act->skeleton_type = -1;
				}
				else {
					act->skeleton_type = get_skeleton(act->coremodel, skeleton_name);
				}
			} else if (!strcmp(name, "walk_speed")) { // unused
				act->walk_speed= get_float_value(item);
			} else if (!strcmp(name, "run_speed")) { // unused
				act->run_speed= get_float_value(item);
			} else if (!strcmp(name, "defaults")) {
				defaults= item;
			} else if (!strcmp(name, "frames")) {
				ok &= parse_actor_frames(act, item->children, defaults);
			} else if (!strcmp(name, "shirt")) {
				ok &= parse_actor_shirt(act, item, defaults);
			} else if (!strcmp(name, "hskin")) {
				ok &= parse_actor_skin(act, item, defaults);
			} else if (!strcmp(name, "hair")) {
				ok &= parse_actor_hair(act, item, defaults);
			} else if (!strcmp(name, "boots")) {
				ok &= parse_actor_boots(act, item, defaults);
			} else if (!strcmp(name, "legs")) {
				ok &= parse_actor_legs(act, item, defaults);
			} else if (!strcmp(name, "cape")) {
				ok &= parse_actor_cape(act, item, defaults);
			} else if (!strcmp(name, "head")) {
				ok &= parse_actor_head(act, item, defaults);
			} else if (!strcmp(name, "shield")) {
				ok &= parse_actor_shield(act, item, defaults);
			} else if (!strcmp(name, "weapon")) {
				ok &= parse_actor_weapon(act, item, defaults);
			} else if (!strcmp(name, "helmet")) {
				ok &= parse_actor_helmet(act, item, defaults);
			} else if (!strcmp(name, "neck")) {

				ok &= parse_actor_neck(act, item, defaults);
			} else if (!strcmp(name, "sounds")) {
				ok &= parse_actor_sounds(act, item->children);
			} else if(xmlStrcasecmp(item->name, (xmlChar*)"actor_attachment") == 0) {
				int id = get_int_property(item, "id");
				if (id < 0 || id >= MAX_ACTOR_DEFS) {
					LOG_ERROR("Unable to find id/property node %s\n", item->name);
					ok = 0;
				}
				else
					ok &= parse_actor_attachment(act, item, id);
			} else {
				LOG_ERROR("Unknown actor attribute \"%s\"", item->name);
				ok= 0;
			}
		} else if (item->type == XML_ENTITY_REF_NODE) {
			ok &= parse_actor_nodes(act, item->children, defaults);
		}
	}
	return ok;
}

int parse_actor_script(const xmlNode *cfg)
{
	int ok, act_idx, i;
	int j;
	actor_types *act;
	struct CalCoreSkeleton *skel;

	if(cfg == NULL || cfg->children == NULL) return 0;

	act_idx= get_int_property(cfg, "id");
/*	if(act_idx < 0){
		act_idx= get_property(cfg, "type", "actor type", actor_type_dict);
	}
*/
	if (act_idx < 0 || act_idx >= MAX_ACTOR_DEFS){
		char	str[256];
		char    name[256];

		safe_strncpy(name, get_string_property(cfg, "type"), sizeof(name));
		safe_snprintf(str, sizeof(str), "Data Error in %s(%d): Actor ID out of range %d",
			name, act_idx, act_idx
		);
		LOG_ERROR(str);
		return 0;
	}

	act= &(actors_defs[act_idx]);
	// watch for loading an actor more then once
	if(act->actor_type > 0 || *act->actor_name){
		char	str[256];
		char    name[256];

		safe_strncpy(name, get_string_property(cfg, "type"), sizeof(name));
		safe_snprintf(str, sizeof(str), "Data Error in %s(%d): Already loaded %s(%d)",
			name, act_idx, act->actor_name, act->actor_type
		);
		LOG_ERROR(str);
	}
	ok= 1;
	act->actor_type= act_idx;	// memorize the ID & name to help in debugging
	safe_strncpy(act->actor_name, get_string_property(cfg, "type"), sizeof(act->actor_name));
	actor_check_string(act, "actor", "name", act->actor_name);

	//Initialize Cal3D settings
	act->coremodel= NULL;
	act->actor_scale= 1.0;
	act->scale= 1.0;
	act->mesh_scale= 1.0;
	act->skel_scale= 1.0;
	act->group_count= 0;
	for (i=0; i<16; ++i) {
		safe_strncpy(act->idle_group[i].name, "", sizeof(act->idle_group[i].name));
		act->idle_group[i].count= 0;
	}

	for (i = 0; i < NUM_ACTOR_FRAMES; i++) {
		act->cal_frames[i].anim_index= -1;
		act->cal_frames[i].sound= -1;
	}
	act->battlecry.sound = -1;

	for (i = 0; i < MAX_ACTOR_DEFS; ++i)
	{
		for (j = 0; j < NUM_ATTACHED_ACTOR_FRAMES; j++) {
			attached_actors_defs[act_idx].actor_type[i].cal_frames[j].anim_index = -1;
			attached_actors_defs[act_idx].actor_type[i].cal_frames[j].sound = -1;
	}
	}


	ok= parse_actor_nodes(act, cfg, NULL);

	// TODO: add error checking for missing actor information

	//Actor def parsed, now setup the coremodel
	if (act->coremodel!=NULL)
	{
		skel=CalCoreModel_GetCoreSkeleton(act->coremodel);
		if(skel){
			CalCoreSkeleton_Scale(skel,act->skel_scale);
		}

		// If this not an enhanced actor, load the single mesh and exit
		if(!act->head || strcmp (act->head[0].model_name, "") == 0)
		{
			act->shirt = (shirt_part*)calloc(actor_part_sizes[ACTOR_SHIRT_SIZE], sizeof(shirt_part));
			act->shirt[0].mesh_index= cal_load_mesh(act, act->file_name, NULL); //save the single meshindex as torso
		}
	}

	return ok;
}

int parse_actor_defs(const xmlNode *node)
{
	const xmlNode *def;
	int ok = 1;

	for (def = node->children; def; def = def->next)
	{
		if (def->type == XML_ELEMENT_NODE)
		{
			if (xmlStrcasecmp (def->name, (xmlChar*)"actor") == 0)
			{
				ok &= parse_actor_script (def);
			}
			else
			{
				LOG_ERROR("parse error: actor or include expected");
				ok = 0;
			}
		}
		else if (def->type == XML_ENTITY_REF_NODE)
		{
			ok &= parse_actor_defs (def->children);
		}
	}

	return ok;
}


int read_actor_defs (const char *dir, const char *index)
{
	const xmlNode *root;
	xmlDoc *doc;
	char fname[120];
	int ok = 1;

	safe_snprintf (fname, sizeof(fname), "%s/%s", dir, index);
#if LIBXML_VERSION >= 21200
	xmlParserCtxtPtr ctxt = xmlNewParserCtxt();
	xmlCtxtSetMaxAmplification(ctxt, 1000);
	doc = xmlCtxtReadFile(ctxt, fname, NULL, XML_PARSE_NOENT);
	xmlFreeParserCtxt(ctxt);
#else
	doc = xmlReadFile(fname, NULL, XML_PARSE_NOENT);
#endif
	if (doc == NULL) {
		LOG_ERROR("Unable to read actor definition file %s", fname);
		return 0;
	}

	root = xmlDocGetRootElement (doc);
	if (root == NULL) {
		LOG_ERROR("Unable to parse actor definition file %s", fname);
		ok = 0;
	} else if (xmlStrcasecmp (root->name, (xmlChar*)"actors") != 0) {
		LOG_ERROR("Unknown key \"%s\" (\"actors\" expected).", root->name);
		ok = 0;
	} else {
		ok = parse_actor_defs (root);
	}

	xmlFreeDoc (doc);
	return ok;
}

void init_actor_defs()
{
	// initialize the whole thing to zero
	memset (actors_defs, 0, sizeof (actors_defs));
	memset (attached_actors_defs, 0, sizeof (attached_actors_defs));
	set_invert_v_coord();
	read_actor_defs ("actor_defs", "actor_defs.xml");
}

void free_actor_defs()
{
	int i;
	for (i=0; i<MAX_ACTOR_DEFS; i++)
	{
		if (actors_defs[i].head)
			free(actors_defs[i].head);
		if (actors_defs[i].shield)
			free(actors_defs[i].shield);
		if (actors_defs[i].cape)
			free(actors_defs[i].cape);
		if (actors_defs[i].helmet)
			free(actors_defs[i].helmet);
		if (actors_defs[i].neck)
			free(actors_defs[i].neck);
		if (actors_defs[i].weapon)
			free(actors_defs[i].weapon);
		if (actors_defs[i].shirt)
			free(actors_defs[i].shirt);
		if (actors_defs[i].skin)
			free(actors_defs[i].skin);
		if (actors_defs[i].hair)
			free(actors_defs[i].hair);
		if (actors_defs[i].boots)
			free(actors_defs[i].boots);
		if (actors_defs[i].legs)
			free(actors_defs[i].legs);
		if (actors_defs[i].hardware_model)
			clear_buffers(&actors_defs[i]);
		CalCoreModel_Delete(actors_defs[i].coremodel);
	}
}
