/////////////////////////////////////////////////////////////////////
// ENDLESS SKY - BY AVOC @ eft-clan.com
// If you wish to use this sky, please leave this part in the shader
// or give proper credit in the readme
/////////////////////////////////////////////////////////////////////

textures/endless_sky/endless_sky
{
	qer_editorimage textures/avoc_common/skybox_editor
	
	skyParms textures/endless_sky/endless 2048 -

	q3map_sunExt 1.00 .90 .80 200 270 45	// R G B Intensity Angle Pitch


	q3map_skylight 150 4                    //amount iterations
	q3map_noFog

	surfaceparm sky                         //flags compiler that this is sky
	surfaceparm noimpact
	surfaceparm nolightmap
	surfaceparm nodlight
	nopicmip
	nomipmaps

}