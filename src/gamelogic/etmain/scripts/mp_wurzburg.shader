textures/mp_wurzburg/dirt_m03icmp_brown
{
	qer_editorimage textures/temperate_sd/dirt_m03icmp_brown
	surfaceparm trans
	implicitMap textures/temperate_sd/dirt_m03icmp_brown
}

textures/mp_wurzburg/dirt_m04cmp_brown
{
	qer_editorimage textures/temperate_sd/dirt_m04cmp_brown
	surfaceparm trans
	implicitMap textures/temperate_sd/dirt_m04cmp_brown
}

textures/mp_wurzburg/fog
{
	qer_editorimage textures/sfx/fog_grey1
	
	surfaceparm nodraw
	surfaceparm nonsolid
	surfaceparm trans
	surfaceparm fog
	
	fogparms ( 0.09411 0.09803 0.12549 ) 3192
}

textures/mp_wurzburg/lmterrain2_base
{
	q3map_normalimage textures/sd_bumpmaps/normalmap_terrain
	
	q3map_lightmapsize 512 512
	q3map_lightmapMergable
	q3map_lightmapaxis z
	
	q3map_tcGen ivector ( 512 0 0 ) ( 0 512 0 )
	q3map_tcMod scale 2 2
	q3map_tcMod rotate 37
	
	surfaceparm grasssteps
	surfaceparm landmine	
}

textures/mp_wurzburg/lmterrain2_foliage_base
{
	q3map_baseShader textures/mp_wurzburg/lmterrain2_base

	q3map_foliage models/foliage/grassfoliage1.md3 1.25 48 0.1 2
	q3map_foliage models/foliage/grassfoliage2.md3 1.1 48 0.1 2
	q3map_foliage models/foliage/grassfoliage3.md3 1 48 0.1 2
}

textures/mp_wurzburg/lmterrain2_foliage_fade
{
	q3map_baseShader textures/mp_wurzburg/lmterrain2_base

	q3map_foliage models/foliage/grassfoliage1.md3 1.25 64 0.1 2
	q3map_foliage models/foliage/grassfoliage2.md3 1.1 64 0.1 2
	q3map_foliage models/foliage/grassfoliage3.md3 1 64 0.1 2
}

textures/mp_wurzburg/lmterrain2_0
{
	q3map_baseshader textures/mp_wurzburg/lmterrain2_base

	{
		map textures/temperate_sd/master_grass_dirt3
		rgbgen identity
	}
	{
		lightmap $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbgen identity
	}
}

textures/mp_wurzburg/lmterrain2_1
{
	q3map_baseshader textures/mp_wurzburg/lmterrain2_foliage_base

	{
		map textures/temperate_sd/grass_path1
		rgbgen identity
	}
	{
		lightmap $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbgen identity
	}
}

textures/mp_wurzburg/lmterrain2_2
{
	q3map_baseshader textures/mp_wurzburg/lmterrain2_base

	{
		map textures/temperate_sd/grass_dense1
		rgbgen identity
	}
	{
		lightmap $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbgen identity
	}
}

textures/mp_wurzburg/lmterrain2_3
{
	q3map_baseshader textures/mp_wurzburg/lmterrain2_base

	{
		map textures/temperate_sd/rock_ugly_brown
		rgbgen identity
	}
	{
		lightmap $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbgen identity
	}
}

textures/mp_wurzburg/lmterrain2_4
{
	q3map_baseshader textures/mp_wurzburg/lmterrain2_base

	{
		map textures/temperate_sd/dirt3
		rgbgen identity
	}
	{
		lightmap $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbgen identity
	}
}

textures/mp_wurzburg/lmterrain2_5
{
	q3map_baseshader textures/mp_wurzburg/lmterrain2_base

	{
		map textures/temperate_sd/grass_ml03cmp
		rgbgen identity
	}
	{
		lightmap $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbgen identity
	}
}

textures/mp_wurzburg/lmterrain2_0to1
{
	q3map_baseshader textures/mp_wurzburg/lmterrain2_foliage_fade

	{
		map textures/temperate_sd/master_grass_dirt3
		rgbgen identity
	}
	{
		map textures/temperate_sd/grass_path1
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
	}
	{
		lightmap $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbgen identity
	}
}

textures/mp_wurzburg/lmterrain2_0to2
{
	q3map_baseshader textures/mp_wurzburg/lmterrain2_base

	{
		map textures/temperate_sd/master_grass_dirt3
		rgbgen identity
	}
	{
		map textures/temperate_sd/grass_dense1
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
	}
	{
		lightmap $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbgen identity
	}
}

textures/mp_wurzburg/lmterrain2_0to3
{
	q3map_baseshader textures/mp_wurzburg/lmterrain2_base
	
	{
		map textures/temperate_sd/master_grass_dirt3
		rgbgen identity
	}
	{
		map textures/temperate_sd/rock_ugly_brown
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
	}
	{
		lightmap $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbgen identity
	}
}

textures/mp_wurzburg/lmterrain2_0to4
{
	q3map_baseshader textures/mp_wurzburg/lmterrain2_base
	
	{
		map textures/temperate_sd/master_grass_dirt3
		rgbgen identity
	}
	{
		map textures/temperate_sd/dirt3
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
	}
	{
		lightmap $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbgen identity
	}
}

textures/mp_wurzburg/lmterrain2_0to5
{
	q3map_baseshader textures/mp_wurzburg/lmterrain2_base
	
	{
		map textures/temperate_sd/master_grass_dirt3
		rgbgen identity
	}
	{
		map textures/temperate_sd/grass_ml03cmp
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
	}
	{
		lightmap $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbgen identity
	}
}

textures/mp_wurzburg/lmterrain2_1to2
{
	q3map_baseshader textures/mp_wurzburg/lmterrain2_foliage_base
	
	{
		map textures/temperate_sd/grass_path1
		rgbgen identity
	}
	{
		map textures/temperate_sd/grass_dense1
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
	}
	{
		lightmap $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbgen identity
	}
}

textures/mp_wurzburg/lmterrain2_1to3
{
	q3map_baseshader textures/mp_wurzburg/lmterrain2_foliage_base
	
	{
		map textures/temperate_sd/grass_path1
		rgbgen identity
	}
	{
		map textures/temperate_sd/rock_ugly_brown
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
	}
	{
		lightmap $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbgen identity
	}
}

textures/mp_wurzburg/lmterrain2_1to4
{
	q3map_baseshader textures/mp_wurzburg/lmterrain2_foliage_base
	
	{
		map textures/temperate_sd/grass_path1
		rgbgen identity
	}
	{
		map textures/temperate_sd/dirt3
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
	}
	{
		lightmap $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbgen identity
	}
}

textures/mp_wurzburg/lmterrain2_1to5
{
	q3map_baseshader textures/mp_wurzburg/lmterrain2_foliage_base
	
	{
		map textures/temperate_sd/grass_path1
		rgbgen identity
	}
	{
		map textures/temperate_sd/grass_ml03cmp
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
	}
	{
		lightmap $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbgen identity
	}
}

textures/mp_wurzburg/lmterrain2_2to3
{
	q3map_baseshader textures/mp_wurzburg/lmterrain2_base
	
	{
		map textures/temperate_sd/grass_dense1
		rgbgen identity
	}
	{
		map textures/temperate_sd/rock_ugly_brown
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
	}
	{
		lightmap $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbgen identity
	}
}

textures/mp_wurzburg/lmterrain2_2to4
{
	q3map_baseshader textures/mp_wurzburg/lmterrain2_base
	
	{
		map textures/temperate_sd/grass_dense1
		rgbgen identity
	}
	{
		map textures/temperate_sd/dirt3
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
	}
	{
		lightmap $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbgen identity
	}
}

textures/mp_wurzburg/lmterrain2_2to5
{
	q3map_baseshader textures/mp_wurzburg/lmterrain2_base
	
	{
		map textures/temperate_sd/grass_dense1
		rgbgen identity
	}
	{
		map textures/temperate_sd/grass_ml03cmp
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
	}
	{
		lightmap $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbgen identity
	}
}

textures/mp_wurzburg/lmterrain2_3to4
{
	q3map_baseshader textures/mp_wurzburg/lmterrain2_base
	
	{
		map textures/temperate_sd/rock_ugly_brown
		rgbgen identity
	}
	{
		map textures/temperate_sd/dirt3
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
	}
	{
		lightmap $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbgen identity
	}
}

textures/mp_wurzburg/lmterrain2_3to5
{
	q3map_baseshader textures/mp_wurzburg/lmterrain2_base
	
	{
		map textures/temperate_sd/rock_ugly_brown
		rgbgen identity
	}
	{
		map textures/temperate_sd/grass_ml03cmp
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
	}
	{
		lightmap $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbgen identity
	}
}

textures/mp_wurzburg/lmterrain2_4to5
{
	q3map_baseshader textures/mp_wurzburg/lmterrain2_base
	
	{
		map textures/temperate_sd/dirt3
		rgbgen identity
	}
	{
		map textures/temperate_sd/grass_ml03cmp
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
	}
	{
		lightmap $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbgen identity
	}
}

//**********************************************
// rain FX
//base shader has problem with texture scaling
//should be fixed by me or ydnar later
//back to the old school way - Chris-
//**********************************************

textures/mp_wurzburg/road_wet
{
//	q3map_foliage models/foliage/raincircle0.md3 1 64 0.1 2
//	q3map_foliage models/foliage/raincircle1.md3 0.8 64 0.1 2
//	q3map_foliage models/foliage/raincircle2.md3 0.6 64 0.1 2
//	q3map_foliage models/foliage/raincircle3.md3 0.9 64 0.1 2
//	q3map_foliage models/foliage/raincircle4.md3 0.7 64 0.1 2
//	q3map_foliage models/foliage/raincircle5.md3 0.5 64 0.1 2
	qer_editorimage textures/temperate_sd/dirt_m03icmp_brown
	surfaceparm trans
	implicitMap textures/temperate_sd/dirt_m03icmp_brown
}

textures/mp_wurzburg/road
{
	
	qer_editorimage textures/temperate_sd/dirt_m03icmp_brown
	surfaceparm trans
	implicitMap textures/temperate_sd/dirt_m03icmp_brown
}


textures/mp_wurzburg/road_puddle1
{
	q3map_foliage models/foliage/raincircle0.md3 1 64 0.1 2
	q3map_foliage models/foliage/raincircle1.md3 0.8 64 0.1 2
	q3map_foliage models/foliage/raincircle2.md3 0.6 64 0.1 2
	q3map_foliage models/foliage/raincircle3.md3 0.9 64 0.1 2
	q3map_foliage models/foliage/raincircle4.md3 0.7 64 0.1 2
	q3map_foliage models/foliage/raincircle5.md3 0.5 64 0.1 2
	qer_editorimage textures/temperate_sd/road_puddle1
	surfaceparm trans
	surfaceparm splash
		
	{
		map textures/effects/envmap_radar
		rgbGen identity
		tcMod scale 0.5 0.5
		tcGen environment
	}
	{
		map textures/liquids_sd/puddle_specular
		rgbGen identity
		tcMod scale 2 2
		blendFunc GL_SRC_ALPHA GL_ONE
		tcGen environment
	}
	{
		map textures/temperate_sd/road_puddle1
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen identity
	}
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
}

textures/mp_wurzburg/road_bigpuddle
{
	q3map_foliage models/foliage/raincircle0.md3 1 64 0.1 2
	q3map_foliage models/foliage/raincircle1.md3 0.8 64 0.1 2
	q3map_foliage models/foliage/raincircle2.md3 0.6 64 0.1 2
	q3map_foliage models/foliage/raincircle3.md3 0.9 64 0.1 2
	q3map_foliage models/foliage/raincircle4.md3 0.7 64 0.1 2
	q3map_foliage models/foliage/raincircle5.md3 0.5 64 0.1 2
	qer_editorimage textures/temperate_sd/road_bigpuddle
	surfaceparm trans
	surfaceparm splash
	
	{
		map textures/effects/envmap_radar
		rgbGen identity
		tcMod scale 0.5 0.5
		tcGen environment
	}
	{
		map textures/liquids_sd/puddle_specular
		rgbGen identity
		tcMod scale 2 2
		blendFunc GL_SRC_ALPHA GL_ONE
		tcGen environment
	}
	{
		map textures/temperate_sd/road_bigpuddle
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen identity
	}
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
}


textures/mp_wurzburg/borderroad
{
	qer_editorimage textures/temperate_sd/dirt_m04cmp_brown
	surfaceparm trans
	implicitMap textures/temperate_sd/dirt_m04cmp_brown
}

textures/mp_wurzburg/wood_m02_wet
{
//	q3map_foliage models/foliage/raincircle0.md3 1 64 0.1 2
//	q3map_foliage models/foliage/raincircle1.md3 0.8 64 0.1 2
//	q3map_foliage models/foliage/raincircle2.md3 0.6 64 0.1 2
//	q3map_foliage models/foliage/raincircle3.md3 0.9 64 0.1 2
//	q3map_foliage models/foliage/raincircle4.md3 0.7 64 0.1 2
//	q3map_foliage models/foliage/raincircle5.md3 0.5 64 0.1 2
	qer_editorimage textures/wood/wood_m02
	implicitMap textures/wood/wood_m02
}

textures/mp_wurzburg/gy_ml03a_wet
{
//	q3map_foliage models/foliage/raincircle0.md3 1 64 0.1 2
//	q3map_foliage models/foliage/raincircle1.md3 0.8 64 0.1 2
//	q3map_foliage models/foliage/raincircle2.md3 0.6 64 0.1 2
//	q3map_foliage models/foliage/raincircle3.md3 0.9 64 0.1 2
//	q3map_foliage models/foliage/raincircle4.md3 0.7 64 0.1 2
//	q3map_foliage models/foliage/raincircle5.md3 0.5 64 0.1 2
	qer_editorimage textures/graveyard/gy_ml03a
	implicitMap textures/graveyard/gy_ml03a
}

textures/mp_wurzburg/debri_m05_wet
{
//	q3map_foliage models/foliage/raincircle0.md3 1 64 0.1 2
//	q3map_foliage models/foliage/raincircle1.md3 0.8 64 0.1 2
//	q3map_foliage models/foliage/raincircle2.md3 0.6 64 0.1 2
//	q3map_foliage models/foliage/raincircle3.md3 0.9 64 0.1 2
//	q3map_foliage models/foliage/raincircle4.md3 0.7 64 0.1 2
//	q3map_foliage models/foliage/raincircle5.md3 0.5 64 0.1 2
	qer_editorimage textures/rubble/debri_m05
	implicitMap textures/rubble/debri_m05
}

textures/mp_wurzburg/wood_m16_wet
{
//	q3map_foliage models/foliage/raincircle0.md3 1 64 0.1 2
//	q3map_foliage models/foliage/raincircle1.md3 0.8 64 0.1 2
//	q3map_foliage models/foliage/raincircle2.md3 0.6 64 0.1 2
//	q3map_foliage models/foliage/raincircle3.md3 0.9 64 0.1 2
//	q3map_foliage models/foliage/raincircle4.md3 0.7 64 0.1 2
//	q3map_foliage models/foliage/raincircle5.md3 0.5 64 0.1 2
	qer_editorimage textures/wood/wood_m16
	implicitMap textures/wood/wood_m16
}

textures/mp_wurzburg/wall_c01_wet
{
//	q3map_foliage models/foliage/raincircle0.md3 1 64 0.1 2
//	q3map_foliage models/foliage/raincircle1.md3 0.8 64 0.1 2
//	q3map_foliage models/foliage/raincircle2.md3 0.6 64 0.1 2
//	q3map_foliage models/foliage/raincircle3.md3 0.9 64 0.1 2
//	q3map_foliage models/foliage/raincircle4.md3 0.7 64 0.1 2
//	q3map_foliage models/foliage/raincircle5.md3 0.5 64 0.1 2
	qer_editorimage textures/sleepy/wall_c01
	implicitMap textures/sleepy/wall_c01
}

textures/mp_wurzburg/metal_wet1
{
	qer_editorimage textures/metals_sd/metal_ref1

	{
		map textures/effects/envmap_radar
		rgbGen identity
		tcMod scale 0.5 0.5
		tcGen environment
	}
	{
		map textures/liquids_sd/puddle_specular
		rgbGen identity
		tcMod scale 2 2
		blendFunc GL_SRC_ALPHA GL_ONE
		tcGen environment
	}
	{
		map textures/metals_sd/metal_ref1
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		
		rgbGen identity
	}
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
}


textures/mp_wurzburg/metal_wet2
{
	q3map_foliage models/foliage/raincircle0.md3 1 64 0.1 2
	q3map_foliage models/foliage/raincircle1.md3 0.8 64 0.1 2
	q3map_foliage models/foliage/raincircle2.md3 0.6 64 0.1 2
	q3map_foliage models/foliage/raincircle3.md3 0.9 64 0.1 2
	q3map_foliage models/foliage/raincircle4.md3 0.7 64 0.1 2
	q3map_foliage models/foliage/raincircle5.md3 0.5 64 0.1 2
	qer_editorimage textures/metals_sd/metal_ref1

	{
		map textures/effects/envmap_radar
		rgbGen identity
		tcMod scale 0.5 0.5
		tcGen environment
	}
	{
		map textures/liquids_sd/puddle_specular
		rgbGen identity
		tcMod scale 2 2
		blendFunc GL_SRC_ALPHA GL_ONE
		tcGen environment
	}
	{
		map textures/metals_sd/metal_ref1
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen identity
	}
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
}
