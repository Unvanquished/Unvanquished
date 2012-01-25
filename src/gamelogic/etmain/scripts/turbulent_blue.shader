/////////////////////////////////////////////////////////////////////
// TURBULENT BLUE SKY - BY AVOC @ eft-clan.com
// If you wish to use this sky, please leave this part in the shader
// or give proper credit in the readme
/////////////////////////////////////////////////////////////////////

textures/turbulent_blue/turbulent_sky
{
	qer_editorimage textures/turbulent_blue/turbulent_up
	
	skyParms textures/turbulent_blue/turbulent 2048 -

	q3map_sunExt 0.90 0.90 0.90 190 220 35	// R G B Intensity Angle Pitch


	q3map_skylight 100 4                    //amount iterations
	q3map_noFog

	surfaceparm sky                         //flags compiler that this is sky
	surfaceparm noimpact
	surfaceparm nolightmap
	surfaceparm nodlight
	nopicmip
	nomipmaps

}