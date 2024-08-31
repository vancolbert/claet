#include <stdlib.h>
#include <string.h>

#include "roche.h"
#include "bags.h"
#include "3d_objects.h"
#include "asc.h"
#include "cursors.h"
#include "elwindows.h"
#include "errors.h"
#include "gamewin.h"
#include "init.h"
#include "interface.h"
#include "multiplayer.h"
#include "textures.h"
#include "tiles.h"
#include "translate.h"
#include "gl_init.h"

roche roche_list[NUM_ROCHES];
void put_roche_on_ground(int roche_x, int roche_y, int roche_id) /// stolen from put_bag_on_ground(int, int, int);
{
    float x,y,z;
    int obj_3d_id;
	//now, get the Z position
	if(roche_y*tile_map_size_x*6+roche_x>tile_map_size_x*tile_map_size_y*6*6) {
		//Warn about this error:
		LOG_ERROR("A roche was placed OUTSIDE the map!\n");
		return;
	}

	z=-2.2f+height_map[roche_y*tile_map_size_x*6+roche_x]*0.2f;
	//convert from height values to meters
	x=(float)roche_x/2;
	y=(float)roche_y/2;
	//center the object (slightly randomized)
	x = x + 0.25f;
	y = y + 0.25f;

	obj_3d_id=add_e3d("./3dobjects/rock_small4.e3d", x, y, z,
		get_bag_tilt(roche_x, roche_y, roche_id, tile_map_size_x, tile_map_size_y), 0, /// we use bags funct, should rename function into "3D_tilt & 3D_rotation"
		get_bag_rotation(roche_x, roche_y, roche_id, tile_map_size_x, tile_map_size_y),
		1 ,0 ,1.0f ,1.0f, 1.0f, 1);

    roche_list[roche_id].x=roche_x;
	roche_list[roche_id].y=roche_y;
	roche_list[roche_id].obj_3d_id=obj_3d_id;
}
void add_roches_from_list (const Uint8 *data)
{
	Uint16 roches_no;
	int i;
	int roche_x,roche_y,my_offset;
	float x,y,z;
	int obj_3d_id, roche_id;

	roches_no=data[0];

	if(roches_no > NUM_ROCHES) {
		return;//something nasty happened
	}

	for(i=0;i<roches_no;i++) {
		my_offset=i*5+1;
		roche_x=SDL_SwapLE16(*((Uint16 *)(data+my_offset)));
		roche_y=SDL_SwapLE16(*((Uint16 *)(data+my_offset+2)));
		roche_id=*((Uint8 *)(data+my_offset+4));
		if(roche_id >= NUM_ROCHES) {
			continue;
		}
		//now, get the Z position
		if(roche_y*tile_map_size_x*6+roche_x>tile_map_size_x*tile_map_size_y*6*6)  {
			//Warn about this error!
			LOG_ERROR("A roche was located OUTSIDE the map!\n");
			continue;
		}

		z=-2.2f+height_map[roche_y*tile_map_size_x*6+roche_x]*0.2f;
		//convert from height values to meters
		x=(float)roche_x/2;
		y=(float)roche_y/2;
		//center the object (slightly randomized)
		x = x + 0.25f;
		y = y + 0.25f;


		// Now, find a place into the bags list, so we can destroy the bag properly
		if (roche_list[roche_id].obj_3d_id != -1) {
			char	buf[256];
			// oops, slot already taken!
			safe_snprintf(buf, sizeof(buf), "Oops, trying to add an existing roche! id=%d\n", roche_id);
			LOG_ERROR(buf);
			return;
		}

		obj_3d_id = add_e3d("./3dobjects/rock_small4.e3d", x, y, z,
			get_bag_tilt(roche_x, roche_y, roche_id, tile_map_size_x, tile_map_size_y), 0,
			get_bag_rotation(roche_x, roche_y, roche_id, tile_map_size_x, tile_map_size_y), /// see put_roche_on_ground comment about theses functions
			1, 0, 1.0f, 1.0f, 1.0f, 1);
		roche_list[roche_id].x=roche_x;
		roche_list[roche_id].y=roche_y;
		roche_list[roche_id].obj_3d_id=obj_3d_id;
	}
}
void remove_roche(int roche_id)
{
	if (roche_id >= NUM_ROCHES) return;

	if (roche_list[roche_id].obj_3d_id == -1) {
		// oops, no roche in that slot!
		LOG_ERROR("Oops, double-removal of roche!\n");
		return;
	}

	destroy_3d_object(roche_list[roche_id].obj_3d_id);
	roche_list[roche_id].obj_3d_id=-1;
}

void remove_all_roches(void){
	int i;

	for(i=0; i<NUM_ROCHES; i++){    // clear roches list!!!!
		roche_list[i].obj_3d_id= -1;
	}
}
int harvest_roche(int object_id)
{
	int i;
	Uint8 str[4];
	for(i=0;i<NUM_ROCHES;i++){
		if(roche_list[i].obj_3d_id==object_id){
			str[0]= HARVEST;
			str[1]= i;
			my_tcp_send(my_socket,str,2);
			return 1;
		}
	}
	return 0;
}
