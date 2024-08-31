/*!
 * \file
 * \ingroup 	display
 * \brief 	Handles rendering and loading 2d objects
 */
#ifndef __OBJ_2D_H__
#define __OBJ_2D_H__
#include "vmath.h"
#include "bbox_tree.h"
#ifdef __cplusplus
extern "C" {
#endif
/*!
 *\name 	2D Object array sizes
 *		The sizes of the arrays where we keep 2d objects and 2d object definitions
 */
/*! \{ */
#define MAX_OBJ_2D 15000 /*!<Maximum number of 2d objects in a map*/
#define MAX_OBJ_2D_DEF 1000 /*!<Maximum number of loaded 2d object definitions*/
/*! \} */
/*!
 * obj_2d_def is loaded from a .2do-file and is shared amongst all objects of that type in the obj_2d_def_cache array
 */
typedef struct {
	char file_name[128]; /*!< name of the file that contains the definition of the 2d object */
	/*! \name start and end coordinates of the texture @{ */
	float u_start; /*!< start position of the u coordinate */
	float u_end; /*!< end position of the u coordinate */
	float v_start; /*!< start position of the v coordinate */
	float v_end; /*!< end position of the v coordiante */
	/*! @} */
	/*! \name size of the 2d object @{ */
	float x_size; /*!< size in x direction */
	float y_size; /*!< size in y direction */
	/*! @} */
	float alpha_test; /*!< Use alpha?*/
	int object_type; /*!< Is this a ground, fence or plant object?
	                  * ground: don't change rotation
	                  * plant: put in an upwards rotation (x_rot+=90) and set z_rot=-rz
	                  * fence: put in an upwards rotation (x_rot+=90)
	                  */
	int texture_id; /*!< The location in the texture cache. */
} obj_2d_def;
/*!
 * The obj_2d determines the position and rotation of the given 2d object. Furthermore it determines the type
 */
typedef struct {
	/*! \name position of the object @{ */
	float x_pos;
	float y_pos;
	float z_pos;
	/*! @} */
	/*! \name rotation of the object @{ */
	float x_rot;
	float y_rot;
	float z_rot;
	/*! @} */
	MATRIX4x4 matrix; /*!< translation and rotaion matrix */
	char display; /*!< flag determining whether the object is to be shown on screen. */
	char state; /*!< state flag for future expansion & data alignment. */
	obj_2d_def *obj_pointer; /**< Points to the 2d object type in the obj_2d_def list */
	short cluster;
} obj_2d;
extern obj_2d *obj_2d_list[MAX_OBJ_2D]; /*!< The 2d object array - holds all 2d objects on that map*/
extern float texture_scale; /*!< scaling factor for textures */
/*
 * \ingroup	display_2d
 * \brief	Displays the 2d object given by object_id
 *
 * 	Displays the 2D object given by object_id
 */
void draw_2d_object(obj_2d *object_id);
/*!
 * \ingroup	display_2d
 * \brief	Displays the 2dobjects in the obj_2d_list array
 *
 *         	Parses through the obj_2d_list, checking for an object within the viewing distance (dist_x^2+dist_y^2<=220)
 *
 * \sa 		draw_2d_object
 * \sa		obj_2d_list
 * \callgraph
 */
void display_2d_objects();
/*!
 * \ingroup	load_2d
 * \brief	Get the bounding box of a 2D object
 *
 * 		Compute the bounding box for the 2D object with ID \a id.
 *
 * \param	id  The position on obj_2d_list of the object
 * \param	box Pointer to the resulting bounding box
 * \retval int	0 on failure, 1 on success
 */
int get_2d_bbox(int id, AABBOX *box);
/*!
 * \ingroup     load_2d
 * \brief       Adds a 2d object at the given location.
 *
 *              Adds a 2d object at the given location.
 *              It's usually called in the map loading process. Requires a location and rotation for the 2d object, that's loaded from the$
 *
 * \param       id_hint   Hint on the position of the object in the list. If
 *                        this spot is already taken, a vacant spot will be
 *                        chosen, but chances are that mouse click events on
 *                        object won;t work because of a wrong ID.
 * \param       file_name The filename of the object we wish to add
 * \param       x_pos     The x position
 * \param       y_pos     The y position
 * \param       z_pos     The z position
 * \param       x_rot     The x rotation
 * \param       y_rot     The y rotation
 * \param       z_rot     The z rotation
 * \retval int  Returns -1 on failure and the location in the obj_2d_list if it succeeds
 * \callgraph
 */
int add_2d_obj(int id_hint, const char *file_name, float x_pos, float y_pos, float z_pos, float x_rot, float y_rot, float z_rot, unsigned int dynamic);
/*!
 * \ingroup	load_2d
 * \brief	Show or hide one or more 2D map objects
 *
 * 		Show or hide 2D map objects.
 *		This routine is usually under server control to allow dynamically enabling or disabling seeing objects
 *
 * \param	display_flag whether the objects are to be displayed or not
 * \param	ptr pointer to an array of object ID's to be affected
 * \param	len the length in bytes of the array
 * \callgraph
 */
void set_2d_object(Uint8 display, const void *ptr, int len);
/*!
 * \ingroup	load_2d
 * \brief	Set the state for one or more 2D map objects
 *
 * 		Set the sate for 2D map objects.
 *		This routine is usually under server control to allow dynamically setting a state for an object, this is for future expansion
 *
 * \param	display_flag whether the objects are to be displayed or not
 * \param	ptr pointer to an array of object ID's to be affected
 * \param	len the length in bytes of the array
 * \callgraph
 */
void state_2d_object(Uint8 state, const void *ptr, int len);
/*!
 * \ingroup	display_2d
 * \brief	Destroys the 2d object at position i in the obj_2d_list
 *
 * 		Destroys the 2d object on position i in the obj_2d_list - frees the memory and sets the obj_2d_list[i]=NULL.
 *
 * \param	i The position in the obj_2d_list
 *
 * \callgraph
 */
void destroy_2d_object(int i);
/*!
 * \ingroup	display_2d
 * \brief	Destroys all current 2d objects
 *
 * 		Destroys all 2d objects currently in the obj_2d_list
 *
 * \callgraph
 */
void destroy_all_2d_objects(void);
/*!
 * \ingroup	display_2d
 * \brief	Destroys all current 2d object defs
 *
 * 		Destroys all 2d object defs currently in the obj_2d_def_cache
 *
 * \callgraph
 */
void destroy_all_2d_object_defs(void);
/*!
 * \ingroup	load_2d
 * \brief	Searches for a 2d ground object at a location
 *
 * 		It searches for a 2d ground object at the specified location
 *
 * \param	x_pos		The x position to search for
 * \param	y_pos		The y position to search for
 * \retval char			Returns the object's filename if found, "" otherwise.
 *
 * \sa add_e3d_at_id
 *
 * \callgraph
 */
const char *get_2dobject_at_location(float x_pos, float y_pos);
#ifdef __cplusplus
} // extern "C"
#endif
#endif
