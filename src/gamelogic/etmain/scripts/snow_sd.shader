//======================================================================
// snow_sd.shader
// Last edit: 09 august 2011 Eonfge
//
//======================================================================
textures/snow_sd/alphatree_snow
{
	qer_alphafunc gequal 0.5
	cull none
	surfaceparm alphashadow
	surfaceparm nomarks
	surfaceparm nonsolid
	surfaceparm trans
	implicitMask -
}

textures/snow_sd/alphatree_snow2
{
	qer_alphafunc gequal 0.5
	cull none
	surfaceparm alphashadow
	surfaceparm nomarks
	surfaceparm nonsolid
	surfaceparm trans
	implicitMask -
}

textures/snow_sd/alphatree_snow3
{
	qer_alphafunc gequal 0.5
	cull none
	surfaceparm alphashadow
	surfaceparm nomarks
	surfaceparm nonsolid
	surfaceparm trans
	implicitMask -
}

textures/snow_sd/alphatree_snow4
{
	qer_alphafunc gequal 0.5
	cull none
	surfaceparm alphashadow
	surfaceparm nomarks
	surfaceparm nonsolid
	surfaceparm trans
	implicitMask -
}

textures/snow_sd/alphatreeline_snow
{
	qer_alphafunc gequal 0.5
	cull none
	surfaceparm alphashadow
	surfaceparm nomarks
	surfaceparm nonsolid
	surfaceparm trans
	implicitMask -
}

textures/snow_sd/ametal_m03_snow
{
	surfaceparm metalsteps
	implicitMap -
	
	qer_editorimage textures/snow_sd/ametal_m03_snow
	diffusemap		textures/snow_sd/ametal_m03_snow
	bumpmap         displacemap( textures/snow_sd/ametal_m03_snow_norm, invertColor(textures/snow_sd/ametal_m03_snow_disp) )
	specularmap		textures/snow_sd/ametal_m03_snow_spec
}

textures/snow_sd/ametal_m04a_snow
{
	surfaceparm metalsteps
	implicitMap -
	
	qer_editorimage textures/snow_sd/ametal_m04a_snow
	diffusemap		textures/snow_sd/ametal_m04a_snow
	bumpmap         displacemap( textures/snow_sd/ametal_m04a_snow_norm, invertColor(textures/snow_sd/ametal_m04a_snow_disp) )
	specularmap		textures/snow_sd/ametal_m04a_snow_spec
}

textures/snow_sd/ametal_m04a_snowfade
{
	surfaceparm metalsteps
	implicitMap -
	
	qer_editorimage textures/snow_sd/ametal_m04a_snowfade
	diffusemap		textures/snow_sd/ametal_m04a_snowfade
	bumpmap         displacemap( textures/snow_sd/ametal_m04a_snowfade_norm, invertColor(textures/snow_sd/ametal_m04a_snowfade_disp) )
	specularmap		textures/snow_sd/ametal_m04a_snowfade_spec
}

textures/snow_sd/bigrock_rounded_faint
{
	qer_editorimage textures/snow_sd/bigrock_rounded_faint
	diffusemap		textures/snow_sd/bigrock_rounded_faint
	bumpmap         displacemap( textures/snow_sd/bigrock_rounded_faint_norm, invertColor(textures/snow_sd/bigrock_rounded_faint_disp) )
	specularmap		textures/snow_sd/bigrock_rounded_faint_spec
}

textures/snow_sd/bunkertrim_snow
{
	qer_editorimage textures/snow_sd/bunkertrim_snow
	q3map_nonplanar
	q3map_shadeangle 160
	implicitMap -
	
	diffusemap		textures/snow_sd/bunkertrim_snow
	bumpmap         displacemap( textures/snow_sd/bunkertrim_snow_norm, invertColor(textures/snow_sd/bunkertrim_snow_disp) )
	specularmap		textures/snow_sd/bunkertrim_snow_spec
	
	
}

//==========================================================================
// Corner/edges of axis fueldump bunker buildings, needs phong goodness. // Eonfge: whatevar.... could you not keep it on alphabet?
//==========================================================================
textures/snow_sd/bunkertrim3_snow
{
	q3map_nonplanar
	q3map_shadeangle 179
	qer_editorimage textures/snow_sd/bunkertrim3_snow
	
	diffusemap		textures/snow_sd/bunkertrim3_snow
	bumpmap         displacemap( textures/snow_sd/bunkertrim3_snow_norm, invertColor(textures/snow_sd/bunkertrim3_snow_disp) )
	specularmap		textures/snow_sd/bunkertrim3_snow_spec
	
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/snow_sd/bunkertrim3_snow
		blendFunc filter
	}
	
}

textures/snow_sd/bunkerwall_lrg02
{
	qer_editorimage textures/snow_sd/bunkerwall_lrg02
	q3map_nonplanar
	q3map_shadeangle 80
	implicitMap -
	
	diffusemap		textures/snow_sd/bunkerwall_lrg02
	bumpmap         displacemap( textures/snow_sd/bunkerwall_lrg02_norm, invertColor(textures/snow_sd/bunkerwall_lrg02_disp) )
	specularmap		textures/snow_sd/bunkerwall_lrg02_spec
}

textures/snow_sd/coal_snow
{
	qer_editorimage textures/snow_sd/coal_snow
	
	diffusemap		textures/snow_sd/coal_snow
	bumpmap         displacemap( textures/snow_sd/coal_snow_norm, invertColor(textures/snow_sd/coal_snow_disp) )
	specularmap		textures/snow_sd/coal_snow_spec
}

textures/snow_sd/concretec05_snow
{
	qer_editorimage textures/snow_sd/concretec05_snow
	
	diffusemap		textures/snow_sd/concretec05_snow
	bumpmap         displacemap( textures/snow_sd/concretec05_snow_norm, invertColor(textures/snow_sd/concretec05_snow_disp) )
	specularmap		textures/snow_sd/concretec05_snow_spec
}



textures/snow_sd/icelake2
{
	qer_editorimage textures/snow_sd/icelake2
	
	{
                stage diffusemap
                map             textures/snow_sd/icelake2
                alphaTest       0.5
      }
	bumpmap         displacemap( textures/snow_sd/icelake2_norm, invertColor(textures/snow_sd/icelake2_disp) )
	specularmap		textures/snow_sd/icelake2_spec
}



textures/snow_sd/miltary_m04_snow
{
	qer_editorimage textures/snow_sd/miltary_m04_snow
	
	diffusemap		textures/snow_sd/miltary_m04_snow
	bumpmap         displacemap( textures/snow_sd/miltary_m04_snow_norm, invertColor(textures/snow_sd/miltary_m04_snow_disp) )
	specularmap		textures/snow_sd/miltary_m04_snow_spec
}


textures/snow_sd/icey_lake
{
	qer_trans 0.80
	qer_editorimage textures/snow_sd/icelake.tga
	surfaceparm slick
	{
		map textures/effects/envmap_ice.tga
		tcgen environment
	}
	{
		map textures/snow_sd/icelake.tga
		blendfunc blend
		tcmod scale 0.35 0.35
	}
	{
		map $lightmap
		blendfunc filter
		rgbGen identity
	}
}

// Used in fueldump on the icy river.
// Note: Apply this at a scale of 2.0x2.0 so it aligns correctly
// ydnar: added depthwrite and sort key so it dlights correctly
textures/snow_sd/icelake2
{
	qer_trans 0.80
	qer_editorimage textures/snow_sd/icelake2.tga
	sort seethrough
	surfaceparm slick
	surfaceparm trans
	tesssize 256

	{
		map textures/effects/envmap_ice2.tga
		tcgen environment
		blendfunc blend
	}
	{
		map textures/snow_sd/icelake2.tga
		blendfunc blend
	}
	{
		map $lightmap
		blendfunc filter
		rgbGen identity
		depthWrite
	}
	{
		map textures/detail_sd/snowdetail_heavy.tga
		blendFunc GL_DST_COLOR GL_SRC_COLOR
		rgbgen identity
		tcMod scale 4 4
		detail
	}
}

// Note: Apply this at a scale of 2.0x2.0 so it aligns correctly
textures/snow_sd/icelake2_opaque
{
	qer_editorimage textures/snow_sd/icelake2.tga
	surfaceparm slick
	tesssize 256

	{
		map textures/effects/envmap_ice2.tga
		tcgen environment
		rgbGen identity
	}
	{
		map textures/snow_sd/icelake2.tga
		blendfunc blend
	}
	{
		map $lightmap
		blendfunc filter
		rgbGen identity
	}
	{
		map textures/detail_sd/snowdetail_heavy.tga
		blendFunc GL_DST_COLOR GL_SRC_COLOR
		rgbgen identity
		tcMod scale 4 4
		detail
	}
}

textures/snow_sd/icelake3
{
	qer_editorimage textures/snow_sd/icelake3
	
	{
                stage diffusemap
                map             textures/snow_sd/icelake3
                alphaTest       0.5
       }
	bumpmap         displacemap( textures/snow_sd/icelake2_norm, invertColor(textures/snow_sd/icelake2_disp) )
	specularmap		textures/snow_sd/icelake2_spec
}

textures/snow_sd/mesh_c03_snow
{
    qer_editorimage textures/snow_sd/mesh_c03_snow
        
	qer_alphafunc gequal 0.5
	cull none
	nomipmaps
	nopicmip
	surfaceparm alphashadow
	surfaceparm nomarks
	surfaceparm nonsolid
	surfaceparm trans
	implicitMask -
		
        {
                stage diffusemap
                map             textures/snow_sd/mesh_c03_snow
                alphaTest       0.5
        }
        bumpmap         displacemap( textures/snow_sd/mesh_c03_snow_norm, invertColor(textures/snow_sd/mesh_c03_snow_disp) )
        specularmap             textures/snow_sd/mesh_c03_snow_spec
}

textures/snow_sd/metal_m04g2_snow
{
	qer_editorimage textures/snow_sd/metal_m04g2_snow
	surfaceparm metalsteps
	implicitMap -
	
	diffusemap		textures/snow_sd/metal_m04g2_snow
	bumpmap         displacemap( textures/snow_sd/metal_m04g2_snow_norm, invertColor(textures/snow_sd/metal_m04g2_snow_disp) )
	specularmap		textures/snow_sd/metal_m04g2_snow_spec
}

textures/snow_sd/mroof_snow
{
	qer_editorimage textures/snow_sd/mroof_snow
	surfaceparm roofsteps
	implicitMap -
	
	diffusemap		textures/snow_sd/mroof_snow
	bumpmap         displacemap( textures/snow_sd/mroof_snow_norm, invertColor(textures/snow_sd/mroof_snow_disp) )
	specularmap		textures/snow_sd/mroof_snow_spec
}

textures/snow_sd/mxrock4b_snow
{
	qer_editorimage textures/snow_sd/mxrock4b_snow
	
	diffusemap		textures/snow_sd/mxrock4b_snow
	bumpmap         displacemap( textures/snow_sd/mxrock4b_snow_norm, invertColor(textures/snow_sd/mxrock4b_snow_disp) )
	specularmap		textures/snow_sd/mxrock4b_snow_spec
}

textures/snow_sd/snow_noisy
{
	qer_editorimage textures/snow_sd/snow_noisy
	
	diffusemap		textures/snow_sd/snow_noisy
	bumpmap         displacemap( textures/snow_sd/snow_noisy_norm, invertColor(textures/snow_sd/snow_noisy_disp) )
	specularmap		textures/snow_sd/snow_noisy_spec
}

textures/snow_sd/snow_path01
{
	qer_editorimage textures/snow_sd/snow_path01
	
	diffusemap		textures/snow_sd/snow_path01
	bumpmap         displacemap( textures/snow_sd/snow_path01_norm, invertColor(textures/snow_sd/snow_path01_disp) )
	specularmap		textures/snow_sd/snow_path01_spec
}

textures/snow_sd/snow_road01
{
	qer_editorimage textures/snow_sd/snow_road01
	
	diffusemap		textures/snow_sd/snow_road01
	bumpmap         displacemap( textures/snow_sd/snow_road01_norm, invertColor(textures/snow_sd/snow_road01_disp) )
	specularmap		textures/snow_sd/snow_road01_spec
}


textures/snow_sd/snow_var01
{
	qer_editorimage textures/snow_sd/snow_var01
	
	diffusemap		textures/snow_sd/snow_var01
	bumpmap         displacemap( textures/snow_sd/snow_var01_norm, invertColor(textures/snow_sd/snow_var01_disp) )
	specularmap		textures/snow_sd/snow_var01_spec
}

textures/snow_sd/snow_var01_big
{
	qer_editorimage textures/snow_sd/snow_var01_big
	
	diffusemap		textures/snow_sd/snow_var01_big
	bumpmap         displacemap( textures/snow_sd/ssnow_var01_big_norm, invertColor(textures/snow_sd/snow_var01_big_disp) )
	specularmap		textures/snow_sd/snow_var01_big_spec
}

textures/snow_sd/snow_var02
{
	qer_editorimage textures/snow_sd/snow_var02
	
	diffusemap		textures/snow_sd/snow_var02
	bumpmap         displacemap( textures/snow_sd/snow_var02_norm, invertColor(textures/snow_sd/snow_var02_disp) )
	specularmap		textures/snow_sd/snow_var02_spec
}

textures/snow_sd/sub1_snow
{
	surfaceparm metalsteps
	implicitMap -
	
	qer_editorimage textures/snow_sd/sub1_snow
	
	diffusemap		textures/snow_sd/sub1_snow
	bumpmap         displacemap( textures/snow_sd/sub1_snow_norm, invertColor(textures/snow_sd/sub1_snow_disp) )
	specularmap		textures/snow_sd/sub1_snow_spec
}

textures/snow_sd/sub1_snow2
{
	surfaceparm metalsteps
	implicitMap -
	
	qer_editorimage textures/snow_sd/sub1_snow2
	
	diffusemap		textures/snow_sd/sub1_snow2
	bumpmap         displacemap( textures/snow_sd/sub1_snow2_norm, invertColor(textures/snow_sd/sub1_snow2_disp) )
	specularmap		textures/snow_sd/sub1_snow2_spec
}

textures/snow_sd/wirefence01_snow
{
	cull none
	surfaceparm alphashadow
	surfaceparm nomarks
	surfaceparm nonsolid
	surfaceparm trans
	implicitMask -
}

textures/snow_sd/wood_m05a_snow
{
	surfaceparm woodsteps
	implicitMap -
	
	qer_editorimage textures/snow_sd/wood_m05a_snow
	
	diffusemap		textures/snow_sd/wood_m05a_snow
	bumpmap         displacemap( textures/snow_sd/wood_m05a_snow_norm, invertColor(textures/snow_sd/wood_m05a_snow_disp) )
	specularmap		textures/snow_sd/wood_m05a_snow_spec
}

textures/snow_sd/wood_m06b_snow
{
	surfaceparm woodsteps
	implicitMap -
}

textures/snow_sd/fuse_box_snow
{
	qer_editorimage textures/snow_sd/fuse_box_snow
	surfaceparm metalsteps
	implicitMap -
	
	diffusemap		textures/snow_sd/fuse_box_snow
	bumpmap         displacemap( textures/snow_sd/fuse_box_snow_norm, invertColor(textures/snow_sd/fuse_box_snow_disp) )
	specularmap		textures/snow_sd/fuse_box_snow_spec
}

textures/snow_sd/s_castle_m03a_trim
{
	qer_editorimage textures/snow_sd/s_castle_m03a_trim
	
	
	diffusemap		textures/snow_sd/s_castle_m03a_trim
	bumpmap         displacemap( textures/snow_sd/s_castle_m03a_trim_norm, invertColor(textures/snow_sd/s_castle_m03a_trim_disp) )
	specularmap		textures/snow_sd/s_castle_m03a_trim_spec
}

textures/snow_sd/s_dirt_m03i_2_big
{
	qer_editorimage textures/snow_sd/s_dirt_m03i_2_big
	
	
	diffusemap		textures/snow_sd/s_dirt_m03i_2_big
	bumpmap         displacemap( textures/snow_sd/s_dirt_m03i_2_big_norm, invertColor(textures/snow_sd/s_dirt_m03i_2_big_disp) )
	specularmap		textures/snow_sd/s_dirt_m03i_2_big_spec
}

textures/snow_sd/s_grass_ml03b_big
{
	qer_editorimage textures/snow_sd/s_grass_ml03b_big
	
	diffusemap		textures/snow_sd/s_grass_ml03b_big
	bumpmap         displacemap( textures/snow_sd/s_grass_ml03b_big_norm, invertColor(textures/snow_sd/s_grass_ml03b_big_disp) )
	specularmap		textures/snow_sd/s_dirt_m03i_2_big_spec
}

textures/snow_sd/snow_muddy
{
	qer_editorimage textures/snow_sd/snow_muddy
	
	diffusemap		textures/snow_sd/snow_muddy
	bumpmap         displacemap( textures/snow_sd/snow_muddy_norm, invertColor(textures/snow_sd/snow_muddy_disp) )
	specularmap		textures/snow_sd/snow_muddy_spec
}

textures/snow_sd/xmetal_m02_snow
{
	surfaceparm metalsteps
	implicitMap -
	
	qer_editorimage textures/snow_sd/xmetal_m02_snow
	
	diffusemap		textures/snow_sd/xmetal_m02_snow
	bumpmap         displacemap( textures/snow_sd/xmetal_m02_snow_norm, invertColor(textures/snow_sd/xmetal_m02_snow_disp) )
	specularmap		textures/snow_sd/xmetal_m02_snow_spec
	
}

textures/snow_sd/xmetal_m02t_snow
{
	surfaceparm metalsteps
	implicitMap -
	
	qer_editorimage textures/snow_sd/xmetal_m02t_snow
	
	diffusemap		textures/snow_sd/xmetal_m02t_snow
	bumpmap         displacemap( textures/snow_sd/xmetal_m02t_snow_norm, invertColor(textures/snow_sd/xmetal_m02t_snow_disp) )
	specularmap		textures/snow_sd/xmetal_m02t_snow_spec
}

//==========================================================================
// Various terrain decals textures
//==========================================================================

// ydnar: added "sort banner" and changed blendFunc so they fog correctly
// note: the textures were inverted and Brightness/Contrast applied so they
// will work properly with the new blendFunc (this is REQUIRED!)

textures/snow_sd/snow_track03 
{ 
	qer_editorimage textures/snow_sd/snow_track03.tga
	q3map_nonplanar 
	q3map_shadeangle 120 
	surfaceparm trans 
	surfaceparm nonsolid 
	surfaceparm pointlight
	surfaceparm nomarks
	polygonOffset
	
	sort decal
	
	{
		map textures/snow_sd/snow_track03.tga
       		blendFunc GL_ZERO GL_ONE_MINUS_SRC_COLOR
		rgbGen identity
	}
}

textures/snow_sd/snow_track03_faint
{ 
	qer_editorimage textures/snow_sd/snow_track03.tga
	q3map_nonplanar 
	q3map_shadeangle 120 
	surfaceparm trans 
	surfaceparm nonsolid 
	surfaceparm pointlight
	surfaceparm nomarks
	polygonOffset
	
	sort decal
	
	{
		map textures/snow_sd/snow_track03.tga
       		blendFunc GL_ZERO GL_ONE_MINUS_SRC_COLOR
		rgbGen const ( 0.5 0.5 0.5 )
	}
}

textures/snow_sd/snow_track03_end 
{ 
	qer_editorimage textures/snow_sd/snow_track03_end.tga
	q3map_nonplanar 
	q3map_shadeangle 120 
	surfaceparm trans 
	surfaceparm nonsolid 
	surfaceparm pointlight
	surfaceparm nomarks
	polygonOffset
	
	sort decal
	
	{
		map textures/snow_sd/snow_track03_end.tga
        	blendFunc GL_ZERO GL_ONE_MINUS_SRC_COLOR
		rgbGen identity
	}
}

textures/snow_sd/snow_track03_end_faint 
{ 
	qer_editorimage textures/snow_sd/snow_track03_end.tga
	q3map_nonplanar 
	q3map_shadeangle 120 
	surfaceparm trans 
	surfaceparm nonsolid 
	surfaceparm pointlight
	surfaceparm nomarks
	polygonOffset
	
	sort decal
	
	{
		map textures/snow_sd/snow_track03_end.tga
        	blendFunc GL_ZERO GL_ONE_MINUS_SRC_COLOR
		rgbGen const ( 0.5 0.5 0.5 )
	}
}

textures/snow_sd/river_edge_snowy 
{ 
	qer_editorimage textures/snow_sd/river_edge_snowy
	q3map_nonplanar 
	q3map_shadeangle 120 
	surfaceparm trans 
	surfaceparm nonsolid 
	surfaceparm pointlight
	surfaceparm nomarks
	polygonOffset
	
	sort decal
	
	{
		stage 			diffusemap
		map             textures/snow_sd/river_edge_snowy
		alphaTest       0.5
    }
		
        bumpmap         displacemap( textures/snow_sd/river_edge_snowy_norm, invertColor(textures/snow_sd/river_edge_snowy_disp) )
        specularmap  	textures/snow_sd/river_edge_snowy_spec
	
	{
		map textures/snow_sd/river_edge_snowy
        	blend blend
		rgbGen identity
	}
}
