#include <stdlib.h>
#include "actors.h"
#include "actor_init.h"
#include "cal.h"
#include "draw_scene.h"
#include "errors.h"
#include "font.h"
#include "global.h"
#include "load_gl_extensions.h"
#include "shadows.h"
#include "translate.h"
#include "asc.h"
#include "sound.h"
#include "tiles.h"
#include "io/elfilewrapper.h"
#include "io/cal3d_io_wrapper.h"



void cal_play_anim_sound(actor *pActor, struct cal_anim anim){

	unsigned int *cookie;

	cookie = &pActor->cur_anim_sound_cookie;
	// Check if we need a walking sound
	if (pActor->moving && !pActor->fighting){
		handle_walking_sound(pActor, anim.sound);
	} else {
		if (check_sound_loops(*cookie))
				stop_sound(*cookie);
		*cookie = 0;

		if (anim.sound > -1 && !pActor->dead){
			// We are going to try letting sounds continue until finished, except looping sounds of course
			// Found a sound, so add it
			*cookie = add_sound_object_gain(anim.sound,
									 2*pActor->x_pos,
									 2*pActor->y_pos,
									 pActor->actor_id == yourself ? 1 : 0,
									 anim.sound_scale);
		}
	}

}





void cal_actor_set_anim_delay(int id, struct cal_anim anim, float delay)
{
	actor *pActor = actors_list[id];
	struct CalMixer *mixer;
	int i;

	//char str[255];
	//sprintf(str, "actor:%d anim:%d type:%d delay:%f\0",id,anim.anim_index,anim.kind,delay);
	//LOG_TO_CONSOLE(c_green2,str);

	if (pActor==NULL)
		return;

	if (pActor->calmodel==NULL)
		return;

	if (pActor->cur_anim.anim_index==anim.anim_index)
		return;

	//this shouldnt happend but its happends if actor doesnt have
	//animation so we add this workaround to prevent "freezing"
	if(anim.anim_index==-1){
        attachment_props *att_props;
		if(	pActor->sitting==1 ){
			//we dont have sitting anim so cancel it
			pActor->sitting=0;
		}
		pActor->stop_animation=0;
        att_props = get_attachment_props_if_held(pActor);
		if (att_props)
		{
			anim.anim_index = att_props->cal_frames[cal_attached_idle_frame].anim_index;
			anim.duration = 0.5; // we're not really idle, so only play a short version as placeholder
			anim.duration_scale = att_props->cal_frames[cal_attached_idle_frame].duration_scale;
		}
		else
		{
			anim.anim_index = actors_defs[pActor->actor_type].cal_frames[cal_actor_idle1_frame].anim_index;
			anim.duration = 0.5; // we're not really idle, so only play a short version as placeholder
			anim.duration_scale = actors_defs[pActor->actor_type].cal_frames[cal_actor_idle1_frame].duration_scale;
		}
		anim.kind = cycle;
	}

	mixer=CalModel_GetMixer(pActor->calmodel);

	//Stop previous animation if needed
	if (pActor->IsOnIdle!=1 && (pActor->cur_anim.anim_index!=-1)) {
		if (pActor->cur_anim.kind==cycle) {
			//little more smooth
			delay+=cal_cycle_blending_delay;
			CalMixer_ClearCycle(mixer,pActor->cur_anim.anim_index, delay);
		}
		if (pActor->cur_anim.kind==action) {
			CalMixer_RemoveAction(mixer,pActor->cur_anim.anim_index);
			//change from action to new action or cycle should be smooth
			if(anim.duration>0.0f)
				delay=anim.duration;
			else
				delay+=cal_action_blending_delay;
		}
	}else{
		//we starting from unkown state (prev anim == -1)
		//so we add some delay to blend into new state
		if(anim.duration>0.0f)
			delay=anim.duration;
		else
			delay+=cal_action_blending_delay;
	}

	//seems to be unusable - groups are always empty???
	if (pActor->IsOnIdle==1) {
		for (i=0;i<actors_defs[pActor->actor_type].group_count;++i) {
			CalMixer_ClearCycle(mixer,pActor->cur_idle_anims[i].anim_index, delay);
		}
	}

	if (anim.kind==cycle){
		CalMixer_BlendCycle(mixer,anim.anim_index,1.0f, delay);
		CalMixer_SetAnimationTime(mixer, 0.0f);	//always start at the beginning of a cycling animation
	} else {
		CalMixer_ExecuteAction_Stop(mixer,anim.anim_index,delay,0.0);
	}

	pActor->cur_anim=anim;
	pActor->anim_time=0.0;
	pActor->last_anim_update= cur_time;
	pActor->stop_animation = anim.kind;

	CalModel_Update(pActor->calmodel,0.0001);//Make changes take effect now
	build_actor_bounding_box(pActor);



	if (pActor->cur_anim.anim_index==-1)
		pActor->busy=0;
	pActor->IsOnIdle=0;
	cal_play_anim_sound(pActor, pActor->cur_anim);

}

void cal_actor_set_anim(int id,struct cal_anim anim)
{
	cal_actor_set_anim_delay(id, anim, 0.05f);
}

void cal_set_anim_sound(struct cal_anim *my_cal_anim, const char *sound, const char *sound_scale)
{
	if (sound)
		my_cal_anim->sound = get_index_for_sound_type_name(sound);
	else
		my_cal_anim->sound = -1;

	if (sound_scale && strcasecmp(sound_scale, ""))
		my_cal_anim->sound_scale = atof(sound_scale);
	else
		my_cal_anim->sound_scale = 1.0f;
}


struct cal_anim cal_load_anim(actor_types *act, const char *str, const char *sound, const char *sound_scale, int duration)
{
	char fname[255]={0};
	struct cal_anim res={-1,cycle,0,0.0f
	,-1
	};
	struct CalCoreAnimation *coreanim;
	int i;

	if (sscanf (str, "%254s %d", fname, (int*)(&res.kind)) != 2)
	{
		LOG_ERROR("Bad animation formation: %s", str);
		return res;
	}

	if (have_sound_config && sound && strcasecmp(sound, ""))
	{
		i = get_index_for_sound_type_name(sound);
		if (i == -1)
			LOG_ERROR("Unknown sound (%s) in actor def: %s", sound, act->actor_name);
		else
			res.sound = i;
	}
	else
		res.sound = -1;

	if (sound_scale && strcasecmp(sound_scale, ""))
		res.sound_scale = atof(sound_scale);
	else
		res.sound_scale = 1.0f;

	res.anim_index=CalCoreModel_ELLoadCoreAnimation(act->coremodel,fname,act->scale);
	if(res.anim_index == -1) {
		LOG_ERROR("Cal3d error: %s: %s\n", fname, CalError_GetLastErrorDescription());
		return res;
	}
	coreanim=CalCoreModel_GetCoreAnimation(act->coremodel,res.anim_index);

	if (coreanim) {
		res.duration=CalCoreAnimation_GetDuration(coreanim);
		if (duration > 0) res.duration_scale = res.duration/(duration*0.001f);
		else res.duration_scale = 1.0f;
	} else {
		LOG_ERROR(no_animation_err_str, fname);
	}
	return res;
}



void cal_render_bones(actor *act)
{
	float lines[1024][2][3];
	float points[1024][3];
	int nrLines;
	int nrPoints;
	int currLine;
	int currPoint;
	struct CalSkeleton *skel;

	skel=CalModel_GetSkeleton(act->calmodel);
	nrLines = CalSkeleton_GetBoneLines(skel,&lines[0][0][0]);

	glLineWidth(2.0f);
	glColor3f(1.0f, 1.0f, 1.0f);

	glLineStipple(1, 0x3030);
	glEnable(GL_LINE_STIPPLE);
	glBegin(GL_LINES);

	for(currLine = 0; currLine < nrLines; currLine++) {
    		glVertex3f(lines[currLine][0][0], lines[currLine][0][1], lines[currLine][0][2]);
    		glVertex3f(lines[currLine][1][0], lines[currLine][1][1], lines[currLine][1][2]);
	}

	glEnd();
	glDisable(GL_LINE_STIPPLE);

  	// draw the bone points
  	nrPoints = CalSkeleton_GetBonePoints(skel,&points[0][0]);

	glPointSize(4.0f);
	glColor3f(0.0f, 1.0f, 1.0f);
	glBegin(GL_POINTS);
	for(currPoint = 0; currPoint < nrPoints; currPoint++) {
		glVertex3f(points[currPoint][0], points[currPoint][1], points[currPoint][2]);
	}
	glEnd();


	glLineWidth(1.0f);


}


static __inline__ void render_submesh(int meshId, int submeshCount, struct CalRenderer * pCalRenderer, float meshVertices[30000][3], float meshNormals[30000][3], float meshTextureCoordinates[30000][2], CalIndex meshFaces[50000][3], Uint32 use_lightning, Uint32 use_textures)
{
	int submeshId;
	int faceCount=0;

	for(submeshId = 0; submeshId < submeshCount; submeshId++) {
		// select mesh and submesh for further data access
		if(CalRenderer_SelectMeshSubmesh(pCalRenderer,meshId, submeshId)) {
			// get the transformed vertices of the submesh
			CalRenderer_GetVertices(pCalRenderer,&meshVertices[0][0]);

			// get the transformed normals of the submesh
			if (use_lightning)
			{
				CalRenderer_GetNormals(pCalRenderer,&meshNormals[0][0]);
			}

			// get the texture coordinates of the submesh
			if (use_textures)
			{
				CalRenderer_GetTextureCoordinates(pCalRenderer,0,&meshTextureCoordinates[0][0]);
			}

			// get the faces of the submesh
			faceCount = CalRenderer_GetFaces(pCalRenderer, &meshFaces[0][0]);

			// set the vertex and normal buffers
			glVertexPointer(3, GL_FLOAT, 0, &meshVertices[0][0]);
			if (use_lightning)
			{
				glEnableClientState(GL_NORMAL_ARRAY);
				glNormalPointer(GL_FLOAT, 0, &meshNormals[0][0]);
			}
			else
			{
				glDisableClientState(GL_NORMAL_ARRAY);
			}

			// draw the submesh
			if (use_textures)
			{
				glEnableClientState(GL_TEXTURE_COORD_ARRAY);
				glTexCoordPointer(2, GL_FLOAT, 0, &meshTextureCoordinates[0][0]);
			}
			else
			{
				glDisableClientState(GL_TEXTURE_COORD_ARRAY);
			}

			if(sizeof(CalIndex)==2)
				glDrawElements(GL_TRIANGLES, faceCount * 3, GL_UNSIGNED_SHORT, &meshFaces[0][0]);
			else
				glDrawElements(GL_TRIANGLES, faceCount * 3, GL_UNSIGNED_INT, &meshFaces[0][0]);
		}
	}
}


void cal_render_actor(actor *act, Uint32 use_lightning, Uint32 use_textures, Uint32 use_glow)
{
	struct CalRenderer *pCalRenderer;
	int meshCount,meshId,submeshCount/*,submeshId, vertexCount*/;
	float points[1024][3];
	static float meshVertices[30000][3];
	static float meshNormals[30000][3];
	static float meshTextureCoordinates[30000][2];
	static CalIndex meshFaces[50000][3];
	struct CalSkeleton *skel;
	struct CalMesh *_mesh;
	struct CalCoreMesh *_coremesh;
	struct CalCoreMesh *_weaponmesh;
	struct CalCoreMesh *_shieldmesh;
	//int boneid=-1;
	float reverse_scale;
	//int glow=-1;

	if(act->calmodel==NULL) {
		return;//Wtf!?
	}
	skel=CalModel_GetSkeleton(act->calmodel);

	glPushMatrix();
	// actor model rescaling
	if(actors_defs[act->actor_type].actor_scale != 1.0){
		glScalef(actors_defs[act->actor_type].actor_scale, actors_defs[act->actor_type].actor_scale, actors_defs[act->actor_type].actor_scale);
	}
	// the dynamic scaling
	if(act->scale != 1.0f){
		glScalef(act->scale,act->scale,act->scale);
	}


	// get the renderer of the model
		pCalRenderer = CalModel_GetRenderer(act->calmodel);
		// begin the rendering loop
		if(CalRenderer_BeginRendering(pCalRenderer)){
			// set global OpenGL states
			if(!act->ghost && act->has_alpha){
				glEnable(GL_ALPHA_TEST);
				glAlphaFunc(GL_GREATER,0.06f);
				glEnable(GL_BLEND);
				glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
				//glDisable(GL_CULL_FACE);
			}

			// will use vertex arrays, so enable them
			glEnableClientState(GL_VERTEX_ARRAY);

			// get the number of meshes
			meshCount = CalRenderer_GetMeshCount(pCalRenderer);

			// check for weapons or shields being worn
			if (act->is_enhanced_model) {
				if(actors_defs[act->actor_type].weapon[act->cur_weapon].mesh_index!=-1) _weaponmesh=CalCoreModel_GetCoreMesh(actors_defs[act->actor_type].coremodel,actors_defs[act->actor_type].weapon[act->cur_weapon].mesh_index);
				else _weaponmesh=NULL;
				if(act->body_parts->shield_meshindex!=-1) _shieldmesh=CalCoreModel_GetCoreMesh(actors_defs[act->actor_type].coremodel,act->body_parts->shield_meshindex);
				else _shieldmesh=NULL;
			} else {
				// non-enhanced never have weapon or shields
				_weaponmesh=NULL;
				_shieldmesh=NULL;
			}

			// render all meshes of the model
			for(meshId = 0; meshId < meshCount; meshId++){
				// get the number of submeshes
   				submeshCount = CalRenderer_GetSubmeshCount(pCalRenderer,meshId);

				_mesh=CalModel_GetAttachedMesh(act->calmodel,meshId);//Get current rendered mesh
				_coremesh=CalMesh_GetCoreMesh(_mesh);//Get the coremesh

				if(act->is_enhanced_model && (_weaponmesh || _shieldmesh)) {
					//Special treatment for weapons and shields only for enhanced models
					int glow=-1;
					int boneid=-1;

					if (_coremesh==_weaponmesh) boneid=26;//If it's a weapon snap to WeaponR bone
					else if (_coremesh==_shieldmesh) boneid=21;//If it's a shield snap to WeaponL bone
					if (boneid!=-1) {
						glPushMatrix();
						reverse_scale= 1.0/actors_defs[act->actor_type].skel_scale;
						CalSkeleton_GetBonePoints(skel,&points[0][0]);

						glTranslatef(points[boneid][0],points[boneid][1],points[boneid][2]);
						glScalef(reverse_scale,reverse_scale,reverse_scale);
						glTranslatef(-points[boneid][0],-points[boneid][1],-points[boneid][2]);

						// find the proper place to bind this object to
						switch(boneid){
							case 26:
								if(actors_defs[act->actor_type].weapon[act->cur_weapon].glow>0){
									glow=actors_defs[act->actor_type].weapon[act->cur_weapon].glow;
								}
								break;
							default:
								break;
						}
					}

					// now check for a glowing weapon
					if(glow>0 && use_glow){
						glEnable(GL_COLOR_MATERIAL);
						glBlendFunc(GL_ONE,GL_SRC_ALPHA);
						if(!act->ghost && !(act->buffs & BUFF_INVISIBILITY)) {
							glEnable(GL_BLEND);
							glDisable(GL_LIGHTING);
						}

						if(use_shadow_mapping){
							glPushAttrib(GL_TEXTURE_BIT|GL_ENABLE_BIT);
							ELglActiveTextureARB(shadow_unit);
							glDisable(depth_texture_target);
							disable_texgen();
							ELglActiveTextureARB(GL_TEXTURE0);
						}

						glColor4f(glow_colors[glow].r, glow_colors[glow].g, glow_colors[glow].b, 0.5f);
						glPushMatrix();
						glScalef(0.99f, 0.99f, 0.99f);
						render_submesh(meshId, submeshCount, pCalRenderer, meshVertices, meshNormals, meshTextureCoordinates, meshFaces, 0, use_textures);
						glPopMatrix();

						glColor4f(glow_colors[glow].r, glow_colors[glow].g, glow_colors[glow].b, 0.85f);
						render_submesh(meshId, submeshCount, pCalRenderer, meshVertices, meshNormals, meshTextureCoordinates, meshFaces, 0, use_textures);
						glColor4f(glow_colors[glow].r, glow_colors[glow].g, glow_colors[glow].b, 0.99f);
						glPushMatrix();
						glScalef(1.01f, 1.01f, 1.01f);
						render_submesh(meshId, submeshCount, pCalRenderer, meshVertices, meshNormals, meshTextureCoordinates, meshFaces, 0, use_textures);
						glPopMatrix();

						if(use_shadow_mapping){
							glPopAttrib();
						}
						glColor3f(1.0f, 1.0f, 1.0f);
						glDisable(GL_COLOR_MATERIAL);
						if(!act->ghost && !(act->buffs & BUFF_INVISIBILITY)) {
							glDisable(GL_BLEND);
							glEnable(GL_LIGHTING);
						} else {
							glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
							if((act->buffs & BUFF_INVISIBILITY)) {
								glColor4f(1.0f, 1.0f, 1.0f, 0.25f);
							}
						}
					} else {
						// enhanced actors without glowing items
						render_submesh(meshId, submeshCount, pCalRenderer, meshVertices, meshNormals, meshTextureCoordinates, meshFaces, use_lightning, use_textures);
					}
					if(boneid >= 0){
						//if this was a weapon or shield, restore the transformation matrix
						glPopMatrix();
					}
				} else {
					// non-enhanced actors, or enhanced without attached meshes
					render_submesh(meshId, submeshCount, pCalRenderer, meshVertices, meshNormals, meshTextureCoordinates, meshFaces, use_lightning, use_textures);
				}
			}

			// clear vertex array state
			glDisableClientState(GL_NORMAL_ARRAY);
			glDisableClientState(GL_VERTEX_ARRAY);
			glDisableClientState(GL_TEXTURE_COORD_ARRAY);
			if(!act->ghost && act->has_alpha){
				glDisable(GL_ALPHA_TEST);
				//glEnable(GL_CULL_FACE);
				glDisable(GL_BLEND);
			}

			// end the rendering
			CalRenderer_EndRendering(pCalRenderer);
		}

	glColor3f(1,1,1);

	glPopMatrix();
}

void cal_get_actor_bone_local_position(actor *in_act, int in_bone_id, float *in_shift, float *out_pos)
{
	struct CalSkeleton *skel;
	struct CalBone *bone;
	struct CalVector *point;

    if (in_bone_id < 0) return;

	skel = CalModel_GetSkeleton(in_act->calmodel);

    if (in_bone_id >= CalSkeleton_GetBonesNumber(skel)) return;

	bone = CalSkeleton_GetBone(skel, in_bone_id);
	point = CalBone_GetTranslationAbsolute(bone);

	memcpy(out_pos, CalVector_Get(point), 3*sizeof(float));

	if (in_shift) {
		struct CalQuaternion *rot;
		struct CalVector *vect;
		float *tmp;
		rot = CalBone_GetRotationAbsolute(bone);
		vect = CalVector_New();
		CalVector_Set(vect, in_shift[0], in_shift[1], in_shift[2]);
		CalVector_Transform(vect, rot);
		tmp = CalVector_Get(vect);
		out_pos[0] += tmp[0];
		out_pos[1] += tmp[1];
		out_pos[2] += tmp[2];
		CalVector_Delete(vect);
	}
}

void cal_get_actor_bone_absolute_position(actor *in_act, int in_bone_id, float *in_shift, float *out_pos)
{
	float act_rot[9];
	float pos[3];
	get_actor_rotation_matrix(in_act, act_rot);
	cal_get_actor_bone_local_position(in_act, in_bone_id, in_shift, pos);
	transform_actor_local_position_to_absolute(in_act, pos, act_rot, out_pos);
}
