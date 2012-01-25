/////////////////////////////////////////////////////////////////////
// OVERCAST - BY AVOC @ eft-clan.com
// If you wish to use this sky, please leave this part in the shader
// or give proper credit in the readme
/////////////////////////////////////////////////////////////////////

textures/overcast_sky/overcast_sky
{
	qer_editorimage textures/overcast_sky/overcast_editor
	
	skyParms textures/overcast_sky/overcast 2048 -

	q3map_sunExt 0.55 0.55 0.65 100 180 35	// R G B Intensity Angle Pitch


	q3map_skylight 100 4                    //amount iterations
	q3map_noFog

	surfaceparm sky                         //flags compiler that this is sky
	surfaceparm noimpact
	surfaceparm nolightmap
	surfaceparm nodlight
	nopicmip
	nomipmaps
	nocompress

//	{
//		map textures/avoc_common/overcast_clouds.tga
//		blendfunc gl_src_alpha gl_one_minus_src_alpha
//		tcMod scroll -0.005 0.005
//		tcmod scale 2 2
//	}

}
