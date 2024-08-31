#ifndef __CAL_TYPES_H__
#define __CAL_TYPES_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef enum{
	cycle=0,
 	action=1
}  cal_animation_type;

struct cal_anim
{
	int anim_index;
	cal_animation_type kind;
	float duration;
	float duration_scale;
	int sound;
	float sound_scale;
};

enum {
	cal_actor_walk_frame = 0,
	cal_actor_run_frame = 1,
	cal_actor_die1_frame = 2,
	cal_actor_die2_frame = 3,
	cal_actor_pain1_frame = 4,
	cal_actor_pain2_frame = 11,
	cal_actor_pick_frame = 5,
	cal_actor_drop_frame = 6,
	cal_actor_idle1_frame = 7,
	cal_actor_idle2_frame = 38,
	cal_actor_idle_sit_frame = 14,
	cal_actor_harvest_frame = 8,
	cal_actor_attack_cast_frame = 9,
	cal_actor_attack_ranged_frame = 10,
	cal_actor_sit_down_frame = 12,
	cal_actor_stand_up_frame = 13,
	cal_actor_in_combat_frame = 16,
	cal_actor_out_combat_frame = 17,
	cal_actor_combat_idle_frame = 15,
	cal_actor_attack_up_1_frame = 18,
	cal_actor_attack_up_2_frame = 19,
	cal_actor_attack_up_3_frame = 20,
	cal_actor_attack_up_4_frame = 21,
	cal_actor_attack_up_5_frame = 22,
	cal_actor_attack_up_6_frame = 23,
	cal_actor_attack_up_7_frame = 24,
	cal_actor_attack_up_8_frame = 25,
	cal_actor_attack_up_9_frame = 26,
	cal_actor_attack_up_10_frame = 27,
	cal_actor_attack_down_1_frame = 28,
	cal_actor_attack_down_2_frame = 29,
	cal_actor_attack_down_3_frame = 30,
	cal_actor_attack_down_4_frame = 31,
	cal_actor_attack_down_5_frame = 32,
	cal_actor_attack_down_6_frame = 33,
	cal_actor_attack_down_7_frame = 34,
	cal_actor_attack_down_8_frame = 35,
	cal_actor_attack_down_9_frame = 36,
	cal_actor_attack_down_10_frame = 37,
    cal_actor_salut_frame = 39,
        cal_actor_danse_frame = 40,
        cal_actor_salto_frame = 41,
        cal_actor_roue_frame = 42,
        NUM_ACTOR_FRAMES = 43
};

enum {
	cal_weapon_attack_up_1_frame = 0,
	cal_weapon_attack_up_2_frame = 1,
	cal_weapon_attack_up_3_frame = 2,
	cal_weapon_attack_up_4_frame = 3,
	cal_weapon_attack_up_5_frame = 4,
	cal_weapon_attack_up_6_frame = 5,
	cal_weapon_attack_up_7_frame = 6,
	cal_weapon_attack_up_8_frame = 7,
	cal_weapon_attack_up_9_frame = 8,
	cal_weapon_attack_up_10_frame = 9,
	cal_weapon_attack_down_1_frame = 10,
	cal_weapon_attack_down_2_frame = 11,
	cal_weapon_attack_down_3_frame = 12,
	cal_weapon_attack_down_4_frame = 13,
	cal_weapon_attack_down_5_frame = 14,
	cal_weapon_attack_down_6_frame = 15,
	cal_weapon_attack_down_7_frame = 16,
	cal_weapon_attack_down_8_frame = 17,
	cal_weapon_attack_down_9_frame = 18,
	cal_weapon_attack_down_10_frame = 19,
	cal_weapon_range_in_frame = 20,
	cal_weapon_range_out_frame = 21,
	cal_weapon_range_idle_frame = 22,
	cal_weapon_range_fire_frame = 23,
	cal_weapon_range_fire_out_frame = 24,
	NUM_WEAPON_FRAMES = 25
};



enum {
	cal_attached_walk_frame = 0, /*!< walk animation to use for the held actor */
	cal_attached_run_frame = 1, /*!< run animation to use for the held actor */
	cal_attached_idle_frame = 2, /*!< idle animation to use for the held actor */
	cal_attached_pain_frame = 3, /*!< pain animation to use for the held actor */
	NUM_ATTACHED_ACTOR_FRAMES = 4
};

#ifdef __cplusplus
} // extern "C"
#endif

#endif // __CAL_TYPES_H__
