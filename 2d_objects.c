#include <ctype.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <errno.h>
#include "2d_objects.h"
#include "asc.h"
#include "draw_scene.h"
#include "elmemory.h"
#include "errors.h"
#include "init.h"
#include "load_gl_extensions.h"
#include "map.h"
#include "platform.h"
#include "shadows.h"
#include "textures.h"
#include "tiles.h"
#include "translate.h"
#include "io/elfilewrapper.h"
#include "cluster.h"
#include "fsaa/fsaa.h"

#define INVALID -1
#define GROUND 0
#define PLANT 1
#define FENCE 2

obj_2d *obj_2d_list[MAX_OBJ_2D];

static obj_2d_def* obj_2d_def_cache[MAX_OBJ_2D_DEF];
static int obj_2d_cache_used = 0;

int map_meters_size_x;
int map_meters_size_y;
float texture_scale=12.0;

void draw_2d_object(obj_2d *object_id)
{
	float render_x_start,render_y_start,u_start,v_start,u_end,v_end;
	float x_pos,y_pos,z_pos;
	float x_rot,y_rot,z_rot;
	float x_size,y_size;
	int object_type;
	obj_2d_def *obj_def_pointer;

	if(!object_id->display) return;	// not currently on the map, ignore it

	obj_def_pointer=object_id->obj_pointer;

	u_start=obj_def_pointer->u_start;
	u_end=obj_def_pointer->u_end;
 	v_start=obj_def_pointer->v_start;
	v_end=obj_def_pointer->v_end;
	x_size=obj_def_pointer->x_size;
	y_size=obj_def_pointer->y_size;
	render_x_start=-x_size/2.0f;

	object_type=obj_def_pointer->object_type;
	if(object_type==GROUND)render_y_start=-y_size/2;
	else	render_y_start=0;

	glPushMatrix();//we don't want to affect the rest of the scene

	x_pos=object_id->x_pos;
	y_pos=object_id->y_pos;
	z_pos=object_id->z_pos;
	if (object_type != PLANT)
	{
		glMultMatrixf(object_id->matrix);
		z_rot = object_id->z_rot;
	}
	else
	{
		glTranslatef (x_pos, y_pos, 0);

		x_rot = object_id->x_rot + 90;
		y_rot = object_id->y_rot;
		z_rot=-rz;
		glRotatef(z_rot, 0.0f, 0.0f, 1.0f);
		glRotatef(x_rot, 1.0f, 0.0f, 0.0f);
		glRotatef(y_rot, 0.0f, 1.0f, 0.0f);
	}

	bind_texture(obj_def_pointer->texture_id);

	if (dungeon || (!clouds_shadows && !use_shadow_mapping))
		{
			glBegin(GL_QUADS);

			glTexCoord2f(u_start,v_start);
			glVertex3f(render_x_start,render_y_start,z_pos);

			glTexCoord2f(u_start,v_end);
			glVertex3f(render_x_start,render_y_start+y_size,z_pos);

			glTexCoord2f(u_end,v_end);
			glVertex3f(render_x_start+x_size,render_y_start+y_size,z_pos);

			glTexCoord2f(u_end,v_start);
			glVertex3f(render_x_start+x_size,render_y_start,z_pos);

	    		glEnd();
		}
	else
		{
			float m,x,y,x1,y1;
			float cos_m,sin_m;

			glBegin(GL_QUADS);

			m=(-z_rot)*M_PI/180;
			cos_m=cos(m);
			sin_m=sin(m);

			x=render_x_start;
			y=render_y_start;
			x1=x*cos_m+y*sin_m;
			y1=y*cos_m-x*sin_m;
			x1=x_pos+x1;
			y1=y_pos+y1;

			ELglMultiTexCoord2fARB(base_unit,u_start,v_start);
			ELglMultiTexCoord2fARB(detail_unit,x1/texture_scale
								 +clouds_movement_u,y1/texture_scale
								 +clouds_movement_v);
			glVertex3f(x,y,z_pos);

			x=render_x_start;
			y=render_y_start+y_size;
			x1=x*cos_m+y*sin_m;
			y1=y*cos_m-x*sin_m;
			x1=x_pos+x1;
			y1=y_pos+y1;

			ELglMultiTexCoord2fARB(base_unit,u_start,v_end);
			ELglMultiTexCoord2fARB(detail_unit,x1/texture_scale
								 +clouds_movement_u,y1/texture_scale
								 +clouds_movement_v);
			glVertex3f(x,y,z_pos);

			x=render_x_start+x_size;
			y=render_y_start+y_size;
			x1=x*cos_m+y*sin_m;
			y1=y*cos_m-x*sin_m;
			x1=x_pos+x1;
			y1=y_pos+y1;

			ELglMultiTexCoord2fARB(base_unit,u_end,v_end);
			ELglMultiTexCoord2fARB(detail_unit,x1/texture_scale
								 +clouds_movement_u,y1/texture_scale
								 +clouds_movement_v);
			glVertex3f(x,y,z_pos);

			x=render_x_start+x_size;
			y=render_y_start;
			x1=x*cos_m+y*sin_m;
			y1=y*cos_m-x*sin_m;
			x1=x_pos+x1;
			y1=y_pos+y1;


			ELglMultiTexCoord2fARB(base_unit,u_end,v_start);
			ELglMultiTexCoord2fARB(detail_unit,x1/texture_scale
								 +clouds_movement_u,y1/texture_scale
								 +clouds_movement_v);
			glVertex3f(x,y,z_pos);
    		glEnd();
		}
	glPopMatrix();//restore the scene
}

static void parse_2d0(const char* desc, Uint32 len, const char* cur_dir,
	obj_2d_def *def)
{
	char name[256], value[256];
	const char *cp, *cp_end;
	Uint32 i;

	int file_x_len = -1, file_y_len = -1;
	int u_start = -1, u_end = -1, v_start = -1, v_end = -1;

	def->x_size = def->y_size = def->alpha_test = -1;

	cp = desc;
	cp_end = cp + len;
	while (1)
	{
		// skip whitespace
		while (cp < cp_end && isspace(*cp))
			cp++;
		if (cp >= cp_end) break;

		// copy the key
		i = 0;
		while (cp < cp_end && i < sizeof(name)-1 && !isspace(*cp) && *cp != ':' && *cp != '=')
			name[i++] = *cp++;
		name[i] = '\0';
		if (cp >= cp_end) break;

		// skip separators
		while (cp < cp_end && (isspace(*cp) || *cp == ':' || *cp == '='))
		{
			if (*cp == '\n')
				break;
			cp++;
		}
		if (cp >= cp_end) break;
		if (*cp == '\n') continue; // no value

		// copy value
		i = 0;
		while (cp < cp_end && i < sizeof(value)-1 && !isspace(*cp))
			value[i++] = *cp++;
		value[i] = '\0';

		if (!strcasecmp(name, "file_x_len"))
			file_x_len = atoi(value);
		else if (!strcasecmp(name, "file_y_len"))
			file_y_len = atoi(value);
		else if (!strcasecmp(name, "u_start"))
			u_start = atoi(value);
		else if (!strcasecmp(name, "u_end"))
			u_end = atoi(value);
		else if (!strcasecmp(name, "v_start"))
			v_start = atoi(value);
		else if (!strcasecmp(name, "v_end"))
			v_end = atoi(value);
		else if (!strcasecmp(name, "x_size"))
			def->x_size = atof(value);
		else if (!strcasecmp(name, "y_size"))
			def->y_size = atof(value);
		else if (!strcasecmp(name, "alpha_test"))
			def->alpha_test = atof(value);
		else if (!strcasecmp(name, "texture"))
		{
			char texture_file_name[256];
			safe_snprintf(texture_file_name, sizeof(texture_file_name),
				"%s/%s", cur_dir, value);
			def->texture_id = load_texture_cached(texture_file_name, tt_mesh);
		}
		else if (!strcmp(name, "type"))
		{
			switch (*value)
			{
				case 'g':
				case 'G': def->object_type = GROUND; break;
				case 'p':
				case 'P': def->object_type = PLANT; break;
				case 'f':
				case 'F': def->object_type = FENCE; break;
				default:  def->object_type = INVALID;
			}
		}
	}

	def->u_start = (float)u_start/file_x_len;
	def->u_end = (float)u_end/file_x_len;
	def->v_start = 1.0f + (float)v_start/file_y_len;
	def->v_end = 1.0f + (float)v_end/file_y_len;
	if (def->alpha_test < 0)
		def->alpha_test = 0;
}

static obj_2d_def* load_obj_2d_def(const char *file_name)
{
	int f_size;
	el_file_ptr file = NULL;
	char cur_dir[200]={0};
	obj_2d_def *cur_object;
	const char *obj_file_mem;
	const char* sep;

	sep = strrchr(file_name, '/');
	if (!sep || sep == file_name)
		*cur_dir = '\0';
	else
		my_strncp(cur_dir, file_name, sep-file_name+1);

	file = el_open(file_name);
	if (!file)
	{
		LOG_ERROR("%s: %s \"%s\": %s\n", reg_error_str, cant_open_file, file_name, strerror(errno));
		return NULL;
	}

	obj_file_mem = el_get_pointer(file);
	if (!obj_file_mem)
	{
		LOG_ERROR("%s: %s (read)\"%s\"\n", reg_error_str, cant_open_file, file_name);
		el_close(file);
		return NULL;
	}
	f_size = el_get_size(file);

	//ok, the file is loaded, so parse it
	cur_object=calloc(1, sizeof(obj_2d_def));
	my_strncp(cur_object->file_name, file_name,
		sizeof(cur_object->file_name));
	parse_2d0(obj_file_mem, f_size, cur_dir, cur_object);

	el_close(file);

	return cur_object;
}

static int cache_cmp_string(const void* str, const void *dptr)
{
	const obj_2d_def *def = *((const obj_2d_def**)dptr);
	return strcmp(str, def->file_name);
}

//Tests to see if an obj_2d object is already loaded.
//If it is, return the handle.
//If not, load it, and return the handle
static obj_2d_def* load_obj_2d_def_cache(const char* file_name)
{
	obj_2d_def *def, **defp;
	int i;

	defp = bsearch(file_name, obj_2d_def_cache,
		obj_2d_cache_used, sizeof(obj_2d_def*),
		cache_cmp_string);
	if (defp)
		return *defp;

	//asc not found in the cache, so load it ...
	def = load_obj_2d_def(file_name);

	// no object found, so nothing to store in the cache
	if (def == NULL)
		return NULL;

	// ... and store it
	if (obj_2d_cache_used < MAX_OBJ_2D_DEF)
	{
		for (i = 0; i < obj_2d_cache_used; i++)
		{
			if (strcmp(file_name, obj_2d_def_cache[i]->file_name) <= 0)
			{
				memmove(obj_2d_def_cache+(i+1), obj_2d_def_cache+i,
					(obj_2d_cache_used-i)*sizeof(obj_2d_def*));
				break;
			}
		}
		obj_2d_def_cache[i] = def;
		obj_2d_cache_used++;
	}

	return def;
}

int get_2d_bbox (int id, AABBOX* box)
{
	const obj_2d* obj;
	const obj_2d_def* def;
	float len_x;
	float len_y;

	if (id < 0 || id >= MAX_OBJ_2D || obj_2d_list[id] == NULL)
		return 0;

	obj = obj_2d_list[id];
	def = obj->obj_pointer;
	if (def == NULL)
		return 0;

	len_x = def->x_size;
	len_y = def->y_size;

	box->bbmin[X] = -len_x*0.5f;
	box->bbmax[X] =  len_x*0.5f;
	if (def->object_type == GROUND)
	{
		box->bbmin[Y] = -len_y*0.5f;
		box->bbmax[Y] =  len_y*0.5f;
	}
	else
	{
		box->bbmin[Y] = 0.0f;
		box->bbmax[Y] = len_y;
		if (def->object_type == PLANT)
		{
			box->bbmin[X] *= M_SQRT2;
			box->bbmax[X] *= M_SQRT2;
			box->bbmin[Y] *= M_SQRT2;
			box->bbmax[Y] *= M_SQRT2;
		}
	}
	box->bbmin[Z] = obj->z_pos;
	box->bbmax[Z] = obj->z_pos;

	matrix_mul_aabb (box, obj->matrix);

	return 1;
}

int add_2d_obj(int id_hint, const char* file_name,
	float x_pos, float y_pos, float z_pos,
	float x_rot, float y_rot, float z_rot, unsigned int dynamic)
{
	int id;
	char fname[128];
	obj_2d_def *returned_obj_2d_def;
	obj_2d *our_object;
	unsigned int alpha_test, texture_id;
	AABBOX bbox;

	id = id_hint;
	if (obj_2d_list[id])
	{
		// occupied, find a free spot in the obj_2d_list
		for(id = 0; id < MAX_OBJ_2D; id++)
		{
			if(!obj_2d_list[id])
				break;
		}
		if (id >= MAX_OBJ_2D)
		{
			LOG_ERROR("2D object list is full");
			return -1;
		}
	}

	// convert filename to lower case and replace any '\' by '/'
	clean_file_name(fname, file_name, sizeof(fname));

	returned_obj_2d_def = load_obj_2d_def_cache(fname);
	if(!returned_obj_2d_def)
	{
		LOG_ERROR ("%s: %s: %s", reg_error_str, cant_load_2d_object, fname);
		return -1;
	}

	our_object = calloc(1, sizeof(obj_2d));
	our_object->x_pos = x_pos;
	our_object->y_pos = y_pos;
	our_object->z_pos = z_pos;
	our_object->x_rot = x_rot;
	our_object->y_rot = y_rot;
	our_object->z_rot = z_rot;
	our_object->obj_pointer = returned_obj_2d_def;
	our_object->display = 1;
	our_object->state = 0;

	obj_2d_list[id] = our_object;

	if (returned_obj_2d_def->object_type == PLANT)
	{
		x_rot += 90.0f;
		z_rot = 0.0f;
	}
	else if (returned_obj_2d_def->object_type == FENCE)
	{
		x_rot += 90.0f;
	}
	calc_rotation_and_translation_matrix(our_object->matrix, x_pos, y_pos, 0.0f, x_rot, y_rot, z_rot);

	our_object->cluster = get_cluster((int)(x_pos/0.5f), (int)(y_pos/0.5f));
	current_cluster = our_object->cluster;

	alpha_test = returned_obj_2d_def->alpha_test ? 1 : 0;
	texture_id = returned_obj_2d_def->texture_id;

	if (get_2d_bbox(id, &bbox))
	{
		if (main_bbox_tree_items != NULL && dynamic == 0)
			add_2dobject_to_list(main_bbox_tree_items, id, bbox, alpha_test, texture_id);
		else
			add_2dobject_to_abt(main_bbox_tree, id, bbox, alpha_test, texture_id, dynamic);
	}

	return id;
}

const char* get_2dobject_at_location(float x_pos, float y_pos)
{
	int i;
	float offset = 0.5f;
	for (i = 0; i < MAX_OBJ_2D; i++)
	{
		if (obj_2d_list[i]
			&& obj_2d_list[i]->x_pos > (x_pos - offset) && obj_2d_list[i]->x_pos < (x_pos + offset)
			&& obj_2d_list[i]->y_pos > (y_pos - offset) && obj_2d_list[i]->y_pos < (y_pos + offset)
			&& obj_2d_list[i]->display && obj_2d_list[i]->obj_pointer->object_type == GROUND)
		{
			return obj_2d_list[i]->obj_pointer->file_name;
		}
	}
	return "";
}


void display_2d_objects()
{
	unsigned int i, l, start, stop;


	//First draw everyone with the same alpha test
	if (fsaa > 1)
	{
		glEnable(GL_MULTISAMPLE);
	}
	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER, 0.18f);

	if (!dungeon && !(!clouds_shadows && !use_shadow_mapping))
	{
		if(clouds_shadows)
		{
			//bind the detail texture
			ELglActiveTextureARB(detail_unit);
			glEnable(GL_TEXTURE_2D);
			//glBindTexture(GL_TEXTURE_2D, texture_cache[ground_detail_text].texture_id);
			bind_texture_unbuffered(ground_detail_text);
		}
		ELglActiveTextureARB(base_unit);
		glEnable(GL_TEXTURE_2D);
	}

	get_intersect_start_stop(main_bbox_tree, TYPE_2D_NO_ALPHA_OBJECT, &start, &stop);
	for (i = start; i < stop; i++)
	{
		l = get_intersect_item_ID(main_bbox_tree, i);
		draw_2d_object(obj_2d_list[l]);
	}

	//Then draw all that needs a change
	get_intersect_start_stop(main_bbox_tree, TYPE_2D_ALPHA_OBJECT, &start, &stop);
	for (i = start; i < stop; i++)
	{
		l = get_intersect_item_ID(main_bbox_tree, i);
		glAlphaFunc(GL_GREATER, obj_2d_list[l]->obj_pointer->alpha_test);
		draw_2d_object(obj_2d_list[l]);
	}

	if (!dungeon && !(!clouds_shadows && !use_shadow_mapping))
	{
    		//disable the multitexturing
		ELglActiveTextureARB(detail_unit);
		glDisable(GL_TEXTURE_2D);
		ELglActiveTextureARB(base_unit);
	}

	if (fsaa > 1)
	{
		glDisable(GL_MULTISAMPLE);
	}
	glDisable(GL_ALPHA_TEST);
}

void destroy_2d_object(int i)
{
	if (i < 0 || i >= MAX_OBJ_2D || !obj_2d_list[i])
		return;
	delete_2dobject_from_abt(main_bbox_tree, i, obj_2d_list[i]->obj_pointer->alpha_test);
	free(obj_2d_list[i]);
	obj_2d_list[i] = NULL;
}

void destroy_all_2d_objects()
{
	int i;
	for (i = 0; i < MAX_OBJ_2D; i++)
	{
		free(obj_2d_list[i]);
		obj_2d_list[i] = NULL;
	}
}

void destroy_all_2d_object_defs()
{
	int i;
	for (i = 0; i < obj_2d_cache_used; i++)
	{
		free(obj_2d_def_cache[i]);
		obj_2d_def_cache[i] = NULL;
	}
	obj_2d_cache_used=0;
}

// for support of the 1.0.3 server, change if an object is to be displayed or not
void set_2d_object (Uint8 display, const void *ptr, int len)
{
	const Uint32 *id_ptr = ptr;

	// first look for the override to process ALL objects
	if (len < sizeof(*id_ptr) ){
		int	i;

		for (i = 0; i < MAX_OBJ_2D; i++)
		{
			if (obj_2d_list[i]){
				obj_2d_list[i]->display= display;
			}
		}
	} else {
		int idx = 0;

		while(len >= sizeof(*id_ptr)){
			Uint32 obj_id = SDL_SwapLE32(id_ptr[idx]);

			if(obj_id < MAX_OBJ_2D && obj_2d_list[obj_id]){
				obj_2d_list[obj_id]->display= display;
			}
	    	idx++;
			len -= sizeof (*id_ptr);
		}
	}
}

// for future expansion
void state_2d_object (Uint8 state, const void *ptr, int len)
{
	const Uint32 *id_ptr = ptr;

	// first look for the override to process ALL objects
	if (len < sizeof(*id_ptr) ){
		int	i;

		for (i = 0; i < MAX_OBJ_2D; i++)
		{
			if (obj_2d_list[i]){
				obj_2d_list[i]->state= state;
			}
		}
	} else {
		int idx = 0;

		while(len >= sizeof(*id_ptr)){
			Uint32 obj_id = SDL_SwapLE32(id_ptr[idx]);

			if(obj_id < MAX_OBJ_2D && obj_2d_list[obj_id]){
				obj_2d_list[obj_id]->state= state;
				idx++;
				len -= sizeof (*id_ptr);
			}
	    	idx++;
			len -= sizeof (*id_ptr);
		}
	}
}
