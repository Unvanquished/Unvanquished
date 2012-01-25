/////////////////////////////////////////////////////////////////////
// FROZEN SKY - BY AVOC @ eft-clan.com
// If you wish to use this sky, please leave this part in the shader
// or give proper credit in the readme
// Rendered by Paul aka Scary - paul@dumpendiezooi.nl
/////////////////////////////////////////////////////////////////////

textures/darkness_sky/darkness_sky
{
	qer_editorimage textures/darkness_sky/editor.jpg
	
	skyParms textures/darkness_sky/darkness_sky 2048 -

	q3map_sunExt 0.68 0.68 0.90 200 250 50	// R G B Intensity Angle Pitch


	q3map_skylight 100 4                    //amount iterations
	q3map_noFog
	
	surfaceparm sky                         //flags compiler that this is sky
	surfaceparm noimpact
	surfaceparm nolightmap
	surfaceparm nodlight
	nopicmip
	nomipmaps

}

