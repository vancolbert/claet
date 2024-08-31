#include <stdlib.h>
#include <math.h>
#ifndef WINDOWS
 #include <locale.h>
#endif
#include <errno.h>
#include "particles.h"
#include "asc.h"
#include "draw_scene.h"
#include "errors.h"
#include "gl_init.h"
#include "init.h"
#include "pathfinder.h"
#include "string.h"
#include "sound.h"
#include "textures.h"
#include "tiles.h"
#include "translate.h"
#include "vmath.h"
#include "cluster.h"
#include "eye_candy_wrapper.h"
#include "translate.h"
#include "io/elpathwrapper.h"
#include "io/elfilewrapper.h"
#include "3d_objects.h"
#include "lights.h"
#include "image_loading.h"
/* NOTE: This file contains implementations of the following, currently unused, and commented functions:
 *          Look at the end of the file.
 *
 * void dump_part_sys_info();
 */
int real_add_particle_sys(const char *file_name, float x_pos, float y_pos, float z_pos, unsigned int dynamic);
#define TELEPORTER_PARTICLE_SYS 0
#define TELEPORT_PARTICLE_SYS 1
#define BAG_PARTICLE_SYS 2
#define BURST_PARTICLE_SYS 3
#define FIRE_PARTICLE_SYS 4
#define FOUNTAIN_PARTICLE_SYS 5
#define PARTICLE_RANDOM(min, max) (min + (max - min) * (rand() / (float)RAND_MAX))
#define PARTICLE_RANDOM2(min, max) (min + 0.5 * (max - min) + 0.5 * (max - min) / (float)((rand() % 200) - 100 + 0.5))
#define PART_SYS_VISIBLE_DIST_SQ 20 * 20
#define MAX_PARTICLE_TEXTURES 16 // The maximum number of textures used for particle systems
int use_point_particles = 1;
int particles_percentage = 100;
int enable_blood = 0;
SDL_mutex *particles_list_mutex; // used for locking between the timer and main threads
static int particle_textures[MAX_PARTICLE_TEXTURES];
particle_sys *particles_list[MAX_PARTICLE_SYSTEMS];
/******************************************************
*           PARTICLE SYSTEM DEFINITIONS              *
******************************************************/
#define MAX_PARTICLE_DEFS 500
particle_sys_def *defs_list[MAX_PARTICLE_DEFS];
void destroy_all_particle_defs();
int part_strcmp(char *s1, char *s2) {
	while (*s1 && *s2) {
		if (*s1 != *s2 && (*s1 != '/' && *s1 != '\\' && *s2 != '/' && *s2 != '\\')) {
			return 1;
		}
		s1++;
		s2++;
	}
	return *s1 != *s2;
}
// Grum: perhaps the addition of a sound definition to the files would warrant
// a version number update (from 2 to 3), but it'll still work with v. 2 system
// (without sound, of course), so I'll leave it
#define PARTICLE_DEF_VERSION 2
particle_sys_def *load_particle_def(const char *filename) {
	int version = 0, i;
	char cleanpath[128];
	int fscanf_error = 0;
	FILE *f = NULL;
	particle_sys_def *def = NULL;
	clean_file_name(cleanpath, filename, sizeof(cleanpath));
	// Check if it's already loaded
	for (i = 0; i < MAX_PARTICLE_DEFS; i++) {
		if (defs_list[i] && !part_strcmp(cleanpath, defs_list[i]->file_name)) {
			return defs_list[i];
		}
	}
	// Check if we have a free slot for it
	for (i = 0; i < MAX_PARTICLE_DEFS; i++) {
		if (!defs_list[i]) {
			defs_list[i] = (particle_sys_def *)calloc(1, sizeof(particle_sys_def));
			def = defs_list[i];
			break;
		}
	}
	if (!def) {
		return NULL;
	}
	f = open_file_data(cleanpath, "r");
	if (f == NULL) {
		LOG_ERROR("%s: %s \"%s\": %s\n", reg_error_str, cant_open_file, cleanpath, strerror(errno));
		free(def);
		defs_list[i] = NULL;
		return NULL;
	}
	// initialize defaults
	def->sound_nr = -1;
	if (fscanf(f, "%i\n", &version) != 1) {
		fscanf_error = 1;
	}
	if (version != PARTICLE_DEF_VERSION) {
		LOG_ERROR(particles_filever_wrong, filename, version, PARTICLE_DEF_VERSION);
		fclose(f);
		return NULL;
	}
#ifndef WINDOWS
	setlocale(LC_NUMERIC, "en_US");
#endif
	// System info
	safe_snprintf(def->file_name, sizeof(def->file_name), "%s", filename);
	if (!fscanf_error && fscanf(f, "%i\n", &def->part_sys_type) != 1) {
		fscanf_error = 2;
	}
	if (!fscanf_error && fscanf(f, "%x,%x\n", &def->sblend, &def->dblend) != 2) {
		fscanf_error = 3;
	}
	if (!fscanf_error && fscanf(f, "%i\n", &def->total_particle_no) != 1) {
		fscanf_error = 4;
	}
	def->total_particle_no *= (float)particles_percentage / 100.0;
	if (!fscanf_error && fscanf(f, "%i\n", &def->ttl) != 1) {
		fscanf_error = 5;
	}
	if (!fscanf_error && fscanf(f, "%i\n", &def->part_texture) != 1) {
		fscanf_error = 6;
	}
	if (!fscanf_error && fscanf(f, "%f\n", &def->part_size) != 1) {
		fscanf_error = 7;
	}
	if (!fscanf_error && fscanf(f, "%i\n", &def->random_func) != 1) {
		fscanf_error = 8;
	}
	// Particle creation info
	if (!fscanf_error && fscanf(f, "%f,%f,%f\n", &def->minx, &def->miny, &def->minz) != 3) {
		fscanf_error = 9;
	}
	if (!fscanf_error && fscanf(f, "%f,%f,%f\n", &def->maxx, &def->maxy, &def->maxz) != 3) {
		fscanf_error = 10;
	}
	if (!fscanf_error && fscanf(f, "%f\n", &def->constrain_rad_sq) != 1) {
		fscanf_error = 11;
	}
	if (!fscanf_error && fscanf(f, "%f,%f,%f\n", &def->vel_minx, &def->vel_miny, &def->vel_minz) != 3) {
		fscanf_error = 12;
	}
	if (!fscanf_error && fscanf(f, "%f,%f,%f\n", &def->vel_maxx, &def->vel_maxy, &def->vel_maxz) != 3) {
		fscanf_error = 13;
	}
	if (!fscanf_error && fscanf(f, "%f,%f,%f,%f\n", &def->minr, &def->ming, &def->minb, &def->mina) != 4) {
		fscanf_error = 14;
	}
	if (!fscanf_error && fscanf(f, "%f,%f,%f,%f\n", &def->maxr, &def->maxg, &def->maxb, &def->maxa) != 4) {
		fscanf_error = 15;
	}
	// Particle update info
	if (!fscanf_error && fscanf(f, "%f,%f,%f\n", &def->acc_minx, &def->acc_miny, &def->acc_minz) != 3) {
		fscanf_error = 16;
	}
	if (!fscanf_error && fscanf(f, "%f,%f,%f\n", &def->acc_maxx, &def->acc_maxy, &def->acc_maxz) != 3) {
		fscanf_error = 17;
	}
	if (!fscanf_error && fscanf(f, "%f,%f,%f,%f\n", &def->mindr, &def->mindg, &def->mindb, &def->minda) != 4) {
		fscanf_error = 18;
	}
	if (!fscanf_error && fscanf(f, "%f,%f,%f,%f\n", &def->maxdr, &def->maxdg, &def->maxdb, &def->maxda) != 4) {
		fscanf_error = 19;
	}
	if (!fscanf_error && fscanf(f, "%i\n", &def->use_light) != 1) {
		fscanf_error = 20;
	}
	if (!fscanf_error && fscanf(f, "%f,%f,%f\n", &def->lightx, &def->lighty, &def->lightz) != 3) {
		fscanf_error = 21;
	}
	if (!fscanf_error && fscanf(f, "%f,%f,%f\n", &def->lightr, &def->lightg, &def->lightb) != 3) {
		fscanf_error = 22;
	}
	if (!fscanf_error && fscanf(f, "%d\n", &def->sound_nr) != 3) {
		fscanf_error = 23;
	}
	// Most particale files lack the last few lines and so always fail the test - only report really bad ones.
	if (fscanf_error != 0 && fscanf_error < 20) {
		LOG_ERROR("%s(): fscanf error file=[%s] line %d\n", __FUNCTION__, filename, fscanf_error);
	}
	if (def->total_particle_no > MAX_PARTICLES) {
		LOG_ERROR(particle_system_overrun, filename, def->total_particle_no, MAX_PARTICLES);
		def->total_particle_no = MAX_PARTICLES;
	}
	if (def->constrain_rad_sq > 0.0) {
		float rad = sqrt(def->constrain_rad_sq);
		int fixed = def->minx > rad || def->maxx < -rad || def->miny > rad || def->maxy < -rad;
		if (def->minx > rad) {
			def->minx = rad - 0.1;
		}
		if (def->maxx < -rad) {
			def->maxx = -rad + 0.1;
		}
		if (def->miny > rad) {
			def->miny = rad - 0.1;
		}
		if (def->maxy < -rad) {
			def->maxy = -rad + 0.1;
		}
		if (def->minx * def->maxx > 0.0 || def->miny * def->maxy > 0.0) {
			float dist = def->minx * def->minx + def->miny * def->miny;
			if (dist > def->constrain_rad_sq) {
				def->minx *= sqrt(def->constrain_rad_sq / dist) - 0.1;
				def->miny *= sqrt(def->constrain_rad_sq / dist) - 0.1;
				fixed = 1;
			}
			dist = def->minx * def->minx + def->maxy * def->maxy;
			if (dist > def->constrain_rad_sq) {
				def->minx *= sqrt(def->constrain_rad_sq / dist) - 0.1;
				def->maxy *= sqrt(def->constrain_rad_sq / dist) - 0.1;
				fixed = 1;
			}
			dist = def->maxx * def->maxx + def->maxy * def->maxy;
			if (dist > def->constrain_rad_sq) {
				def->maxx *= sqrt(def->constrain_rad_sq / dist) - 0.1;
				def->maxy *= sqrt(def->constrain_rad_sq / dist) - 0.1;
				fixed = 1;
			}
			dist = def->maxx * def->maxx + def->miny * def->miny;
			if (dist > def->constrain_rad_sq) {
				def->maxx *= sqrt(def->constrain_rad_sq / dist) - 0.1;
				def->miny *= sqrt(def->constrain_rad_sq / dist) - 0.1;
				fixed = 1;
			}
		}
		if (fixed) {
			LOG_ERROR(particle_strange_pos, filename);
		}
	}
	fclose(f);
	return def;
}
static __inline__ void calc_particle_random_min_max(float f1, float f2, float *v_min, float *v_max) {
	if (f1 < f2) {
		*v_min = f1;
		*v_max = f2;
	} else {
		*v_min = f2;
		*v_max = f1;
	}
}
static __inline__ void calc_particle_random2_min_max(float f1, float f2, float *v_min, float *v_max) {
	*v_min = (f1 + f2) * 0.5f - abs(f2 - f1);
	*v_max = (f1 + f2) * 0.5f + abs(f2 - f1);
}
void calc_bounding_box_for_particle_sys(AABBOX *bbox, particle_sys *system_id) {
	unsigned int count;
	float p_max, p_min, p_step, sq;
	VECTOR3 pv_min, pv_max, pv_v_min, pv_v_max, pv_acc_min, pv_acc_max;
	sq = sqrt(system_id->def->constrain_rad_sq);
	if (system_id->def->random_func == 0) {
		calc_particle_random_min_max(system_id->def->vel_minx, system_id->def->vel_maxx, &p_min, &p_max);
		pv_v_min[X] = p_min;
		pv_v_max[X] = p_max;
		calc_particle_random_min_max(system_id->def->vel_miny, system_id->def->vel_maxy, &p_min, &p_max);
		pv_v_min[Y] = p_min;
		pv_v_max[Y] = p_max;
		calc_particle_random_min_max(system_id->def->vel_minz, system_id->def->vel_maxz, &p_min, &p_max);
		pv_v_min[Z] = p_min;
		pv_v_max[Z] = p_max;
		if (system_id->def->constrain_rad_sq > 0.0f) {
			pv_min[X] = -sq;
			pv_min[Y] = -sq;
			pv_max[X] = sq;
			pv_max[Y] = sq;
		} else {
			calc_particle_random_min_max(system_id->def->minx, system_id->def->maxx, &p_min, &p_max);
			pv_min[X] = p_min;
			pv_max[X] = p_max;
			calc_particle_random_min_max(system_id->def->miny, system_id->def->maxy, &p_min, &p_max);
			pv_min[Y] = p_min;
			pv_max[Y] = p_max;
		}
		calc_particle_random_min_max(system_id->def->minz, system_id->def->maxz, &p_min, &p_max);
		pv_min[Z] = p_min;
		pv_max[Z] = p_max;
	} else {
		calc_particle_random2_min_max(system_id->def->vel_minx, system_id->def->vel_maxx, &p_min, &p_max);
		pv_v_min[X] = p_min;
		pv_v_max[X] = p_max;
		calc_particle_random2_min_max(system_id->def->vel_miny, system_id->def->vel_maxy, &p_min, &p_max);
		pv_v_min[Y] = p_min;
		pv_v_max[Y] = p_max;
		calc_particle_random2_min_max(system_id->def->vel_minz, system_id->def->vel_maxz, &p_min, &p_max);
		pv_v_min[Z] = p_min;
		pv_v_max[Z] = p_max;
		if (system_id->def->constrain_rad_sq > 0.0f) {
			pv_min[X] = -sq;
			pv_min[Y] = -sq;
			pv_max[X] = sq;
			pv_max[Y] = sq;
		} else {
			calc_particle_random2_min_max(system_id->def->minx, system_id->def->maxx, &p_min, &p_max);
			pv_min[X] = p_min;
			pv_max[X] = p_max;
			calc_particle_random2_min_max(system_id->def->miny, system_id->def->maxy, &p_min, &p_max);
			pv_min[Y] = p_min;
			pv_max[Y] = p_max;
		}
		calc_particle_random2_min_max(system_id->def->minz, system_id->def->maxz, &p_min, &p_max);
		pv_min[Z] = p_min;
		pv_max[Z] = p_max;
	}
	switch (system_id->def->part_sys_type) {
	case (TELEPORTER_PARTICLE_SYS):
	case (TELEPORT_PARTICLE_SYS):
		calc_particle_random2_min_max(system_id->def->acc_minx, system_id->def->acc_maxx, &p_min, &p_max);
		pv_acc_min[X] = p_min;
		pv_acc_max[X] = p_max;
		calc_particle_random2_min_max(system_id->def->acc_miny, system_id->def->acc_maxy, &p_min, &p_max);
		pv_acc_min[Y] = p_min;
		pv_acc_max[Y] = p_max;
		calc_particle_random2_min_max(system_id->def->acc_minz, system_id->def->acc_maxz, &p_min, &p_max);
		pv_acc_min[Z] = p_min;
		pv_acc_max[Z] = p_max;
		p_step = pv_acc_min[Z] + pv_v_min[Z];
		count = ceil(2.0f / p_step);
		bbox->bbmin[X] = min2f(pv_min[X], pv_min[X] + count * (pv_v_min[X] + pv_acc_min[X]));
		bbox->bbmin[Y] = min2f(pv_min[Y], pv_min[Y] + count * (pv_v_min[Y] + pv_acc_max[X]));
		bbox->bbmin[Z] = min2f(pv_min[Z], pv_min[Z] + count * (pv_v_min[Z] + pv_acc_min[Y]));
		bbox->bbmax[X] = max2f(pv_max[X], pv_max[X] + count * (pv_v_max[X] + pv_acc_max[Y]));
		bbox->bbmax[Y] = max2f(pv_max[Y], pv_max[Y] + count * (pv_v_max[Y] + pv_acc_min[Z]));
		bbox->bbmax[Z] = max2f(pv_max[Z], pv_max[Z] + count * (pv_v_max[Z] + pv_acc_min[Z]));
		break;
	case (BAG_PARTICLE_SYS):
		calc_particle_random2_min_max(system_id->def->acc_minx, system_id->def->acc_maxx, &p_min, &p_max);
		pv_acc_min[X] = p_min;
		pv_acc_max[X] = p_max;
		calc_particle_random2_min_max(system_id->def->acc_miny, system_id->def->acc_maxy, &p_min, &p_max);
		pv_acc_min[Y] = p_min;
		pv_acc_max[Y] = p_max;
		calc_particle_random2_min_max(system_id->def->acc_minz, system_id->def->acc_maxz, &p_min, &p_max);
		pv_acc_min[Z] = p_min;
		pv_acc_max[Z] = p_max;
		p_step = pv_acc_min[Z] + pv_v_min[Z];
		count = ceil(1.0f / p_step);
		bbox->bbmin[X] = min2f(pv_min[X], pv_min[X] + count * (pv_v_min[X] + pv_acc_min[X]));
		bbox->bbmin[Y] = min2f(pv_min[Y], pv_min[Y] + count * (pv_v_min[Y] + pv_acc_max[X]));
		bbox->bbmin[Z] = min2f(pv_min[Z], pv_min[Z] + count * (pv_v_min[Z] + pv_acc_min[Y]));
		bbox->bbmax[X] = max2f(pv_max[X], pv_max[X] + count * (pv_v_max[X] + pv_acc_max[Y]));
		bbox->bbmax[Y] = max2f(pv_max[Y], pv_max[Y] + count * (pv_v_max[Y] + pv_acc_min[Z]));
		bbox->bbmax[Z] = max2f(pv_max[Z], pv_max[Z] + count * (pv_v_max[Z] + pv_acc_min[Z]));
		break;
	case (BURST_PARTICLE_SYS):
		sq *= 3;
		bbox->bbmin[X] = -sq;
		bbox->bbmin[Y] = -sq;
		bbox->bbmin[Z] = -sq;
		bbox->bbmax[X] = sq;
		bbox->bbmax[Y] = sq;
		bbox->bbmax[Z] = sq;
		break;
	case (FIRE_PARTICLE_SYS):
	case (FOUNTAIN_PARTICLE_SYS):
		calc_particle_random_min_max(system_id->def->acc_minx, system_id->def->acc_maxx, &p_min, &p_max);
		pv_acc_min[X] = p_min;
		pv_acc_max[X] = p_max;
		calc_particle_random_min_max(system_id->def->acc_miny, system_id->def->acc_maxy, &p_min, &p_max);
		pv_acc_min[Y] = p_min;
		pv_acc_max[Y] = p_max;
		calc_particle_random_min_max(system_id->def->acc_minz, system_id->def->acc_maxz, &p_min, &p_max);
		pv_acc_min[Z] = p_min;
		pv_acc_max[Z] = p_max;
		if (system_id->def->random_func == 0) {
			calc_particle_random_min_max(system_id->def->mina, system_id->def->maxa, &p_min, &p_max);
		} else {
			calc_particle_random2_min_max(system_id->def->mina, system_id->def->maxa, &p_min, &p_max);
		}
		p_step = p_min;
		calc_particle_random_min_max(system_id->def->minda, system_id->def->maxda, &p_min, &p_max);
		count = ceil(p_max / (-p_step));
		bbox->bbmin[X] = min2f(pv_min[X], pv_min[X] + count * pv_v_min[X] + count * (count + 1) * 0.5f * pv_acc_min[X]);
		bbox->bbmin[Y] = min2f(pv_min[Y], pv_min[Y] + count * pv_v_min[Y] + count * (count + 1) * 0.5f * pv_acc_max[X]);
		bbox->bbmin[Z] = min2f(pv_min[Z], pv_min[Z] + count * pv_v_min[Z] + count * (count + 1) * 0.5f * pv_acc_min[Y]);
		bbox->bbmax[X] = max2f(pv_max[X], pv_max[X] + count * pv_v_max[X] + count * (count + 1) * 0.5f * pv_acc_max[Y]);
		bbox->bbmax[Y] = max2f(pv_max[Y], pv_max[Y] + count * pv_v_max[Y] + count * (count + 1) * 0.5f * pv_acc_min[Z]);
		bbox->bbmax[Z] = max2f(pv_max[Z], pv_max[Z] + count * pv_v_max[Z] + count * (count + 1) * 0.5f * pv_acc_min[Z]);
		break;
	}
	bbox->bbmin[X] += system_id->x_pos;
	bbox->bbmin[Y] += system_id->y_pos;
	bbox->bbmin[Z] += system_id->z_pos;
	bbox->bbmax[X] += system_id->x_pos;
	bbox->bbmax[Y] += system_id->y_pos;
	bbox->bbmax[Z] += system_id->z_pos;
}
static __inline__ void destroy_partice_sys_without_lock(int i) {
	if ((i < 0) || (i >= MAX_PARTICLE_SYSTEMS)) {
		return;
	}
	if (particles_list[i] == NULL) {
		return;
	}
	if (particles_list[i]->def && particles_list[i]->def->use_light && lights_list[particles_list[i]->light]) {
		destroy_light(particles_list[i]->light);
	}
	stop_sound_at_location(particles_list[i]->x_pos, particles_list[i]->y_pos);
	delete_particle_from_abt(main_bbox_tree, i);
	free(particles_list[i]);
	particles_list[i] = NULL;
}
// void destroy_particle_sys(int i)
// {
// }
/*******************************************************************
*            INITIALIZATION AND CLEANUP FUNCTIONS                 *
*******************************************************************/
// Threading support for particles_list
void init_particles() {
	int i;
	for (i = 0; i < MAX_PARTICLE_TEXTURES; i++) {
		char buffer[256], filename[256];
		safe_snprintf(filename, sizeof(filename), "./textures/particle%d", i);
		if (check_image_name(filename, sizeof(buffer), buffer) == 1) {
			particle_textures[i] = load_texture_cached(buffer, tt_mesh);
		} else {
			particle_textures[i] = -1;
		}
	}
	particles_list_mutex = SDL_CreateMutex();
	LOCK_PARTICLES_LIST(); // lock it to avoid timing issues
	for (i = 0; i < MAX_PARTICLE_SYSTEMS; i++) {
		particles_list[i] = NULL;
	}
	for (i = 0; i < MAX_PARTICLE_DEFS; i++) {
		defs_list[i] = NULL;
	}
	UNLOCK_PARTICLES_LIST(); // release now that we are done
}
void end_particles() {
	LOCK_PARTICLES_LIST();
	destroy_all_particles();
	destroy_all_particle_defs();
	UNLOCK_PARTICLES_LIST();
	SDL_DestroyMutex(particles_list_mutex);
	particles_list_mutex = NULL;
}
void destroy_all_particle_defs() {
	int i;
	for (i = 0; i < MAX_PARTICLE_DEFS; i++) {
		if (defs_list[i] != NULL) {
			free(defs_list[i]);
			defs_list[i] = NULL;
		}
	}
}
void destroy_all_particles() {
	int i;
	LOCK_PARTICLES_LIST();
	for (i = 0; i < MAX_PARTICLE_SYSTEMS; i++) {
		destroy_partice_sys_without_lock(i);
	}
	UNLOCK_PARTICLES_LIST();
}
void add_fire_at_tile(int kind, Uint16 x_tile, Uint16 y_tile) {
	float x = 0.5f * x_tile + 0.25f;
	float y = 0.5f * y_tile + 0.25f;
	float z = 0.0;
	int snd;
	switch (kind) {
	case 2:
		ec_create_campfire(x, y, z, 0.0, 1.0, (poor_man ? 6 : 10), 3.1);
		snd = get_sound_index_for_particle_file_name("./particles/fire_big.part");
		break;
	case 1:
	default:
		ec_create_campfire(x, y, z, 0.0, 1.0, (poor_man ? 6 : 10), 2.4);
		snd = get_sound_index_for_particle_file_name("./particles/fire_small.part");
		break;
	}
	if (sound_on && snd >= 0) {
		add_particle_sound(snd, x_tile, y_tile);
	}
}
void remove_fire_at_tile(Uint16 x_tile, Uint16 y_tile) {
	float x = 0.5f * x_tile + 0.25f;
	float y = 0.5f * y_tile + 0.25f;
	ec_delete_effect_loc_type(x, y, EC_CAMPFIRE);
	stop_sound_at_location(x_tile, y_tile);
	return;
}
/*********************************************************************
*          CREATION OF NEW PARTICLES AND SYSTEMS                    *
*********************************************************************/
void rotate_vector3f(float *vector, float x, float y, float z) {
	// rotation matrixes
	float rot_x[9];
	float rot_y[9];
	float rot_z[9];
	float result_x[3];
	float result_y[3];
	MAT3_ROT_X(rot_x, x * (M_PI / 180.0f));
	MAT3_ROT_Y(rot_y, y * (M_PI / 180.0f));
	MAT3_ROT_Z(rot_z, z * (M_PI / 180.0f));
	// rotate around x achsis
	MAT3_VECT3_MULT(result_x, rot_x, vector);
	// rotate around y achsis
	MAT3_VECT3_MULT(result_y, rot_y, result_x);
	// rotate around z achsis
	MAT3_VECT3_MULT(vector, rot_z, result_y);
}
void add_ec_effect_to_e3d(object3d *e3d) {
	ec_bounds *bounds = ec_create_bounds_list();
	float shift[3] = {0.0f, 0.0f, 0.0f};
	// useful for debugging:
	if (strstr(e3d->file_name, "/lantern1.e3d")) {
		ec_add_smooth_polygon_bound(bounds, 2.0, 0.25);
		shift[2] += 0.25f; // add height
		rotate_vector3f(shift, e3d->x_rot, e3d->y_rot, e3d->z_rot);
		ec_create_fireflies(e3d->x_pos + shift[0], e3d->y_pos + shift[1], e3d->z_pos + shift[2], 1.0, 1.0, 0.00625, 1.0, bounds);
	} else if (strstr(e3d->file_name, "/lantern2.e3d")) {
		ec_add_smooth_polygon_bound(bounds, 2.0, 0.25);
		shift[2] += 0.25f; // add height
		rotate_vector3f(shift, e3d->x_rot, e3d->y_rot, e3d->z_rot);
		ec_create_fireflies(e3d->x_pos + shift[0], e3d->y_pos + shift[1], e3d->z_pos + shift[2], 1.0, 1.0, 0.005, 1.0, bounds);
	} else if (strstr(e3d->file_name, "/lantern3.e3d")) {
		ec_add_smooth_polygon_bound(bounds, 2.0, 0.25);
		shift[2] += 0.25f; // add height
		rotate_vector3f(shift, e3d->x_rot, e3d->y_rot, e3d->z_rot);
		ec_create_fireflies(e3d->x_pos + shift[0], e3d->y_pos + shift[1], e3d->z_pos + shift[2], 1.0, 1.0, 0.005, 1.0, bounds);
	} else if (strstr(e3d->file_name, "/light1.e3d")) {
		ec_add_smooth_polygon_bound(bounds, 2.0, 0.33);
		shift[2] += 2.85f; // add height
		rotate_vector3f(shift, e3d->x_rot, e3d->y_rot, e3d->z_rot);
		ec_create_fireflies(e3d->x_pos + shift[0], e3d->y_pos + shift[1], e3d->z_pos + shift[2], 1.0, 1.0, 0.01, 1.0, bounds);
	} else if (strstr(e3d->file_name, "/light2.e3d")) {
		ec_add_smooth_polygon_bound(bounds, 2.0, 0.4);
		shift[2] += 2.95f; // add height
		rotate_vector3f(shift, e3d->x_rot, e3d->y_rot, e3d->z_rot);
		ec_create_fireflies(e3d->x_pos + shift[0], e3d->y_pos + shift[1], e3d->z_pos + shift[2], 1.0, 1.0, 0.0125, 1.0, bounds);
	} else if (strstr(e3d->file_name, "/light3.e3d")) {
		ec_add_smooth_polygon_bound(bounds, 2.0, 0.33);
		shift[2] += 3.5f; // add height
		shift[0] -= 0.33f; // the light is not centered
		rotate_vector3f(shift, e3d->x_rot, e3d->y_rot, e3d->z_rot);
		ec_create_fireflies(e3d->x_pos + shift[0], e3d->y_pos + shift[1], e3d->z_pos + shift[2], 1.0, 1.0, 0.015, 1.0, bounds);
	} else if (strstr(e3d->file_name, "/light4.e3d")) {
		ec_add_smooth_polygon_bound(bounds, 2.0, 0.4);
		shift[2] += 1.75f; // add height
		ec_create_fireflies(e3d->x_pos, e3d->y_pos, e3d->z_pos + 1.75f, 1.0, 1.0, 0.0075, 1.0, bounds);
	}
	ec_free_bounds_list(bounds);
}
// Wrapper for map sounds (checks for existing sounds in similar location for multi-particle-system effects)
int add_map_particle_sys(const char *file_name, float x_pos, float y_pos, float z_pos, unsigned int dynamic) {
	int snd;
	if (!no_sound) {
		snd = get_sound_index_for_particle_file_name(file_name);
		if (snd >= 0) {
			add_map_sound(snd, (x_pos - 0.25f) * 2, (y_pos - 0.25f) * 2);
		}
	}
	return real_add_particle_sys(file_name, x_pos, y_pos, z_pos, dynamic);
}
// Wrapper for regular (event triggered) particle systems (no location check)
int add_particle_sys(const char *file_name, float x_pos, float y_pos, float z_pos, unsigned int dynamic) {
	int snd;
	if (!no_sound) {
		snd = get_sound_index_for_particle_file_name(file_name);
		if (snd >= 0) {
			add_particle_sound(snd, (x_pos - 0.25f) * 2, (y_pos - 0.25f) * 2);
		}
	}
	return real_add_particle_sys(file_name, x_pos, y_pos, z_pos, dynamic);
}
int real_add_particle_sys(const char *file_name, float x_pos, float y_pos, float z_pos, unsigned int dynamic) {
	if (use_eye_candy) {
		if (!strncmp("fou", file_name + 12, 3)) {
			ec_create_fountain(x_pos, y_pos, z_pos + 0.15, 0.0, 1.0, (z_pos >= 0.8 ? z_pos - 0.8 : 0.0), 0, 1.0, (poor_man ? 6 : 10));
		} else if ((use_fancy_smoke) && (!strncmp("smo", file_name + 12, 3))) {
			if (file_name[17] == '1') {
				ec_create_smoke(x_pos, y_pos, z_pos, 0.0, 1.0, 0.3, (poor_man ? 6 : 10));
			} else if (file_name[17] == '2') {
				ec_create_smoke(x_pos, y_pos, z_pos, 0.0, 1.0, 0.45, (poor_man ? 6 : 10));
			} else if (file_name[17] == '3') {
				ec_create_smoke(x_pos, y_pos, z_pos, 0.0, 1.0, 1.6, (poor_man ? 6 : 10));
			} else if (file_name[17] == '_') {
				ec_create_smoke(x_pos, y_pos, z_pos, 0.0, 1.0, 1.1, (poor_man ? 6 : 10));
			} else {
				ec_create_smoke(x_pos, y_pos, z_pos, 0.0, 1.0, 0.5, (poor_man ? 6 : 10));
			}
		} else if (!strncmp("tel", file_name + 12, 3)) {
			if (file_name[21] == 'i') {
				ec_create_selfmagic_teleport_to_the_portals_room(x_pos, y_pos, z_pos, (poor_man ? 6 : 10));
			} else if (file_name[21] == 'o') {
				ec_create_selfmagic_teleport_to_the_portals_room(x_pos, y_pos, z_pos, (poor_man ? 6 : 10));
			} else {
				ec_create_teleporter(x_pos, y_pos, z_pos, 0.0, 1.0, 1.0, (poor_man ? 6 : 10));
			}
		} else if (!strncmp("fir", file_name + 12, 3)) {
			if (!strncmp("big", file_name + 17, 3)) {
				ec_create_campfire(x_pos, y_pos, z_pos, 0.0, 1.0, (poor_man ? 6 : 10), 1.5);
			} else if (!strncmp("for", file_name + 17, 3)) {
				ec_create_campfire(x_pos, y_pos - 0.2, z_pos, 0.0, 1.0, (poor_man ? 6 : 10), 2.0);
			} else if (!strncmp("min", file_name + 17, 3)) {
				ec_create_campfire(x_pos, y_pos, z_pos, 0.0, 1.0, (poor_man ? 6 : 10), 0.4);
			} else if (!strncmp("sma", file_name + 17, 3)) {
				ec_create_campfire(x_pos, y_pos, z_pos, 0.0, 1.0, (poor_man ? 6 : 10), 0.6);
			} else if (!strncmp("tor", file_name + 17, 3)) {
				ec_create_lamp(x_pos, y_pos, z_pos, 0.0, 1.0, 1.6, (poor_man ? 6 : 10));
			} else {
				particle_sys_def *def = load_particle_def(file_name);
				if (!def) {
					return -1;
				}
				return create_particle_sys(def, x_pos, y_pos, z_pos, dynamic);
			}
		} else if (!strncmp("can", file_name + 12, 3)) {
			ec_create_candle(x_pos, y_pos, z_pos, 0.0, 1.0, 0.7, (poor_man ? 6 : 10));
		} else {
			particle_sys_def *def = load_particle_def(file_name);
			if (!def) {
				return -1;
			}
			return create_particle_sys(def, x_pos, y_pos, z_pos, dynamic);
		}
	} else {
		particle_sys_def *def = load_particle_def(file_name);
		if (!def) {
			return -1;
		}
		return create_particle_sys(def, x_pos, y_pos, z_pos, dynamic);
	}
	// If we got here, the eye candy system handled this particle
	// system. Return an invalid particle ID to signal that nothing
	// was added to particles_list, but not -1 since this is not an
	// error.
	return -2;
}
int add_particle_sys_at_tile(const char *file_name, int x_tile, int y_tile, unsigned int dynamic) {
	float z;
	z = get_tile_height(x_tile, y_tile);
	return add_particle_sys(file_name, (float)x_tile / 2.0 + 0.25f, (float)y_tile / 2.0 + 0.25f, z, dynamic);
}
void create_particle(particle_sys *sys, particle *result) {
	particle_sys_def *def = sys->def;
	if (def->random_func == 0) {
		do {
			result->x = PARTICLE_RANDOM(def->minx, def->maxx);
			result->y = PARTICLE_RANDOM(def->miny, def->maxy);
			result->z = PARTICLE_RANDOM(def->minz, def->maxz);
		} while (def->constrain_rad_sq > 0 && (result->x * result->x + result->y * result->y) > def->constrain_rad_sq);
		result->vx = PARTICLE_RANDOM(def->vel_minx, def->vel_maxx);
		result->vy = PARTICLE_RANDOM(def->vel_miny, def->vel_maxy);
		result->vz = PARTICLE_RANDOM(def->vel_minz, def->vel_maxz);
		result->r = PARTICLE_RANDOM(def->minr, def->maxr);
		result->g = PARTICLE_RANDOM(def->ming, def->maxg);
		result->b = PARTICLE_RANDOM(def->minb, def->maxb);
		result->a = PARTICLE_RANDOM(def->mina, def->maxa);
	} else {
		do {
			result->x = PARTICLE_RANDOM2(def->minx, def->maxx);
			result->y = PARTICLE_RANDOM2(def->miny, def->maxy);
			result->z = PARTICLE_RANDOM2(def->minz, def->maxz);
		} while (def->constrain_rad_sq > 0 && (result->x * result->x + result->y * result->y) > def->constrain_rad_sq);
		result->vx = PARTICLE_RANDOM2(def->vel_minx, def->vel_maxx);
		result->vy = PARTICLE_RANDOM2(def->vel_miny, def->vel_maxy);
		result->vz = PARTICLE_RANDOM2(def->vel_minz, def->vel_maxz);
		result->r = PARTICLE_RANDOM2(def->minr, def->maxr);
		result->g = PARTICLE_RANDOM2(def->ming, def->maxg);
		result->b = PARTICLE_RANDOM2(def->minb, def->maxb);
		result->a = PARTICLE_RANDOM2(def->mina, def->maxa);
	}
	result->x += sys->x_pos;
	result->y += sys->y_pos;
	result->z += sys->z_pos;
	result->free = 0;
}
int create_particle_sys(particle_sys_def *def, float x, float y, float z, unsigned int dynamic) {
	int i, psys;
	particle_sys *system_id;
	particle *p;
	AABBOX bbox;
	memset(&bbox, '\0', sizeof(bbox));
	if (!def) {
		return -1;
	}
	// allocate memory for this particle system
	system_id = (particle_sys *)calloc(1, sizeof(particle_sys));
	LOCK_PARTICLES_LIST();
	// now, find a place for this system
	for (psys = 0; psys < MAX_PARTICLE_SYSTEMS; psys++) {
		if (!particles_list[psys]) {
			particles_list[psys] = system_id;
			break;
		}
	}
	if (psys == MAX_PARTICLE_SYSTEMS) {
		free(system_id);
		UNLOCK_PARTICLES_LIST();
		return -1;
	}
	system_id->x_pos = x;
	system_id->y_pos = y;
	system_id->z_pos = z;
	system_id->def = def;
	system_id->particle_count = def->total_particle_no;
	system_id->ttl = def->ttl;
	if (def->use_light) {
		system_id->light = add_light(def->lightx + x, def->lighty + y, def->lightz + z, def->lightr, def->lightg, def->lightb, 1.0f, dynamic);
	}
	for (i = 0, p = &system_id->particles[0]; i < def->total_particle_no; i++, p++) {
		create_particle(system_id, p);
	}
	system_id->cluster = get_cluster((int)(x / 0.5f), (int)(y / 0.5f));
	current_cluster = system_id->cluster;
	calc_bounding_box_for_particle_sys(&bbox, system_id);
	if ((main_bbox_tree_items != NULL) && (dynamic == 0)) {
		add_particle_sys_to_list(main_bbox_tree_items, psys, bbox, def->sblend, def->dblend);
	} else {
		add_particle_to_abt(main_bbox_tree, psys, bbox, def->sblend, def->dblend, dynamic);
	}
	UNLOCK_PARTICLES_LIST();
	return psys;
}
/**********************************************************************
*                      RENDERING FUNCTIONS                           *
**********************************************************************/
void draw_text_particle_sys(particle_sys *system_id) {
	int i;
	float z_len = 0.065f * system_id->def->part_size;
	float x_len = z_len * cos(-rz * M_PI / 180.0);
	float y_len = z_len * sin(-rz * M_PI / 180.0);
	particle *p;
	LOCK_PARTICLES_LIST(); // lock it to avoid timing issues
	CHECK_GL_ERRORS();
	bind_texture(particle_textures[system_id->def->part_texture]);
	for (i = 0, p = &system_id->particles[0]; i < system_id->def->total_particle_no; i = i + 5, p = p + 5) {
		if (!p->free) {
			glPushMatrix();
			glTranslatef(p->x, p->y, p->z);
			glBegin(GL_TRIANGLE_STRIP);
			glColor4f(p->r, p->g, p->b, p->a);
			glTexCoord2f(0.0f, 1.0f);
			glVertex3f(-x_len, -y_len, +z_len);
			glTexCoord2f(0.0f, 0.0f);
			glVertex3f(-x_len, -y_len, -z_len);
			glTexCoord2f(1.0f, 1.0f);
			glVertex3f(x_len, y_len, +z_len);
			glTexCoord2f(1.0f, 0.0f);
			glVertex3f(x_len, y_len, -z_len);
			glEnd();
			glPopMatrix();
		}
	}
	UNLOCK_PARTICLES_LIST(); // release now that we are done
	CHECK_GL_ERRORS();
}
void draw_point_particle_sys(particle_sys *system_id) {
	int i;
	particle *p;
	CHECK_GL_ERRORS();
	glEnable(GL_POINT_SPRITE_NV);
	glTexEnvf(GL_POINT_SPRITE_NV, GL_COORD_REPLACE_NV, GL_TRUE);
	glPointSize(system_id->def->part_size * (5.5f - zoom_level) * 4.4f);
	bind_texture(particle_textures[system_id->def->part_texture]);
	glBegin(GL_POINTS);
	LOCK_PARTICLES_LIST(); // lock it to avoid timing issues
	for (i = 0, p = &system_id->particles[0]; i < system_id->def->total_particle_no; i++, p++) {
		if (!p->free) {
			glColor4f(p->r, p->g, p->b, p->a);
			glVertex3f(p->x, p->y, p->z);
		}
	}
	UNLOCK_PARTICLES_LIST(); // release now that we are done
	glEnd();
	glDisable(GL_POINT_SPRITE_NV);
	CHECK_GL_ERRORS();
}
void display_particles() {
	GLenum sblend = GL_SRC_ALPHA, dblend = GL_ONE;
	unsigned int i, l, start, stop;
	if (!particles_percentage) {
		return;
	}
	CHECK_GL_ERRORS();
	glPushAttrib(GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT);
	glDepthMask(GL_FALSE);
	glEnable(GL_BLEND);
	glEnable(GL_CULL_FACE);
	glDisable(GL_LIGHTING);
	glBlendFunc(sblend, dblend);
	LOCK_PARTICLES_LIST();
	// Perhaps we should have a depth sort here..?
	get_intersect_start_stop(main_bbox_tree, TYPE_PARTICLE_SYSTEM, &start, &stop);
	for (i = start; i < stop; i++) {
		l = get_intersect_item_ID(main_bbox_tree, i);
		if (!particles_list[l]) {
			continue;
		}
		if ((particles_list[l]->def->sblend != sblend) || (particles_list[l]->def->dblend != dblend)) {
			sblend = particles_list[l]->def->sblend;
			dblend = particles_list[l]->def->dblend;
			glBlendFunc(sblend, dblend);
		}
		if (use_point_particles) {
			draw_point_particle_sys(particles_list[l]);
		} else {
			draw_text_particle_sys(particles_list[l]);
		}
	}
	UNLOCK_PARTICLES_LIST();
	glDisable(GL_CULL_FACE); // Intel fix
	glPopAttrib();
	CHECK_GL_ERRORS();
}
/******************************************************************************
*                           UPDATE FUNCTIONS                                 *
******************************************************************************/
void update_fountain_sys(particle_sys *system_id) {
	int i, j;
	int total_particle_no;
	int particles_to_add = 0;
	particle *p;
	total_particle_no = system_id->def->total_particle_no;
	// see if we need to add new particles
	LOCK_PARTICLES_LIST();
	if (system_id->ttl) {
		particles_to_add = total_particle_no - system_id->particle_count;
	}
	if (particles_to_add) {
		for (j = i = 0; i < particles_to_add; i++) {
			// find a free space
			for (; j < total_particle_no; j++) {
				if (system_id->particles[j].free) {
					create_particle(system_id, &(system_id->particles[j]));
					system_id->particle_count++;
					break;
				}
			}
		}
	}
	// excellent, now we have to actually update the particles
	// find used particles
	for (j = 0, p = &system_id->particles[0]; j < total_particle_no; j++, p++) {
		if (!p->free) {
			if (p->a < 0.0f) {
				if (system_id->particle_count) {
					system_id->particle_count--;
				}
				continue;
			}
			if (p->z < 0.0f) {
				p->z = 0.001f;
				p->vz = -p->vz;
			}
			p->x += p->vx;
			p->y += p->vy;
			p->z += p->vz;
			p->vx += PARTICLE_RANDOM(system_id->def->acc_minx, system_id->def->acc_maxx);
			p->vy += PARTICLE_RANDOM(system_id->def->acc_miny, system_id->def->acc_maxy);
			p->vz += PARTICLE_RANDOM(system_id->def->acc_minz, system_id->def->acc_maxz);
			p->r += PARTICLE_RANDOM(system_id->def->mindr, system_id->def->maxdr);
			p->g += PARTICLE_RANDOM(system_id->def->mindg, system_id->def->maxdg);
			p->b += PARTICLE_RANDOM(system_id->def->mindb, system_id->def->maxdb);
			p->a += PARTICLE_RANDOM(system_id->def->minda, system_id->def->maxda);
		}
	}
	UNLOCK_PARTICLES_LIST();
}
void update_burst_sys(particle_sys *system_id) {
	int j;
	int total_particle_no;
	particle *p;
	total_particle_no = system_id->def->total_particle_no;
	LOCK_PARTICLES_LIST();
	// find used particles
	for (j = 0, p = &system_id->particles[0]; j < total_particle_no; j++, p++) {
		if (!p->free) {
			float distx = p->x - system_id->x_pos;
			float disty = p->y - system_id->y_pos;
			float distz = p->z - system_id->z_pos;
			float dist_sq = distx * distx + disty * disty + distz * distz;
			if (dist_sq > system_id->def->constrain_rad_sq * 9.0 || dist_sq < 0.01) {
				if (system_id->particle_count) {
					system_id->particle_count--;
				}
				continue;
			}
			if (p->vx > -0.01 && p->vx < 0.01 && p->vy > -0.01 && p->vy < 0.01 && p->vz > -0.01 && p->vz < 0.01) {
				float len = 0.25 / sqrt(dist_sq);
				p->vx = distx * len;
				p->vy = disty * len;
				p->vz = distz * len;
			}
			p->x += p->vx;
			p->y += p->vy;
			p->z += p->vz;
			p->r += PARTICLE_RANDOM(system_id->def->mindr, system_id->def->maxdr);
			p->g += PARTICLE_RANDOM(system_id->def->mindg, system_id->def->maxdg);
			p->b += PARTICLE_RANDOM(system_id->def->mindb, system_id->def->maxdb);
			p->a += PARTICLE_RANDOM(system_id->def->minda, system_id->def->maxda);
		}
	}
	UNLOCK_PARTICLES_LIST();
}
void update_fire_sys(particle_sys *system_id) {
	int i;
	int particles_to_add = 0;
	int total_particle_no = system_id->def->total_particle_no;
	particle *p;
	int j;
	// see if we need to add new particles
	LOCK_PARTICLES_LIST();
	if (system_id->ttl) {
		particles_to_add = total_particle_no - system_id->particle_count;
	}
	for (j = i = 0; i < particles_to_add; i++) {
		// find a free space
		for (; j < total_particle_no; j++) {
			if (system_id->particles[j].free) {
				// finally, we found a spot
				create_particle(system_id, &(system_id->particles[j]));
				// increase the particle count
				system_id->particle_count++;
				j++;
				break; // done looping
			}
		}
	}
	// excellent, now we have to actually update the particles
	// find a used particle
	for (j = 0, p = &system_id->particles[0]; j < total_particle_no; j++, p++) {
		if (!p->free) {
			if (p->a < 0.0f) {
				if (system_id->particle_count) {
					system_id->particle_count--;
				}
				continue;
			}
			// Fires don't use acceleration as usual...
			p->x += p->vx + PARTICLE_RANDOM(system_id->def->acc_minx, system_id->def->acc_maxx);
			p->y += p->vy + PARTICLE_RANDOM(system_id->def->acc_miny, system_id->def->acc_maxy);
			p->z += p->vz + PARTICLE_RANDOM(system_id->def->acc_minz, system_id->def->acc_maxz);
			p->r += PARTICLE_RANDOM(system_id->def->mindr, system_id->def->maxdr);
			p->g += PARTICLE_RANDOM(system_id->def->mindg, system_id->def->maxdg);
			p->b += PARTICLE_RANDOM(system_id->def->mindb, system_id->def->maxdb);
			p->a += PARTICLE_RANDOM(system_id->def->minda, system_id->def->maxda);
		}
	}
	UNLOCK_PARTICLES_LIST();
}
void update_teleporter_sys(particle_sys *system_id) {
	int i;
	int particles_to_add = 0;
	int total_particle_no = system_id->def->total_particle_no;
	particle *p;
	int j;
	// see if we need to add new particles
	LOCK_PARTICLES_LIST();
	if (system_id->ttl) {
		particles_to_add = total_particle_no - system_id->particle_count;
	}
	for (j = i = 0; i < particles_to_add; i++) {
		// find a free space
		for (; j < total_particle_no; j++) {
			if (system_id->particles[j].free) {
				// finally, we found a spot
				create_particle(system_id, &(system_id->particles[j]));
				if (system_id->particles[j].z < system_id->z_pos) {
					system_id->particles[j].z = system_id->z_pos;
				}
				// increase the particle count
				system_id->particle_count++;
				break; // done looping
			}
		}
	}
	// excellent, now we have to actually update the particles
	// find used particles
	for (j = 0, p = &system_id->particles[0]; j < total_particle_no; j++, p++) {
		if (!p->free) {
			if (p->z > system_id->z_pos + 2.0f) {
				if (system_id->particle_count) {
					system_id->particle_count--;
				}
				continue;
			}
			// Teleporters don't use acceleration as usual...
			p->x += p->vx + PARTICLE_RANDOM2(system_id->def->acc_minx, system_id->def->acc_maxx);
			p->y += p->vy + PARTICLE_RANDOM2(system_id->def->acc_miny, system_id->def->acc_maxy);
			p->z += p->vz + PARTICLE_RANDOM2(system_id->def->acc_minz, system_id->def->acc_maxz);
			p->r += PARTICLE_RANDOM2(system_id->def->mindr, system_id->def->maxdr);
			p->g += PARTICLE_RANDOM2(system_id->def->mindg, system_id->def->maxdg);
			p->b += PARTICLE_RANDOM2(system_id->def->mindb, system_id->def->maxdb);
			p->a += PARTICLE_RANDOM2(system_id->def->minda, system_id->def->maxda);
		}
	}
	UNLOCK_PARTICLES_LIST();
}
void update_teleport_sys(particle_sys *system_id) {
	int i;
	int total_particle_no = system_id->def->total_particle_no;
	int particles_to_add = 0;
	particle *p;
	int j;
	// see if we need to add new particles
	LOCK_PARTICLES_LIST();
	if (system_id->ttl) {
		particles_to_add = total_particle_no - system_id->particle_count;
	}
	for (j = i = 0; i < particles_to_add; i++) {
		// find a free space
		for (; j < total_particle_no; j++) {
			if (system_id->particles[j].free) {
				create_particle(system_id, &(system_id->particles[j]));
				system_id->particles[j].x = system_id->x_pos;
				system_id->particles[j].y = system_id->y_pos;
				system_id->particles[j].z = system_id->z_pos;
				system_id->particle_count++;
				break;
			}
		}
	}
	// excellent, now we have to actually update the particles
	// find used particles
	for (j = 0, p = &system_id->particles[0]; j < total_particle_no; j++, p++) {
		if (!p->free) {
			if (p->z > system_id->z_pos + 2.0f) {
				if (system_id->particle_count) {
					system_id->particle_count--;
				}
				continue;
			}
			// Teleports don't use acceleration as usual...
			p->x += p->vx + PARTICLE_RANDOM2(system_id->def->acc_minx, system_id->def->acc_maxx);
			p->y += p->vy + PARTICLE_RANDOM2(system_id->def->acc_miny, system_id->def->acc_maxy);
			p->z += p->vz + PARTICLE_RANDOM2(system_id->def->acc_minz, system_id->def->acc_maxz);
			p->r += PARTICLE_RANDOM2(system_id->def->mindr, system_id->def->maxdr);
			p->g += PARTICLE_RANDOM2(system_id->def->mindg, system_id->def->maxdg);
			p->b += PARTICLE_RANDOM2(system_id->def->mindb, system_id->def->maxdb);
			p->a += PARTICLE_RANDOM2(system_id->def->minda, system_id->def->maxda);
		}
	}
	UNLOCK_PARTICLES_LIST();
}
void update_bag_part_sys(particle_sys *system_id) {
	int i;
	int total_particle_no = system_id->def->total_particle_no;
	int particles_to_add = 0;
	particle *p;
	int j;
	// see if we need to add new particles
	LOCK_PARTICLES_LIST();
	if (system_id->ttl) {
		particles_to_add = total_particle_no - system_id->particle_count;
	}
	for (j = i = 0; i < particles_to_add; i++) {
		// find a free space
		for (; j < total_particle_no; j++) {
			if (system_id->particles[j].free) {
				// finally, we found a spot
				create_particle(system_id, &(system_id->particles[j]));
				if (system_id->particles[j].z < system_id->z_pos) {
					system_id->particles[j].z = system_id->z_pos;
				}
				// increase the particle count
				system_id->particle_count++;
				break; // done looping
			}
		}
	}
	// excellent, now we have to actually update the particles
	// find used particles
	for (j = 0, p = &system_id->particles[0]; j < total_particle_no; j++, p++) {
		if (!p->free) {
			if (p->z > system_id->z_pos + 1.0f) {
				if (system_id->particle_count) {
					system_id->particle_count--;
				}
				continue;
			}
			// Bags don't use acceleration as usual...
			p->x += p->vx + PARTICLE_RANDOM2(system_id->def->acc_minx, system_id->def->acc_maxx);
			p->y += p->vy + PARTICLE_RANDOM2(system_id->def->acc_miny, system_id->def->acc_maxy);
			p->z += p->vz + PARTICLE_RANDOM2(system_id->def->acc_minz, system_id->def->acc_maxz);
			p->r += PARTICLE_RANDOM2(system_id->def->mindr, system_id->def->maxdr);
			p->g += PARTICLE_RANDOM2(system_id->def->mindg, system_id->def->maxdg);
			p->b += PARTICLE_RANDOM2(system_id->def->mindb, system_id->def->maxdb);
			p->a += PARTICLE_RANDOM2(system_id->def->minda, system_id->def->maxda);
		}
	}
	UNLOCK_PARTICLES_LIST();
}
void update_particles() {
	unsigned int i, l, start, stop;
	if (!particles_percentage) {
		return;
	}
	LOCK_PARTICLES_LIST();
	for (i = 0; i < MAX_PARTICLE_SYSTEMS; i++) {
		if (particles_list[i]) {
			// Systems with a TTL need to be updated, even if they are far away
			if (particles_list[i]->ttl < 0) {
				continue;
			}
			switch (particles_list[i]->def->part_sys_type) {
			case TELEPORTER_PARTICLE_SYS:
				update_teleporter_sys(particles_list[i]);
				break;
			case TELEPORT_PARTICLE_SYS:
				update_teleport_sys(particles_list[i]);
				break;
			case BAG_PARTICLE_SYS:
				update_bag_part_sys(particles_list[i]);
				break;
			case BURST_PARTICLE_SYS:
				update_burst_sys(particles_list[i]);
				break;
			case FIRE_PARTICLE_SYS:
				update_fire_sys(particles_list[i]);
				break;
			case FOUNTAIN_PARTICLE_SYS:
				update_fountain_sys(particles_list[i]);
				break;
			}
			if (particles_list[i]->ttl > 0) {
				particles_list[i]->ttl--;
			}
			// if there are no more particles to add, and the TTL expired, then kill this evil system
			if (!particles_list[i]->ttl && !particles_list[i]->particle_count) {
				destroy_partice_sys_without_lock(i);
			}
		}
	}
	get_intersect_start_stop(main_bbox_tree, TYPE_PARTICLE_SYSTEM, &start, &stop);
	for (i = start; i < stop; i++) {
		l = get_intersect_item_ID(main_bbox_tree, i);
		if (!particles_list[l]) {
			continue;
		}
		if (particles_list[l]->ttl > 0) {
			continue;
		}
		switch (particles_list[l]->def->part_sys_type) {
		case TELEPORTER_PARTICLE_SYS:
			update_teleporter_sys(particles_list[l]);
			break;
		case TELEPORT_PARTICLE_SYS:
			update_teleport_sys(particles_list[l]);
			break;
		case BAG_PARTICLE_SYS:
			update_bag_part_sys(particles_list[l]);
			break;
		case BURST_PARTICLE_SYS:
			update_burst_sys(particles_list[l]);
			break;
		case FIRE_PARTICLE_SYS:
			update_fire_sys(particles_list[l]);
			break;
		case FOUNTAIN_PARTICLE_SYS:
			update_fountain_sys(particles_list[l]);
			break;
		}
	}
	UNLOCK_PARTICLES_LIST();
}
/******************************************************************************
*                        MISC HELPER FUNCTIONS                               *
******************************************************************************/
void add_teleporters_from_list(const Uint8 *teleport_list) {
	Uint16 teleporters_no;
	int i;
	int teleport_x, teleport_y, my_offset;
	float x, y, z;
	teleporters_no = SDL_SwapLE16(*((Uint16 *)(teleport_list)));
	LOCK_PARTICLES_LIST(); // lock it to avoid timing issues
	for (i = 0; i < teleporters_no; i++) {
		my_offset = i * 5 + 2;
		teleport_x = SDL_SwapLE16(*((Uint16 *)(teleport_list + my_offset)));
		teleport_y = SDL_SwapLE16(*((Uint16 *)(teleport_list + my_offset + 2)));
		// later on, maybe we want to have different visual types
		// now, get the Z position
		if (!get_tile_valid(teleport_x, teleport_y)) {
			continue;
		}
		z = get_tile_height(teleport_x, teleport_y);
		// convert from height values to meters
		x = (float)teleport_x / 2;
		y = (float)teleport_y / 2;
		// center the object
		x = x + 0.25f;
		y = y + 0.25f;
		add_particle_sys("./particles/teleporter.part", x, y, z, 1);
		add_e3d("./3dobjects/portal1.e3d", x, y, z, 0, 0, 0, 1, 0, 1.0f, 1.0f, 1.0f, 1);
		// mark the teleporter as an unwalkable so that the pathfinder
		// won't try to plot a path through it
		pf_tile_map[teleport_y * tile_map_size_x * 6 + teleport_x].z = 0;
	}
	UNLOCK_PARTICLES_LIST();
}
/* currently UNUSED
   void dump_part_sys_info()
   {
        char str[256];
        int i,partdefs=0,partsys=0;
        LOG_TO_CONSOLE(c_grey1,particle_system_dump);
        if(!particles_percentage)
                {
                        LOG_TO_CONSOLE(c_grey1,particles_disabled_str);
                        return;
                }
        if(have_point_sprite)
          LOG_TO_CONSOLE(c_grey1,point_sprites_enabled);
        else
          LOG_TO_CONSOLE(c_grey1,using_textured_quads);
        LOG_TO_CONSOLE(c_grey1,definitions_str);
        for(i=0;i<MAX_PARTICLE_DEFS;i++)
                if(defs_list[i])
                        {
                                partdefs++;
                                LOG_TO_CONSOLE(c_grey1,defs_list[i]->file_name);
                        }
        safe_snprintf(str, sizeof(str), "#%s: %i",my_tolower(definitions_str),partdefs);
        LOG_TO_CONSOLE(c_grey1,str);
        for(i=0;i<MAX_PARTICLE_SYSTEMS;i++)
                if(particles_list[i])partsys++;
        safe_snprintf(str, sizeof(str), "#%s: %i",part_sys_str,partsys);
        LOG_TO_CONSOLE(c_grey1,str);
        safe_snprintf(str, sizeof(str), "#%s: %i%%",part_part_str,particles_percentage);
        LOG_TO_CONSOLE(c_grey1,str);
   }
 */
