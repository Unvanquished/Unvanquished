/////////////////////////////////////////////////////////////////////
// FROZEN SKY - BY AVOC @ eft-clan.com
// If you wish to use this sky, please leave this part in the shader
// or give proper credit in the readme
// Rendered by Paul aka Scary - paul@dumpendiezooi.nl
/////////////////////////////////////////////////////////////////////

textures/frozen_sky/frozen_sky
{
	qer_editorimage textures/frozen_sky/frozen_up
	
	skyParms textures/frozen_sky/frozen 2048 -

	q3map_sunExt 0.90 0.68 0.77 200 220 25	// R G B Intensity Angle Pitch


	q3map_skylight 100 4                    //amount iterations
	q3map_noFog

	surfaceparm sky                         //flags compiler that this is sky
	surfaceparm noimpact
	surfaceparm nolightmap
	surfaceparm nodlight
	nopicmip
	nomipmaps

}

