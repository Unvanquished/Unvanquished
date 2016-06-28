/*
===========================================================================

Daemon GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.

This file is part of the Daemon GPL Source Code (Daemon Source Code).

Daemon Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Daemon Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Daemon Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Daemon Source Code is also subject to certain additional terms.
You should have received a copy of these additional terms immediately following the
terms and conditions of the GNU General Public License which accompanied the Daemon
Source Code.  If not, please request a copy in writing from id Software at the address
below.

If you have questions concerning this license or the applicable additional terms, you
may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville,
Maryland 20850 USA.

===========================================================================
*/

#include "tr_local.h"

#define	LL(x) x=LittleLong(x)

static INLINE void *IQMPtr( iqmHeader_t *header, int offset ) {
	return ( (byte *)header ) + offset;
}

static bool IQM_CheckRange( iqmHeader_t *header, int offset,
				int count, int size, const char *mod_name,
				const char *section ) {
	int section_end;

	// return true if the range specified by offset, count and size
	// doesn't fit into the file
	if( count <= 0 ) {
		Log::Warn("R_LoadIQM: %s has a negative count in the %s section (%d).",
			  mod_name, section, count );
		return true;
	}
	if( offset < 0 ) {
		Log::Warn("R_LoadIQM: %s has a negative offset to the %s section (%d).",
			  mod_name, section, offset );
		return true;
	}
	if( offset > (int) header->filesize ) {
		Log::Warn("R_LoadIQM: %s has an offset behind the end of file to the %s section (%d).",
			  mod_name, section, offset );
		return true;
	}

	section_end = offset + count * size;
	if( section_end > (int) header->filesize || section_end < 0 ) {
		Log::Warn("R_LoadIQM: %s has the section %s exceeding the end of file (%d).",
			  mod_name, section, section_end );
		return true;
	}
	return false;
}

static bool LoadIQMFile( void *buffer, unsigned filesize, const char *mod_name,
			     size_t *len_names )
{
	iqmHeader_t		*header;
	iqmVertexArray_t	*vertexarray;
	iqmTriangle_t		*triangle;
	iqmMesh_t		*mesh;
	iqmJoint_t		*joint;
	iqmPose_t		*pose;
	iqmBounds_t		*bounds;
	iqmAnim_t		*anim;

	if( filesize < sizeof(iqmHeader_t) ) {
		Log::Warn("R_LoadIQModel: file size of %s is too small.",
			  mod_name );
		return false;
	}

	header = (iqmHeader_t *)buffer;
	if( Q_strncmp( header->magic, IQM_MAGIC, sizeof(header->magic) ) ) {
		Log::Warn("R_LoadIQModel: file %s doesn't contain an IQM header.",
			  mod_name );
		return false;
	}

	LL( header->version );
	if( header->version != IQM_VERSION ) {
		Log::Warn("R_LoadIQM: %s is a unsupported IQM version (%d), only version %d is supported.",
			  mod_name, header->version, IQM_VERSION);
		return false;
	}

	LL( header->filesize );
	if( header->filesize > filesize || header->filesize > 1<<24 ) {
		Log::Warn("R_LoadIQM: %s has an invalid file size %d.",
			  mod_name, header->filesize );
		return false;
	}

	LL( header->flags );
	LL( header->num_text );
	LL( header->ofs_text );
	LL( header->num_meshes );
	LL( header->ofs_meshes );
	LL( header->num_vertexarrays );
	LL( header->num_vertexes );
	LL( header->ofs_vertexarrays );
	LL( header->num_triangles );
	LL( header->ofs_triangles );
	LL( header->ofs_adjacency );
	LL( header->num_joints );
	LL( header->ofs_joints );
	LL( header->num_poses );
	LL( header->ofs_poses );
	LL( header->num_anims );
	LL( header->ofs_anims );
	LL( header->num_frames );
	LL( header->num_framechannels );
	LL( header->ofs_frames );
	LL( header->ofs_bounds );
	LL( header->num_comment );
	LL( header->ofs_comment );
	LL( header->num_extensions );
	LL( header->ofs_extensions );

	// check and swap vertex arrays
	if( IQM_CheckRange( header, header->ofs_vertexarrays,
			    header->num_vertexarrays,
			    sizeof(iqmVertexArray_t),
			    mod_name, "vertexarray" ) ) {
		return false;
	}
	vertexarray = ( iqmVertexArray_t* )IQMPtr( header, header->ofs_vertexarrays );
	for(unsigned i = 0; i < header->num_vertexarrays; i++, vertexarray++ ) {
		int	j, n, *intPtr;

		if( vertexarray->size <= 0 || vertexarray->size > 4 ) {
			Log::Warn("R_LoadIQM: %s contains an invalid vertexarray size.",
				  mod_name );
			return false;
		}

		// total number of values
		n = header->num_vertexes * vertexarray->size;

		switch( vertexarray->format ) {
		case IQM_BYTE:
		case IQM_UBYTE:
			// 1 byte, no swapping necessary
			if( IQM_CheckRange( header, vertexarray->offset, n,
					    sizeof(byte), mod_name, "vertexarray" ) ) {
				return false;
			}
			break;
		case IQM_INT:
		case IQM_UINT:
		case IQM_FLOAT:
			// 4-byte swap
			if( IQM_CheckRange( header, vertexarray->offset, n,
					    sizeof(float), mod_name, "vertexarray" ) ) {
				return false;
			}
			intPtr = ( int* )IQMPtr( header, vertexarray->offset );
			for( j = 0; j < n; j++, intPtr++ ) {
				LL( *intPtr );
			}
			break;
		default:
			// not supported
			Log::Warn("R_LoadIQM: file %s uses an unsupported vertex format.",
				  mod_name );
			return false;
			break;
		}

		switch( vertexarray->type ) {
		case IQM_POSITION:
		case IQM_NORMAL:
			if( vertexarray->format != IQM_FLOAT ||
			    vertexarray->size != 3 ) {
				Log::Warn("R_LoadIQM: file %s uses an unsupported vertex format.",
					  mod_name );
				return false;
			}
			break;
		case IQM_TANGENT:
			if( vertexarray->format != IQM_FLOAT ||
			    vertexarray->size != 4 ) {
				Log::Warn("R_LoadIQM: file %s uses an unsupported vertex format.",
					  mod_name );
				return false;
			}
			break;
		case IQM_TEXCOORD:
			if( vertexarray->format != IQM_FLOAT ||
			    vertexarray->size != 2 ) {
				Log::Warn("R_LoadIQM: file %s uses an unsupported vertex format.",
					  mod_name );
				return false;
			}
			break;
		case IQM_BLENDINDEXES:
		case IQM_BLENDWEIGHTS:
			if( vertexarray->format != IQM_UBYTE ||
			    vertexarray->size != 4 ) {
				Log::Warn("R_LoadIQM: file %s uses an unsupported vertex format.",
					  mod_name );
				return false;
			}
			break;
		case IQM_COLOR:
			if( vertexarray->format != IQM_UBYTE ||
			    vertexarray->size != 4 ) {
				Log::Warn("R_LoadIQM: file %s uses an unsupported vertex format.",
					  mod_name );
				return false;
			}
			break;
		}
	}

	// check and swap triangles
	if( IQM_CheckRange( header, header->ofs_triangles,
			    header->num_triangles, sizeof(iqmTriangle_t),
			    mod_name, "triangle" ) ) {
		return false;
	}
	triangle = ( iqmTriangle_t* )IQMPtr( header, header->ofs_triangles );
	for(unsigned i = 0; i < header->num_triangles; i++, triangle++ ) {
		LL( triangle->vertex[0] );
		LL( triangle->vertex[1] );
		LL( triangle->vertex[2] );

		if( triangle->vertex[0] > header->num_vertexes ||
		    triangle->vertex[1] > header->num_vertexes ||
		    triangle->vertex[2] > header->num_vertexes ) {
			return false;
		}
	}

	*len_names = 0;

	// check and swap meshes
	if( IQM_CheckRange( header, header->ofs_meshes,
			    header->num_meshes, sizeof(iqmMesh_t),
			    mod_name, "mesh" ) ) {
		return false;
	}
	mesh = ( iqmMesh_t* )IQMPtr( header, header->ofs_meshes );
	for(unsigned i = 0; i < header->num_meshes; i++, mesh++) {
		LL( mesh->name );
		LL( mesh->material );
		LL( mesh->first_vertex );
		LL( mesh->num_vertexes );
		LL( mesh->first_triangle );
		LL( mesh->num_triangles );

		if( mesh->first_vertex >= header->num_vertexes ||
		    mesh->first_vertex + mesh->num_vertexes > header->num_vertexes ||
		    mesh->first_triangle >= header->num_triangles ||
		    mesh->first_triangle + mesh->num_triangles > header->num_triangles ||
		    mesh->name >= header->num_text ||
		    mesh->material >= header->num_text ) {
			Log::Warn("R_LoadIQM: file %s contains an invalid mesh.",
				  mod_name );
			return false;
		}
		*len_names += strlen( ( char* )IQMPtr( header, header->ofs_text
					      + mesh->name ) ) + 1;
	}

	// check and swap joints
	if( header->num_joints != 0 &&
	    IQM_CheckRange( header, header->ofs_joints,
			    header->num_joints, sizeof(iqmJoint_t),
			    mod_name, "joint" ) ) {
		return false;
	}
	joint = ( iqmJoint_t* )IQMPtr( header, header->ofs_joints );
	for(unsigned i = 0; i < header->num_joints; i++, joint++ ) {
		LL( joint->name );
		LL( joint->parent );
		LL( joint->translate[0] );
		LL( joint->translate[1] );
		LL( joint->translate[2] );
		LL( joint->rotate[0] );
		LL( joint->rotate[1] );
		LL( joint->rotate[2] );
		LL( joint->rotate[3] );
		LL( joint->scale[0] );
		LL( joint->scale[1] );
		LL( joint->scale[2] );

		if( joint->parent < -1 ||
		    joint->parent >= (int)header->num_joints ||
		    joint->name >= header->num_text ) {
			Log::Warn("R_LoadIQM: file %s contains an invalid joint.",
				  mod_name );
			return false;
		}
		if( joint->scale[0] < 0.0f ||
			(int)( joint->scale[0] - joint->scale[1] ) ||
			(int)( joint->scale[1] - joint->scale[2] ) ) {
			Log::Warn("R_LoadIQM: file %s contains an invalid scale: %f %f %f",
				  mod_name, joint->scale[0], joint->scale[1], joint->scale[2] );
			return false;
		}
		*len_names += strlen( ( char* )IQMPtr( header, header->ofs_text
					      + joint->name ) ) + 1;
	}

	// check and swap poses
	if( header->ofs_poses > 0 ) {
		if( IQM_CheckRange( header, header->ofs_poses,
				    header->num_poses, sizeof(iqmPose_t),
				    mod_name, "pose" ) ) {
			return false;
		}
		pose = ( iqmPose_t* )IQMPtr( header, header->ofs_poses );
		for(unsigned i = 0; i < header->num_poses; i++, pose++ ) {
			LL( pose->parent );
			LL( pose->mask );
			LL( pose->channeloffset[0] );
			LL( pose->channeloffset[1] );
			LL( pose->channeloffset[2] );
			LL( pose->channeloffset[3] );
			LL( pose->channeloffset[4] );
			LL( pose->channeloffset[5] );
			LL( pose->channeloffset[6] );
			LL( pose->channeloffset[7] );
			LL( pose->channeloffset[8] );
			LL( pose->channeloffset[9] );
			LL( pose->channelscale[0] );
			LL( pose->channelscale[1] );
			LL( pose->channelscale[2] );
			LL( pose->channelscale[3] );
			LL( pose->channelscale[4] );
			LL( pose->channelscale[5] );
			LL( pose->channelscale[6] );
			LL( pose->channelscale[7] );
			LL( pose->channelscale[8] );
			LL( pose->channelscale[9] );
		}
	}

	if( header->ofs_bounds )
	{
		// check and swap model bounds
		if(IQM_CheckRange(header, header->ofs_bounds,
				  header->num_frames, sizeof(*bounds),
				  mod_name, "bounds" ))
		{
			return false;
		}
		bounds = ( iqmBounds_t* )IQMPtr( header, header->ofs_bounds );
		for(unsigned i = 0; i < header->num_poses; i++)
		{
			LL(bounds->bbmin[0]);
			LL(bounds->bbmin[1]);
			LL(bounds->bbmin[2]);
			LL(bounds->bbmax[0]);
			LL(bounds->bbmax[1]);
			LL(bounds->bbmax[2]);

			bounds++;
		}
	}

	// check and swap animations
	if( header->ofs_anims ) {
		if( IQM_CheckRange( header, header->ofs_anims,
				    header->num_anims, sizeof(iqmAnim_t),
				    mod_name, "animation" ) ) {
			return false;
		}
		anim = ( iqmAnim_t* )IQMPtr( header, header->ofs_anims );
		for(unsigned i = 0; i < header->num_anims; i++, anim++ ) {
			LL( anim->name );
			LL( anim->first_frame );
			LL( anim->num_frames );
			LL( anim->framerate );
			LL( anim->flags );

			*len_names += strlen( ( char* )IQMPtr( header, header->ofs_text
							       + anim->name ) ) + 1;
		}
	}

	return true;
}

/*
=================
BuildTangents

Compute Tangent and Bitangent vectors from the IQM 4-float input data
=================
*/
void BuildTangents( int n, float *input, float *normals, float *tangents,
		    float *bitangents )
{
	int i;
	vec3_t crossProd;

	for( i = 0; i < n; i++ ) {
		VectorCopy( input, tangents );
		CrossProduct( normals, input, crossProd );
		VectorScale( crossProd, input[ 3 ], bitangents );

		input      += 4;
		normals    += 3;
		tangents   += 3;
		bitangents += 3;
	}
}

/*
=================
R_LoadIQModel

Load an IQM model and compute the joint matrices for every frame.
=================
*/
bool R_LoadIQModel( model_t *mod, void *buffer, int filesize,
			const char *mod_name ) {
	iqmHeader_t		*header;
	iqmVertexArray_t	*vertexarray;
	iqmTriangle_t		*triangle;
	iqmMesh_t		*mesh;
	iqmJoint_t		*joint;
	iqmPose_t		*pose;
	iqmAnim_t		*anim;
	unsigned short		*framedata;
	char			*str, *name;
	int		len;
	transform_t		*trans, *poses;
	float			*bounds;
	size_t			size, len_names;
	IQModel_t		*IQModel;
	IQAnim_t		*IQAnim;
	srfIQModel_t		*surface;
	vboData_t               vboData;
	float                   *weightbuf;
	int                     *indexbuf;
	i16vec4_t               *qtangentbuf;
	VBO_t                   *vbo;
	IBO_t                   *ibo;
	void                    *ptr;
	u8vec4_t                *weights;

	if( !LoadIQMFile( buffer, filesize, mod_name, &len_names ) ) {
		return false;
	}

	header = (iqmHeader_t *)buffer;

	// compute required space
	size = sizeof(IQModel_t);
	size += header->num_meshes * sizeof( srfIQModel_t );
	size += header->num_anims * sizeof( IQAnim_t );
	size += header->num_joints * sizeof( transform_t );
	size = PAD( size, 16 );
	size += header->num_joints * header->num_frames * sizeof( transform_t );
	if(header->ofs_bounds)
		size += header->num_frames * 6 * sizeof(float);	// model bounds
	size += header->num_vertexes * 3 * sizeof(float);	// positions
	size += header->num_vertexes * 3 * sizeof(float);	// normals
	size += header->num_vertexes * 3 * sizeof(float);	// tangents
	size += header->num_vertexes * 3 * sizeof(float);	// bitangents
	size += header->num_vertexes * 2 * sizeof(int16_t);	// texcoords
	size += header->num_vertexes * 4 * sizeof(byte);	// blendIndexes
	size += header->num_vertexes * 4 * sizeof(byte);	// blendWeights
	size += header->num_vertexes * 4 * sizeof(byte);	// colors
	size += header->num_triangles * 3 * sizeof(int);	// triangles
	size += header->num_joints * sizeof(int);		// parents
	size += len_names;					// joint and anim names

	IQModel = (IQModel_t *)ri.Hunk_Alloc( size, ha_pref::h_low );
	mod->type = modtype_t::MOD_IQM;
	mod->iqm = IQModel;
	ptr = IQModel + 1;

	// fill header and setup pointers
	IQModel->num_vertexes = header->num_vertexes;
	IQModel->num_triangles = header->num_triangles;
	IQModel->num_frames   = header->num_frames;
	IQModel->num_surfaces = header->num_meshes;
	IQModel->num_joints   = header->num_joints;
	IQModel->num_anims    = header->num_anims;

	IQModel->surfaces = (srfIQModel_t *)ptr;
	ptr = IQModel->surfaces + header->num_meshes;

	if( header->ofs_anims ) {
		IQModel->anims = (IQAnim_t *)ptr;
		ptr = IQModel->anims + header->num_anims;
	} else {
		IQModel->anims = nullptr;
	}

	IQModel->joints = (transform_t *)PADP(ptr, 16);
	ptr = IQModel->joints + header->num_joints;

	if( header->ofs_poses ) {
		poses = (transform_t *)ptr;
		ptr = poses + header->num_poses * header->num_frames;
	} else {
		poses = nullptr;
	}

	if( header->ofs_bounds ) {
		bounds = (float *)ptr;
		ptr = bounds + 6 * header->num_frames;
	} else {
		bounds = nullptr;
	}

	IQModel->positions = (float *)ptr;
	ptr = IQModel->positions + 3 * header->num_vertexes;

	IQModel->normals = (float *)ptr;
	ptr = IQModel->normals + 3 * header->num_vertexes;

	IQModel->tangents = (float *)ptr;
	ptr = IQModel->tangents + 3 * header->num_vertexes;

	IQModel->bitangents = (float *)ptr;
	ptr = IQModel->bitangents + 3 * header->num_vertexes;

	IQModel->texcoords = (int16_t *)ptr;
	ptr = IQModel->texcoords + 2 * header->num_vertexes;

	IQModel->blendIndexes = (byte *)ptr;
	ptr = IQModel->blendIndexes + 4 * header->num_vertexes;

	IQModel->blendWeights = (byte *)ptr;
	ptr = IQModel->blendWeights + 4 * header->num_vertexes;

	IQModel->colors = (byte *)ptr;
	ptr = IQModel->colors + 4 * header->num_vertexes;

	IQModel->jointParents = (int *)ptr;
	ptr = IQModel->jointParents + header->num_joints;

	IQModel->triangles = (int *)ptr;
	ptr = IQModel->triangles + 3 * header->num_triangles;

	str                   = (char *)ptr;
	IQModel->jointNames   = str;

	// copy joint names
	joint = ( iqmJoint_t* )IQMPtr( header, header->ofs_joints );
	for(unsigned i = 0; i < header->num_joints; i++, joint++ ) {
		name = ( char* )IQMPtr( header, header->ofs_text + joint->name );
		len = strlen( name ) + 1;
		Com_Memcpy( str, name, len );
		str += len;
	}

	// setup animations
	IQAnim = IQModel->anims;
	anim = ( iqmAnim_t* )IQMPtr( header, header->ofs_anims );
	for(int i = 0; i < IQModel->num_anims; i++, IQAnim++, anim++ ) {
		IQAnim->num_frames   = anim->num_frames;
		IQAnim->framerate    = anim->framerate;
		IQAnim->num_joints   = header->num_joints;
		IQAnim->flags        = anim->flags;
		IQAnim->jointParents = IQModel->jointParents;
		if( poses ) {
			IQAnim->poses    = poses + anim->first_frame * header->num_poses;
		} else {
			IQAnim->poses    = nullptr;
		}
		if( bounds ) {
			IQAnim->bounds   = bounds + anim->first_frame * 6;
		} else {
			IQAnim->bounds    = nullptr;
		}
		IQAnim->name         = str;
		IQAnim->jointNames   = IQModel->jointNames;

		name = ( char* )IQMPtr( header, header->ofs_text + anim->name );
		len = strlen( name ) + 1;
		Com_Memcpy( str, name, len );
		str += len;
	}

	// calculate joint transforms
	trans = IQModel->joints;
	joint = ( iqmJoint_t* )IQMPtr( header, header->ofs_joints );
	for(unsigned i = 0; i < header->num_joints; i++, joint++, trans++ ) {
		if( joint->parent >= (int) i ) {
			Log::Warn("R_LoadIQModel: file %s contains an invalid parent joint number.",
				  mod_name );
			return false;
		}

		TransInitRotationQuat( joint->rotate, trans );
		TransAddScale( joint->scale[0], trans );
		TransAddTranslation( joint->translate, trans );

		if( joint->parent >= 0 ) {
			TransCombine( trans, &IQModel->joints[ joint->parent ],
				      trans );
		}

		IQModel->jointParents[i] = joint->parent;
	}

	// calculate pose transforms
	framedata = ( short unsigned int* )IQMPtr( header, header->ofs_frames );
	trans = poses;
	for(unsigned i = 0; i < header->num_frames; i++ ) {
		pose = ( iqmPose_t* )IQMPtr( header, header->ofs_poses );
		for(unsigned j = 0; j < header->num_poses; j++, pose++, trans++ ) {
			vec3_t	translate;
			quat_t	rotate;
			vec3_t	scale;

			translate[0] = pose->channeloffset[0];
			if( pose->mask & 0x001)
				translate[0] += *framedata++ * pose->channelscale[0];
			translate[1] = pose->channeloffset[1];
			if( pose->mask & 0x002)
				translate[1] += *framedata++ * pose->channelscale[1];
			translate[2] = pose->channeloffset[2];
			if( pose->mask & 0x004)
				translate[2] += *framedata++ * pose->channelscale[2];
			rotate[0] = pose->channeloffset[3];
			if( pose->mask & 0x008)
				rotate[0] += *framedata++ * pose->channelscale[3];
			rotate[1] = pose->channeloffset[4];
			if( pose->mask & 0x010)
				rotate[1] += *framedata++ * pose->channelscale[4];
			rotate[2] = pose->channeloffset[5];
			if( pose->mask & 0x020)
				rotate[2] += *framedata++ * pose->channelscale[5];
			rotate[3] = pose->channeloffset[6];
			if( pose->mask & 0x040)
				rotate[3] += *framedata++ * pose->channelscale[6];
			scale[0] = pose->channeloffset[7];
			if( pose->mask & 0x080)
				scale[0] += *framedata++ * pose->channelscale[7];
			scale[1] = pose->channeloffset[8];
			if( pose->mask & 0x100)
				scale[1] += *framedata++ * pose->channelscale[8];
			scale[2] = pose->channeloffset[9];
			if( pose->mask & 0x200)
				scale[2] += *framedata++ * pose->channelscale[9];

			if( scale[0] < 0.0f ||
			    (int)( scale[0] - scale[1] ) ||
			    (int)( scale[1] - scale[2] ) ) {
				Log::Warn("R_LoadIQM: file %s contains an invalid scale.", mod_name );
				return false;
			    }

			// construct transformation
			TransInitRotationQuat( rotate, trans );
			TransAddScale( scale[0], trans );
			TransAddTranslation( translate, trans );
		}
	}

	// copy vertexarrays and indexes
	vertexarray = ( iqmVertexArray_t* )IQMPtr( header, header->ofs_vertexarrays );
	for(unsigned i = 0; i < header->num_vertexarrays; i++, vertexarray++ ) {
		int	n;

		// total number of values
		n = header->num_vertexes * vertexarray->size;

		switch( vertexarray->type ) {
		case IQM_POSITION:
			ClearBounds( IQModel->bounds[ 0 ], IQModel->bounds[ 1 ] );
			Com_Memcpy( IQModel->positions,
				    IQMPtr( header, vertexarray->offset ),
				    n * sizeof(float) );
			for( int j = 0; j < n; j += vertexarray->size ) {
				AddPointToBounds( &IQModel->positions[ j ],
						  IQModel->bounds[ 0 ],
						  IQModel->bounds[ 1 ] );
			}
			IQModel->internalScale = BoundsMaxExtent( IQModel->bounds[ 0 ], IQModel->bounds[ 1 ] );
			if( IQModel->internalScale > 0.0f ) {
				float inverseScale = 1.0f / IQModel->internalScale;
				for( int j = 0; j < n; j += vertexarray->size ) {
					VectorScale( &IQModel->positions[ j ],
						     inverseScale,
						     &IQModel->positions[ j ] );
				}
			}

			break;
		case IQM_NORMAL:
			Com_Memcpy( IQModel->normals,
				    IQMPtr( header, vertexarray->offset ),
				    n * sizeof(float) );
			break;
		case IQM_TANGENT:
			BuildTangents( header->num_vertexes,
				       ( float* )IQMPtr( header, vertexarray->offset ),
				       IQModel->normals, IQModel->tangents,
				       IQModel->bitangents );
			break;
		case IQM_TEXCOORD:
			for( int j = 0; j < n; j++ ) {
				IQModel->texcoords[ j ] = floatToHalf( ((float *)IQMPtr( header, vertexarray->offset ))[ j ] );
			}
			break;
		case IQM_BLENDINDEXES:
			Com_Memcpy( IQModel->blendIndexes,
				    IQMPtr( header, vertexarray->offset ),
				    n * sizeof(byte) );
			break;
		case IQM_BLENDWEIGHTS:
			weights = (u8vec4_t *)IQMPtr( header, vertexarray->offset );
			for(unsigned j = 0; j < header->num_vertexes; j++ ) {
				IQModel->blendWeights[ 4 * j + 0 ] = 255 - weights[ j ][ 1 ] - weights[ j ][ 2 ] - weights[ j ][ 3 ];
				IQModel->blendWeights[ 4 * j + 1 ] = weights[ j ][ 1 ];
				IQModel->blendWeights[ 4 * j + 2 ] = weights[ j ][ 2 ];
				IQModel->blendWeights[ 4 * j + 3 ] = weights[ j ][ 3 ];
			}
			break;
		case IQM_COLOR:
			Com_Memcpy( IQModel->colors,
				    IQMPtr( header, vertexarray->offset ),
				    n * sizeof(byte) );
			break;
		}
	}

	// copy triangles
	triangle = ( iqmTriangle_t* )IQMPtr( header, header->ofs_triangles );
	for(unsigned i = 0; i < header->num_triangles; i++, triangle++ ) {
		IQModel->triangles[3*i+0] = triangle->vertex[0];
		IQModel->triangles[3*i+1] = triangle->vertex[1];
		IQModel->triangles[3*i+2] = triangle->vertex[2];
	}

	// convert data where necessary and create VBO
	if( r_vboModels->integer && glConfig2.vboVertexSkinningAvailable
	    && IQModel->num_joints <= glConfig2.maxVertexSkinningBones ) {

		if( IQModel->blendIndexes ) {
			indexbuf = (int *)ri.Hunk_AllocateTempMemory( sizeof(int[4]) * IQModel->num_vertexes );
			for(int i = 0; i < IQModel->num_vertexes; i++ ) {
				indexbuf[ 4 * i + 0 ] = IQModel->blendIndexes[ 4 * i + 0 ];
				indexbuf[ 4 * i + 1 ] = IQModel->blendIndexes[ 4 * i + 1 ];
				indexbuf[ 4 * i + 2 ] = IQModel->blendIndexes[ 4 * i + 2 ];
				indexbuf[ 4 * i + 3 ] = IQModel->blendIndexes[ 4 * i + 3 ];
			}
		} else {
			indexbuf = nullptr;
		}
		if( IQModel->blendWeights ) {
			const float weightscale = 1.0f / 255.0f;

			weightbuf = (float *)ri.Hunk_AllocateTempMemory( sizeof(vec4_t) * IQModel->num_vertexes );
			for(int i = 0; i < IQModel->num_vertexes; i++ ) {
				if( IQModel->blendWeights[ 4 * i + 0 ] == 0 &&
				    IQModel->blendWeights[ 4 * i + 1 ] == 0 &&
				    IQModel->blendWeights[ 4 * i + 2 ] == 0 &&
				    IQModel->blendWeights[ 4 * i + 3 ] == 0 )
					IQModel->blendWeights[ 4 * i + 0 ] = 255;

				weightbuf[ 4 * i + 0 ] = weightscale * IQModel->blendWeights[ 4 * i + 0 ];
				weightbuf[ 4 * i + 1 ] = weightscale * IQModel->blendWeights[ 4 * i + 1 ];
				weightbuf[ 4 * i + 2 ] = weightscale * IQModel->blendWeights[ 4 * i + 2 ];
				weightbuf[ 4 * i + 3 ] = weightscale * IQModel->blendWeights[ 4 * i + 3 ];
			}
		} else {
			weightbuf = nullptr;
		}

		qtangentbuf = (i16vec4_t *)ri.Hunk_AllocateTempMemory( sizeof( i16vec4_t ) * IQModel->num_vertexes );

		for(int i = 0; i < IQModel->num_vertexes; i++ ) {
			R_TBNtoQtangents( &IQModel->tangents[ 3 * i ],
					  &IQModel->bitangents[ 3 * i ],
					  &IQModel->normals[ 3 * i ],
					  qtangentbuf[ i ] );
		}

		vboData.xyz = (vec3_t *)IQModel->positions;
		vboData.qtangent = qtangentbuf;
		vboData.numFrames = 0;
		vboData.color = (u8vec4_t *)IQModel->colors;
		vboData.st = (i16vec2_t *)IQModel->texcoords;
		vboData.noLightCoords = true;
		vboData.boneIndexes = (int (*)[4])indexbuf;
		vboData.boneWeights = (vec4_t *)weightbuf;
		vboData.numVerts = IQModel->num_vertexes;


		vbo = R_CreateStaticVBO( "IQM surface VBO", vboData,
					 vboLayout_t::VBO_LAYOUT_SKELETAL );

		if( qtangentbuf ) {
			ri.Hunk_FreeTempMemory( qtangentbuf );
		}
		if( weightbuf ) {
			ri.Hunk_FreeTempMemory( weightbuf );
		}
		if( indexbuf ) {
			ri.Hunk_FreeTempMemory( indexbuf );
		}

		// create IBO
		ibo = R_CreateStaticIBO( "IQM surface IBO", ( glIndex_t* )IQModel->triangles, IQModel->num_triangles * 3 );
	} else {
		vbo = nullptr;
		ibo = nullptr;
	}

	// register shaders
	// overwrite the material offset with the shader index
	mesh = ( iqmMesh_t* )IQMPtr( header, header->ofs_meshes );
	surface = IQModel->surfaces;
	for(unsigned i = 0; i < header->num_meshes; i++, mesh++, surface++ ) {
		surface->surfaceType = surfaceType_t::SF_IQM;

		if( mesh->name ) {
			surface->name = str;
			name = ( char* )IQMPtr( header, header->ofs_text + mesh->name );
			len = strlen( name ) + 1;
			Com_Memcpy( str, name, len );
			str += len;
		} else {
			surface->name = nullptr;
		}

		surface->shader = R_FindShader( ( char* )IQMPtr(header, header->ofs_text + mesh->material),
						shaderType_t::SHADER_3D_DYNAMIC, RSF_DEFAULT );
		if( surface->shader->defaultShader )
			surface->shader = tr.defaultShader;
		surface->data = IQModel;
		surface->first_vertex = mesh->first_vertex;
		surface->num_vertexes = mesh->num_vertexes;
		surface->first_triangle = mesh->first_triangle;
		surface->num_triangles = mesh->num_triangles;
		surface->vbo = vbo;
		surface->ibo = ibo;
	}

	// copy model bounds
	if(header->ofs_bounds)
	{
		iqmBounds_t *ptr = ( iqmBounds_t* )IQMPtr( header, header->ofs_bounds );
		for(unsigned i = 0; i < header->num_frames; i++)
		{
			VectorCopy( ptr->bbmin, bounds );
			bounds += 3;
			VectorCopy( ptr->bbmax, bounds );
			bounds += 3;

			ptr++;
		}
	}

	// register animations
	IQAnim = IQModel->anims;
	if( header->num_anims == 1 ) {
		RE_RegisterAnimationIQM( mod_name, IQAnim );
	}
	for(unsigned i = 0; i < header->num_anims; i++, IQAnim++ ) {
		char name[ MAX_QPATH ];

		Com_sprintf( name, MAX_QPATH, "%s:%s", mod_name, IQAnim->name );
		RE_RegisterAnimationIQM( name, IQAnim );
	}

	// build VBO

	return true;
}

/*
=============
R_CullIQM
=============
*/
static void R_CullIQM( trRefEntity_t *ent ) {
	vec3_t     localBounds[ 2 ];
	float      scale = ent->e.skeleton.scale;
	IQModel_t *model = tr.currentModel->iqm;
	IQAnim_t  *anim = model->anims;
	float     *bounds;

	// use the bounding box by the model
	bounds = model->bounds[0];
	VectorCopy( bounds, localBounds[ 0 ] );
	VectorCopy( bounds + 3, localBounds[ 1 ] );

	if ( anim && ( bounds = anim->bounds ) ) {
		// merge bounding box provided by the animation
		BoundsAdd( localBounds[ 0 ], localBounds[ 1 ],
			   bounds, bounds + 3 );
	}

	// merge bounding box provided by skeleton
	BoundsAdd( localBounds[ 0 ], localBounds[ 1 ],
		   ent->e.skeleton.bounds[ 0 ], ent->e.skeleton.bounds[ 1 ] );

	VectorScale( localBounds[0], scale, ent->localBounds[ 0 ] );
	VectorScale( localBounds[1], scale, ent->localBounds[ 1 ] );

	
	R_SetupEntityWorldBounds(ent);

	switch ( R_CullBox( ent->worldBounds ) )
	{
	case cullResult_t::CULL_IN:
		tr.pc.c_box_cull_md5_in++;
		ent->cull = cullResult_t::CULL_IN;
		return;
	case cullResult_t::CULL_CLIP:
		tr.pc.c_box_cull_md5_clip++;
		ent->cull = cullResult_t::CULL_CLIP;
		return;
	case cullResult_t::CULL_OUT:
	default:
		tr.pc.c_box_cull_md5_out++;
		ent->cull = cullResult_t::CULL_OUT;
		return;
	}
}

/*
=================
R_AddIQMSurfaces

Add all surfaces of this model
=================
*/
void R_AddIQMSurfaces( trRefEntity_t *ent ) {
	IQModel_t		*IQModel;
	srfIQModel_t		*surface;
	int                     i, j;
	bool                personalModel;
	int                     fogNum;
	shader_t                *shader;
	skin_t                  *skin;

	IQModel = tr.currentModel->iqm;
	surface = IQModel->surfaces;

	// don't add third_person objects if not in a portal
	personalModel = (ent->e.renderfx & RF_THIRD_PERSON) &&
	  tr.viewParms.portalLevel == 0;

	//
	// cull the entire model if merged bounding box of both frames
	// is outside the view frustum.
	//
	R_CullIQM( ent );

	if ( ent->cull == cullResult_t::CULL_OUT )
	{
		return;
	}

	//
	// set up lighting now that we know we aren't culled
	//
	if ( !personalModel || r_shadows->integer > Util::ordinal(shadowingMode_t::SHADOWING_BLOB) ) {
		R_SetupEntityLighting( &tr.refdef, ent, nullptr );
	}

	//
	// see if we are in a fog volume
	//
	fogNum = R_FogWorldBox( ent->worldBounds );

	for ( i = 0 ; i < IQModel->num_surfaces ; i++ ) {
		if(ent->e.customShader)
			shader = R_GetShaderByHandle( ent->e.customShader );
		else if(ent->e.customSkin > 0 && ent->e.customSkin < tr.numSkins)
		{
			skin = R_GetSkinByHandle(ent->e.customSkin);
			shader = tr.defaultShader;

			if (surface->name && *surface->name) {
				for(j = 0; j < skin->numSurfaces; j++)
				{
					if (!strcmp(skin->surfaces[j]->name, surface->name))
					{
						shader = skin->surfaces[j]->shader;
						break;
					}
				}
			}

			if ( shader == tr.defaultShader && i >= 0 && i < skin->numSurfaces && skin->surfaces[ i ] )
			{
				shader = skin->surfaces[ i ]->shader;
			}
		} else {

			shader = surface->shader;

			if ( ent->e.altShaderIndex > 0 && ent->e.altShaderIndex < MAX_ALTSHADERS &&
				shader->altShader[ ent->e.altShaderIndex ].index )
			{
				shader = R_GetShaderByHandle( shader->altShader[ ent->e.altShaderIndex ].index );
			}
		}

		// we will add shadows even if the main object isn't visible in the view

		if( !personalModel ) {
			R_AddDrawSurf( ( surfaceType_t *)surface, shader, -1, fogNum );
		}

		surface++;
	}
}
